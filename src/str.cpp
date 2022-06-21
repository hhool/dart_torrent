#include "str.hpp"
#include "dart_torrent.h"
#include "memory.hpp"
#include <boost/functional/hash.hpp>
#include <unordered_map>
#include <sstream>


std::unordered_map<size_t, char*> g_str_cache;
std::unordered_map<libtorrent::sha1_hash, char*> g_hash_cache;
const char* lt_c_str(const std::string& str) { return str.c_str(); }
const char* lt_c_str(const boost::string_view& str) { return str.data(); }

template<class StringLike>
char* lt_strdup(const StringLike& str) {
    size_t len = str.length();
    char* cached_str = static_cast<char*>(lt_allocate(len+1));
    memcpy(cached_str, lt_c_str(str), len);
    cached_str[len] = '\0';
    return cached_str;
}

template <class StringLike>
char* _lt_cached_str(const StringLike& str) {
    static boost::hash<StringLike> hasher;
    size_t hash = hasher(str);
    if (g_str_cache.find(hash) != g_str_cache.end()) {
        return g_str_cache[hash];
    }
    char* cached_str = lt_strdup(str);
    g_str_cache[hash] = cached_str;
    return cached_str;
}

char* lt_cached_str(const std::string& str) {
    return _lt_cached_str(str);
}

char* lt_cached_str(const boost::string_view& str) {
    return _lt_cached_str(str);
}

char* lt_cached_str(libtorrent::sha1_hash hash) {
    auto it = g_hash_cache.find(hash);
    if (it != g_hash_cache.end()) {
        return it->second;
    }
    lt_stringsteam ss;
    ss << hash;
    return lt_strdup(ss.str());
}