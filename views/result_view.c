#include "result_view.h"
#include <gui/elements.h>
#include <furi.h>
#include <string.h>

/* Layout, 128x64. tools_gen_mockups.py mirrors these constants, so the README
 * screenshots stay honest when a row moves. */
#define RV_NAME_BASE  9 /* FontPrimary baseline, card name */
#define RV_RULE_Y     11
#define RV_BAND_Y     13 /* inverted band bar */
#define RV_BAND_H     12
#define RV_SCORE_BASE 44 /* FontBigNumbers baseline */
#define RV_INFO_X     52 /* right-hand column: clone cost */
#define RV_INFO_B1    33
#define RV_INFO_B2    43
#define RV_BARS_Y     46 /* the credential, drawn as the barcode it is */
#define RV_BARS_H     8
#define RV_FOOT_Y     55
#define RV_FOOT_BASE  62

struct ResultView {
    View* view;
    ResultViewCallback cb;
    void* ctx;
};

typedef struct {
    LfGrade grade;
    uint8_t data[BST_MAX_DATA];
    uint8_t data_len;
    bool has;
} ResultModel;

/* Truncate a copy of `src` so it fits within `max_w` px in the current font.
 *
 * Shortens in place: each pass moves the ".." one character left, and since it
 * only ever writes at or after the cut, the surviving prefix is never touched.
 * Working inside `out` (rather than a scratch buffer) keeps the result bounded
 * by the caller's buffer no matter how long the source is. */
static void fit_text(Canvas* canvas, const char* src, int max_w, char* out, size_t out_sz) {
    if(out_sz == 0) return;
    snprintf(out, out_sz, "%s", src);
    if(canvas_string_width(canvas, out) <= max_w) return;
    if(out_sz < 4) {
        out[0] = '\0';
        return;
    }

    size_t keep = strlen(out);
    if(keep > out_sz - 3) keep = out_sz - 3;
    while(keep > 0) {
        keep--;
        out[keep] = '.';
        out[keep + 1] = '.';
        out[keep + 2] = '\0';
        if(canvas_string_width(canvas, out) <= max_w) return;
    }
}

/* The credential's bits, as bars. A 125 kHz badge carries a number in the clear
 * and nothing else; drawing it as a barcode is not a metaphor. */
static void rv_draw_bits(Canvas* canvas, const uint8_t* data, uint8_t len) {
    canvas_draw_frame(canvas, 2, RV_BARS_Y, 124, RV_BARS_H);

    const int ix = 4;
    const int iy = RV_BARS_Y + 2;
    const int iw = 120;
    const int ih = RV_BARS_H - 4;

    if(len == 0) {
        /* No payload (unread, or a format with no data): a dashed rail, so the
         * strip never looks like an all-zero identifier. */
        for(int x = ix; x < ix + iw; x += 4) {
            canvas_draw_line(canvas, x, iy + ih / 2, x + 1, iy + ih / 2);
        }
        return;
    }

    int bits = (int)len * 8;
    if(bits > 60) bits = 60; /* past this the bars merge into a smear */
    int step = iw / bits;
    if(step < 1) step = 1;
    const int bw = (step > 2) ? step - 1 : 1;

    for(int i = 0; i < bits; i++) {
        const bool on = (data[i / 8] >> (7 - (i % 8))) & 1u;
        if(on) canvas_draw_box(canvas, ix + i * step, iy, bw, ih);
    }
}

