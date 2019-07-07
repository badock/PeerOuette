import subprocess
import sys
import os
import time
import threading
import psutil

TIMEOUT_SECONDS = 55 * 60


class MonThread (threading.Thread):
    def __init__(self, cmd):
        threading.Thread.__init__(self)
        self.cmd = cmd
        self.return_code = None
        self.is_running = True
        self.stop_thread = False
        self.proc = None

    def run(self):
        self.proc = subprocess.Popen(self.cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)

        while self.proc.returncode is None and not self.stop_thread:

            for line in iter(self.proc.stdout.readline, b''):
                print("%s" % line.decode().strip())

                if "Elapsed time for package" in line.decode():
                    # Suspending the process
                    print(f"[build.py] suspending process {self.proc.pid}")
                    p = psutil.Process(self.proc.pid)
                    p.suspend()

                    # Copy the .hunter folder in the cached folder
                    print("[build.py] saving cache")

                    if sys.platform == 'win32':
                        cmd2 = "tar -zcf c:/vcpkg_cache.tgz C:/Tools/vcpkg/installed --force-local"
                        subprocess.call(cmd2)
                    else:
                        cmd2 = "tar -zcf /opt/travis_cache/vcpkg_cache.tgz $HOME/vcpkg/installed --force-local"
                        subprocess.call(cmd2, shell=True)

                    # Resuming the process
                    print(f"[build.py] resuming process {self.proc.pid}")
                    p.resume()


            for line in iter(self.proc.stderr.readline, b''):
                print("%s" % line.decode().strip())

            self.proc.communicate()

        self.return_code = self.proc.returncode
        self.is_running = False

    def stop(self):
        self.stop_thread = True
        if self.proc is not None:
            print("I will kill the process")
            if sys.platform == 'win32':
                print("Win32")
                subprocess.call(['taskkill', '/F', '/T', '/PID', str(self.proc.pid)])
            else:
                print("Linux/MACOS")
                self.proc.terminate()

if __name__ == "__main__":
    cwd = os.getcwd()
    print("current folder: %s" % cwd)
    cmd = " ".join(sys.argv[1:])

    print("I will launch '%s'" % (cmd))

    start_epoch = time.time()

    thread = MonThread(cmd)
    thread.start()

    while time.time() < start_epoch + TIMEOUT_SECONDS and thread.return_code is None:
        sys.stdout.flush()
        time.sleep(1)

    if thread.return_code is None:
        print("ask to stop process")
        thread.stop()
    else:
        print("process has exited by its own!")

    sys.stdout.flush()
    sys.exit(thread.return_code if thread.return_code is not None else 0)
