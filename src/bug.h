#ifndef BTS_BUG_H
#define BTS_BUG_H

#include <glib.h>
#include "mailer.h"

typedef struct {
    gint number;          /* 0 if unknown */
    gchar *package;       /* may be NULL/unknown */
    gchar *title;
    gchar *severity;      /* may be NULL/unknown */
    gchar *status;         /* "open" / "done" - best-effort guess from subject */
    GList *headers;        /* GList<MailHeader*> belonging to this bug, oldest first */
} Bug;

void bug_free(Bug *b);

/* Groups a flat list of MailHeader* (as returned by mailer_list_headers)
 * into per-bug threads by scanning Subject lines for "Bug#NNNNNN".
 * Headers that don't look like BTS traffic are skipped.
 * Returns GList<Bug*> sorted by bug number descending (most recent first),
 * does NOT take ownership of `headers`. */
GList *bug_group_from_headers(GList *headers);

#endif /* BTS_BUG_H */
