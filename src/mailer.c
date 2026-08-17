#include "mailer.h"
#include <curl/curl.h>
#include <gio/gio.h>
#include <string.h>
#include <time.h>

/* ------------------------------------------------------------------ */
/* small helpers                                                       */
/* ------------------------------------------------------------------ */

void
mail_header_free(MailHeader *h)
{
    if (!h) return;
    g_free(h->uid);
    g_free(h->subject);
    g_free(h->from);
    g_free(h->date);
    g_free(h->message_id);
    g_free(h);
}

/* Extracts the bare "user@host" part of an address that may include a
 * display name, e.g. `Foo Bar <foo@bar.com>` -> `foo@bar.com`. */
static gchar *
extract_addr(const gchar *full)
{
    if (!full) return g_strdup("");
    const gchar *lt = strchr(full, '<');
    const gchar *gt = strchr(full, '>');
    if (lt && gt && gt > lt) {
        return g_strndup(lt + 1, gt - lt - 1);
    }
    return g_strdup(g_strstrip(g_strdup(full)));
}

/* Splits a comma/semicolon separated address list into a curl_slist of
 * bare addresses wrapped in <>, appending to (and returning) `list`. */
static struct curl_slist *
append_rcpts(struct curl_slist *list, const gchar *addrs)
{
    if (!addrs || !*addrs) return list;
    gchar **parts = g_strsplit_set(addrs, ",;", -1);
    for (int i = 0; parts[i]; i++) {
        gchar *trimmed = g_strdup(parts[i]);
        g_strstrip(trimmed);
        if (*trimmed) {
            gchar *addr = extract_addr(trimmed);
            gchar *wrapped = g_strdup_printf("<%s>", addr);
            list = curl_slist_append(list, wrapped);
            g_free(wrapped);
            g_free(addr);
        }
        g_free(trimmed);
    }
    g_strfreev(parts);
    return list;
}

static gchar *
build_imap_url(AppConfig *cfg, const gchar *folder_suffix)
{
    const gchar *scheme = cfg->imap_ssl ? "imaps" : "imap";
    return g_strdup_printf("%s://%s:%d/%s%s", scheme, cfg->imap_host, cfg->imap_port,
                            cfg->imap_folder ? cfg->imap_folder : "INBOX",
                            folder_suffix ? folder_suffix : "");
}

/* generic curl write-to-GString callback */
static size_t
write_to_gstring(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    GString *buf = (GString *)userdata;
    g_string_append_len(buf, ptr, size * nmemb);
    return size * nmemb;
}

static CURL *
make_imap_handle(AppConfig *cfg, GError **error)
{
    CURL *curl = curl_easy_init();
    if (!curl) {
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_FAILED, "Could not initialise libcurl");
        return NULL;
    }
    curl_easy_setopt(curl, CURLOPT_USERNAME, cfg->imap_user);
    curl_easy_setopt(curl, CURLOPT_PASSWORD, cfg->imap_pass);
    if (!cfg->imap_ssl) {
        curl_easy_setopt(curl, CURLOPT_USE_SSL, (long)CURLUSESSL_ALL);
    }
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 20L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);
    return curl;
}

/* ------------------------------------------------------------------ */
/* IMAP: listing message headers                                       */
/* ------------------------------------------------------------------ */

/* Un-folds RFC822 header continuation lines ("\r\n " / "\r\n\t" -> " ") so
 * each logical header ends up on a single line, which keeps the regexes
 * below simple. */
static gchar *
unfold_headers(const gchar *raw)
{
    GString *out = g_string_new(NULL);
    for (const gchar *p = raw; *p; p++) {
        if ((p[0] == '\r' && p[1] == '\n' && (p[2] == ' ' || p[2] == '\t'))) {
            g_string_append_c(out, ' ');
            p += 2;
        } else if (p[0] == '\n' && (p[1] == ' ' || p[1] == '\t')) {
            g_string_append_c(out, ' ');
            p += 1;
        } else {
            g_string_append_c(out, *p);
        }
    }
    return g_string_free(out, FALSE);
}

static gchar *
regex_first_match(const gchar *text, const gchar *pattern)
{
    GRegex *re = g_regex_new(pattern, G_REGEX_CASELESS | G_REGEX_MULTILINE, 0, NULL);
    GMatchInfo *mi = NULL;
    gchar *result = NULL;
    if (re && g_regex_match(re, text, 0, &mi)) {
        result = g_match_info_fetch(mi, 1);
        g_strstrip(result);
    }
    if (mi) g_match_info_free(mi);
    if (re) g_regex_unref(re);
    return result;
}

