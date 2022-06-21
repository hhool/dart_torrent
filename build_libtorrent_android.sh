#!/bin/sh
echo "This script is build for Android NDK 23"
ROOT=`pwd`
DEFINES="-DTORRENT_HAVE_MMAP=0 -DTORRENT_NO_DEPRECATE -DTORRENT_DISABLE_SUPERSEEDING -DTORRENT_DISABLE_SHARE_MODE -DBOOST_ALL_NO_LIB -DBOOST_ASIO_ENABLE_CANCELIO -DBOOST_ASIO_HAS_STD_CHRONO -DBOOST_MULTI_INDEX_DISABLE_SERIALIZATION -DBOOST_NO_DEPRECATED -DBOOST_SYSTEM_NO_DEPRECATED -DBOOST_ASIO_DISABLE_EPOLL"
BASE_BUILD_FLAGS="-fvisibility=default $DEFINES"
RELEASE_BUILD_FLAGS="-O3 -DNDEBUG $BASE_BUILD_FLAGS"
DEBUG_BUILD_FLAGS="-O0 -g3 -DDEBUG $BASE_BUILD_FLAGS"
RELEASE_CMAKE_FLAGS="-DCMAKE_BUILD_TYPE=Release"
DEBUG_CMAKE_FLAGS="-DCMAKE_BUILD_TYPE=Debug"
THREADS="-j$(sysctl -n hw.ncpu)"
LIBTORRENT_VERSION="2.0.6"
BOOST_VERSION="1.78.0"
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
CMAKE_EXEC=`which cmake`
if [ -z "${CMAKE_EXEC}" ]; then
    CMAKE_PATH="${ANDROID_HOME}/cmake"
    CMAKE_EXEC=`find ${CMAKE_PATH} -type f -name "cmake" -maxdepth 3 | head -1`
    CMAKE_BIN_PATH=`dirname ${CMAKE_EXEC}`
fi
if [ -z "${CMAKE_EXEC}" ]; then
    echo "CMake not found. Please install CMake"
    exit 1
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
if [ `find $ROOT/lib/android/openssl -iname '*.so' -maxdepth 2 | wc -l` -lt 8 ]; then
    sh $ROOT/build_openssl_android.sh
    if [ $? -ne 0 ]; then
        echo "Failed to build openssl"
        exit 1
    fi
fi
if [ `find $ROOT/lib/android/boost/*/lib -iname '*.a' -maxdepth 2 | wc -l` -lt 4 ]; then
    cd $ROOT/external/boost-android
        sh build-android.sh $NDK_PATH --boost=$BOOST_VERSION --target-version=23 --with-libraries="thread"
        if [ $? -ne 0 ]; then
            echo "Boost build failed"
            exit 1
        fi
        mv build/out $ROOT/lib/android/boost
    cd $ROOT
fi
BOOST_ROOT=${ROOT}/lib/android/boost
if [ ! -d "${BOOST_ROOT}" ]; then
    echo "Submodule external/boost-android is not built correctly before running this script!"
    exit 1
fi
TOOLCHAIN_ROOT=${NDK_PATH}/toolchains
LLVM_BIN_PATH=${TOOLCHAIN_ROOT}/llvm/prebuilt/$(ls ${NDK_PATH}/toolchains/llvm/prebuilt/ | head -1)/bin
export ANDROID_NDK_HOME=${NDK_PATH}
export ANDROID_NDK=${NDK_PATH}
export ANDROID_NDK_ROOT=${NDK_PATH}
echo "make Found in ${MAKE_EXEC}"
echo "Android NDK Found in: ${NDK_PATH}"
echo "Boost Found in: ${BOOST_ROOT}"
echo "Toolchain Found in: ${TOOLCHAIN_ROOT}"
rm -rf $ROOT/lib/android/libtorrent

