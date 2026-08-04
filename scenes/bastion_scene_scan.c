#include "../bastion_i.h"

void bastion_scene_scan_on_enter(void* context) {
    BastionApp* app = context;

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
            lf_grade_evaluate(&app->capture.reading, &app->grade);
            app->have_result = true;
            bastion_notify_graded(app, &app->grade);
            if(app->settings.logging) bst_store_log_append(&app->grade);
            view_dispatcher_send_custom_event(
                app->view_dispatcher, BastionCustomEventBadgeRead);
        }
        consumed = true;
    }
    return consumed;
}

void bastion_scene_scan_on_exit(void* context) {
    BastionApp* app = context;
    badge_reader_stop(app->reader);
}
