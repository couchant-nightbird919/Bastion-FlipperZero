/**
 * Bastion's read layer: a thin, strictly read-only wrapper around the
 * firmware's LF-RFID worker.
 *
 * The worker energises the 125 kHz coil and runs the firmware's own decoder
 * bank against whatever comes back. Bastion uses the read path and nothing
 * else - it never calls the write, emulate or raw-emulate entry points, so it
 * cannot program a blank, cannot pretend to be your badge, and leaves the tag
 * exactly as it found it.
 *
 * Threading: the worker calls back on its own thread. The callback only records
 * a stage and a protocol id under a mutex; every dictionary read happens later,
 * on the GUI thread, inside badge_reader_take() and only after the worker has
 * been stopped - so nothing reads the decoder's buffers while it is still
 * writing them.
 */
#pragma once

#include <furi.h>
#include <lib/lfrfid/lfrfid_worker.h>

#include "lf_grade.h"

#ifdef __cplusplus
extern "C" {
#endif

/** What the coil is doing right now - drives the scan animation. */
typedef enum {
    BadgeStageIdle, /* not started */
    BadgeStageSensing, /* field up, nothing in it yet */
    BadgeStageTagPresent, /* something is loading the coil */
    BadgeStageDemodASK, /* trying the ASK/OOK demodulator */
    BadgeStageDemodPSK, /* trying the PSK demodulator */
    BadgeStageDecoded, /* a protocol matched; ready to take */
} BadgeStage;

/** Which demodulators to run. Auto alternates; forcing one helps a marginal
 *  read on a tag whose carrier you already know. */
typedef enum {
    BadgeModeAuto = 0,
    BadgeModeASK,
    BadgeModePSK,
    BadgeModeCount,
} BadgeMode;

/** Everything one successful read produced. */
typedef struct {
    LfReading reading; /* pure payload, handed straight to the grader */
    char fw_name[24]; /* the firmware's own name for the protocol */
    char manufacturer[24]; /* vendor string, when the decoder supplies one */
} BadgeCapture;

typedef struct BadgeReader BadgeReader;

BadgeReader* badge_reader_alloc(void);
void badge_reader_free(BadgeReader* reader);

/** Raise the field and start decoding. Safe to call when already running. */
void badge_reader_start(BadgeReader* reader, BadgeMode mode);

/** Drop the field. Idempotent - scene exits call it unconditionally. */
void badge_reader_stop(BadgeReader* reader);

/** Current stage, for the scan view. Cheap; poll it from the tick handler. */
BadgeStage badge_reader_stage(BadgeReader* reader);

/**
 * If a badge has been decoded, stop the worker, extract everything and return
 * true - exactly once per read. `rendered` (optional) receives the decoder's
 * own multi-line field dump for the report.
 */
bool badge_reader_take(BadgeReader* reader, BadgeCapture* out, FuriString* rendered);

#ifdef __cplusplus
}
#endif
