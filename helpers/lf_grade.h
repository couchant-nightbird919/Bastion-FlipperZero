/**
 * Bastion's brain: turn a decoded 125 kHz credential into a security grade.
 *
 * Deliberately free of every Flipper header. The grade is the whole product,
 * so it is compiled for the host and pinned by tests (see test/), and that only
 * works if this file depends on nothing but the C standard library.
 *
 * The honest headline first: **no 125 kHz credential the Flipper can read has
 * authentication.** Every one of them answers any reader in range with the same
 * fixed number, forever. So the auth term - worth 45 of the 100 points - is
 * zero across the board, and nothing here can score above 55. That is not a
 * quirk of the scale; it is the finding. What still varies, and what Bastion
 * measures, is *how much worse than that* a given format is.
 */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BST_MAX_FINDINGS 7u
#define BST_MAX_DATA     16u

/* Mirrors LFRFIDProtocol from the firmware, entry for entry, so the reader can
 * map one to the other with a cast. bst_proto_from_lfrfid() guards the join and
 * a static assert in badge_reader.c pins LfProtoUnknown == LFRFIDProtocolMax. */
typedef enum {
    LfProtoEM4100 = 0,
    LfProtoEM410032,
    LfProtoEM410016,
    LfProtoElectra,
    LfProtoH10301,
    LfProtoIdteck,
    LfProtoIndala26,
    LfProtoIOProxXSF,
    LfProtoAwid,
    LfProtoFDXA,
    LfProtoFDXB,
    LfProtoHidGeneric,
    LfProtoHidExGeneric,
    LfProtoPyramid,
    LfProtoViking,
    LfProtoJablotron,
    LfProtoParadox,
    LfProtoPACStanley,
    LfProtoKeri,
    LfProtoGallagher,
    LfProtoNexwatch,
    LfProtoSecurakey,
    LfProtoGProxII,
    LfProtoNoralsy,

    LfProtoUnknown, /* nothing decoded; must equal LFRFIDProtocolMax */
    LfProtoCount,
} LfProto;

/** How the tag talks. Matters because it decides *who* can clone it. */
typedef enum {
    LfModASK, /* amplitude - every bargain-bin cloner speaks this */
    LfModFSK,
    LfModPSK,
    LfModUnknown,
} LfMod;

/** What it costs an attacker to make a working copy. */
typedef enum {
    LfCloneInstant, /* a handheld cloner off a marketplace, one button */
    LfCloneQuick, /* needs FSK/PSK demodulation: Flipper, Proxmark */
    LfCloneTooled, /* needs a tool that understands the encoding */
    LfCloneNotAKey, /* not an access credential at all */
    LfCloneUnknown,
} LfCloneClass;

/** The verdict band - this is what varies, and what you compare badges by. */
typedef enum {
    LfBandBroadcast, /* plaintext fixed ID, no obfuscation worth the word */
    LfBandCloneable, /* structured, still plaintext on the wire */
    LfBandObscured, /* proprietary encoding: obscurity, not security */
    LfBandNotAKey, /* animal / livestock transponder */
    LfBandUnread, /* no decode */
    LfBandCount,
} LfBand;

typedef enum {
    LfFindCritical, /* [x] a break, not a nitpick */
    LfFindWarn, /* [!] a real weakness */
    LfFindGood, /* [+] a genuine (relative) strength */
    LfFindInfo, /* [i] neutral fact about this card */
} LfFindSeverity;

typedef struct {
    LfFindSeverity sev;
    char text[62];
} LfFinding;

/** The four terms the score is built from. Exposed so the tests can pin them
 *  individually instead of only checking the total. */
typedef struct {
    uint8_t auth; /*  0..45  proves itself to the reader?      always 0 at LF */
    uint8_t integrity; /*  0..15  can a mangled/forged ID be detected?           */
    uint8_t obscurity; /*  0..25  does the payload need a decoder?               */
    uint8_t keyspace; /*  0..15  how much is left to guess inside one site?     */
} LfScoreParts;

/** What the reader hands the grader. Pure data - no Flipper types. */
typedef struct {
    LfProto proto;
    uint8_t data[BST_MAX_DATA];
    uint8_t data_len;
    LfMod mod; /* observed during the read, not assumed */
    uint32_t validate_count; /* how many agreeing repeats the decoder saw */
} LfReading;

typedef struct {
    int score; /* 0..100 - see the note at the top about the ceiling */
    bool scored; /* false for animal tags: a number would be a lie */
    char letter[4]; /* "A+".."F", or "-" when !scored */
    LfBand band;
    LfCloneClass clone;
    LfScoreParts parts;

    char name[34]; /* "HID Prox H10301 26-bit" */
    char family[22]; /* "HID Prox" */
    char headline[54]; /* one line, sits under the grade on the report */
    char id_line[40]; /* "FC 45 - Card 12345", or hex */

    uint16_t id_bits; /* bits the format carries */
    uint16_t guess_bits; /* bits an attacker still has to guess on-site */

    char verdict[640];
    LfFinding findings[BST_MAX_FINDINGS];
    uint8_t finding_num;
} LfGrade;

/** The one entry point. `out` is fully written; nothing is left stale. */
void lf_grade_evaluate(const LfReading* reading, LfGrade* out);

const char* lf_band_label(LfBand band); /* "BROADCAST" .. "UNREAD" */
const char* lf_band_blurb(LfBand band); /* one sentence for the report */
const char* lf_clone_time(LfCloneClass c); /* "~2 s" */
const char* lf_clone_label(LfCloneClass c); /* "Any handheld cloner" - report */
const char* lf_clone_short(LfCloneClass c); /* "Cheap cloner" - the 74 px column */
const char* lf_mod_label(LfMod mod); /* "ASK" / "FSK" / "PSK" / "?" */
const char* lf_severity_glyph(LfFindSeverity sev); /* "[x]" "[!]" "[+]" "[i]" */
const char* lf_proto_name(LfProto proto); /* display name, safe for any input */

/** Letter for a raw score, on the same scale Warden uses for 13.56 MHz cards -
 *  so an F here and an F there mean the same thing. */
const char* lf_score_letter(int score);

#ifdef __cplusplus
}
#endif
