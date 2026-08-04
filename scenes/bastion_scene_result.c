#include "../bastion_i.h"

static void bastion_scene_result_cb(void* context, ResultEvent event) {
    BastionApp* app = context;
    view_dispatcher_send_custom_event(
        app->view_dispatcher,
        (event == ResultEventReport) ? BastionCustomEventReport : BastionCustomEventRescan);
}

void bastion_scene_result_on_enter(void* context) {
    BastionApp* app = context;

    result_view_set_result(app->result_view, &app->grade, &app->capture.reading);
    result_view_set_callback(app->result_view, bastion_scene_result_cb, app);
    view_dispatcher_switch_to_view(app->view_dispatcher, BastionViewResult);
}

bool bastion_scene_result_on_event(void* context, SceneManagerEvent event) {
    BastionApp* app = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        switch(event.event) {
        case BastionCustomEventReport:
            scene_manager_next_scene(app->scene_manager, BastionSceneReport);
            consumed = true;
            break;
        case BastionCustomEventRescan:
            /* Back to the (transient) scan scene, which re-arms the field. */
            scene_manager_search_and_switch_to_previous_scene(
                app->scene_manager, BastionSceneScan);
            consumed = true;
            break;
        default:
            break;
        }
    } else if(event.type == SceneManagerEventTypeBack) {
        /* Skip the transient scan scene and land back on the main menu. */
        scene_manager_search_and_switch_to_previous_scene(app->scene_manager, BastionSceneStart);
        consumed = true;
    }
    return consumed;
}

void bastion_scene_result_on_exit(void* context) {
    UNUSED(context);
}
