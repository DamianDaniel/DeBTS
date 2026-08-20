/* Not part of the app build - a throwaway check that bug_group_from_headers
 * copes with the real subject-line formats the Debian BTS actually sends.
 * Run with: gcc $(pkg-config --cflags --libs glib-2.0) -I../src test_bug_grouping.c ../src/bug.c -o /tmp/t && /tmp/t
 */
#include <stdio.h>
#include "../src/bug.h"

static MailHeader *
mk(const gchar *subject)
{
    MailHeader *h = g_new0(MailHeader, 1);
    h->uid = g_strdup("1");
    h->subject = g_strdup(subject);
    h->from = g_strdup("owner@bugs.debian.org");
    h->date = g_strdup("Mon, 17 Aug 2026 10:00:00 +0000");
    return h;
}

int main(void) {
    GList *headers = NULL;
    headers = g_list_append(headers, mk("Bug#1029384: 6tunnel: FTBFS with new gcc"));
    headers = g_list_append(headers, mk("Bug#1029384: Acknowledgement (6tunnel: FTBFS with new gcc)"));
    headers = g_list_append(headers, mk("Bug#1029384: Info received (fixed upstream)"));
    headers = g_list_append(headers, mk("Bug#1029384: marked as done (6tunnel: FTBFS with new gcc)"));
    headers = g_list_append(headers, mk("Bug#1050000: another-package: something else"));
    headers = g_list_append(headers, mk("Some totally unrelated newsletter"));

    GList *bugs = bug_group_from_headers(headers);
    printf("Grouped into %d bug thread(s):\n", g_list_length(bugs));
    for (GList *l = bugs; l; l = l->next) {
        Bug *b = l->data;
        printf("  #%d title=\"%s\" status=%s messages=%d\n",
               b->number, b->title ? b->title : "(null)", b->status,
               g_list_length(b->headers));
    }

    if (g_list_length(bugs) != 2) {
        printf("FAIL: expected 2 bug threads\n");
        return 1;
    }
    printf("OK\n");
    return 0;
}
