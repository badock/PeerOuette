#!/bin/bash

sudo apt update;
sudo apt install -y gcc-6 g++-6 python3 python3-pip;
export CC=gcc-6;
export CXX=g++-6;
export CC_FOR_BUILD=gcc-6;
export CXX_FOR_BUILD=g++-6;

mkdir -p $HOME/travis_cache

ls -lh $HOME/travis_cache
pip3 install psutil

git clone https://github.com/Microsoft/vcpkg.git $HOME/vcpkg
pushd $HOME/vcpkg
  ./bootstrap-vcpkg.sh

  export CC=gcc;
  export CXX=g++;
  export CC_FOR_BUILD=gcc;
  export CXX_FOR_BUILD=g++;

  if [ ! -f $HOME/travis_cache/vcpkg_export.zip ]; then
    echo "install grpc from scratch"
    ./vcpkg install grpc
    ./vcpkg export --zip grpc
    ARCHIVE_FILE=$(ls vcpkg*.zip | tail -n 1)
    echo "ARCHIVE_FILE=$ARCHIVE_FILE"
    mv $ARCHIVE_FILE $HOME/travis_cache/vcpkg_export.zip
    echo "checking after export"
    ls -lh $HOME/travis_cache/
  else
    echo "reuse grpc from cache"
    echo "checking before import"
    unzip $HOME/travis_cache/vcpkg_export.zip -d /tmp/vcpkg_export
    cp -R /tmp/vcpkg_export/vcpkg-export-*/* .
  fi
   ./vcpkg integrate install
popd

#export CC=gcc;
#export CXX=g++;
#export CC_FOR_BUILD=gcc;
#export CXX_FOR_BUILD=g++;

#sudo apt install -y ffmpeg;
sudo apt install -y libavcodec-dev libavformat-dev libavutil-dev libswresample-dev libswscale-dev;
sudo apt install -y libsdl1.2-dev libsdl-image1.2 libsdl-image1.2-dev libsdl-ttf2.0-0 libsdl-ttf2.0-dev libsdl-mixer1.2 libsdl-mixer1.2-dev
sudo apt install -y libsdl2-dev;

exit 0