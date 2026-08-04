#pragma once

#include <gui/scene_manager.h>

// Generate the scene id enum
#define ADD_SCENE(prefix, name, id) BastionScene##id,
typedef enum {
#include "bastion_scene_config.h"
    BastionSceneNum,
} BastionScene;
#undef ADD_SCENE

extern const SceneManagerHandlers bastion_scene_handlers;

// Generate scene handler prototypes
#define ADD_SCENE(prefix, name, id)                                            \
    void prefix##_scene_##name##_on_enter(void* context);                      \
    bool prefix##_scene_##name##_on_event(void* context, SceneManagerEvent e); \
    void prefix##_scene_##name##_on_exit(void* context);
#include "bastion_scene_config.h"
#undef ADD_SCENE
