#ifndef SQL_HPP
#define SQL_HPP

#include <sqlite3.h>
#include <libtorrent/torrent_handle.hpp>
#include <libtorrent/add_torrent_params.hpp>
#include <libtorrent/sha1_hash.hpp>

void lt_init_session_db(const char* db_path);
void lt_update_resume_data(libtorrent::sha1_hash hash, libtorrent::add_torrent_params params, libtorrent::torrent_handle&);
void lt_remove_resume_data(libtorrent::sha1_hash hash);

template <size_t N>
constexpr size_t string_length(const char (&)[N]) {
    return N - 1;
}

static const char CREATE_TABLE_SQL[] = 
"CREATE TABLE IF NOT EXISTS torrents (hash TEXT PRIMARY KEY NOT NULL, resume BLOB NOT NULL, name TEXT, save_path TEXT);"; // Add name column if it doesn't exist

static const char FETCH_RESUME_DATA_SQL[] = 
"SELECT resume, name, save_path FROM torrents;";

static const char UPDATE_RESUME_DATA_SQL[] = 
"REPLACE INTO torrents (hash, resume, name, save_path) VALUES (?,?,?,?);";

static const char REMOVE_RESUME_DATA_SQL[] =
"DELETE FROM torrents WHERE hash = ?;";
#endif // SQL_HPP