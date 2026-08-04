#include "../bastion_i.h"

/* Ticks are 100 ms. A grader that sits on "Sensing..." indefinitely leaves the
 * user guessing whether the badge is wrong, the placement is wrong, or the app
 * is broken - so after twenty seconds Bastion answers with the one verdict it
 * can honestly give: nothing decoded, and here is why that happens. */
#define BST_SCAN_TIMEOUT_TICKS 200u

/* Hand the result scene a verdict and move to it. */
static void bastion_scan_deliver(BastionApp* app) {
    lf_grade_evaluate(&app->capture.reading, &app->grade);
    bastion_notify_graded(app, &app->grade);
    view_dispatcher_send_custom_event(app->view_dispatcher, BastionCustomEventBadgeRead);
}

void bastion_scene_scan_on_enter(void* context) {
    BastionApp* app = context;

    scene_manager_set_scene_state(app->scene_manager, BastionSceneScan, 0);
    scan_view_reset(app->scan_view);
    scan_view_set_mode(app->scan_view, (BadgeMode)app->settings.mode);
    badge_reader_start(app->reader, (BadgeMode)app->settings.mode);
    view_dispatcher_switch_to_view(app->view_dispatcher, BastionViewScan);
}

bool bastion_scene_scan_on_event(void* context, SceneManagerEvent event) {
    BastionApp* app = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        if(event.event == BastionCustomEventBadgeRead) {
            scene_manager_next_scene(app->scene_manager, BastionSceneResult);
            consumed = true;
        }
    } else if(event.type == SceneManagerEventTypeTick) {
        scan_view_tick(app->scan_view);
        scan_view_set_stage(app->scan_view, badge_reader_stage(app->reader));

        if(badge_reader_take(app->reader, &app->capture, app->decoded_fields)) {
            bastion_scan_deliver(app);
            /* Only real reads reach the log; a timeout is not a badge. */
            if(app->settings.logging) bst_store_log_append(&app->grade);
        } else {
            uint32_t ticks =
                scene_manager_get_scene_state(app->scene_manager, BastionSceneScan) + 1;
            scene_manager_set_scene_state(app->scene_manager, BastionSceneScan, ticks);

            if(ticks >= BST_SCAN_TIMEOUT_TICKS) {
                badge_reader_stop(app->reader);
                memset(&app->capture, 0, sizeof(app->capture));
                app->capture.reading.proto = LfProtoUnknown;
                furi_string_reset(app->decoded_fields);
                bastion_scan_deliver(app);
            }
        }
        consumed = true;
    }
    return consumed;
}

void bastion_scene_scan_on_exit(void* context) {
    BastionApp* app = context;
    badge_reader_stop(app->reader);
}
