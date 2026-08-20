#ifndef BTS_UDD_H
#define BTS_UDD_H

#include <glib.h>
#include "bug.h"
#include "btsweb.h"

/* Queries the public UDD mirror (udd-mirror.debian.net) - a real,
 * public, read-only copy of Debian's own bug database. No scraping,
 * no bot concerns: this is a service meant for exactly this. Returns
 * GList<Bug*>, or NULL if the query had no filters or found nothing. */
GList *udd_search(const BtsSearchQuery *query, GError **error);

#endif /* BTS_UDD_H */
