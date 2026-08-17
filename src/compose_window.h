#ifndef BTS_COMPOSE_WINDOW_H
#define BTS_COMPOSE_WINDOW_H

#include "app_context.h"

/* Opens a composer for filing a brand new bug (mails submit@bugs.debian.org
 * with Package/Version/Severity pseudo-headers). */
void compose_window_new_bug(AppContext *ctx);

/* Opens a composer for following up on an existing bug (mails
 * <number>@bugs.debian.org). `subject_hint` may be NULL. */
void compose_window_reply(AppContext *ctx, gint bug_number, const gchar *subject_hint);

#endif /* BTS_COMPOSE_WINDOW_H */
