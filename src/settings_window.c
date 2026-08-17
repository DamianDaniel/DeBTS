#include "settings_window.h"

typedef struct {
    GtkWidget *imap_host, *imap_port, *imap_ssl, *imap_user, *imap_pass, *imap_folder;
    GtkWidget *smtp_host, *smtp_port, *smtp_ssl, *smtp_user, *smtp_pass;
    GtkWidget *from_name, *from_email;
} SettingsWidgets;

static GtkWidget *
labeled_row(GtkGrid *grid, gint row, const gchar *label, GtkWidget *widget)
{
    GtkWidget *l = gtk_label_new(label);
    gtk_label_set_xalign(GTK_LABEL(l), 0.0);
    gtk_grid_attach(grid, l, 0, row, 1, 1);
    gtk_widget_set_hexpand(widget, TRUE);
    gtk_grid_attach(grid, widget, 1, row, 1, 1);
    return widget;
}

static GtkWidget *
build_imap_tab(SettingsWidgets *w, AppConfig *cfg)
{
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 6);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    gtk_container_set_border_width(GTK_CONTAINER(grid), 12);

    w->imap_host = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(w->imap_host), cfg->imap_host);
    labeled_row(GTK_GRID(grid), 0, "IMAP server", w->imap_host);

    w->imap_port = gtk_spin_button_new_with_range(1, 65535, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(w->imap_port), cfg->imap_port);
    labeled_row(GTK_GRID(grid), 1, "Port", w->imap_port);

    w->imap_ssl = gtk_check_button_new_with_label("Use SSL/TLS (IMAPS)");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w->imap_ssl), cfg->imap_ssl);
    gtk_grid_attach(GTK_GRID(grid), w->imap_ssl, 1, 2, 1, 1);

    w->imap_user = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(w->imap_user), cfg->imap_user);
    labeled_row(GTK_GRID(grid), 3, "Username", w->imap_user);

    w->imap_pass = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(w->imap_pass), FALSE);
    gtk_entry_set_text(GTK_ENTRY(w->imap_pass), cfg->imap_pass);
    labeled_row(GTK_GRID(grid), 4, "Password", w->imap_pass);

    w->imap_folder = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(w->imap_folder), cfg->imap_folder);
    gtk_entry_set_placeholder_text(GTK_ENTRY(w->imap_folder), "INBOX");
    labeled_row(GTK_GRID(grid), 5, "Folder to scan for BTS mail", w->imap_folder);

    GtkWidget *hint = gtk_label_new(
        "Tip: create a mail filter that sorts mail from *@bugs.debian.org\n"
        "into its own folder, and point this at that folder.");
    gtk_label_set_xalign(GTK_LABEL(hint), 0.0);
    gtk_style_context_add_class(gtk_widget_get_style_context(hint), "bts-command-help");
    gtk_grid_attach(GTK_GRID(grid), hint, 0, 6, 2, 1);

    return grid;
}

static GtkWidget *
build_smtp_tab(SettingsWidgets *w, AppConfig *cfg)
{
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 6);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    gtk_container_set_border_width(GTK_CONTAINER(grid), 12);

    w->smtp_host = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(w->smtp_host), cfg->smtp_host);
    labeled_row(GTK_GRID(grid), 0, "SMTP server", w->smtp_host);

    w->smtp_port = gtk_spin_button_new_with_range(1, 65535, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(w->smtp_port), cfg->smtp_port);
    labeled_row(GTK_GRID(grid), 1, "Port", w->smtp_port);

    w->smtp_ssl = gtk_check_button_new_with_label("Use SSL/TLS (SMTPS, e.g. port 465)");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w->smtp_ssl), cfg->smtp_ssl);
    gtk_grid_attach(GTK_GRID(grid), w->smtp_ssl, 1, 2, 1, 1);

    w->smtp_user = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(w->smtp_user), cfg->smtp_user);
    labeled_row(GTK_GRID(grid), 3, "Username", w->smtp_user);

    w->smtp_pass = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(w->smtp_pass), FALSE);
    gtk_entry_set_text(GTK_ENTRY(w->smtp_pass), cfg->smtp_pass);
    labeled_row(GTK_GRID(grid), 4, "Password", w->smtp_pass);

    GtkWidget *hint = gtk_label_new(
        "Outgoing mail (bug reports, control commands, replies) is sent\n"
        "directly through this SMTP account - not relayed through anything else.");
    gtk_label_set_xalign(GTK_LABEL(hint), 0.0);
    gtk_style_context_add_class(gtk_widget_get_style_context(hint), "bts-command-help");
    gtk_grid_attach(GTK_GRID(grid), hint, 0, 5, 2, 1);

    return grid;
}

