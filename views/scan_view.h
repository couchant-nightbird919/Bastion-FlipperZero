#pragma once

#include <gui/view.h>
#include "../helpers/badge_reader.h"

/* The read screen: the Flipper's coil on the left, a badge on the right, and a
 * 125 kHz carrier travelling between them. The wave only runs once the field is
 * up, and the badge fills in the moment something loads the coil - so the
 * animation reports what the hardware is actually doing rather than spinning
 * decoratively while nothing happens. */

typedef struct ScanView ScanView;

ScanView* scan_view_alloc(void);
void scan_view_free(ScanView* v);
View* scan_view_get_view(ScanView* v);

void scan_view_reset(ScanView* v);
void scan_view_tick(ScanView* v);
void scan_view_set_stage(ScanView* v, BadgeStage stage);
void scan_view_set_mode(ScanView* v, BadgeMode mode);
