#ifndef BTS_BTSWEB_H
#define BTS_BTSWEB_H

#include <glib.h>

/* Filters for a BTS search - same fields pkgreport.cgi understands. */
typedef struct {
    gchar *package;
    gchar *src;
    gchar *maintainer;
    gchar *submitter;
    gchar *severity;
    gchar *status;
    gchar *tag;
    gchar *owner;
} BtsSearchQuery;

/* Builds the real pkgreport.cgi URL for these filters, for loading in
 * the embedded browser (defaults to dist=unstable, newest bugs first). */
gchar *btsweb_build_search_url(const BtsSearchQuery *query);

#endif /* BTS_BTSWEB_H */