static GList *
parse_fetch_response(const gchar *response)
{
    GList *out = NULL;
    gchar *unfolded = unfold_headers(response);

    /* Split into per-message chunks at each "* <n> FETCH" line start. */
    GRegex *split_re = g_regex_new("^\\* *\\d+ FETCH", G_REGEX_MULTILINE, 0, NULL);
    GMatchInfo *mi = NULL;
    GArray *starts = g_array_new(FALSE, FALSE, sizeof(gint));
    if (split_re && g_regex_match(split_re, unfolded, 0, &mi)) {
        do {
            gint start, end;
            g_match_info_fetch_pos(mi, 0, &start, &end);
            g_array_append_val(starts, start);
        } while (g_match_info_next(mi, NULL));
    }
    if (mi) g_match_info_free(mi);
    if (split_re) g_regex_unref(split_re);

    gint len = (gint) strlen(unfolded);
    for (guint i = 0; i < starts->len; i++) {
        gint s = g_array_index(starts, gint, i);
        gint e = (i + 1 < starts->len) ? g_array_index(starts, gint, i + 1) : len;
        gchar *chunk = g_strndup(unfolded + s, e - s);

        MailHeader *h = g_new0(MailHeader, 1);
        h->uid = regex_first_match(chunk, "UID (\\d+)");
        h->subject = regex_first_match(chunk, "^Subject: *(.*)$");
        h->from = regex_first_match(chunk, "^From: *(.*)$");
        h->date = regex_first_match(chunk, "^Date: *(.*)$");
        h->message_id = regex_first_match(chunk, "^Message-ID: *(.*)$");

        if (!h->subject) h->subject = g_strdup("(no subject)");
        if (!h->from) h->from = g_strdup("(unknown sender)");

        if (h->uid) {
            out = g_list_append(out, h);
        } else {
            mail_header_free(h);
        }
        g_free(chunk);
    }

    g_array_free(starts, TRUE);
    g_free(unfolded);
    return out;
}

GList *
mailer_list_headers(AppConfig *cfg, GError **error)
{
    CURL *curl = make_imap_handle(cfg, error);
    if (!curl) return NULL;

    gchar *url = build_imap_url(cfg, NULL);
    GString *buf = g_string_new(NULL);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    /* Fetch the fields we need for every message currently in the folder. */
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST,
        "FETCH 1:* (UID BODY.PEEK[HEADER.FIELDS (SUBJECT FROM DATE MESSAGE-ID)])");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_gstring);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, buf);

    CURLcode res = curl_easy_perform(curl);
    GList *headers = NULL;
    if (res != CURLE_OK) {
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_FAILED,
                    "IMAP fetch failed: %s", curl_easy_strerror(res));
    } else {
        headers = parse_fetch_response(buf->str);
    }

    g_string_free(buf, TRUE);
    g_free(url);
    curl_easy_cleanup(curl);
    return headers;
}

gchar *
mailer_fetch_full(AppConfig *cfg, const gchar *uid, GError **error)
{
    CURL *curl = make_imap_handle(cfg, error);
    if (!curl) return NULL;

    gchar *suffix = g_strdup_printf(";UID=%s", uid);
    gchar *url = build_imap_url(cfg, suffix);
    GString *buf = g_string_new(NULL);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_gstring);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, buf);

    CURLcode res = curl_easy_perform(curl);
    gchar *result = NULL;
    if (res != CURLE_OK) {
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_FAILED,
                    "IMAP message fetch failed: %s", curl_easy_strerror(res));
    } else {
        result = g_strdup(buf->str);
    }

    g_string_free(buf, TRUE);
    g_free(suffix);
    g_free(url);
    curl_easy_cleanup(curl);
    return result;
}

GList *
mailer_list_folders(AppConfig *cfg, GError **error)
{
    CURL *curl = make_imap_handle(cfg, error);
    if (!curl) return NULL;

    const gchar *scheme = cfg->imap_ssl ? "imaps" : "imap";
    gchar *url = g_strdup_printf("%s://%s:%d/", scheme, cfg->imap_host, cfg->imap_port);
    GString *buf = g_string_new(NULL);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "LIST \"\" \"*\"");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_gstring);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, buf);

    CURLcode res = curl_easy_perform(curl);
    GList *folders = NULL;
    if (res != CURLE_OK) {
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_FAILED,
                    "IMAP LIST failed: %s", curl_easy_strerror(res));
    } else {
        gchar **lines = g_strsplit(buf->str, "\n", -1);
        for (int i = 0; lines[i]; i++) {
            gchar *name = regex_first_match(lines[i], "\"([^\"]*)\"\\s*$");
            if (name) folders = g_list_append(folders, name);
        }
        g_strfreev(lines);
    }

    g_string_free(buf, TRUE);
    g_free(url);
    curl_easy_cleanup(curl);
    return folders;
}

