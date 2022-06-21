#ifndef LIBTORRENT_H
#define LIBTORRENT_H

#define EXPORT_TO_DART __attribute__((visibility("default"))) __attribute__((used))

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus /* If this is a C++ compiler, use C linkage */
extern "C" {
#endif // __cplusplus

/* Debug */
EXPORT_TO_DART const char* lt_print_queue_last(); 
EXPORT_TO_DART void lt_pop_print_queue(); 
EXPORT_TO_DART ptrdiff_t lt_cache_used();
EXPORT_TO_DART double lt_cache_used_percentage();

/* Session */
EXPORT_TO_DART void lt_add_torrent(const char* magnet_url, const char* save_path, bool is_torrent_file); // returns hash of the torrent
EXPORT_TO_DART void lt_init_session(const char* db_path);

/* Torrent list */
EXPORT_TO_DART size_t lt_n_torrents();
EXPORT_TO_DART bool lt_need_update();
EXPORT_TO_DART void lt_updated();

/* Torrent properties */
typedef struct {
    char* name;
    char* save_path;
    const char* state;
    float progress;
    int64_t total_size;
    int download_rate;
    bool paused;
    bool finished;
} torrent_result;
EXPORT_TO_DART torrent_result* lt_query_torrent(size_t torrent_index);

/* Torrent control */
EXPORT_TO_DART void lt_pause_torrent(size_t torrent_index);
EXPORT_TO_DART void lt_resume_torrent(size_t torrent_index);
EXPORT_TO_DART void lt_remove_torrent(size_t torrent_index);

/* Torrent files */
typedef struct {
    const void* p_torrent_info;
    int n_files;
} files_result;
typedef struct {
    char* name;
    char* path;
    int64_t size;
} file_result;
EXPORT_TO_DART files_result* lt_query_files(size_t torrent_index);
EXPORT_TO_DART file_result* lt_query_file(files_result* query, int32_t index);
#ifdef __cplusplus /* If this is a C++ compiler, end C linkage */
}
#endif // __cplusplus

#endif // LIBTORRENT_H