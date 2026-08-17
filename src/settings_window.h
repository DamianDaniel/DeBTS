#ifndef BTS_SETTINGS_WINDOW_H
#define BTS_SETTINGS_WINDOW_H

#include "app_context.h"

/* Shows a modal dialog to edit the mail account settings stored in
 * ctx->cfg. Saves to disk on OK. */
void settings_window_show(AppContext *ctx);

#endif /* BTS_SETTINGS_WINDOW_H */
