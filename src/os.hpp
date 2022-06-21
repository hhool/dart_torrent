#ifndef OS_HPP
#define OS_HPP

#if defined(DEBUG) && !defined(NDEBUG)
    #define TORRENT_DEBUG 1
#endif
#if defined(NDEBUG)
    #define TORRENT_DEBUG 0
#endif

#ifdef __ANDROID__
    #define TORRENT_OS_ANDROID 1
#else
    #define TORRENT_OS_ANDROID 0
#endif
#if __APPLE__
    #include <TargetConditionals.h>
    #if TARGET_OS_IPHONE || TARGET_OS_IOS || TARGET_IPHONE_SIMULATOR
        #define TORRENT_OS_IOS 1
    #else
        #define TORRENT_OS_IOS 0
    #endif
#else
    #define TORRENT_OS_IOS 0
#endif

#endif