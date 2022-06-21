#include <sstream>
#include "misc.hpp"
#include "str.hpp"
#include "dart_torrent.h"
using namespace libtorrent;

session* g_ses = lt_new_session();

session* lt_new_session() {
    settings_pack pack;
    pack.set_int(settings_pack::alert_mask,
        alert_category::error
        | alert_category::status
        | alert_category::file_progress
        | alert_category::storage
    );
    pack.set_int(settings_pack::active_seeds, 0);
    pack.set_int(settings_pack::active_downloads, 5);
    pack.set_int(settings_pack::active_checking, 3);
    pack.set_int(settings_pack::active_dht_limit, -1);
    pack.set_int(settings_pack::active_tracker_limit, 1000);
    pack.set_int(settings_pack::aio_threads, 32);
    pack.set_int(settings_pack::suggest_mode, 0);
    pack.set_int(settings_pack::mixed_mode_algorithm, 0);
    pack.set_int(settings_pack::choking_algorithm, settings_pack::rate_based_choker);

    pack.set_bool(settings_pack::prefer_udp_trackers, true);
    pack.set_bool(settings_pack::prefer_rc4, true);
    pack.set_bool(settings_pack::rate_limit_ip_overhead, false);
    pack.set_bool(settings_pack::piece_extent_affinity, true);
    pack.set_bool(settings_pack::enable_dht, true);
    pack.set_bool(settings_pack::enable_upnp, true);
    pack.set_bool(settings_pack::enable_lsd, true);
    pack.set_bool(settings_pack::enable_natpmp, true);
    pack.set_bool(settings_pack::enable_incoming_tcp, true);
    pack.set_bool(settings_pack::enable_outgoing_tcp, true);
    pack.set_bool(settings_pack::enable_incoming_utp, true);
    pack.set_bool(settings_pack::enable_outgoing_utp, true);
    pack.set_bool(settings_pack::auto_sequential, true);
    pack.set_bool(settings_pack::support_share_mode, false);
    pack.set_bool(settings_pack::seeding_outgoing_connections, false);
    pack.set_bool(settings_pack::no_connect_privileged_ports, true);
    pack.set_bool(settings_pack::anonymous_mode, true);
    pack.set_bool(settings_pack::allow_i2p_mixed, false);
    pack.set_bool(settings_pack::allow_multiple_connections_per_ip, true);
    pack.set_bool(settings_pack::announce_to_all_tiers, false);
    pack.set_bool(settings_pack::announce_to_all_trackers, true);
    pack.set_bool(settings_pack::close_redundant_connections, true);
    pack.set_bool(settings_pack::use_dht_as_fallback, true);
    pack.set_bool(settings_pack::use_parole_mode, false);
    pack.set_bool(settings_pack::upnp_ignore_nonrouters, true);
    log("[libTorrent] Settings applied");
    return new session(std::move(pack));
}

session& lt_get_session()
{
    return *g_ses;
}

char* lt_hash_str(sha1_hash hash) {
    return lt_cached_str(hash);
}

char* lt_hash_str(const libtorrent::torrent_handle& handle) {
    return lt_cached_str(handle.info_hashes().get_best());
}

libtorrent::sha1_hash& lt_info_hash_cast(void* p_info_hash) {
    return *static_cast<sha1_hash*>(p_info_hash);
}

const char* lt_torrent_state_str(int state)
{
    switch(state) {
        case torrent_status::checking_files: return "Checking";
        case torrent_status::downloading_metadata: return "Downloading metadata";
        case torrent_status::downloading: return "Downloading";
        case torrent_status::finished: return "Finished";
        case torrent_status::seeding: return "Seeding";
        case torrent_status::checking_resume_data: return "Checking resume";
        default: return "<>";
    }
}