#ifndef BTS_CONFIG_H
#define BTS_CONFIG_H

#include <glib.h>

typedef struct {
    gchar *imap_host;
    gint   imap_port;
    gboolean imap_ssl;
    gchar *imap_user;
    gchar *imap_pass;
    gchar *imap_folder;      /* folder to scan for BTS mail, e.g. "INBOX" or "INBOX.Debian-BTS" */

    gchar *smtp_host;
    gint   smtp_port;
    gboolean smtp_ssl;
    gchar *smtp_user;
    gchar *smtp_pass;

    gchar *from_name;
    gchar *from_email;
} AppConfig;

/* Loads config from $XDG_CONFIG_HOME/debts/config.ini (or defaults if missing). */
AppConfig *config_load(void);

/* Persists config to disk with 0600 permissions (contains credentials). */
gboolean config_save(AppConfig *cfg, GError **error);

void config_free(AppConfig *cfg);

#endif /* BTS_CONFIG_H */
