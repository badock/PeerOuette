#!/bin/bash

sudo apt install -y gcc-6;
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

sudo apt install -y ffmpeg;
sudo apt install -y sdl sdl_image sdl_mixer sdl_ttf portmidi;
sudo apt install -y sdl2;

exit 0