#include "bastion_i.h"
#include <string.h>

/* ---------------------------------------------------------- feedback ----- *
 * Three outcomes, three distinct signals, so the verdict lands before you look
 * at the screen: a flat two-tone for a plaintext broadcast credential, a
 * single amber blip for an obscured one, and a neutral chirp for anything
 * Bastion will not grade. */

static const NotificationSequence seq_broadcast = {
    &message_red_255,
    &message_vibro_on,
    &message_delay_100,
    &message_vibro_off,
    &message_delay_50,
    &message_note_gs4,
    &message_delay_100,
    &message_note_ds4,
    &message_delay_100,
    &message_sound_off,
    &message_red_0,
    NULL,
};

static const NotificationSequence seq_cloneable = {
    &message_red_255,
    &message_delay_100,
    &message_note_e5,
    &message_delay_100,
    &message_sound_off,
    &message_red_0,
    NULL,
};

static const NotificationSequence seq_obscured = {
    &message_red_255,
    &message_green_255, // red + green = amber
    &message_delay_250,
    &message_red_0,
    &message_green_0,
    &message_note_c5,
    &message_delay_50,
    &message_note_g5,
    &message_delay_50,
    &message_sound_off,
    NULL,
};

static const NotificationSequence seq_neutral = {
    &message_blue_255,
    &message_delay_100,
    &message_blue_0,
    &message_note_c5,
    &message_delay_50,
    &message_sound_off,
    NULL,
};

void bastion_notify_graded(BastionApp* app, const LfGrade* grade) {
    furi_assert(app);
    furi_assert(grade);

    const NotificationSequence* seq;
    switch(grade->band) {
    case LfBandBroadcast:
        seq = &seq_broadcast;
        break;
    case LfBandCloneable:
        seq = &seq_cloneable;
        break;
    case LfBandObscured:
        seq = &seq_obscured;
        break;
    default:
        seq = &seq_neutral;
        break;
    }

    /* The sequences mix LED, vibro and tone; if the user turned all three off,
     * stay silent rather than firing a half-muted one. */
    if(app->settings.led || app->settings.sound || app->settings.vibro) {
        notification_message(app->notifications, seq);
    }
}

/* ------------------------------------------------ view dispatcher glue ---- */
static bool bastion_custom_event_callback(void* context, uint32_t event) {
    BastionApp* app = context;
    return scene_manager_handle_custom_event(app->scene_manager, event);
}

static bool bastion_back_event_callback(void* context) {
    BastionApp* app = context;
    return scene_manager_handle_back_event(app->scene_manager);
}

static void bastion_tick_event_callback(void* context) {
    BastionApp* app = context;
    scene_manager_handle_tick_event(app->scene_manager);
}

/* --------------------------------------------------------- lifecycle ----- */
static BastionApp* bastion_app_alloc(void) {
    BastionApp* app = malloc(sizeof(BastionApp));
    memset(app, 0, sizeof(BastionApp));

    app->gui = furi_record_open(RECORD_GUI);
    app->notifications = furi_record_open(RECORD_NOTIFICATION);

    app->view_dispatcher = view_dispatcher_alloc();
    app->scene_manager = scene_manager_alloc(&bastion_scene_handlers, app);

    view_dispatcher_set_event_callback_context(app->view_dispatcher, app);
    view_dispatcher_set_custom_event_callback(app->view_dispatcher, bastion_custom_event_callback);
    view_dispatcher_set_navigation_event_callback(
        app->view_dispatcher, bastion_back_event_callback);
    view_dispatcher_set_tick_event_callback(app->view_dispatcher, bastion_tick_event_callback, 100);

    app->settings.mode = BadgeModeAuto;
    app->settings.sound = true;
    app->settings.vibro = true;
    app->settings.led = true;
    app->settings.logging = true;
    bst_store_settings_load(&app->settings);

    app->reader = badge_reader_alloc();
    app->decoded_fields = furi_string_alloc();

    app->submenu = submenu_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher, BastionViewSubmenu, submenu_get_view(app->submenu));

    app->var_item_list = variable_item_list_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher,
        BastionViewSettings,
        variable_item_list_get_view(app->var_item_list));

    app->widget = widget_alloc();
    view_dispatcher_add_view(app->view_dispatcher, BastionViewWidget, widget_get_view(app->widget));

    app->scan_view = scan_view_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher, BastionViewScan, scan_view_get_view(app->scan_view));

    app->result_view = result_view_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher, BastionViewResult, result_view_get_view(app->result_view));

    view_dispatcher_attach_to_gui(app->view_dispatcher, app->gui, ViewDispatcherTypeFullscreen);
    return app;
}

static void bastion_app_free(BastionApp* app) {
    furi_assert(app);

    badge_reader_stop(app->reader);

    view_dispatcher_remove_view(app->view_dispatcher, BastionViewSubmenu);
    view_dispatcher_remove_view(app->view_dispatcher, BastionViewSettings);
    view_dispatcher_remove_view(app->view_dispatcher, BastionViewWidget);
    view_dispatcher_remove_view(app->view_dispatcher, BastionViewScan);
    view_dispatcher_remove_view(app->view_dispatcher, BastionViewResult);

    submenu_free(app->submenu);
    variable_item_list_free(app->var_item_list);
    widget_free(app->widget);
    scan_view_free(app->scan_view);
    result_view_free(app->result_view);

    view_dispatcher_free(app->view_dispatcher);
    scene_manager_free(app->scene_manager);

    badge_reader_free(app->reader);
    furi_string_free(app->decoded_fields);

    furi_record_close(RECORD_NOTIFICATION);
    furi_record_close(RECORD_GUI);

    free(app);
}

int32_t bastion_app(void* p) {
    UNUSED(p);
    BastionApp* app = bastion_app_alloc();
    scene_manager_next_scene(app->scene_manager, BastionSceneStart);
    view_dispatcher_run(app->view_dispatcher);
    bastion_app_free(app);
    return 0;
}