/* ------------------------------------------------------------------ */
/* SMTP send                                                            */
/* ------------------------------------------------------------------ */

struct upload_ctx {
    const gchar *data;
    size_t remaining;
};

static size_t
read_from_ctx(char *ptr, size_t size, size_t nmemb, void *userp)
{
    struct upload_ctx *ctx = (struct upload_ctx *)userp;
    size_t room = size * nmemb;
    if (room == 0 || ctx->remaining == 0) return 0;
    size_t n = ctx->remaining < room ? ctx->remaining : room;
    memcpy(ptr, ctx->data, n);
    ctx->data += n;
    ctx->remaining -= n;
    return n;
}

gboolean
mailer_send(AppConfig *cfg,
            const gchar *to,
            const gchar *cc,
            const gchar *subject,
            const gchar *body,
            const gchar *extra_headers,
            GError **error)
{
    CURL *curl = curl_easy_init();
    if (!curl) {
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_FAILED, "Could not initialise libcurl");
        return FALSE;
    }

    /* Build the RFC 822 message. */
    GString *msg = g_string_new(NULL);
    gchar *date = NULL;
    {
        GDateTime *now = g_date_time_new_now_local();
        date = g_date_time_format(now, "%a, %d %b %Y %H:%M:%S %z");
        g_date_time_unref(now);
    }
    gchar *msgid = g_strdup_printf("<%" G_GINT64_FORMAT ".%d@debts>",
                                    g_get_real_time(), g_random_int());

    g_string_append_printf(msg, "From: %s <%s>\r\n",
                            cfg->from_name && *cfg->from_name ? cfg->from_name : cfg->from_email,
                            cfg->from_email);
    g_string_append_printf(msg, "To: %s\r\n", to ? to : "");
    if (cc && *cc) g_string_append_printf(msg, "Cc: %s\r\n", cc);
    g_string_append_printf(msg, "Subject: %s\r\n", subject ? subject : "");
    g_string_append_printf(msg, "Date: %s\r\n", date);
    g_string_append_printf(msg, "Message-ID: %s\r\n", msgid);
    g_string_append(msg, "MIME-Version: 1.0\r\n");
    g_string_append(msg, "Content-Type: text/plain; charset=UTF-8\r\n");
    g_string_append(msg, "Content-Transfer-Encoding: 8bit\r\n");
    g_string_append(msg, "User-Agent: debts\r\n");
    if (extra_headers && *extra_headers) g_string_append(msg, extra_headers);
    g_string_append(msg, "\r\n");
    g_string_append(msg, body ? body : "");
    if (!body || body[strlen(body) - 1] != '\n') g_string_append(msg, "\r\n");

    g_free(date);
    g_free(msgid);

    const gchar *scheme = cfg->smtp_ssl ? "smtps" : "smtp";
    gchar *url = g_strdup_printf("%s://%s:%d", scheme, cfg->smtp_host, cfg->smtp_port);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    if (!cfg->smtp_ssl) {
        /* opportunistic STARTTLS on plain submission ports */
        curl_easy_setopt(curl, CURLOPT_USE_SSL, (long)CURLUSESSL_TRY);
    }
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

    if (cfg->smtp_user && *cfg->smtp_user) {
        curl_easy_setopt(curl, CURLOPT_USERNAME, cfg->smtp_user);
        curl_easy_setopt(curl, CURLOPT_PASSWORD, cfg->smtp_pass);
    }

    gchar *from_addr = g_strdup_printf("<%s>", cfg->from_email);
    curl_easy_setopt(curl, CURLOPT_MAIL_FROM, from_addr);

    struct curl_slist *rcpts = NULL;
    rcpts = append_rcpts(rcpts, to);
    rcpts = append_rcpts(rcpts, cc);
    curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, rcpts);

    struct upload_ctx ctx = { .data = msg->str, .remaining = msg->len };
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_from_ctx);
    curl_easy_setopt(curl, CURLOPT_READDATA, &ctx);
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 20L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);

    CURLcode res = curl_easy_perform(curl);
    gboolean ok = (res == CURLE_OK);
    if (!ok) {
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_FAILED,
                    "SMTP send failed: %s", curl_easy_strerror(res));
    }

    curl_slist_free_all(rcpts);
    g_free(from_addr);
    g_free(url);
    g_string_free(msg, TRUE);
    curl_easy_cleanup(curl);
    return ok;
}
