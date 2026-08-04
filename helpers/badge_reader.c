#include "badge_reader.h"

#include <toolbox/protocols/protocol_dict.h>
#include <lib/lfrfid/protocols/lfrfid_protocols.h>

/* LfProto mirrors LFRFIDProtocol entry for entry so the ids can be cast across.
 * If the firmware ever inserts a protocol, this stops the build instead of
 * silently grading every badge as the wrong format. */
_Static_assert(
    (int)LfProtoUnknown == (int)LFRFIDProtocolMax,
    "LfProto has drifted from the firmware's LFRFIDProtocol enum");

struct BadgeReader {
    ProtocolDict* dict;
    LFRFIDWorker* worker;
    bool thread_running; /* worker thread started */
    bool reading; /* read mode armed */

    FuriMutex* lock;
    /* --- written by the worker thread, read by the GUI thread --- */
    BadgeStage stage;
    ProtocolId decoded; /* PROTOCOL_NO until something matches */
};

/* Runs on the worker thread. Keep it to bookkeeping - no dictionary reads, no
 * allocation, no GUI calls. */
static void badge_reader_cb(LFRFIDWorkerReadResult result, ProtocolId protocol, void* context) {
    BadgeReader* reader = context;

    furi_mutex_acquire(reader->lock, FuriWaitForever);
    switch(result) {
    case LFRFIDWorkerReadSenseCardStart:
        if(reader->stage < BadgeStageDecoded) reader->stage = BadgeStageTagPresent;
        break;
    case LFRFIDWorkerReadSenseCardEnd:
        /* The tag left the field before anything decoded - back to waiting. */
        if(reader->stage < BadgeStageDecoded) reader->stage = BadgeStageSensing;
        break;
    case LFRFIDWorkerReadStartASK:
        if(reader->stage < BadgeStageDecoded) reader->stage = BadgeStageDemodASK;
        break;
    case LFRFIDWorkerReadStartPSK:
        if(reader->stage < BadgeStageDecoded) reader->stage = BadgeStageDemodPSK;
        break;
    case LFRFIDWorkerReadDone:
        reader->decoded = protocol;
        reader->stage = BadgeStageDecoded;
        break;
    default:
        break;
    }
    furi_mutex_release(reader->lock);
}

BadgeReader* badge_reader_alloc(void) {
    BadgeReader* reader = malloc(sizeof(BadgeReader));
    memset(reader, 0, sizeof(BadgeReader));

    reader->dict = protocol_dict_alloc(lfrfid_protocols, LFRFIDProtocolMax);
    reader->worker = lfrfid_worker_alloc(reader->dict);
    reader->lock = furi_mutex_alloc(FuriMutexTypeNormal);
    reader->stage = BadgeStageIdle;
    reader->decoded = PROTOCOL_NO;
    return reader;
}

void badge_reader_free(BadgeReader* reader) {
    furi_assert(reader);
    badge_reader_stop(reader);
    lfrfid_worker_free(reader->worker);
    protocol_dict_free(reader->dict);
    furi_mutex_free(reader->lock);
    free(reader);
}

void badge_reader_start(BadgeReader* reader, BadgeMode mode) {
    furi_assert(reader);
    if(reader->reading) return;

    furi_mutex_acquire(reader->lock, FuriWaitForever);
    reader->stage = BadgeStageSensing;
    reader->decoded = PROTOCOL_NO;
    furi_mutex_release(reader->lock);

    if(!reader->thread_running) {
        lfrfid_worker_start_thread(reader->worker);
        reader->thread_running = true;
    }

    LFRFIDWorkerReadType type;
    switch(mode) {
    case BadgeModeASK:
        type = LFRFIDWorkerReadTypeASKOnly;
        break;
    case BadgeModePSK:
        type = LFRFIDWorkerReadTypePSKOnly;
        break;
    default:
        type = LFRFIDWorkerReadTypeAuto;
        break;
    }

    lfrfid_worker_read_start(reader->worker, type, badge_reader_cb, reader);
    reader->reading = true;
}

void badge_reader_stop(BadgeReader* reader) {
    furi_assert(reader);
    if(reader->reading) {
        lfrfid_worker_stop(reader->worker);
        reader->reading = false;
    }
    if(reader->thread_running) {
        lfrfid_worker_stop_thread(reader->worker);
        reader->thread_running = false;
    }
    furi_mutex_acquire(reader->lock, FuriWaitForever);
    reader->stage = BadgeStageIdle;
    furi_mutex_release(reader->lock);
}

BadgeStage badge_reader_stage(BadgeReader* reader) {
    furi_assert(reader);
    furi_mutex_acquire(reader->lock, FuriWaitForever);
    BadgeStage stage = reader->stage;
    furi_mutex_release(reader->lock);
    return stage;
}

bool badge_reader_take(BadgeReader* reader, BadgeCapture* out, FuriString* rendered) {
    furi_assert(reader);
    furi_assert(out);

    furi_mutex_acquire(reader->lock, FuriWaitForever);
    const bool ready = (reader->stage == BadgeStageDecoded);
    const ProtocolId protocol = reader->decoded;
    if(ready) reader->decoded = PROTOCOL_NO; /* take() succeeds once per read */
    furi_mutex_release(reader->lock);

    if(!ready || protocol == PROTOCOL_NO || protocol >= LFRFIDProtocolMax) return false;

    /* Silence the worker before touching the dictionary: the decoders write
     * into those same buffers, and a read mid-update would produce an ID that
     * belongs to no card at all. */
    badge_reader_stop(reader);

    memset(out, 0, sizeof(*out));
    LfReading* r = &out->reading;
    r->proto = (LfProto)protocol;

    /* The grader knows each format's real carrier. The firmware only tracks two
     * demodulators (ASK and PSK) and runs the FSK formats through the ASK path,
     * so reporting what the demodulator did would label HID prox as ASK and
     * wrongly accuse it of being readable by the cheapest cloners. Leave it
     * unknown and let the grader's own table answer. */
    r->mod = LfModUnknown;

    const size_t size = protocol_dict_get_data_size(reader->dict, protocol);
    r->data_len = (uint8_t)(size > BST_MAX_DATA ? BST_MAX_DATA : size);
    if(r->data_len > 0) {
        protocol_dict_get_data(reader->dict, protocol, r->data, r->data_len);
    }
    r->validate_count = protocol_dict_get_validate_count(reader->dict, protocol);

    const char* name = protocol_dict_get_name(reader->dict, protocol);
    const char* manufacturer = protocol_dict_get_manufacturer(reader->dict, protocol);
    snprintf(out->fw_name, sizeof(out->fw_name), "%s", name ? name : "");
    snprintf(out->manufacturer, sizeof(out->manufacturer), "%s", manufacturer ? manufacturer : "");

    if(rendered) {
        furi_string_reset(rendered);
        protocol_dict_render_data(reader->dict, rendered, protocol);
    }
    return true;
}
