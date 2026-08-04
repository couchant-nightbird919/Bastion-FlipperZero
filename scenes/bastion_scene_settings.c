#include "../bastion_i.h"

typedef enum {
    SettingsIndexMode,
    SettingsIndexLogging,
    SettingsIndexSound,
    SettingsIndexVibro,
    SettingsIndexLed,
    SettingsIndexClearLog,
} SettingsIndex;

static const char* const on_off[] = {"OFF", "ON"};
static const char* const mode_names[BadgeModeCount] = {"AUTO", "ASK", "PSK"};

static void settings_mode_cb(VariableItem* item) {
    BastionApp* app = variable_item_get_context(item);
    uint8_t v = variable_item_get_current_value_index(item);
    app->settings.mode = v;
    variable_item_set_current_value_text(item, mode_names[v]);
}
static void settings_logging_cb(VariableItem* item) {
    BastionApp* app = variable_item_get_context(item);
    uint8_t v = variable_item_get_current_value_index(item);
    app->settings.logging = v;
    variable_item_set_current_value_text(item, on_off[v]);
}
static void settings_sound_cb(VariableItem* item) {
    BastionApp* app = variable_item_get_context(item);
    uint8_t v = variable_item_get_current_value_index(item);
    app->settings.sound = v;
    variable_item_set_current_value_text(item, on_off[v]);
}
static void settings_vibro_cb(VariableItem* item) {
    BastionApp* app = variable_item_get_context(item);
    uint8_t v = variable_item_get_current_value_index(item);
    app->settings.vibro = v;
    variable_item_set_current_value_text(item, on_off[v]);
}
static void settings_led_cb(VariableItem* item) {
    BastionApp* app = variable_item_get_context(item);
    uint8_t v = variable_item_get_current_value_index(item);
    app->settings.led = v;
    variable_item_set_current_value_text(item, on_off[v]);
}

/* OK on "Clear log" wipes the CSV and reports back in the item's value column,
 * so the action confirms itself without a modal. */
static void settings_enter_cb(void* context, uint32_t index) {
    BastionApp* app = context;
    if(index != SettingsIndexClearLog) return;

    const bool removed = bst_store_log_clear();
    if(app->clear_log_item) {
        variable_item_set_current_value_text(app->clear_log_item, removed ? "Done" : "Empty");
    }
    notification_message(
        app->notifications, removed ? &sequence_success : &sequence_blink_blue_100);
}

void bastion_scene_settings_on_enter(void* context) {
    BastionApp* app = context;
    VariableItemList* list = app->var_item_list;
    variable_item_list_reset(list);

    VariableItem* item;

    /* Auto alternates the two demodulators; pinning one helps a marginal read
     * on a tag whose carrier you already know. */
    item = variable_item_list_add(list, "Read mode", BadgeModeCount, settings_mode_cb, app);
    variable_item_set_current_value_index(item, app->settings.mode);
    variable_item_set_current_value_text(item, mode_names[app->settings.mode]);

    item = variable_item_list_add(list, "Log badges", 2, settings_logging_cb, app);
    variable_item_set_current_value_index(item, app->settings.logging);
    variable_item_set_current_value_text(item, on_off[app->settings.logging]);

    item = variable_item_list_add(list, "Sound", 2, settings_sound_cb, app);
    variable_item_set_current_value_index(item, app->settings.sound);
    variable_item_set_current_value_text(item, on_off[app->settings.sound]);

    item = variable_item_list_add(list, "Vibro", 2, settings_vibro_cb, app);
    variable_item_set_current_value_index(item, app->settings.vibro);
    variable_item_set_current_value_text(item, on_off[app->settings.vibro]);

    item = variable_item_list_add(list, "LED", 2, settings_led_cb, app);
    variable_item_set_current_value_index(item, app->settings.led);
    variable_item_set_current_value_text(item, on_off[app->settings.led]);

    item = variable_item_list_add(list, "Clear log", 0, NULL, app);
    variable_item_set_current_value_text(item, "OK");
    app->clear_log_item = item;

    variable_item_list_set_enter_callback(list, settings_enter_cb, app);

    view_dispatcher_switch_to_view(app->view_dispatcher, BastionViewSettings);
}

bool bastion_scene_settings_on_event(void* context, SceneManagerEvent event) {
    UNUSED(context);
    UNUSED(event);
    return false;
}

void bastion_scene_settings_on_exit(void* context) {
    BastionApp* app = context;
    variable_item_list_reset(app->var_item_list);
    app->clear_log_item = NULL; /* reset() freed the row it pointed at */
    bst_store_settings_save(&app->settings);
}
