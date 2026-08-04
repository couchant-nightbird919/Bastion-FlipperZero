#include "../bastion_i.h"

/* The last 20 graded badges, newest first. Auditing a site means walking it
 * with a pocketful of credentials, and nobody remembers the sixth one. */
#define BST_LOG_SHOWN 20

void bastion_scene_log_on_enter(void* context) {
    BastionApp* app = context;
    Widget* widget = app->widget;
    widget_reset(widget);

    FuriString* s = furi_string_alloc();
    furi_string_cat_str(s, "\e#Badge Log\n");

    const uint8_t shown = bst_store_log_render(s, BST_LOG_SHOWN);
    if(shown == 0) {
        furi_string_cat_str(
            s,
            "\nNothing logged yet.\n\n"
            "Grade a badge with logging on\n"
            "and it lands here, newest first.\n\n"
            "The full history is a CSV on the\n"
            "SD card, so a site survey can be\n"
            "opened in a spreadsheet:\n");
        furi_string_cat_printf(s, "%s\n", bst_store_log_path());
    } else {
        furi_string_cat_printf(s, "\e#Showing %u\e#\n", (unsigned)shown);
        furi_string_cat_printf(s, "Full history: %s\n", bst_store_log_path());
    }

    widget_add_text_scroll_element(widget, 0, 0, 128, 64, furi_string_get_cstr(s));
    furi_string_free(s);

    view_dispatcher_switch_to_view(app->view_dispatcher, BastionViewWidget);
}

bool bastion_scene_log_on_event(void* context, SceneManagerEvent event) {
    UNUSED(context);
    UNUSED(event);
    return false;
}

void bastion_scene_log_on_exit(void* context) {
    BastionApp* app = context;
    widget_reset(app->widget);
}
