/*
 * Host tests for the Bastion grading engine.
 *
 * The grade is the entire product, and it is the one part a screenshot cannot
 * vouch for. This builds the real helpers/lf_grade.c - no stubs, no second copy
 * of the logic - so any edit that moves a score, a band boundary or a letter
 * has to be a deliberate one that also updates this file.
 *
 *   make -C test
 */
#include "helpers/lf_grade.h"

#include <stdio.h>
#include <string.h>

static int failures = 0;
static int checks = 0;

static void fail(const char* fmt, ...) __attribute__((format(printf, 1, 2)));

#include <stdarg.h>
static void fail(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    printf("  FAIL  ");
    vprintf(fmt, ap);
    printf("\n");
    va_end(ap);
    failures++;
}

static void expect_int(int got, int want, const char* what) {
    checks++;
    if(got != want) fail("%s: got %d, expected %d", what, got, want);
}

static void expect_str(const char* got, const char* want, const char* what) {
    checks++;
    if(strcmp(got, want) != 0) fail("%s: got \"%s\", expected \"%s\"", what, got, want);
}

static void expect_true(bool cond, const char* what) {
    checks++;
    if(!cond) fail("%s", what);
}

/** Grade a bare protocol with no decoded payload. */
static LfGrade grade_of(LfProto p) {
    LfReading r;
    memset(&r, 0, sizeof(r));
    r.proto = p;
    r.mod = LfModUnknown; /* let the engine use the format's usual carrier */
    LfGrade g;
    lf_grade_evaluate(&r, &g);
    return g;
}

/* Every protocol's expected total, pinned. These are the numbers printed on the
 * README's grade table and shown on the device, so they are part of the
 * product's contract, not an implementation detail. */
typedef struct {
    LfProto proto;
    int score;
    const char* letter;
    LfBand band;
    LfCloneClass clone;
} Expect;

static const Expect expected[] = {
    /* Broadcast band: plaintext ID, nothing worth calling obfuscation. */
    {LfProtoEM410016, 8, "F", LfBandBroadcast, LfCloneInstant},
    {LfProtoH10301, 9, "F", LfBandBroadcast, LfCloneQuick},
    {LfProtoAwid, 12, "F", LfBandBroadcast, LfCloneQuick},
    {LfProtoEM4100, 13, "F", LfBandBroadcast, LfCloneInstant},
    {LfProtoEM410032, 13, "F", LfBandBroadcast, LfCloneInstant},
    {LfProtoHidGeneric, 13, "F", LfBandBroadcast, LfCloneQuick},
    {LfProtoSecurakey, 13, "F", LfBandBroadcast, LfCloneInstant},
    {LfProtoElectra, 14, "F", LfBandBroadcast, LfCloneInstant},
    {LfProtoHidExGeneric, 14, "F", LfBandBroadcast, LfCloneQuick},
    {LfProtoPyramid, 13, "F", LfBandBroadcast, LfCloneQuick},

    /* Cloneable band: structured, still plaintext on the wire. */
    {LfProtoIOProxXSF, 15, "F", LfBandCloneable, LfCloneQuick},
    {LfProtoIdteck, 16, "F", LfBandCloneable, LfCloneInstant},
    {LfProtoPACStanley, 16, "F", LfBandCloneable, LfCloneInstant},
    {LfProtoIndala26, 18, "F", LfBandCloneable, LfCloneQuick},
    {LfProtoNoralsy, 18, "F", LfBandCloneable, LfCloneInstant},
    {LfProtoViking, 19, "F", LfBandCloneable, LfCloneInstant},
    {LfProtoJablotron, 20, "F", LfBandCloneable, LfCloneInstant},
    {LfProtoParadox, 20, "F", LfBandCloneable, LfCloneQuick},
    {LfProtoKeri, 23, "F", LfBandCloneable, LfCloneTooled},

    /* Obscured band: the payload needs a decoder that knows the format. */
    {LfProtoGProxII, 28, "F", LfBandObscured, LfCloneTooled},
    {LfProtoNexwatch, 35, "D", LfBandObscured, LfCloneTooled},
    {LfProtoGallagher, 38, "D", LfBandObscured, LfCloneTooled},
};

