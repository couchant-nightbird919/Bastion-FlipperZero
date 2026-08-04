#include "scan_view.h"
#include <gui/elements.h>
#include <furi.h>

/* Layout, 128x64. Kept as named constants because tools_gen_mockups.py mirrors
 * these exact numbers - if a row moves here it moves in the README too. */
#define SV_HEADER_BASE  9 /* FontPrimary baseline for the title row */
#define SV_RULE_Y       11 /* hairline under the header */
#define SV_FIELD_CY     28 /* centre line of the coil / wave / badge band */
#define SV_WAVE_X0      24 /* wave starts clear of the coil */
#define SV_WAVE_X1      96 /* ...and stops clear of the badge */
#define SV_WAVE_AMP_MAX 8
#define SV_STAGE_BASE   53 /* FontSecondary baseline, stage line */
#define SV_HINT_BASE    63 /* FontSecondary baseline, hint line */

struct ScanView {
    View* view;
};

typedef struct {
    uint32_t phase;
    BadgeStage stage;
    BadgeMode mode;
} ScanModel;

/* One period of a sine, 16 samples, scaled to +/-127. Integer only: this runs
 * inside the draw callback on every frame. */
static const int8_t sv_sin[16] =
    {0, 49, 90, 118, 127, 118, 90, 49, 0, -49, -90, -118, -127, -118, -90, -49};

static const char* sv_stage_text(BadgeStage stage) {
    switch(stage) {
    case BadgeStageTagPresent:
        return "Tag in field";
    case BadgeStageDemodASK:
        return "Demodulating ASK";
    case BadgeStageDemodPSK:
        return "Demodulating PSK";
    case BadgeStageDecoded:
        return "Decoded";
    case BadgeStageSensing:
    default:
        return "Sensing";
    }
}

static const char* sv_mode_text(BadgeMode mode) {
    switch(mode) {
    case BadgeModeASK:
        return "ASK";
    case BadgeModePSK:
        return "PSK";
    default:
        return "AUTO";
    }
}

/* The Flipper's LF coil, face on: nested rings around a core. */
static void sv_draw_coil(Canvas* canvas, int cx, int cy) {
    canvas_draw_rframe(canvas, cx - 8, cy - 8, 17, 17, 5);
    canvas_draw_rframe(canvas, cx - 5, cy - 5, 11, 11, 3);
    canvas_draw_box(canvas, cx - 1, cy - 1, 3, 3);
}

/* The badge. Outlined while we are waiting, solid once it loads the coil. */
static void sv_draw_badge(Canvas* canvas, int x, int y, bool energised) {
    if(energised) {
        canvas_draw_rbox(canvas, x, y, 22, 16, 3);
        canvas_set_color(canvas, ColorWhite);
        canvas_draw_box(canvas, x + 4, y + 4, 14, 3);
        canvas_draw_box(canvas, x + 4, y + 9, 9, 3);
        canvas_set_color(canvas, ColorBlack);
    } else {
        canvas_draw_rframe(canvas, x, y, 22, 16, 3);
        canvas_draw_line(canvas, x + 4, y + 5, x + 17, y + 5);
        canvas_draw_line(canvas, x + 4, y + 10, x + 12, y + 10);
    }
}

static void scan_view_draw(Canvas* canvas, void* model) {
    ScanModel* m = model;
    canvas_clear(canvas);

    /* --- header: what we are doing, and which demodulators are armed --- */
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 2, SV_HEADER_BASE, "Read a Badge");
    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str_aligned(
        canvas, 126, SV_HEADER_BASE, AlignRight, AlignBottom, sv_mode_text(m->mode));
    canvas_draw_line(canvas, 0, SV_RULE_Y, 127, SV_RULE_Y);

    /* --- the field: coil -> carrier -> badge --- */
    sv_draw_coil(canvas, 12, SV_FIELD_CY);

    /* 125 kHz travelling between the two. Tapered at both ends so it reads as
     * a beam leaving the coil rather than a line clipped at the margins. */
    const int span = SV_WAVE_X1 - SV_WAVE_X0;
    int prev_y = SV_FIELD_CY;
    for(int x = SV_WAVE_X0; x <= SV_WAVE_X1; x += 2) {
        const int d = x - SV_WAVE_X0;
        const int edge = (d < span - d) ? d : (span - d); /* distance to nearer end */
        int amp = 2 + (SV_WAVE_AMP_MAX - 2) * edge / (span / 4);
        if(amp > SV_WAVE_AMP_MAX) amp = SV_WAVE_AMP_MAX;

        const uint32_t idx = ((uint32_t)(d / 2) + m->phase) & 15u;
        const int y = SV_FIELD_CY - (sv_sin[idx] * amp) / 127;
        if(x > SV_WAVE_X0) canvas_draw_line(canvas, x - 2, prev_y, x, y);
        prev_y = y;
    }

    sv_draw_badge(canvas, 102, SV_FIELD_CY - 8, m->stage >= BadgeStageTagPresent);

    /* --- stage + hint --- */
    canvas_set_font(canvas, FontSecondary);
    char buf[26];
    char dots[4] = {0};
    const int nd = (m->stage == BadgeStageDecoded) ? 0 : (int)((m->phase / 3) % 4);
    for(int i = 0; i < nd; i++) dots[i] = '.';
    snprintf(buf, sizeof(buf), "%s%s", sv_stage_text(m->stage), dots);
    canvas_draw_str_aligned(canvas, 64, SV_STAGE_BASE, AlignCenter, AlignBottom, buf);

    canvas_draw_str_aligned(
        canvas, 64, SV_HINT_BASE, AlignCenter, AlignBottom, "Hold badge flat to the back");
}

static bool scan_view_input(InputEvent* event, void* context) {
    UNUSED(event);
    UNUSED(context);
    return false; // let Back reach the scene manager
}

ScanView* scan_view_alloc(void) {
    ScanView* v = malloc(sizeof(ScanView));
    v->view = view_alloc();
    view_set_context(v->view, v);
    view_allocate_model(v->view, ViewModelTypeLocking, sizeof(ScanModel));
    view_set_draw_callback(v->view, scan_view_draw);
    view_set_input_callback(v->view, scan_view_input);
    return v;
}

void scan_view_free(ScanView* v) {
    furi_assert(v);
    view_free(v->view);
    free(v);
}

View* scan_view_get_view(ScanView* v) {
    furi_assert(v);
    return v->view;
}

void scan_view_reset(ScanView* v) {
    furi_assert(v);
    with_view_model(
        v->view,
        ScanModel * m,
        {
            m->phase = 0;
            m->stage = BadgeStageSensing;
        },
        true);
}

void scan_view_tick(ScanView* v) {
    furi_assert(v);
    with_view_model(v->view, ScanModel * m, { m->phase++; }, true);
}

void scan_view_set_stage(ScanView* v, BadgeStage stage) {
    furi_assert(v);
    with_view_model(v->view, ScanModel * m, { m->stage = stage; }, true);
}

void scan_view_set_mode(ScanView* v, BadgeMode mode) {
    furi_assert(v);
    with_view_model(v->view, ScanModel * m, { m->mode = mode; }, true);
}