build() 
{
    target=$1
    debug=$2
    CWD=`pwd`
    if [ -z "${debug}" ]; then
        BUILD_FLAGS=$RELEASE_BUILD_FLAGS
        CMAKE_FLAGS=$RELEASE_CMAKE_FLAGS
        PATH_SUFFIX=""
        LOG_SUFFIX="_release"
    else
        BUILD_FLAGS=$DEBUG_BUILD_FLAGS
        CMAKE_FLAGS=$DEBUG_CMAKE_FLAGS
        PATH_SUFFIX="/debug"
        LOG_SUFFIX="_debug"
    fi
    mkdir $BUILD_PATH/$target$PATH_SUFFIX
    cd $BUILD_PATH/$target$PATH_SUFFIX # build/libtorrent/build/$target$PATH_SUFFIX
        echo "In $(pwd)"
        if [ $target == 'armeabi-v7a' ]; then
            export CC="${LLVM_BIN_PATH}/armv7a-linux-androideabi23-clang"
            export CXX="${LLVM_BIN_PATH}/armv7a-linux-androideabi23-clang++"
            export AS="${LLVM_BIN_PATH}/arm-linux-androideabi-as"
        elif [ $target == 'x86' ]; then
            export CC="${LLVM_BIN_PATH}/i686-linux-android23-clang"
            export CXX="${LLVM_BIN_PATH}/i686-linux-android23-clang++"
            export AS="${LLVM_BIN_PATH}/i686-linux-android-as"
        elif [ $target == 'x86_64' ]; then
            export CC="${LLVM_BIN_PATH}/x86_64-linux-android23-clang"
            export CXX="${LLVM_BIN_PATH}/x86_64-linux-android23-clang++"
            export AS="${LLVM_BIN_PATH}/x86_64-linux-android-as"
        else
            export CC="${LLVM_BIN_PATH}/aarch64-linux-android23-clang"
            export CXX="${LLVM_BIN_PATH}/aarch64-linux-android23-clang++"
            export AS="${LLVM_BIN_PATH}/aarch64-linux-android-as"
        fi
        if [ ! -f "${AS}" ]; then
            echo "Error finding assembler for $target"
            cd $CWD
            return 1
        fi
        if [ ! -f "${CC}" ]; then
            echo "Error finding clang for ${target}"
            cd $CWD
            return 1
        fi
        if [ ! -f "${CXX}" ]; then
            echo "Error finding c++ for ${target}"
            cd $CWD
            return 1
        fi
        OPENSSL_ROOT=$ROOT/lib/android/openssl/$target
        OPENSSL_CFG_INCLUDE_DIR=$ROOT/build/openssl/build/$target/include
        export OPENSSL_INCLUDE_DIR=$ROOT/build/openssl/include
        export OPENSSL_LIBRARY_DIR=$ROOT/lib/android/openssl/${target}
        export BOOST_INCLUDE_DIR=`find ${BOOST_ROOT}/${target}/include/ -type d -maxdepth 1 | tail -1`
        BOOST_LIBRARY_DIR=${BOOST_ROOT}/${target}/lib
        if [ -z "${BOOST_INCLUDE_DIR}" ]; then
            echo "Error finding boost for ${target}"
            cd $CWD
            return 1
        fi
        OUTPUT_DIR=$ROOT/lib/android/libtorrent/$target$PATH_SUFFIX
        echo "Output Dir: ${OUTPUT_DIR}"
        $CMAKE_EXEC $LIBTORRENT_ROOT $CMAKE_FLAGS -Dencryption=ON -DBoost_INCLUDE_DIR=$BOOST_INCLUDE_DIR -DBoost_LIBRARY_DIR=$BOOST_LIBRARY_DIR -DOPENSSL_INCLUDE_DIR=$OPENSSL_INCLUDE_DIR -DOPENSSL_CRYPTO_LIBRARY="$OPENSSL_LIBRARY_DIR/libcrypto.so" -DCMAKE_CXX_STANDARD=14 -DCMAKE_C_COMPILER=$CC -DCMAKE_CXX_COMPILER=$CXX -DBUILD_SHARED_LIBS=ON -Dstatic_runtime=OFF -DCMAKE_CXX_FLAGS="-I${OPENSSL_INCLUDE_DIR} -I${OPENSSL_CFG_INCLUDE_DIR} -I$BOOST_ROOT $BUILD_FLAGS" -DCMAKE_SYSTEM_NAME=Android -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_SYSTEM_VERSION=23 -DANDROID_PLATFORM=android-23 -DANDROID_ABI=$target -DCMAKE_ANDROID_ARCH_ABI=$target -DANDROID_NDK=${NDK_PATH} -DCMAKE_ANDROID_NDK=${NDK_PATH} -DCMAKE_TOOLCHAIN_FILE=${NDK_PATH}/build/cmake/android.toolchain.cmake -DCMAKE_MAKE_PROGRAM=$MAKE_EXEC -G "Unix Makefiles"
        if [ $? -ne 0 ]; then
            echo "Error configuring libtorrent for ${target}"
            cd $CWD
            return 1
        fi
        echo "Building..."
        $MAKE_EXEC $THREADS CC=$CC CXX=$CXX AR=$AR LD=$LD AS=$AS | tee build_$target$LOG_SUFFIX.log
        if [ $? -ne 0 ]; then
            echo "Error building libtorrent for ${target}"
            cd $CWD
            return 1
        fi
        mkdir -p $OUTPUT_DIR
        cp libtorrent-rasterbar.so $OUTPUT_DIR/libtorrent-rasterbar.so
    cd $CWD
    return 0
}


cd build
    if [ ! -f "libtorrent-rasterbar-${LIBTORRENT_VERSION}.tar.gz" ]; then
        wget https://github.com/arvidn/libtorrent/releases/download/v$LIBTORRENT_VERSION/libtorrent-rasterbar-$LIBTORRENT_VERSION.tar.gz --no-check-certificate
    fi
    if [ ! -d "libtorrent" ]; then
        tar -xvzf libtorrent-rasterbar-$LIBTORRENT_VERSION.tar.gz
        mv libtorrent-rasterbar-$LIBTORRENT_VERSION libtorrent
    fi
    cd libtorrent # build/libtorrent
        [ ! -d "build" ] && mkdir build
            LIBTORRENT_ROOT=`pwd`
            cd build # build/libtorrent/build
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
                if [ ! -f "${LD}" ]; then
                    echo "Error finding linker"
                    exit 1
                fi
                echo "Using LD: ${LD}"
                if [ ! -f "${AR}" ]; then
                    echo "Error finding archiver"
                    exit 1
                fi
                echo "Using AR: ${AR}"
                TARGETS="armeabi-v7a x86 x86_64 arm64-v8a"
                for target in $TARGETS; do
                    if [ -d "$BUILD_PATH/$target" ]; then
                        echo "Cleaning $target"
                        rm -rf $BUILD_PATH/$target
                    fi
                    mkdir $BUILD_PATH/$target
                    cd $BUILD_PATH/$target # build/libtorrent/build/$target
                        echo "Building Libtorrent (release) for ${target}..."
                        build $target
                        echo "Building Libtorrent (debug) for ${target}..."
                        build $target debug
                    cd .. # build/libtorrent/build
                done
cd $ROOT