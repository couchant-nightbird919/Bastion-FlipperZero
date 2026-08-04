#pragma once

#include <furi.h>
#include <furi_hal.h>
#include <gui/gui.h>
#include <gui/view_dispatcher.h>
#include <gui/scene_manager.h>
#include <gui/modules/submenu.h>
#include <gui/modules/variable_item_list.h>
#include <gui/modules/widget.h>
#include <notification/notification.h>
#include <notification/notification_messages.h>

#include "bastion_icons.h" // generated from icons/ by fbt

#include "helpers/badge_reader.h"
#include "helpers/bst_store.h"
#include "helpers/lf_grade.h"
#include "views/scan_view.h"
#include "views/result_view.h"
#include "scenes/bastion_scene.h"

#define BASTION_VERSION "1.0"

typedef enum {
    BastionViewSubmenu,
    BastionViewScan,
    BastionViewResult,
    BastionViewSettings,
    BastionViewWidget,
} BastionViewId;

typedef enum {
    BastionCustomEventBadgeRead = 100, // the worker decoded a credential
    BastionCustomEventRescan, // user asked to grade another
    BastionCustomEventReport, // user opened the full breakdown
} BastionCustomEvent;

typedef struct {
    Gui* gui;
    ViewDispatcher* view_dispatcher;
    SceneManager* scene_manager;
    NotificationApp* notifications;

    Submenu* submenu;
    VariableItemList* var_item_list;
    /* The "Clear log" row, kept so its enter handler can report back into the
     * value column. The module exposes no way to look an item up by index. */
    VariableItem* clear_log_item;
    Widget* widget;
    ScanView* scan_view;
    ResultView* result_view;

    BadgeReader* reader;
    BastionSettings settings;

    /* the current verdict */
    BadgeCapture capture;
    LfGrade grade;
    FuriString* decoded_fields; // the decoder's own field dump, for the report
    bool have_result;
} BastionApp;

/* feedback (defined in bastion.c), all gated by settings */
void bastion_notify_graded(BastionApp* app, const LfGrade* grade);
