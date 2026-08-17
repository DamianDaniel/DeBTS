#ifndef BTS_BUG_DETAIL_WINDOW_H
#define BTS_BUG_DETAIL_WINDOW_H

#include "app_context.h"
#include "bug.h"

/* Shows the full thread of a bug (fetching each message's raw body over
 * IMAP) with quick actions to reply or run control commands on it. */
void bug_detail_window_show(AppContext *ctx, Bug *bug);

#endif /* BTS_BUG_DETAIL_WINDOW_H */
