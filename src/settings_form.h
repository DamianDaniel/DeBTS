#ifndef BTS_SETTINGS_FORM_H
#define BTS_SETTINGS_FORM_H

#include <gtk/gtk.h>
#include "config.h"

typedef struct {
    GtkWidget *imap_host, *imap_port, *imap_ssl, *imap_user, *imap_pass, *imap_folder;
    GtkWidget *smtp_host, *smtp_port, *smtp_ssl, *smtp_user, *smtp_pass;
    GtkWidget *from_name, *from_email;
} SettingsWidgets;

/* Builds the three account-setup tabs, filled with cfg's current values. */
GtkWidget *settings_form_build_imap_tab(SettingsWidgets *w, AppConfig *cfg);
GtkWidget *settings_form_build_smtp_tab(SettingsWidgets *w, AppConfig *cfg);
GtkWidget *settings_form_build_identity_tab(SettingsWidgets *w, AppConfig *cfg);

/* Copies the form's current values back into cfg. */
void settings_form_apply(SettingsWidgets *w, AppConfig *cfg);

#endif /* BTS_SETTINGS_FORM_H */
