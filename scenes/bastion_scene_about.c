#include "../bastion_i.h"

void bastion_scene_about_on_enter(void* context) {
    BastionApp* app = context;
    Widget* widget = app->widget;
    widget_reset(widget);

    FuriString* s = furi_string_alloc();
    furi_string_cat_str(s, "\e#Bastion " BASTION_VERSION "\n");
    furi_string_cat_str(s, "125 kHz badge grader\n\n");
    furi_string_cat_str(
        s,
        "Hold a 125 kHz badge flat to the\n"
        "back of the Flipper. Bastion reads\n"
        "the format and hands back a plain-\n"
        "English security grade.\n\n");

    furi_string_cat_str(s, "\e#Everything here is an F\n");
    furi_string_cat_str(
        s,
        "That is the finding, not a bug.\n"
        "No 125 kHz credential the Flipper\n"
        "can read has authentication. Every\n"
        "one of them answers any reader in\n"
        "range with the same fixed number,\n"
        "forever. Authentication is worth 45\n"
        "of the 100 points, so nothing at\n"
        "this frequency can pass.\n\n"
        "What still varies - and what you\n"
        "compare badges by - is the score,\n"
        "the band and what a copy costs.\n\n");

    furi_string_cat_str(s, "\e#The four terms\n");
    furi_string_cat_str(
        s,
        "Auth      0/45  proves itself?\n"
        "Integrity 0-15  forgery detected?\n"
        "Obfusc.   0-25  needs a decoder?\n"
        "Keyspace  0-15  left to guess?\n\n");

    furi_string_cat_str(s, "\e#The bands\n");
    furi_string_cat_str(
        s,
        "BROADCAST  plaintext fixed ID\n"
        "CLONEABLE  structured, still plain\n"
        "OBSCURED   proprietary encoding\n"
        "NOT A KEY  animal transponder\n\n");

    furi_string_cat_str(s, "\e#Worth knowing\n");
    furi_string_cat_str(
        s,
        "The only 125 kHz technology with\n"
        "real cryptography is Hitag2 - and\n"
        "its cipher is broken too. There is\n"
        "no secure option at this frequency.\n"
        "The fix is 13.56 MHz: DESFire, Seos\n"
        "or iCLASS SE. Grade those with\n"
        "Warden, Bastion's sibling.\n\n");

    furi_string_cat_str(s, "\e#Read-only, always\n");
    furi_string_cat_str(
        s,
        "Bastion uses the firmware's read\n"
        "path and nothing else. It never\n"
        "writes a blank, never emulates your\n"
        "badge, and leaves the tag exactly\n"
        "as it found it. It tells you what\n"
        "any reader already learns.\n\n");

    furi_string_cat_str(s, "\e#Ethics\n");
    furi_string_cat_str(
        s,
        "Grade badges you own or are\n"
        "authorised to test. Know your own\n"
        "doors before someone else does.\n\n");

    furi_string_cat_printf(s, "Log: %s\n\n", bst_store_log_path());
    furi_string_cat_str(s, "by at0m-b0mb\n");
    furi_string_cat_str(s, "github.com/at0m-b0mb/\nBastion-FlipperZero\n");

    widget_add_text_scroll_element(widget, 0, 0, 128, 64, furi_string_get_cstr(s));
    furi_string_free(s);

    view_dispatcher_switch_to_view(app->view_dispatcher, BastionViewWidget);
}

bool bastion_scene_about_on_event(void* context, SceneManagerEvent event) {
    UNUSED(context);
    UNUSED(event);
    return false;
}

void bastion_scene_about_on_exit(void* context) {
    BastionApp* app = context;
    widget_reset(app->widget);
}
