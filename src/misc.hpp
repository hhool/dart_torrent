#ifndef MISC_HPP
#define MISC_HPP

/*
    Miscellaneous functions that does not expose to dart goes here
*/

#include <libtorrent/session.hpp>
#include <libtorrent/torrent_handle.hpp>

#include <iostream>
#include <fmt/format.h>

#include "os.hpp"

template <typename... Args>
void log(const char* fmt, Args&&... args) {
    if constexpr (!TORRENT_DEBUG) {
        return;
    }
    std::cout << fmt::format(fmt, std::forward<Args>(args)...) << std::endl;
}
char* lt_hash_str(libtorrent::sha1_hash hash);
char* lt_hash_str(const libtorrent::torrent_handle& handle);

libtorrent::session* lt_new_session();
libtorrent::session& lt_get_session();
libtorrent::sha1_hash& lt_info_hash_cast(void* p_info_hash);
const char* lt_torrent_state_str(int state);

#endif // MISC_HPP