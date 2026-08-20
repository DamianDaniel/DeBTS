#ifndef BTS_MAIN_WINDOW_H
#define BTS_MAIN_WINDOW_H

#include "app_context.h"

/* Builds and shows the main application window. */
void main_window_show(AppContext *ctx);

/* Switches to the embedded browser tab and opens this bug's real page. */
void main_window_browse_bug(AppContext *ctx, gint bug_number);

/* Switches to the embedded browser tab and loads this URL. */
void main_window_browse_url(AppContext *ctx, const gchar *url);

#endif /* BTS_MAIN_WINDOW_H */
