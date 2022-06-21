#!/bin/bash
export CC=/usr/bin/clang
export CXX=/usr/bin/clang++
export AR=/usr/bin/ar
export AS=/usr/bin/as
export RANLIB=/usr/bin/ranlib
THREADS="-j$(sysctl -n hw.ncpu)"
cd build
OPENSSL_VERSION="1.1.1o"
if [ ! -f "openssl-${OPENSSL_VERSION}.tar.gz" ]; then
    wget https://www.openssl.org/source/openssl-${OPENSSL_VERSION}.tar.gz --no-check-certificate
fi
tar -xvzf openssl-$OPENSSL_VERSION.tar.gz
mv openssl-$OPENSSL_VERSION openssl_arm64/
tar -xvzf openssl-$OPENSSL_VERSION.tar.gz
mv openssl-$OPENSSL_VERSION openssl_x86_64/
tar -xvzf openssl-$OPENSSL_VERSION.tar.gz
mv openssl-$OPENSSL_VERSION openssl_ios/
cd openssl_arm64
make clean
./Configure darwin64-arm64-cc no-zlib -DNDEBUG -O3 -mmacosx-version-min=10.13 --release
make $THREADS
cd ../
cd openssl_x86_64
make clean
./Configure darwin64-x86_64-cc no-zlib -DNDEBUG -O3 -mmacosx-version-min=10.13 --release
make $THREADS
cd ../
cd openssl_ios
make clean
./Configure ios64-cross no-zlib -isysroot /Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS.sdk -DNDEBUG -O3 -miphoneos-version-min=11.0 --release
make $THREADS
cd ../
[ -f "libcrypto.a" ] && rm libcrypto.a
[ -f "libssl.a" ] && rm libssl.a
lipo -create openssl_arm64/libcrypto.a openssl_x86_64/libcrypto.a -output libcrypto.a
lipo -create openssl_arm64/libssl.a openssl_x86_64/libssl.a -output libssl.a
cp libcrypto.a ../lib/macos
cp libssl.a ../lib/macos
cp openssl_ios/libcrypto.a ../lib/ios/libcrypto.a
cp openssl_ios/libssl.a ../lib/ios/libssl.a
