#include "btsweb.h"

gchar *
btsweb_build_search_url(const BtsSearchQuery *q)
{
    GString *url = g_string_new(
        "https://bugs.debian.org/cgi-bin/pkgreport.cgi?bug-rev=on;dist=unstable");

    #define ADD_FIELD(param, value) \
        if (value && *value) { \
            gchar *enc = g_uri_escape_string(value, NULL, FALSE); \
            g_string_append_printf(url, ";" param "=%s", enc); \
            g_free(enc); \
        }

    ADD_FIELD("package", q->package);
    ADD_FIELD("src", q->src);
    ADD_FIELD("maint", q->maintainer);
    ADD_FIELD("submitter", q->submitter);
    ADD_FIELD("severity", q->severity);
    ADD_FIELD("status", q->status);
    ADD_FIELD("tag", q->tag);
    ADD_FIELD("owner", q->owner);
    #undef ADD_FIELD

    gboolean any = (q->package && *q->package) || (q->src && *q->src) ||
                   (q->maintainer && *q->maintainer) || (q->submitter && *q->submitter) ||
                   (q->severity && *q->severity) || (q->status && *q->status) ||
                   (q->tag && *q->tag) || (q->owner && *q->owner);
    if (!any) {
        g_string_free(url, TRUE);
        return NULL;
    }
    return g_string_free(url, FALSE);
}