static GtkWidget *
build_identity_tab(SettingsWidgets *w, AppConfig *cfg)
{
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 6);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    gtk_container_set_border_width(GTK_CONTAINER(grid), 12);

    w->from_name = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(w->from_name), cfg->from_name);
    labeled_row(GTK_GRID(grid), 0, "Your name", w->from_name);

    w->from_email = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(w->from_email), cfg->from_email);
    labeled_row(GTK_GRID(grid), 1, "Your email address", w->from_email);

    GtkWidget *hint = gtk_label_new(
        "This is the address the BTS will identify you by - it must match\n"
        "the account you send from, since Debian's bug tracker authenticates\n"
        "control commands by From: address, not by password.");
    gtk_label_set_xalign(GTK_LABEL(hint), 0.0);
    gtk_label_set_line_wrap(GTK_LABEL(hint), TRUE);
    gtk_style_context_add_class(gtk_widget_get_style_context(hint), "bts-command-help");
    gtk_grid_attach(GTK_GRID(grid), hint, 0, 2, 2, 1);

    return grid;
}

void
settings_window_show(AppContext *ctx)
{
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Account Settings",
        GTK_WINDOW(ctx->main_window), GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Save", GTK_RESPONSE_OK,
        NULL);
    gtk_window_set_default_size(GTK_WINDOW(dialog), 440, 380);

    SettingsWidgets w = {0};
    GtkWidget *notebook = gtk_notebook_new();
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), build_imap_tab(&w, ctx->cfg), gtk_label_new("Incoming (IMAP)"));
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), build_smtp_tab(&w, ctx->cfg), gtk_label_new("Outgoing (SMTP)"));
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), build_identity_tab(&w, ctx->cfg), gtk_label_new("Identity"));

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_box_pack_start(GTK_BOX(content), notebook, TRUE, TRUE, 0);
    gtk_widget_show_all(dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        AppConfig *cfg = ctx->cfg;
        g_free(cfg->imap_host); cfg->imap_host = g_strdup(gtk_entry_get_text(GTK_ENTRY(w.imap_host)));
        cfg->imap_port = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(w.imap_port));
        cfg->imap_ssl = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w.imap_ssl));
        g_free(cfg->imap_user); cfg->imap_user = g_strdup(gtk_entry_get_text(GTK_ENTRY(w.imap_user)));
        g_free(cfg->imap_pass); cfg->imap_pass = g_strdup(gtk_entry_get_text(GTK_ENTRY(w.imap_pass)));
        g_free(cfg->imap_folder); cfg->imap_folder = g_strdup(gtk_entry_get_text(GTK_ENTRY(w.imap_folder)));

        g_free(cfg->smtp_host); cfg->smtp_host = g_strdup(gtk_entry_get_text(GTK_ENTRY(w.smtp_host)));
        cfg->smtp_port = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(w.smtp_port));
        cfg->smtp_ssl = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w.smtp_ssl));
        g_free(cfg->smtp_user); cfg->smtp_user = g_strdup(gtk_entry_get_text(GTK_ENTRY(w.smtp_user)));
        g_free(cfg->smtp_pass); cfg->smtp_pass = g_strdup(gtk_entry_get_text(GTK_ENTRY(w.smtp_pass)));

        g_free(cfg->from_name); cfg->from_name = g_strdup(gtk_entry_get_text(GTK_ENTRY(w.from_name)));
        g_free(cfg->from_email); cfg->from_email = g_strdup(gtk_entry_get_text(GTK_ENTRY(w.from_email)));

        GError *error = NULL;
        if (!config_save(cfg, &error)) {
            GtkWidget *err = gtk_message_dialog_new(GTK_WINDOW(ctx->main_window),
                GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                "Could not save settings: %s", error->message);
            gtk_dialog_run(GTK_DIALOG(err));
            gtk_widget_destroy(err);
            g_error_free(error);
        }
    }

    gtk_widget_destroy(dialog);
}
