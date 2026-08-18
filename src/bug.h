#ifndef BTS_BUG_H
#define BTS_BUG_H

#include <glib.h>
#include "mailer.h"

typedef struct {
    gint number;
    gchar *package;
    gchar *title;
    gchar *severity;
    gchar *status;      /* open or done, best guess */
    GList *headers;      /* mail in your IMAP folder, oldest first */
} Bug;

void bug_free(Bug *b);

/* Groups mail headers into bug threads by their Bug# subject.
 * Skips mail that isn't BTS traffic. Newest bug first. */
GList *bug_group_from_headers(GList *headers);

/* Copies package/title/severity/status from src into dst,
 * keeping dst's own value where src has none. */
void bug_apply_summary(Bug *dst, const Bug *src);

#endif /* BTS_BUG_H */
