#ifndef STR_HPP
#define STR_HPP

#include <boost/utility/string_view.hpp>
#include <libtorrent/sha1_hash.hpp>
#include <string>

inline const char* lt_c_str(const boost::string_view& str);
inline const char* lt_c_str(const std::string& str);
char* lt_cached_str(const std::string& str);
char* lt_cached_str(const boost::string_view& str);
char* lt_cached_str(libtorrent::sha1_hash hash); // cache string hash
#endif