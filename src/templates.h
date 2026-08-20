#ifndef BTS_TEMPLATES_H
#define BTS_TEMPLATES_H

#include <glib.h>

typedef struct {
    const gchar *name;
    const gchar *package_hint; /* NULL if not applicable */
    const gchar *severity_hint; /* NULL to leave as-is */
    const gchar *body;
} BugTemplate;

extern const BugTemplate bts_templates[];
extern const gint bts_templates_count;

#endif /* BTS_TEMPLATES_H */
