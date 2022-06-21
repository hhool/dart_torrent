

#include <stdexcept>

#include <libtorrent/session.hpp>
#include <libtorrent/read_resume_data.hpp>
#include <libtorrent/write_resume_data.hpp>
#include <libtorrent/bdecode.hpp>

#include "sql.hpp"
#include "misc.hpp"
#include "dart_torrent.h"
#include "trackers.hpp"
#include "memory.hpp"

using namespace libtorrent;

sqlite3* db = nullptr;

void lt_init_session_db(const char* db_path) {
    int ret = sqlite3_open(db_path, &db);

    if (ret) {
        log("[sql] Can't open database: {}", sqlite3_errmsg(db));
        return;
    }

    ret = sqlite3_exec(db, CREATE_TABLE_SQL, nullptr, nullptr, nullptr);

    if(ret != SQLITE_OK){
        log("[sql] Error when creating table: {}", sqlite3_errmsg(db));
    }

    log("[sql] Table initialized");

    sqlite3_stmt* stmt = NULL;
	ret = sqlite3_prepare_v2(db, FETCH_RESUME_DATA_SQL, string_length(FETCH_RESUME_DATA_SQL), &stmt, NULL);

    if (ret != SQLITE_OK || !stmt)
	{
		log("[sql] failed to prepare select statement: {}", sqlite3_errmsg(db));
		return;
	}

	ret = sqlite3_step(stmt);

    session& ses = lt_get_session();
    while (ret == SQLITE_ROW) // load all torrents from previous session
	{
		int bytes = sqlite3_column_bytes(stmt, 0);
		if (bytes > 0)
		{
			const char* buf = static_cast<const char*>(sqlite3_column_blob(stmt, 0));
            auto resume_data = read_resume_data(span<char const>(buf, bytes));
            bytes = sqlite3_column_bytes(stmt, 1);
            if (bytes > 0) {
                resume_data.name = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
            }
            bytes = sqlite3_column_bytes(stmt, 2);
            if (bytes > 0) {
                auto old_path = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
#if TORRENT_OS_IOS // App GUID of iOS may different on every build
                try {
                    auto rel_path = old_path.substr(old_path.find("/Documents/"));
                    auto root = getenv("HOME");
                    resume_data.save_path = root + rel_path;
                } catch(std::exception& e) {
                    resume_data.save_path = old_path;
                }
#else
                resume_data.save_path = old_path;
#endif
            }
            try {
                log("[sql] Loaded resume data: {} (path: '{}/{}')", lt_hash_str(resume_data.info_hashes.get_best()), resume_data.save_path, resume_data.name);
                ses.async_add_torrent(std::move(resume_data));
            }
            catch(std::exception& e) {
                log("[sql] Failed to load resume data: {}", e.what());
            }
		}
        else {
            log("[sql] Empty resume data");
        }
		ret = sqlite3_step(stmt);
	}

    if (ret != SQLITE_DONE)
	{
		log("[sql] failed to step select statement: {}", sqlite3_errmsg(db));
		sqlite3_finalize(stmt);
		return;
	}
    
    sqlite3_finalize(stmt);

    log("[sql] Session initialized");
}

void lt_update_resume_data(sha1_hash hash, add_torrent_params params, torrent_handle& handle) {
    // std::vector<char> buf = write_resume_data_buf(std::move(params));
    auto hash_str = lt_hash_str(hash);
    auto name = params.name;
    auto save_path = params.save_path;
    lt_char_v buf;
    bencode(std::back_inserter(buf), write_resume_data(std::move(params)));

    sqlite3_mutex_enter(sqlite3_db_mutex(db));
    sqlite3_exec(db, "BEGIN TRANSACTION;", NULL, NULL, NULL);

    sqlite3_stmt *stmt;
    int ret = sqlite3_prepare_v2(db, UPDATE_RESUME_DATA_SQL, -1, &stmt, nullptr);
    if (ret != SQLITE_OK)
	{
		log("[sql] failed to prepare update statement: {}", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
		return;
	}

    ret = sqlite3_bind_text(stmt, 1, hash_str, strlen(hash_str), SQLITE_STATIC);
    if (ret != SQLITE_OK)
    {
        log("[sql] failed to bind hash: {}", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return;
    }

    ret = sqlite3_bind_blob(stmt, 2, &buf[0], buf.size(), SQLITE_STATIC);
    if (ret != SQLITE_OK)
    {
        log("[sql] failed to bind resume: {}", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return;
    }

    ret = sqlite3_bind_text(stmt, 3, name.c_str(), name.length(), SQLITE_STATIC);
    if (ret != SQLITE_OK)
    {
        log("[sql] failed to bind name: {}", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return;
    }

    ret = sqlite3_bind_text(stmt, 4, save_path.c_str(), save_path.length(), SQLITE_STATIC);
    if (ret != SQLITE_OK)
    {
        log("[sql] failed to bind save_path: {}", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return;
    }

    ret = sqlite3_step(stmt);
    if (ret != SQLITE_DONE)
    {
        log("[sql] failed to step update statement: {}", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return;
    }

    sqlite3_finalize(stmt);
    sqlite3_exec(db, "END TRANSACTION;", NULL, NULL, NULL);
    sqlite3_mutex_leave(sqlite3_db_mutex(db));

    log("[libTorrent] Resume data updated for {} ({})", name, hash_str);
}

void lt_remove_resume_data(sha1_hash hash) {
    auto hash_str = lt_hash_str(hash);
    sqlite3_stmt *stmt;

    sqlite3_mutex_enter(sqlite3_db_mutex(db));
    sqlite3_exec(db, "BEGIN TRANSACTION;", NULL, NULL, NULL);
    int ret = sqlite3_prepare_v2(db, REMOVE_RESUME_DATA_SQL, -1, &stmt, nullptr);
    if (ret != SQLITE_OK)
    {
        log("[sql] failed to prepare remove statement: {}", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return;
    }

    ret = sqlite3_bind_text(stmt, 1, hash_str, strlen(hash_str), SQLITE_STATIC);
    if (ret != SQLITE_OK)
	{
		log("[sql] failed to bind remove statement: {}", sqlite3_errmsg(db));
		sqlite3_finalize(stmt);
		return;
	}

    ret = sqlite3_step(stmt);
    if (ret != SQLITE_DONE)
	{
		log("[sql] failed to step remove statement: {}", sqlite3_errmsg(db));
		sqlite3_finalize(stmt);
		return;
	}

    sqlite3_finalize(stmt);
    sqlite3_exec(db, "END TRANSACTION;", NULL, NULL, NULL);
    sqlite3_mutex_leave(sqlite3_db_mutex(db));
    log("[libTorrent] Removed resume data for {}", hash_str);
}