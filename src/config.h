#ifndef BTS_CONFIG_H
#define BTS_CONFIG_H

#include <glib.h>

typedef struct {
    gchar *imap_host;
    gint   imap_port;
    gboolean imap_ssl;
    gchar *imap_user;
    gchar *imap_pass;
    gchar *imap_folder;   /* folder with bug mail */

    gchar *smtp_host;
    gint   smtp_port;
    gboolean smtp_ssl;
    gchar *smtp_user;
    gchar *smtp_pass;

    gchar *from_name;
    gchar *from_email;

    gboolean setup_done; /* first-run screen shown */
} AppConfig;

/* Loads saved settings, or defaults. */
AppConfig *config_load(void);

/* Saves settings to disk. */
gboolean config_save(AppConfig *cfg, GError **error);

void config_free(AppConfig *cfg);

#endif /* BTS_CONFIG_H */
