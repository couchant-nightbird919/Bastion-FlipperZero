#include "../bastion_i.h"

typedef enum {
    StartIndexGrade,
    StartIndexLog,
    StartIndexSettings,
    StartIndexAbout,
} StartIndex;

static void bastion_scene_start_submenu_cb(void* context, uint32_t index) {
    BastionApp* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, index);
}

void bastion_scene_start_on_enter(void* context) {
    BastionApp* app = context;
    Submenu* submenu = app->submenu;

    submenu_reset(submenu);
    submenu_set_header(submenu, "Bastion");
    submenu_add_item(
        submenu, "Grade a Badge", StartIndexGrade, bastion_scene_start_submenu_cb, app);
    submenu_add_item(submenu, "Badge Log", StartIndexLog, bastion_scene_start_submenu_cb, app);
    submenu_add_item(
        submenu, "Settings", StartIndexSettings, bastion_scene_start_submenu_cb, app);
    submenu_add_item(submenu, "About", StartIndexAbout, bastion_scene_start_submenu_cb, app);

    submenu_set_selected_item(
        submenu, scene_manager_get_scene_state(app->scene_manager, BastionSceneStart));

    view_dispatcher_switch_to_view(app->view_dispatcher, BastionViewSubmenu);
}

bool bastion_scene_start_on_event(void* context, SceneManagerEvent event) {
    BastionApp* app = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        scene_manager_set_scene_state(app->scene_manager, BastionSceneStart, event.event);
        switch(event.event) {
        case StartIndexGrade:
            scene_manager_next_scene(app->scene_manager, BastionSceneScan);
            consumed = true;
            break;
        case StartIndexLog:
            scene_manager_next_scene(app->scene_manager, BastionSceneLog);
            consumed = true;
            break;
        case StartIndexSettings:
            scene_manager_next_scene(app->scene_manager, BastionSceneSettings);
            consumed = true;
            break;
        case StartIndexAbout:
            scene_manager_next_scene(app->scene_manager, BastionSceneAbout);
            consumed = true;
            break;
        default:
            break;
        }
    }
    return consumed;
}

void bastion_scene_start_on_exit(void* context) {
    BastionApp* app = context;
    submenu_reset(app->submenu);
}
