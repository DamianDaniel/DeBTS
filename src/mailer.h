#ifndef BTS_MAILER_H
#define BTS_MAILER_H

#include <glib.h>
#include "config.h"

typedef struct {
    gchar *uid;         /* IMAP UID, as string */
    gchar *subject;
    gchar *from;
    gchar *date;
    gchar *message_id;
} MailHeader;

void mail_header_free(MailHeader *h);

/* Sends a plain-text RFC 822 message.
 * to/cc may be NULL. extra_headers may be NULL, or a raw block of
 * additional header lines (each ending in \r\n) e.g. "X-Debbugs-Cc: ...\r\n".
 * Returns TRUE on success. */
gboolean mailer_send(AppConfig *cfg,
                      const gchar *to,
                      const gchar *cc,
                      const gchar *subject,
                      const gchar *body,
                      const gchar *extra_headers,
                      GError **error);

/* Lists headers of all messages in cfg->imap_folder, newest last.
 * Returns a GList of MailHeader* (caller frees with mail_header_free via g_list_free_full). */
GList *mailer_list_headers(AppConfig *cfg, GError **error);

/* Fetches the full raw RFC822 source of a single message by UID.
 * Returns a newly allocated string (caller frees with g_free), or NULL on error. */
gchar *mailer_fetch_full(AppConfig *cfg, const gchar *uid, GError **error);

/* Lists the IMAP folders (mailboxes) available on the account. */
GList *mailer_list_folders(AppConfig *cfg, GError **error);

#endif /* BTS_MAILER_H */
