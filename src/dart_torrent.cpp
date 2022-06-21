#include <stdexcept>
#include <limits>
#include <unordered_map>
#include <utility>

#include <libtorrent/session.hpp>
#include <libtorrent/session_params.hpp>
#include <libtorrent/add_torrent_params.hpp>
#include <libtorrent/torrent_handle.hpp>
#include <libtorrent/alert_types.hpp>
#include <libtorrent/torrent_status.hpp>
#include <libtorrent/error_code.hpp>
#include <libtorrent/magnet_uri.hpp>

#include <sqlite3.h>

#include "dart_torrent.h"
#include "trackers.hpp"
#include "sql.hpp"
#include "misc.hpp"
#include "str.hpp"

using namespace libtorrent;

std::vector<torrent_handle> g_torrent_handles;
bool g_need_update = true;

extern "C" {
EXPORT_TO_DART
void lt_init_session(const char* db_path) {
    log("[libTorrent] Initializing session");
    lt_get_session().set_alert_notify([] {
        std::vector<alert*> alerts;
        lt_get_session().pop_alerts(&alerts);
        for (auto* a : alerts) {
            auto* sca = alert_cast<state_changed_alert>(a);
            auto* mr = alert_cast<metadata_received_alert>(a);
            auto* tf = alert_cast<torrent_finished_alert>(a);
            auto* sa = alert_cast<save_resume_data_alert>(a);
            auto* sfa = alert_cast<save_resume_data_failed_alert>(a);
            auto* ra = alert_cast<torrent_removed_alert>(a);
            auto* ea = alert_cast<torrent_error_alert>(a);
            auto* ta = alert_cast<add_torrent_alert>(a);
            if (sca) {
                sca->handle.save_resume_data(torrent_handle::save_info_dict | torrent_handle::only_if_modified);
            }
            else if (mr) {
                mr->handle.save_resume_data();
                g_need_update = true;
            }
            else if (tf) {
                tf->handle.save_resume_data();
            }
            else if (sa) {
                lt_update_resume_data(sa->params.info_hashes.get_best(), sa->params, sa->handle);
            }
            else if (ra) {
                lt_remove_resume_data(ra->info_hashes.get_best());
                g_torrent_handles.erase(std::remove(g_torrent_handles.begin(), g_torrent_handles.end(), ra->handle), g_torrent_handles.end());
                log("[libTorrent] Torrent removed: {}", ra->handle.status().name);
            }
            else if (ea) {
                log("[libTorrent] Torrent error: {}", a->message());
                if (!ea->handle.is_valid()) {
                    continue;
                }
                if ((ea->handle.flags() & torrent_flags::paused) != (torrent_flags_t)0) {
                    ea->handle.resume();
                }
            }
            else if (ta) {
                if (std::find(g_torrent_handles.begin(), g_torrent_handles.end(), ta->handle) != g_torrent_handles.end()) {
                    continue;
                }
                g_torrent_handles.insert(g_torrent_handles.begin(), ta->handle);
                g_need_update = true;
                log("[libTorrent] {}", a->message());
            }
            else if (sfa || sca) {
                log("[libTorrent] {}", a->message());
            }
            else {
                log("[libTorrent] Other: {}", a->message());
            }
        }
    });

    lt_init_session_db(db_path);
}

EXPORT_TO_DART void lt_add_torrent(const char* magnet_url, const char* save_path, bool is_torrent_file) {
    try {
        add_torrent_params atp;
        if (is_torrent_file) {
            atp.ti = std::make_shared<torrent_info>(magnet_url); // load .torrent file
        }
        else {
            atp = parse_magnet_uri(magnet_url);
        }
        atp.max_uploads = 0; // ensure no uploads
        atp.upload_limit = 0; // ensure no uploads
        atp.trackers = TRACKERS;
        atp.save_path = save_path;
        lt_get_session().async_add_torrent(atp);
    } catch(std::exception& e) {
        log("[libTorrent] Error adding torrent: {}\n message: {}", magnet_url, e.what());
    }
}

/* Torrent list */
EXPORT_TO_DART size_t lt_n_torrents() {
    return g_torrent_handles.size();
}

EXPORT_TO_DART bool lt_need_update() {
    return g_need_update;
}

EXPORT_TO_DART void lt_updated() {
    g_need_update = false;
}

/* Torrent properties */
EXPORT_TO_DART torrent_result* lt_query_torrent(size_t torrent_index) {
    if (torrent_index < 0 || torrent_index >= g_torrent_handles.size()) {
        return new torrent_result();
    }
    auto& torrent = g_torrent_handles[torrent_index];
    if (!torrent.is_valid()) {
        log("[libTorrent] (query_torrent) Invalid torrent handle {}", torrent_index);
        return new torrent_result();
    }
    auto status = torrent.status();
    return new torrent_result {
        .name = lt_cached_str(status.name),
        .save_path = lt_cached_str(status.save_path),
        .state = lt_torrent_state_str(status.state),
        .progress = status.progress,
        .total_size = status.total_wanted,
        .download_rate = status.download_rate,
        .paused = (torrent.flags() & torrent_flags::paused) != (torrent_flags_t)0,
        .finished = status.is_seeding
    };
}

/* Torrent control */
EXPORT_TO_DART void lt_pause_torrent(size_t torrent_index) {
    auto& torrent = g_torrent_handles[torrent_index];
    if (!torrent.is_valid()) {
        log("[libTorrent] (pause) Invalid torrent handle {}", torrent_index);
        return;
    }
    torrent.pause();
}

EXPORT_TO_DART void lt_resume_torrent(size_t torrent_index) {
    auto& torrent = g_torrent_handles[torrent_index];
    if (!torrent.is_valid()) {
        log("[libTorrent] (resume) Invalid torrent handle {}", torrent_index);
        return;
    }
    torrent.resume();
}

EXPORT_TO_DART void lt_remove_torrent(size_t torrent_index) {
    try {
        auto& torrent = g_torrent_handles.at(torrent_index);
        auto& ses = lt_get_session();
        ses.remove_torrent(torrent);
    } catch(std::exception& e) {
        log("[libTorrent] (remove) torrent {} ({})", torrent_index, e.what());
    }
}

/* Torrent files */
EXPORT_TO_DART files_result* lt_query_files(size_t torrent_index) {
    if (torrent_index < 0 || torrent_index >= g_torrent_handles.size()) {
        return new files_result();
    }
    auto& torrent = g_torrent_handles[torrent_index];
    if (!torrent.is_valid()) {
        log("[libTorrent] (query_files) Invalid torrent handle {}", torrent_index);
        return new files_result();
    }
    auto* p_info = torrent.torrent_file().get();
    if (p_info == nullptr) {
        return new files_result();
    }
    return new files_result {
        p_info, 
        p_info->num_files()
    };
}

EXPORT_TO_DART file_result* lt_query_file(files_result* query, int32_t index) {
    if (query == nullptr) {
        return nullptr;
    }
    const auto& info = *static_cast<const torrent_info*>(query->p_torrent_info);
    const auto& file_storage = info.files();
    auto idx = (file_index_t)index;
    return new file_result {
        lt_cached_str(file_storage.file_name(idx)),
        lt_cached_str(file_storage.file_path(idx)),
        file_storage.file_size(idx)
    };
}
} // extern "C"