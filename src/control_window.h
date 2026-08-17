#ifndef BTS_CONTROL_WINDOW_H
#define BTS_CONTROL_WINDOW_H

#include "app_context.h"

/* Opens the BTS control-message builder: pick a command, fill its
 * parameters, queue up as many as needed, then send them all in one email
 * to control@bugs.debian.org - independent of any other mail client. */
void control_window_show(AppContext *ctx);

/* Same, but pre-fills the first queued command as an operation on the given
 * bug number (e.g. from a right-click on a bug row). command_key may be
 * NULL to just default to "retitle". */
void control_window_show_for_bug(AppContext *ctx, gint bug_number, const gchar *command_key);

#endif /* BTS_CONTROL_WINDOW_H */
