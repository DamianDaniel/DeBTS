#include <glib.h>
#include <stdio.h>
#include <stdlib.h>

static gint
bug_number_from_uri(const gchar *uri)
{
    if (!uri) return 0;
    GRegex *re1 = g_regex_new("[?&;]bug=(\\d+)", 0, 0, NULL);
    GRegex *re2 = g_regex_new("bugs\\.debian\\.org/(\\d{3,7})(?:[/?#]|$)", 0, 0, NULL);
    GMatchInfo *mi = NULL;
    gint number = 0;

    if (g_regex_match(re1, uri, 0, &mi)) {
        gchar *s = g_match_info_fetch(mi, 1);
        number = atoi(s);
        g_free(s);
    }
    if (mi) g_match_info_free(mi);

    if (number == 0 && g_regex_match(re2, uri, 0, &mi)) {
        gchar *s = g_match_info_fetch(mi, 1);
        number = atoi(s);
        g_free(s);
    }
    if (mi) g_match_info_free(mi);

    g_regex_unref(re1);
    g_regex_unref(re2);
    return number;
}

int main(void) {
    struct { const char *uri; int expect; } cases[] = {
        { "https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=1029384", 1029384 },
        { "https://bugs.debian.org/1029384", 1029384 },
        { "https://bugs.debian.org/1029384/", 1029384 },
        { "https://bugs.debian.org/cgi-bin/pkgreport.cgi?bug-rev=on;dist=unstable;package=6tunnel", 0 },
        { "https://bugs.debian.org/", 0 },
    };
    int fails = 0;
    for (size_t i = 0; i < sizeof(cases)/sizeof(cases[0]); i++) {
        int got = bug_number_from_uri(cases[i].uri);
        printf("%-85s -> %d (expect %d) %s\n", cases[i].uri, got, cases[i].expect,
               got == cases[i].expect ? "OK" : "FAIL");
        if (got != cases[i].expect) fails++;
    }
    return fails;
}
