#include "memory.hpp"
#include "dart_torrent.h"

char* lt_init_str_cache();
// 8MB
constexpr size_t MEM_BLOCK_SIZE = 8 * 1024 * 1024;
char* g_mem_block;
char* g_mem_block_last = lt_init_str_cache();
const char* g_mem_block_end;

char* lt_init_str_cache() {
    g_mem_block = static_cast<char*>(malloc(MEM_BLOCK_SIZE));
    g_mem_block_end = g_mem_block + MEM_BLOCK_SIZE;
    return g_mem_block;
}

EXPORT_TO_DART ptrdiff_t lt_cache_used() {
    return g_mem_block_last - g_mem_block;
}

EXPORT_TO_DART double lt_cache_used_percentage() {
    return static_cast<double>(g_mem_block_last - g_mem_block) / MEM_BLOCK_SIZE * 100.0;
}

void* lt_allocate(size_t n) {
    if (g_mem_block_last + n >= g_mem_block_end) {
         // reset the memory block position, old string cache are invalidated
        g_mem_block_last = g_mem_block;
    }
    char* r = g_mem_block_last;
    g_mem_block_last += n;
    return r;
}