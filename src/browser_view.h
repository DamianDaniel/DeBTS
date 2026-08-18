#ifndef BTS_BROWSER_VIEW_H
#define BTS_BROWSER_VIEW_H

#include "app_context.h"

/* Builds the embedded BTS browser: a real WebKit page of bugs.debian.org,
 * plus a floating toolbar that appears when the page is showing a bug. */
GtkWidget *browser_view_new(AppContext *ctx);

/* Loads a page in the embedded browser. */
void browser_view_load(GtkWidget *view, const gchar *url);

#endif /* BTS_BROWSER_VIEW_H */
