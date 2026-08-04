/**
 * Bastion - persistence. Two jobs, both under the app's own data directory on
 * the SD card:
 *
 *   - Settings survive a reboot. saved_struct gives magic + version + checksum,
 *     so a stale or corrupt file falls back to defaults instead of loading
 *     garbage into an array index.
 *   - Graded badges are appended to a CSV. Auditing a site means walking it
 *     with a handful of badges, and nobody remembers the sixth one.
 */
#pragma once

#include <furi.h>

#include "badge_reader.h"
#include "lf_grade.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t mode; /* BadgeMode */
    bool sound;
    bool vibro;
    bool led;
    bool logging;
} BastionSettings;

/** Settings. load() leaves *s untouched when there is nothing valid to read. */
void bst_store_settings_save(const BastionSettings* s);
void bst_store_settings_load(BastionSettings* s);

/** Append one graded badge to the log. False if the write failed. */
bool bst_store_log_append(const LfGrade* grade);

/**
 * Render the newest entries into `out` as widget markup, newest first.
 * Returns how many were rendered (0 = the log is empty).
 */
uint8_t bst_store_log_render(FuriString* out, uint8_t max);

/** Delete the log. False if there was nothing to delete. */
bool bst_store_log_clear(void);

/** Where the log lives, so the About screen can tell the user. */
const char* bst_store_log_path(void);

#ifdef __cplusplus
}
#endif
