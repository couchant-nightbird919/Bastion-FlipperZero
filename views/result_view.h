#pragma once

#include <gui/view.h>
#include "../helpers/lf_grade.h"

/* The verdict card: the risk band and letter across the top, the score big
 * enough to read at arm's length, what a clone would cost an attacker - and
 * along the bottom, the credential's own bits drawn as a barcode, because that
 * is what a 125 kHz badge actually is.
 *
 * OK opens the full report; Right grades another badge. */

typedef struct ResultView ResultView;

typedef enum {
    ResultEventReport, // OK
    ResultEventRescan, // Right
} ResultEvent;

typedef void (*ResultViewCallback)(void* context, ResultEvent event);

ResultView* result_view_alloc(void);
void result_view_free(ResultView* v);
View* result_view_get_view(ResultView* v);

void result_view_set_callback(ResultView* v, ResultViewCallback cb, void* context);
void result_view_set_result(ResultView* v, const LfGrade* grade, const LfReading* reading);
