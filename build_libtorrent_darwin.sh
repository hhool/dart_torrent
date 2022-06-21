#!/bin/sh
ROOT=$PWD
BUILD_FLAGS="-O3 -DNDEBUG -Dencryption=ON -DTORRENT_HAVE_MMAP=0 -DTORRENT_NO_DEPRECATE -DTORRENT_DISABLE_SUPERSEEDING -DTORRENT_DISABLE_SHARE_MODE"
THREADS="-j$(sysctl -n hw.ncpu)"
LIBTORRENT_VERSION="2.0.6"
if [ ! -d "build/openssl_arm64" ]; then
    sh build_openssl.sh
fi
cd build
    if [ ! -f "libtorrent-rasterbar-$LIBTORRENT_VERSION.tar.gz" ]; then
        wget https://github.com/arvidn/libtorrent/releases/download/v$LIBTORRENT_VERSION/libtorrent-rasterbar-$LIBTORRENT_VERSION.tar.gz --no-check-certificate
    fi
    if [ ! -d "libtorrent_ios" ]; then
        tar -xvzf libtorrent-rasterbar-$LIBTORRENT_VERSION.tar.gz
        mv libtorrent-rasterbar-$LIBTORRENT_VERSION libtorrent_ios/
    fi
    if [ ! -d "libtorrent_macos" ]; then
        tar -xvzf libtorrent-rasterbar-$LIBTORRENT_VERSION.tar.gz
        mv libtorrent-rasterbar-$LIBTORRENT_VERSION libtorrent_macos/
    fi
    cd libtorrent_ios # build/libtorrent_ios
        [ ! -d "build" ] && mkdir build
        cd build # build/libtorrent_ios/build
            ninja clean
            export BOOST_ROOT="$ROOT/build/boost/1.78.0/ios/release/prefix/include"
            cmake .. -Dencryption=ON -DCMAKE_CXX_STANDARD=17 -DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF -Dstatic_runtime=ON -DCMAKE_CXX_FLAGS="-I$ROOT/build/openssl_ios/include -I$BOOST_ROOT $BUILD_FLAGS" --toolchain $ROOT/ios.toolchain.cmake -DPLATFORM=OS64 -DDEPLOYMENT_TARGET=11 -DENABLE_ARC=ON -DENABLE_BITCODE=OFF -G Ninja
            ninja $THREADS
            cp libtorrent-rasterbar.a $ROOT/lib/ios/libtorrent.a
    cd $ROOT/build
    cd libtorrent_macos # build/libtorrent_macos
        [ ! -d "build" ] && mkdir build
        cd build # build/libtorrent_macos/build
            ninja clean
            export BOOST_ROOT="$ROOT/build/boost/1.78.0/macos/release/prefix/include"
            cmake .. -DCMAKE_CXX_STANDARD=17 -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64" -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF -Dstatic_runtime=ON -DCMAKE_CXX_FLAGS="-I$ROOT/build/openssl_arm64/include -I$BOOST_ROOT $BUILD_FLAGS" -DCMAKE_OSX_DEPLOYMENT_TARGET=10.13 -G Ninja
            ninja $THREADS
            cp libtorrent-rasterbar.a $ROOT/lib/macos/libtorrent.a
cd $ROOT