#include "../bastion_i.h"

void bastion_scene_report_on_enter(void* context) {
    BastionApp* app = context;
    Widget* widget = app->widget;
    widget_reset(widget);

    const LfGrade* g = &app->grade;
    const BadgeCapture* c = &app->capture;

    FuriString* s = furi_string_alloc();

    furi_string_cat_printf(s, "\e#%s\n", g->name);
    if(g->scored) {
        furi_string_cat_printf(
            s, "Grade %s   %d/100   %s\n", g->letter, g->score, lf_band_label(g->band));
    } else {
        furi_string_cat_printf(s, "Not graded   %s\n", lf_band_label(g->band));
    }
    furi_string_cat_printf(s, "%s\n\n", g->headline);
    furi_string_cat_printf(s, "%s\n\n", lf_band_blurb(g->band));

    /* --- findings --- */
    furi_string_cat_str(s, "\e#Findings\n");
    for(uint8_t i = 0; i < g->finding_num; i++) {
        furi_string_cat_printf(
            s, "%s %s\n", lf_severity_glyph(g->findings[i].sev), g->findings[i].text);
    }

    /* --- how the number was reached --- *
     * Shown in full because a grade nobody can check is just an opinion. */
    if(g->scored) {
        furi_string_cat_str(s, "\n\e#Score\n");
        furi_string_cat_printf(s, "Authentication  %u/45\n", (unsigned)g->parts.auth);
        furi_string_cat_printf(s, "Integrity       %u/15\n", (unsigned)g->parts.integrity);
        furi_string_cat_printf(s, "Obfuscation     %u/25\n", (unsigned)g->parts.obscurity);
        furi_string_cat_printf(s, "Key space       %u/15\n", (unsigned)g->parts.keyspace);
        furi_string_cat_printf(s, "Total           %d/100\n", g->score);
        furi_string_cat_str(
            s,
            "\nNo 125 kHz credential scores\n"
            "on authentication. That is the\n"
            "45 points nothing here can win.\n");
    }

    /* --- the credential itself --- */
    furi_string_cat_str(s, "\n\e#This badge\n");
    if(c->fw_name[0] != '\0') furi_string_cat_printf(s, "Format: %s\n", c->fw_name);
    if(c->manufacturer[0] != '\0') furi_string_cat_printf(s, "Vendor: %s\n", c->manufacturer);
    furi_string_cat_printf(s, "Family: %s\n", g->family);
    furi_string_cat_printf(s, "ID: %s\n", g->id_line);
    if(g->id_bits > 0) furi_string_cat_printf(s, "Carries: %u bits\n", (unsigned)g->id_bits);
    if(g->guess_bits > 0) {
        furi_string_cat_printf(
            s, "Left to guess on-site: %u bits\n", (unsigned)g->guess_bits);
    }
    if(c->reading.validate_count > 0) {
        furi_string_cat_printf(
            s, "Confirmed by %lu reads\n", (unsigned long)c->reading.validate_count);
    }
    if(c->reading.data_len > 0) {
        furi_string_cat_str(s, "Raw:");
        for(uint8_t i = 0; i < c->reading.data_len; i++) {
            furi_string_cat_printf(s, " %02X", c->reading.data[i]);
        }
        furi_string_cat_str(s, "\n");
    }

    /* The decoder's own field breakdown - facility codes, card numbers and
     * whatever else this particular format carries. */
    if(app->decoded_fields && !furi_string_empty(app->decoded_fields)) {
        furi_string_cat_str(s, "\n\e#Decoded fields\n");
        furi_string_cat(s, app->decoded_fields);
        furi_string_cat_str(s, "\n");
    }

    /* --- attacker cost --- */
    if(g->clone != LfCloneNotAKey && g->clone != LfCloneUnknown) {
        furi_string_cat_str(s, "\n\e#Cost to copy\n");
        furi_string_cat_printf(s, "%s with %s\n", lf_clone_time(g->clone), lf_clone_label(g->clone));
        furi_string_cat_str(
            s,
            "Bastion does not copy anything.\n"
            "It only reads, and tells you what\n"
            "a reader already learns.\n");
    }

    furi_string_cat_printf(s, "\n\e#Verdict\n%s\n", g->verdict);

    widget_add_text_scroll_element(widget, 0, 0, 128, 64, furi_string_get_cstr(s));
    furi_string_free(s);

    view_dispatcher_switch_to_view(app->view_dispatcher, BastionViewWidget);
}

bool bastion_scene_report_on_event(void* context, SceneManagerEvent event) {
    UNUSED(context);
    UNUSED(event);
    return false;
}

void bastion_scene_report_on_exit(void* context) {
    BastionApp* app = context;
    widget_reset(app->widget);
}