int main(void) {
    printf("Bastion grading engine\n");

    /* ---- every protocol's score, letter, band and clone class ---- */
    printf("- per-protocol grades\n");
    for(size_t i = 0; i < sizeof(expected) / sizeof(expected[0]); i++) {
        const Expect* e = &expected[i];
        LfGrade g = grade_of(e->proto);
        char what[96];

        snprintf(what, sizeof(what), "%s score", g.name);
        expect_int(g.score, e->score, what);

        snprintf(what, sizeof(what), "%s letter", g.name);
        expect_str(g.letter, e->letter, what);

        snprintf(what, sizeof(what), "%s band", g.name);
        expect_int((int)g.band, (int)e->band, what);

        snprintf(what, sizeof(what), "%s clone class", g.name);
        expect_int((int)g.clone, (int)e->clone, what);

        snprintf(what, sizeof(what), "%s is scored", g.name);
        expect_true(g.scored, what);

        /* The score must be exactly the sum of its published parts - the
         * report shows the breakdown, so it has to add up on screen. */
        snprintf(what, sizeof(what), "%s parts sum", g.name);
        expect_int(
            (int)g.parts.auth + (int)g.parts.integrity + (int)g.parts.obscurity +
                (int)g.parts.keyspace,
            g.score,
            what);

        /* Nothing at 125 kHz authenticates. If this ever fires, either a real
         * challenge-response format arrived or someone padded the score. */
        snprintf(what, sizeof(what), "%s auth term is zero", g.name);
        expect_int((int)g.parts.auth, 0, what);
    }

    /* The table above must cover every access protocol; a new one added to the
     * enum without a pinned grade should fail here rather than ship unchecked. */
    printf("- coverage\n");
    expect_int(
        (int)(sizeof(expected) / sizeof(expected[0])),
        (int)LfProtoCount - 3, /* minus FDX-A, FDX-B and Unknown */
        "expectation table covers every access protocol");
    for(int p = 0; p < (int)LfProtoCount; p++) {
        if(p == LfProtoFDXA || p == LfProtoFDXB || p == LfProtoUnknown) continue;
        bool found = false;
        for(size_t i = 0; i < sizeof(expected) / sizeof(expected[0]); i++) {
            if((int)expected[i].proto == p) found = true;
        }
        checks++;
        if(!found) fail("protocol %d (%s) has no pinned grade", p, lf_proto_name((LfProto)p));
    }

    /* ---- band boundaries, from both sides ---- */
    printf("- band boundaries\n");
    /* Boundaries live in the engine; probe them through the published scale. */
    expect_str(lf_score_letter(90), "A+", "letter 90");
    expect_str(lf_score_letter(89), "A", "letter 89");
    expect_str(lf_score_letter(80), "A", "letter 80");
    expect_str(lf_score_letter(79), "B", "letter 79");
    expect_str(lf_score_letter(65), "B", "letter 65");
    expect_str(lf_score_letter(64), "C", "letter 64");
    expect_str(lf_score_letter(50), "C", "letter 50");
    expect_str(lf_score_letter(49), "D", "letter 49");
    expect_str(lf_score_letter(35), "D", "letter 35");
    expect_str(lf_score_letter(34), "F", "letter 34");
    expect_str(lf_score_letter(0), "F", "letter 0");
    expect_str(lf_score_letter(-5), "F", "letter negative");
    expect_str(lf_score_letter(1000), "A+", "letter over 100");

    /* Gallagher is the best LF format Bastion can read; if anything ever beats
     * it, the "no 125 kHz card exceeds 55" claim in the README needs revisiting. */
    printf("- ceiling\n");
    int best = 0;
    for(int p = 0; p < (int)LfProtoCount; p++) {
        LfGrade g = grade_of((LfProto)p);
        if(g.scored && g.score > best) best = g.score;
    }
    expect_int(best, 38, "best LF score is Gallagher's");
    checks++;
    if(best >= 50) fail("an LF format reached grade C - the scale claims that is impossible");

    /* ---- animal tags are not graded ---- */
    printf("- animal transponders\n");
    for(int p = LfProtoFDXA; p <= LfProtoFDXB; p++) {
        LfGrade g = grade_of((LfProto)p);
        char what[96];
        snprintf(what, sizeof(what), "%s not scored", g.name);
        expect_true(!g.scored, what);
        snprintf(what, sizeof(what), "%s band", g.name);
        expect_int((int)g.band, (int)LfBandNotAKey, what);
        snprintf(what, sizeof(what), "%s letter", g.name);
        expect_str(g.letter, "-", what);
        snprintf(what, sizeof(what), "%s clone class", g.name);
        expect_int((int)g.clone, (int)LfCloneNotAKey, what);
        /* No "no authentication" finding: it would be nonsense on a pet chip. */
        for(uint8_t i = 0; i < g.finding_num; i++) {
            checks++;
            if(strstr(g.findings[i].text, "No authentication")) {
                fail("%s carries an access-control finding", g.name);
            }
        }
    }

    /* ---- no decode ---- */
    printf("- unread\n");
    {
        LfGrade g = grade_of(LfProtoUnknown);
        expect_true(!g.scored, "unknown not scored");
        expect_int((int)g.band, (int)LfBandUnread, "unknown band");
        expect_str(g.letter, "-", "unknown letter");
        expect_str(g.id_line, "no decode", "unknown id line");
        expect_true(g.finding_num > 0, "unknown still gives advice");
    }

    /* ---- decoded payloads ---- */
    printf("- HID 26-bit decode\n");
    {
        LfReading r;
        memset(&r, 0, sizeof(r));
        r.proto = LfProtoH10301;
        r.mod = LfModFSK;
        r.data_len = 3;
        r.data[0] = 45; /* facility */
        r.data[1] = 0x30; /* card 12345 = 0x3039 */
        r.data[2] = 0x39;
        LfGrade g;
        lf_grade_evaluate(&r, &g);
        expect_str(g.id_line, "FC 45  Card 12345", "H10301 id line");

        bool site_finding = false;
        for(uint8_t i = 0; i < g.finding_num; i++) {
            if(strstr(g.findings[i].text, "Facility 45")) site_finding = true;
        }
        expect_true(site_finding, "H10301 names the actual facility code");
    }

    printf("- EM4100 decode\n");
    {
        LfReading r;
        memset(&r, 0, sizeof(r));
        r.proto = LfProtoEM4100;
        r.mod = LfModASK;
        r.data_len = 5;
        r.data[0] = 0x12;
        r.data[1] = 0x00;
        r.data[2] = 0x34;
        r.data[3] = 0x56;
        r.data[4] = 0x78;
        LfGrade g;
        lf_grade_evaluate(&r, &g);
        expect_str(g.id_line, "Cust 12  SN 3430008", "EM4100 id line");
    }

    printf("- hex fallback\n");
    {
        LfReading r;
        memset(&r, 0, sizeof(r));
        r.proto = LfProtoViking;
        r.data_len = 4;
        for(int i = 0; i < 4; i++) r.data[i] = (uint8_t)(0xA0 + i);
        LfGrade g;
        lf_grade_evaluate(&r, &g);
        expect_str(g.id_line, "A0 A1 A2 A3", "hex id line");
    }

    printf("- hex clipping\n");
    {
        LfReading r;
        memset(&r, 0, sizeof(r));
        r.proto = LfProtoNexwatch;
        r.data_len = BST_MAX_DATA; /* 16 bytes will not fit in id_line */
        for(unsigned i = 0; i < BST_MAX_DATA; i++) r.data[i] = (uint8_t)i;
        LfGrade g;
        lf_grade_evaluate(&r, &g);
        checks++;
        if(strlen(g.id_line) >= sizeof(g.id_line)) fail("id_line overran");
        /* A clipped ID must say so, not quietly look like a shorter card. */
        expect_true(strstr(g.id_line, "..") != NULL, "clipped hex is marked");
        /* And it must end on a whole byte, never half of one. */
        expect_str(g.id_line, "00 01 02 03 04 05 06 07 08 09 0A 0B ..", "clipped hex content");
    }

    /* ---- robustness: nothing here may read out of bounds ---- */
    printf("- robustness\n");
    {
        LfGrade g;
        lf_grade_evaluate(NULL, &g);
        expect_int((int)g.band, (int)LfBandUnread, "NULL reading is treated as unread");

        LfReading r;
        memset(&r, 0, sizeof(r));
        r.proto = (LfProto)999;
        lf_grade_evaluate(&r, &g);
        expect_int((int)g.band, (int)LfBandUnread, "bogus protocol is treated as unread");

        r.proto = LfProtoEM4100;
        r.data_len = 200; /* longer than the buffer it came from */
        lf_grade_evaluate(&r, &g);
        checks++;
        if(strlen(g.id_line) >= sizeof(g.id_line)) fail("over-long data_len overran id_line");

        lf_grade_evaluate(&r, NULL); /* must not crash */
    }

    /* ---- every string the UI can print must exist and fit ---- */
    printf("- copy integrity\n");
    for(int p = 0; p < (int)LfProtoCount; p++) {
        LfGrade g = grade_of((LfProto)p);
        checks++;
        if(g.name[0] == '\0' || g.family[0] == '\0' || g.headline[0] == '\0' ||
           g.verdict[0] == '\0' || g.letter[0] == '\0' || g.id_line[0] == '\0') {
            fail("protocol %d has empty copy", p);
        }
        checks++;
        if(g.finding_num < 3 || g.finding_num > BST_MAX_FINDINGS) {
            fail("protocol %d has %u findings", p, (unsigned)g.finding_num);
        }
        for(uint8_t i = 0; i < g.finding_num; i++) {
            checks++;
            if(g.findings[i].text[0] == '\0') fail("protocol %d finding %u is empty", p, i);
            checks++;
            /* The report renders findings one per line at 128 px; anything past
             * the buffer would have been silently cut by snprintf. */
            if(strlen(g.findings[i].text) >= sizeof(g.findings[i].text) - 1) {
                fail("protocol %d finding %u fills its buffer (likely truncated)", p, i);
            }
        }
        checks++;
        if(strlen(g.verdict) >= sizeof(g.verdict) - 1) {
            fail("protocol %d verdict fills its buffer (likely truncated)", p);
        }
        checks++;
        if(strlen(g.headline) >= sizeof(g.headline) - 1) {
            fail("protocol %d headline fills its buffer (likely truncated)", p);
        }
    }

    /* ---- label helpers clamp bogus input instead of indexing off the end ---- */
    printf("- label clamping\n");
    expect_str(lf_band_label((LfBand)99), "UNREAD", "band label clamp");
    expect_true(lf_band_blurb((LfBand)99)[0] != '\0', "band blurb clamp");
    expect_str(lf_clone_time((LfCloneClass)99), "?", "clone time clamp");
    expect_str(lf_clone_label((LfCloneClass)99), "Unknown", "clone label clamp");
    expect_str(lf_clone_short((LfCloneClass)99), "Unknown", "clone short clamp");
    /* The verdict screen's clone column is 74 px; FontSecondary averages a
     * shade over 5 px a character, so anything past 14 would be ellipsised. */
    for(int c = 0; c <= (int)LfCloneUnknown; c++) {
        checks++;
        if(strlen(lf_clone_short((LfCloneClass)c)) > 14) {
            fail("clone short %d is too wide for the result column", c);
        }
        expect_true(lf_clone_short((LfCloneClass)c)[0] != '\0', "clone short present");
        expect_true(lf_clone_label((LfCloneClass)c)[0] != '\0', "clone label present");
        expect_true(lf_clone_time((LfCloneClass)c)[0] != '\0', "clone time present");
    }
    expect_str(lf_mod_label((LfMod)99), "?", "mod label clamp");
    expect_str(lf_severity_glyph((LfFindSeverity)99), "[i]", "glyph clamp");
    expect_str(lf_proto_name((LfProto)99), "Unknown 125 kHz tag", "proto name clamp");
    for(int b = 0; b < (int)LfBandCount; b++) {
        expect_true(lf_band_label((LfBand)b)[0] != '\0', "band label present");
        expect_true(lf_band_blurb((LfBand)b)[0] != '\0', "band blurb present");
    }

    /* ---- the modulation the reader observed overrides the format default ---- */
    printf("- observed modulation\n");
    {
        LfReading r;
        memset(&r, 0, sizeof(r));
        r.proto = LfProtoEM4100;
        r.mod = LfModASK;
        LfGrade ask;
        lf_grade_evaluate(&r, &ask);
        expect_int((int)ask.clone, (int)LfCloneInstant, "ASK EM4100 is instant to clone");

        bool cheap_cloner = false;
        for(uint8_t i = 0; i < ask.finding_num; i++) {
            if(strstr(ask.findings[i].text, "cheapest cloners")) cheap_cloner = true;
        }
        expect_true(cheap_cloner, "ASK card warns about cheap cloners");
    }

    printf("\n%d checks, %d failures\n", checks, failures);
    if(failures == 0) printf("OK\n");
    return failures == 0 ? 0 : 1;
}
