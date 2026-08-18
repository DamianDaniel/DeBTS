#ifndef BTS_SEARCH_WINDOW_H
#define BTS_SEARCH_WINDOW_H

#include "app_context.h"

/* Opens the BTS search window, with the same filters pkgreport.cgi
 * supports (package, source, maintainer, submitter, severity, status,
 * tag, owner). Results open in the bug detail view on double-click. */
void search_window_show(AppContext *ctx);

#endif /* BTS_SEARCH_WINDOW_H */
