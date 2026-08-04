#include "bst_store.h"

#include <furi_hal.h>
#include <storage/storage.h>
#include <toolbox/saved_struct.h>
#include <toolbox/stream/stream.h>
#include <toolbox/stream/file_stream.h>
#include <datetime/datetime.h>
#include <stdio.h>

#define BST_SETTINGS_PATH    APP_DATA_PATH("settings.bin")
#define BST_LOG_PATH         APP_DATA_PATH("badges.csv")
#define BST_SETTINGS_MAGIC   0xB5
#define BST_SETTINGS_VERSION 1

/* How many entries we hold in memory while scanning the file. The log itself
 * may grow without limit; only the newest slice is ever rendered. */
#define BST_LOG_WINDOW 20

typedef struct {
    uint8_t month, day, hour, minute;
    char letter[4];
    char band[12];
    int16_t score;
    uint8_t scored;
    char name[26];
    char id[26];
} BstLogged;

static void bst_store_ensure_dir(void) {
    Storage* storage = furi_record_open(RECORD_STORAGE);
    storage_common_mkdir(storage, STORAGE_APP_DATA_PATH_PREFIX);
    furi_record_close(RECORD_STORAGE);
}

/* ----------------------------------------------------------- settings ----- */

void bst_store_settings_save(const BastionSettings* s) {
    furi_assert(s);
    bst_store_ensure_dir();
    saved_struct_save(
        BST_SETTINGS_PATH, s, sizeof(BastionSettings), BST_SETTINGS_MAGIC, BST_SETTINGS_VERSION);
}

void bst_store_settings_load(BastionSettings* s) {
    furi_assert(s);
    BastionSettings loaded;
    if(!saved_struct_load(
           BST_SETTINGS_PATH,
           &loaded,
           sizeof(BastionSettings),
           BST_SETTINGS_MAGIC,
           BST_SETTINGS_VERSION)) {
        return; /* nothing valid on disk - the caller keeps its defaults */
    }
    /* Never let a file on the SD card index an array. */
    if(loaded.mode >= BadgeModeCount) loaded.mode = BadgeModeAuto;
    *s = loaded;
}

/* -------------------------------------------------------------- log ------- */

/* Commas and newlines would split a CSV field in two; the ID and name strings
 * come from decoder output, so they get sanitised rather than trusted. */
static void csv_safe(char* out, size_t out_sz, const char* in) {
    size_t n = 0;
    for(; in[n] != '\0' && n + 1 < out_sz; n++) {
        const char c = in[n];
        out[n] = (c == ',' || c == '\n' || c == '\r') ? ' ' : c;
    }
    out[n] = '\0';
}

bool bst_store_log_append(const LfGrade* grade) {
    furi_assert(grade);
    bst_store_ensure_dir();

    DateTime dt;
    furi_hal_rtc_get_datetime(&dt);

    char name[26];
    char id[26];
    csv_safe(name, sizeof(name), grade->name);
    csv_safe(id, sizeof(id), grade->id_line);

    char line[192];
    int n = snprintf(
        line,
        sizeof(line),
        "%04u-%02u-%02u,%02u:%02u,%s,%s,%u,%d,%s,%s\n",
        (unsigned)dt.year,
        (unsigned)dt.month,
        (unsigned)dt.day,
        (unsigned)dt.hour,
        (unsigned)dt.minute,
        grade->letter,
        lf_band_label(grade->band),
        grade->scored ? 1u : 0u,
        grade->score,
        name,
        id);
    if(n <= 0) return false;
    if((size_t)n >= sizeof(line)) n = (int)sizeof(line) - 1;

    Storage* storage = furi_record_open(RECORD_STORAGE);
    File* file = storage_file_alloc(storage);
    bool ok = false;
    if(storage_file_open(file, BST_LOG_PATH, FSAM_WRITE, FSOM_OPEN_APPEND)) {
        ok = storage_file_write(file, line, (size_t)n) == (size_t)n;
        storage_file_close(file);
    }
    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);
    return ok;
}

uint8_t bst_store_log_render(FuriString* out, uint8_t max) {
    furi_assert(out);
    if(max == 0) return 0;
    if(max > BST_LOG_WINDOW) max = BST_LOG_WINDOW;

    Storage* storage = furi_record_open(RECORD_STORAGE);
    Stream* stream = file_stream_alloc(storage);
    /* On the heap: twenty entries is ~1.5 KB, and this runs on a 4 KB app
     * stack that the widget and FuriString are already drawing from. */
    BstLogged* ring = malloc(sizeof(BstLogged) * BST_LOG_WINDOW);
    uint8_t count = 0;
    uint8_t head = 0;

    if(file_stream_open(stream, BST_LOG_PATH, FSAM_READ, FSOM_OPEN_EXISTING)) {
        FuriString* line = furi_string_alloc();
        while(stream_read_line(stream, line)) {
            BstLogged e;
            unsigned year, month, day, hour, minute, scored;
            int score;
            char letter[4] = {0};
            char band[12] = {0};
            char name[26] = {0};
            char id[26] = {0};

            /* Anything that does not parse cleanly is skipped rather than shown
             * as garbage - this is a plain text file a user may well edit. */
            if(sscanf(
                   furi_string_get_cstr(line),
                   "%u-%u-%u,%u:%u,%3[^,],%11[^,],%u,%d,%25[^,],%25[^\r\n]",
                   &year,
                   &month,
                   &day,
                   &hour,
                   &minute,
                   letter,
                   band,
                   &scored,
                   &score,
                   name,
                   id) != 11) {
                continue;
            }

            e.month = (uint8_t)month;
            e.day = (uint8_t)day;
            e.hour = (uint8_t)hour;
            e.minute = (uint8_t)minute;
            e.score = (int16_t)score;
            e.scored = (uint8_t)(scored ? 1 : 0);
            snprintf(e.letter, sizeof(e.letter), "%s", letter);
            snprintf(e.band, sizeof(e.band), "%s", band);
            snprintf(e.name, sizeof(e.name), "%s", name);
            snprintf(e.id, sizeof(e.id), "%s", id);

            ring[head] = e;
            head = (uint8_t)((head + 1) % max);
            if(count < max) count++;
        }
        furi_string_free(line);
    }

    stream_free(stream);
    furi_record_close(RECORD_STORAGE);

    /* Newest first: walk the ring backwards from the most recent write. */
    for(uint8_t i = 0; i < count; i++) {
        uint8_t idx = (uint8_t)((head + max - 1 - i) % max);
        const BstLogged* e = &ring[idx];
        if(e->scored) {
            furi_string_cat_printf(
                out, "\e#%s  %d/100  %s\e#\n", e->letter, (int)e->score, e->band);
        } else {
            furi_string_cat_printf(out, "\e#--  %s\e#\n", e->band);
        }
        furi_string_cat_printf(
            out,
            "%s\n%s\n%02u-%02u %02u:%02u\n\n",
            e->name,
            e->id,
            (unsigned)e->day,
            (unsigned)e->month,
            (unsigned)e->hour,
            (unsigned)e->minute);
    }

    free(ring);
    return count;
}

bool bst_store_log_clear(void) {
    Storage* storage = furi_record_open(RECORD_STORAGE);
    /* storage_simply_remove() reports success when the file was never there, so
     * ask first - otherwise "Cleared" would be claimed over an empty log. */
    const bool existed = storage_common_exists(storage, BST_LOG_PATH);
    const bool removed = storage_simply_remove(storage, BST_LOG_PATH);
    furi_record_close(RECORD_STORAGE);
    return existed && removed;
}

const char* bst_store_log_path(void) {
    return BST_LOG_PATH;
}