static void result_view_draw(Canvas* canvas, void* model) {
    ResultModel* m = model;
    canvas_clear(canvas);
    if(!m->has) return;
    const LfGrade* g = &m->grade;

    /* --- what it is --- */
    canvas_set_font(canvas, FontPrimary);
    char name[40];
    fit_text(canvas, g->name, 124, name, sizeof(name));
    canvas_draw_str(canvas, 2, RV_NAME_BASE, name);
    canvas_draw_line(canvas, 0, RV_RULE_Y, 127, RV_RULE_Y);

    /* --- band bar: the letter on the left, the verdict word across it --- */
    canvas_draw_rbox(canvas, 0, RV_BAND_Y, 128, RV_BAND_H, 2);
    canvas_set_color(canvas, ColorWhite);
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 4, RV_BAND_Y + 10, g->letter);
    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str_aligned(
        canvas, 76, RV_BAND_Y + 6, AlignCenter, AlignCenter, lf_band_label(g->band));
    canvas_set_color(canvas, ColorBlack);

    /* --- the score --- */
    canvas_set_font(canvas, FontBigNumbers);
    if(g->scored) {
        char sc[8];
        snprintf(sc, sizeof(sc), "%d", g->score);
        canvas_draw_str(canvas, 3, RV_SCORE_BASE, sc);
        const int nw = canvas_string_width(canvas, sc);
        canvas_set_font(canvas, FontSecondary);
        canvas_draw_str(canvas, 3 + nw + 2, RV_SCORE_BASE - 1, "/100");
    } else {
        /* Animal tags and failed reads get no number: inventing one would be
         * the only dishonest thing on this screen. */
        canvas_draw_str(canvas, 3, RV_SCORE_BASE, "--");
    }

    /* --- what a copy costs --- */
    canvas_set_font(canvas, FontSecondary);
    char line[24];
    if(g->clone == LfCloneUnknown) {
        /* Nothing decoded: a clone cost here would be meaningless, so point at
         * the report, which carries the placement advice. */
        canvas_draw_str(canvas, RV_INFO_X, RV_INFO_B1, "Nothing read");
        canvas_draw_str(canvas, RV_INFO_X, RV_INFO_B2, "OK for help");
    } else {
        snprintf(line, sizeof(line), "CLONE  %s", lf_clone_time(g->clone));
        canvas_draw_str(canvas, RV_INFO_X, RV_INFO_B1, line);
        fit_text(canvas, lf_clone_short(g->clone), 126 - RV_INFO_X, line, sizeof(line));
        canvas_draw_str(canvas, RV_INFO_X, RV_INFO_B2, line);
    }

    rv_draw_bits(canvas, m->data, m->data_len);

    /* --- footer --- */
    canvas_draw_box(canvas, 0, RV_FOOT_Y, 128, 64 - RV_FOOT_Y);
    canvas_set_color(canvas, ColorWhite);
    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str(canvas, 3, RV_FOOT_BASE, "OK Report");
    canvas_draw_str_aligned(canvas, 125, RV_FOOT_BASE, AlignRight, AlignBottom, "Rescan >");
    canvas_set_color(canvas, ColorBlack);
}

static bool result_view_input(InputEvent* event, void* context) {
    ResultView* v = context;
    if(event->type != InputTypeShort) return false;

    if(event->key == InputKeyOk) {
        if(v->cb) v->cb(v->ctx, ResultEventReport);
        return true;
    }
    if(event->key == InputKeyRight) {
        if(v->cb) v->cb(v->ctx, ResultEventRescan);
        return true;
    }
    return false; // Back falls through to navigation
}

ResultView* result_view_alloc(void) {
    ResultView* v = malloc(sizeof(ResultView));
    v->cb = NULL;
    v->ctx = NULL;
    v->view = view_alloc();
    view_set_context(v->view, v);
    view_allocate_model(v->view, ViewModelTypeLocking, sizeof(ResultModel));
    view_set_draw_callback(v->view, result_view_draw);
    view_set_input_callback(v->view, result_view_input);
    return v;
}

void result_view_free(ResultView* v) {
    furi_assert(v);
    view_free(v->view);
    free(v);
}

View* result_view_get_view(ResultView* v) {
    furi_assert(v);
    return v->view;
}

void result_view_set_callback(ResultView* v, ResultViewCallback cb, void* context) {
    furi_assert(v);
    v->cb = cb;
    v->ctx = context;
}

void result_view_set_result(ResultView* v, const LfGrade* grade, const LfReading* reading) {
    furi_assert(v);
    furi_assert(grade);
    with_view_model(
        v->view,
        ResultModel * m,
        {
            m->grade = *grade;
            m->data_len = 0;
            if(reading) {
                m->data_len = (reading->data_len > BST_MAX_DATA) ? BST_MAX_DATA :
                                                                   reading->data_len;
                memcpy(m->data, reading->data, m->data_len);
            }
            m->has = true;
        },
        true);
}
