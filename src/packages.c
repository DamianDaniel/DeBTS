#include "packages.h"

/* A curated set of the pseudo-packages documented at
 * https://www.debian.org/Bugs/pseudo-packages plus a handful of the most
 * commonly filed-against real packages/collections. The entry is editable
 * so any package name can be typed in directly. */
static const gchar *pseudo_packages[] = {
    "general",
    "base",
    "kernel",
    "installation-reports",
    "release-notes",
    "wnpp",
    "ftp.debian.org",
    "www.debian.org",
    "security.debian.org",
    "release.debian.org",
    "tech-ctte",
    "lists.debian.org",
    "listarchives",
    "mirrors",
    "nm.debian.org",
    "qa.debian.org",
    "sponsorship-requests",
    "press",
    "publicity",
    "cdimage.debian.org",
    "upgrade-reports",
    "spam",
    NULL
};

GtkWidget *
packages_create_combo(void)
{
    GtkWidget *combo = gtk_combo_box_text_new_with_entry();
    for (int i = 0; pseudo_packages[i]; i++) {
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), pseudo_packages[i]);
    }
    GtkWidget *entry = gtk_bin_get_child(GTK_BIN(combo));
    if (GTK_IS_ENTRY(entry)) {
        gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "package name or pseudo-package");

        GtkEntryCompletion *completion = gtk_entry_completion_new();
        GtkListStore *store = gtk_list_store_new(1, G_TYPE_STRING);
        for (int i = 0; pseudo_packages[i]; i++) {
            GtkTreeIter iter;
            gtk_list_store_append(store, &iter);
            gtk_list_store_set(store, &iter, 0, pseudo_packages[i], -1);
        }
        gtk_entry_completion_set_model(completion, GTK_TREE_MODEL(store));
        gtk_entry_completion_set_text_column(completion, 0);
        gtk_entry_completion_set_inline_completion(completion, TRUE);
        gtk_entry_set_completion(GTK_ENTRY(entry), completion);
        g_object_unref(store);
        g_object_unref(completion);
    }
    return combo;
}

void
packages_open_info_page(GtkWindow *parent)
{
    GError *error = NULL;
    gtk_show_uri_on_window(parent, BTS_PSEUDO_PACKAGES_URL, GDK_CURRENT_TIME, &error);
    if (error) {
        GtkWidget *dialog = gtk_message_dialog_new(parent, GTK_DIALOG_MODAL,
            GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
            "Could not open browser: %s", error->message);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        g_error_free(error);
    }
}
