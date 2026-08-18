#include "settings_window.h"
#include "settings_form.h"

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
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
        settings_form_build_imap_tab(&w, ctx->cfg), gtk_label_new("Incoming (IMAP)"));
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
        settings_form_build_smtp_tab(&w, ctx->cfg), gtk_label_new("Outgoing (SMTP)"));
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
        settings_form_build_identity_tab(&w, ctx->cfg), gtk_label_new("Identity"));

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_box_pack_start(GTK_BOX(content), notebook, TRUE, TRUE, 0);
    gtk_widget_show_all(dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        settings_form_apply(&w, ctx->cfg);
        ctx->cfg->setup_done = TRUE;

        GError *error = NULL;
        if (!config_save(ctx->cfg, &error)) {
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
