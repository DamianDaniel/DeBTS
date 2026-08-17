#include "bug.h"
#include <string.h>

void
bug_free(Bug *b)
{
    if (!b) return;
    g_free(b->package);
    g_free(b->title);
    g_free(b->severity);
    g_free(b->status);
    /* headers are owned by the caller of mailer_list_headers, not by Bug */
    g_list_free(b->headers);
    g_free(b);
}

static gint
bug_compare_number_desc(gconstpointer a, gconstpointer b)
{
    const Bug *ba = a, *bb = b;
    return bb->number - ba->number;
}

static gboolean
subject_looks_done(const gchar *subject)
{
    if (!subject) return FALSE;
    gchar *lower = g_ascii_strdown(subject, -1);
    gboolean done = (strstr(lower, "closed") != NULL) ||
                     (g_str_has_prefix(lower, "bug#") && strstr(lower, " done") != NULL) ||
                     (strstr(lower, "marked as done") != NULL);
    g_free(lower);
    return done;
}

GList *
bug_group_from_headers(GList *headers)
{
    GRegex *num_re = g_regex_new("Bug ?#(\\d+)", G_REGEX_CASELESS, 0, NULL);
    GRegex *title_re = g_regex_new("Bug ?#\\d+:\\s*(.*)$", G_REGEX_CASELESS, 0, NULL);
    GHashTable *by_number = g_hash_table_new(g_direct_hash, g_direct_equal);
    GList *ordered = NULL; /* preserves first-seen order for stable output */

    for (GList *l = headers; l; l = l->next) {
        MailHeader *h = (MailHeader *) l->data;
        if (!h->subject) continue;

        GMatchInfo *mi = NULL;
        if (!g_regex_match(num_re, h->subject, 0, &mi)) {
            if (mi) g_match_info_free(mi);
            continue; /* not BTS traffic we recognise */
        }
        gchar *num_str = g_match_info_fetch(mi, 1);
        gint number = atoi(num_str);
        g_free(num_str);
        g_match_info_free(mi);
        if (number <= 0) continue;

        Bug *b = g_hash_table_lookup(by_number, GINT_TO_POINTER(number));
        if (!b) {
            b = g_new0(Bug, 1);
            b->number = number;
            b->status = g_strdup("open");
            g_hash_table_insert(by_number, GINT_TO_POINTER(number), b);
            ordered = g_list_prepend(ordered, b);
        }

        b->headers = g_list_append(b->headers, h);

        /* Try to refine the title from this message's subject line. */
        GMatchInfo *tmi = NULL;
        if (g_regex_match(title_re, h->subject, 0, &tmi)) {
            gchar *t = g_match_info_fetch(tmi, 1);
            g_strstrip(t);
            if (*t && (!b->title || strlen(t) > 3)) {
                g_free(b->title);
                b->title = t;
            } else {
                g_free(t);
            }
        }
        if (tmi) g_match_info_free(tmi);

        if (subject_looks_done(h->subject)) {
            g_free(b->status);
            b->status = g_strdup("done");
        }
    }

    g_regex_unref(num_re);
    g_regex_unref(title_re);
    g_hash_table_destroy(by_number); /* values now owned by `ordered` */

    ordered = g_list_reverse(ordered); /* first-seen order restored */
    ordered = g_list_sort(ordered, bug_compare_number_desc);
    return ordered;
}
