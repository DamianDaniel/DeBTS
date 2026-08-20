#ifndef BTS_LOGIN_WINDOW_H
#define BTS_LOGIN_WINDOW_H

#include "app_context.h"

/* Shows the first-run screen: log in with mail, or continue as guest.
 * Calls on_done(ctx) once the user has picked one, then destroys itself. */
void login_window_show(AppContext *ctx, void (*on_done)(AppContext *));

#endif /* BTS_LOGIN_WINDOW_H */
