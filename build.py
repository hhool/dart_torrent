import os, sys
from pathlib import Path

USAGE = '''
build.py <platform> [...]

Platforms:
macos - Mac OS Universal Binary (x86_64, arm64)
ios - iOS (arm64)
windows - Windows (x86_64, x86)
linux - Linux (x86_64, x86)
android - Android (arm64, armv7)

Option:
--debug - Builds a debug version of the library
'''

argv = sys.argv
argc = len(argv)

if argc < 2:
    print(USAGE)
    sys.exit(1)
    
platform = argv[1]

MACOS_CLANG_EXEC = '/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang++'
LIB_PATH = f'../lib/{platform}'
ROOT = '../'
SRC_DIR = f'{ROOT}/src'
CPP_FILES = [os.path.join(dp, f) for dp, _, fn in os.walk(SRC_DIR) for f in fn if f.endswith('.cpp' or f.endswith('.c'))]
CPP_FILES = list(map(lambda f: f'{SRC_DIR}/{f}', CPP_FILES))
BUILD_FLAGS = "-DFMT_HEADER_ONLY -DTORRENT_HAVE_MMAP=0 -DTORRENT_NO_DEPRECATE -DTORRENT_DISABLE_SUPERSEEDING -DTORRENT_DISABLE_SHARE_MODE -DBOOST_ALL_NO_LIB -DBOOST_ASIO_ENABLE_CANCELIO -DBOOST_ASIO_HAS_STD_CHRONO -DBOOST_MULTI_INDEX_DISABLE_SERIALIZATION -DBOOST_NO_DEPRECATED -DBOOST_SYSTEM_NO_DEPRECATED -DBOOST_ASIO_DISABLE_EPOLL"
CXX_FLAGS = f'--std=c++17 -stdlib=libc++ -fvisibility=default {" ".join(CPP_FILES)} -Wall -I{ROOT}/external/fmt/include -I{ROOT}/external/sqlite3 {BUILD_FLAGS}'
LD_FLAGS = f'-L{LIB_PATH} -ltorrent -lcrypto'
FW_FLAGS = ' -framework SystemConfiguration -framework CoreFoundation -framework Network -framework Security'

if '--debug' not in argv:
    CXX_FLAGS += ' -DNDEBUG -O3'
else:
    CXX_FLAGS += ' -O0 -g'

command_line = ''
os.chdir('./build') # work in build/
if platform == 'macos':
    SDK_PATH='/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk'
    CXX_FLAGS += f' -I{ROOT}/external/boost'
    CXX_FLAGS += f' -I{ROOT}/build/libtorrent/include'
    CXX_FLAGS += f' -I{ROOT}/build/openssl/include'
    command_line = f'{MACOS_CLANG_EXEC} -arch x86_64 -arch arm64 -isysroot {SDK_PATH} -mmacosx-version-min=10.13 -dynamiclib -Wl,-headerpad_max_install_names -o {LIB_PATH}/libdart_torrent.dylib -install_name @rpath/libdart_torrent.dylib {CXX_FLAGS} -L{SDK_PATH}/usr/lib {LD_FLAGS} -F{SDK_PATH}/System/Library/Frameworks {FW_FLAGS}'
elif platform == 'ios':
    SDK_PATH='/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS.sdk'
    CXX_FLAGS += f' -I{ROOT}/external/boost'
    CXX_FLAGS += f' -I{ROOT}/build/libtorrent/include'
    CXX_FLAGS += f' -I{ROOT}/build/openssl/include'
    command_line = f'{MACOS_CLANG_EXEC} -arch arm64 -isysroot {SDK_PATH} -miphoneos-version-min=11.0 -dynamiclib -Wl,-headerpad_max_install_names -rpath @executable_path/Frameworks -rpath @loader_path/Frameworks -o {LIB_PATH}/libdart_torrent.dylib -install_name @rpath/libdart_torrent.dylib {CXX_FLAGS} -L{SDK_PATH}/usr/lib {LD_FLAGS} -F{SDK_PATH}/System/Library/Frameworks {FW_FLAGS}'
elif platform == 'windows':
    pass
elif platform == 'linux':
    pass
elif platform == 'android':
    command_line = "sh build_libtorrent_android.sh"
elif platform == 'gen-cmake':
    with open(f'{ROOT}/CMakeLists.txt', 'w+', encoding='utf-8') as f:
        f.write(
f'''
# THIS FILE IS NOT USED FOR BUILDING THE LIBRARY, ONLY FOR IDE AUTOCOMPLETE.
# TO BUILD THIS LIBRARY, PLEASE USE THE `build.py` IN THE ROOT DIRECTORY.
message(WARNING "Use build.py for building")
cmake_minimum_required(VERSION 3.1)
project(dart_torrent LANGUAGES CXX C)
set(CMAKE_CXX_FLAGS "${{CMAKE_CXX_FLAGS}} {CXX_FLAGS} {BUILD_FLAGS}")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)
find_package(SQLite3 REQUIRED)
if (SQLITE3_FOUND)
    include_directories(${{SQLite3_INCLUDE_DIR}})
    link_directories(${{SQLite3_LIBRARY_DIR}})
endif()
find_package(Boost REQUIRED)
if (Boost_FOUND)
    include_directories(${{Boost_INCLUDE_DIRS}})
    link_directories(${{Boost_LIBRARY_DIRS}})
endif()
find_package(Libtorrent-Rasterbar REQUIRED)
if (Libtorrent-Rasterbar_FOUND)
    include_directories(${{Libtorrent-Rasterbar_INCLUDE_DIRS}})
    link_directories(${{Libtorrent-Rasterbar_LIBRARY_DIRS}})
endif()
include_directories(external)
''')
else:
    print(f'Unknown platform: {platform}')
    sys.exit(1)

print(f'Building with command-line: {command_line}')

if os.system(command_line) != 0:
    print(f'Failed to build for {platform}')
    sys.exit(1)
else:
    print(f'Successfully built for {platform}')