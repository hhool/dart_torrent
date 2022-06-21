#!/bin/sh
echo "This script is build for Android NDK 23"
ROOT=`pwd`
OPENSSL_VERSION="1.1.1o"
THREADS="-j$(sysctl -n hw.ncpu)"
MAKE_EXEC=`which make`
if [ -z "$MAKE_EXEC" ]; then
    echo "make is not found and is required to build OpenSSL"
fi
if [ -z "${ANDROID_HOME}" ]; then
    ANDROID_HOME="${HOME}/Android/sdk"
    if [ ! -d "${ANDROID_HOME}" ]; then
        ANDROID_HOME="${HOME}/android-sdk"
    fi
    if [ ! -d "${ANDROID_HOME}" ]; then
        ANDROID_HOME="${HOME}/Library/Android/sdk"
    fi
    if [ ! -d "${ANDROID_HOME}" ]; then
        echo "Android SDK Home not found. Please set the ANDROID_HOME environment variable to point to your Android SDK directory"
        exit 1
    fi
fi
NDK_PATH="${ANDROID_HOME}/ndk-bundle"
if [ ! -d "${NDK_PATH}" ]; then
    NDK_PATH="${ANDROID_HOME}/ndk"
    if [ ! -d "${NDK_PATH}" ]; then
        echo "Android NDK Home not found. Please set the NDK_PATH environment variable to point to your Android NDK root directory"
        exit 1
    fi
fi
NDK_PATH=`find ${NDK_PATH} -type d -name "build" -maxdepth 2 | head -1`
NDK_PATH=`dirname ${NDK_PATH}`
if [ -z "${NDK_PATH}" ]; then
    echo "Android NDK not found. Please set the NDK_PATH environment variable to point to your Android NDK root directory"
    exit 1
fi
BOOST_ROOT=`find ${ROOT}/external/boost-android -type d -name "boost_*" -maxdepth 2 | head -1`
if [ -z "${BOOST_ROOT}" ]; then
    echo "Boost not found, please run make sure submodule external/boost-android is cloned"
    exit 1
fi
TOOLCHAIN_ROOT=${NDK_PATH}/toolchains
LLVM_BIN_PATH=${TOOLCHAIN_ROOT}/llvm/prebuilt/$(ls ${NDK_PATH}/toolchains/llvm/prebuilt/ | head -1)/bin
export ANDROID_NDK_HOME=${NDK_PATH}
echo "make Found in ${MAKE_EXEC}"
echo "Android NDK Found in: ${NDK_PATH}"
echo "Boost Found in: ${BOOST_ROOT}"
echo "Toolchain Found in: ${TOOLCHAIN_ROOT}"

rm -rf $ROOT/lib/android/openssl
cd build
    if [ ! -f "openssl-${OPENSSL_VERSION}.tar.gz" ]; then
        wget https://www.openssl.org/source/openssl-${OPENSSL_VERSION}.tar.gz --no-check-certificate
    fi
    if [ ! -d "openssl" ]; then
        tar -xvzf openssl-${OPENSSL_VERSION}.tar.gz
        mv openssl-${OPENSSL_VERSION} openssl
    fi
    cd openssl # build/openssl
        OPENSSL_ROOT=`pwd`
        [ ! -d "build" ] && mkdir build
        cd build # build/openssl/build
            BUILD_PATH=`pwd`
            LINKER_BIN_PATH=`find ${TOOLCHAIN_ROOT}/llvm/prebuilt -type d -name "bin" -maxdepth 2 | head -1`
            if [ -z "${LINKER_BIN_PATH}" ]; then
                echo "Error finding linker binaries path"
                exit 1
            fi
            echo "Using Linker Binary Path: ${LINKER_BIN_PATH}"
            export PATH=${LINKER_BIN_PATH}:${PATH}
            export LD=${LINKER_BIN_PATH}/ld
            export AR=${LINKER_BIN_PATH}/llvm-ar
            echo "Using LD: ${LD}"
            if [ ! -f "${AR}" ]; then
                echo "Error finding archiver"
                exit 1
            fi
            echo "Using AR: ${AR}"
            TARGETS="armeabi-v7a x86 x86_64 arm64-v8a"
            for target in $TARGETS; do
                if [ -d "$BUILD_PATH/$target" ]; then
                    rm -rf $BUILD_PATH/$target
                fi
                mkdir $BUILD_PATH/$target
                cd $BUILD_PATH/$target # build/openssl/build/$target
                    CWD=`pwd`
                    OUTPUT_DIR=${ROOT}/lib/android/openssl/${target}
                    echo "Building OpenSSL for ${target}"
                    
                    if [ $target == 'armeabi-v7a' ]; then
                        export CC="${LLVM_BIN_PATH}/armv7a-linux-androideabi23-clang"
                        export CXX="${LLVM_BIN_PATH}/armv7a-linux-androideabi23-clang++"
                        export AS="${LLVM_BIN_PATH}/arm-linux-androideabi-as"
                        OS="android-armeabi"
                    elif [ $target == 'x86' ]; then
                        export CC="${LLVM_BIN_PATH}/i686-linux-android23-clang"
                        export CXX="${LLVM_BIN_PATH}/i686-linux-android23-clang++"
                        export AS="${LLVM_BIN_PATH}/i686-linux-android-as"
                        OS="android-x86"
                    elif [ $target == 'x86_64' ]; then
                        export CC="${LLVM_BIN_PATH}/x86_64-linux-android23-clang"
                        export CXX="${LLVM_BIN_PATH}/x86_64-linux-android23-clang++"
                        export AS="${LLVM_BIN_PATH}/x86_64-linux-android-as"
                        OS="android64-x86_64"
                    else
                        export CC="${LLVM_BIN_PATH}/aarch64-linux-android23-clang"
                        export CXX="${LLVM_BIN_PATH}/aarch64-linux-android23-clang++"
                        export AS="${LLVM_BIN_PATH}/aarch64-linux-android-as"
                        OS="android64-aarch64"
                    fi
                    if [ ! -f "${AS}" ]; then
                        echo "Error finding assembler"
                        exit 1
                    fi
                    echo "Using AS: ${AS}"
                    if [ ! -f "${CC}" ]; then
                        echo "Error finding clang for ${target}"
                        continue
                    fi
                    if [ ! -f "${CXX}" ]; then
                        echo "Error finding c++ for ${target}"
                        continue
                    fi
                    if ${OPENSSL_ROOT}/Configure ${OS} shared no-zlib -I${OPENSSL_ROOT}/include -I${OPENSSL_ROOT} -Iinclude -O3 -DNDEBUG ; then
                        $MAKE_EXEC $THREADS CC=$CC CXX=$CXX AR=$AR LD=$LD AS=$AS SHLIB_VERSION_NUMBER= SHLIB_EXT=.so
                        if [ $? -ne 0 ]; then
                            echo "Error building openssl for ${target}"
                            continue
                        fi
                        mkdir -p ${OUTPUT_DIR}
                        cp *.so ${OUTPUT_DIR}
                    else
                        echo "Error configuring openssl for ${target}"
                    fi
            done
cd $ROOT