#include "compose_window.h"
#include "packages.h"
#include "mailer.h"
#include "control.h"

static const gchar *severities[] = {
    "critical", "grave", "serious", "important",
    "normal", "minor", "wishlist", NULL
};

typedef struct {
    AppContext *ctx;
    gboolean is_new_bug;
    gint bug_number;

    GtkWidget *window;
    GtkWidget *package_combo; /* new-bug only */
    GtkWidget *version_entry; /* new-bug only */
    GtkWidget *severity_combo; /* new-bug only */
    GtkWidget *subject_entry;
    GtkWidget *cc_entry;
    GtkTextView *body_view;
    GtkWidget *status_label;
} ComposeCtx;

static void
on_info_clicked(GtkButton *btn, gpointer user_data)
{
    (void) btn;
    ComposeCtx *cc = user_data;
    packages_open_info_page(GTK_WINDOW(cc->window));
}

static void
on_cancel_clicked(GtkButton *btn, gpointer user_data)
{
    (void) btn;
    ComposeCtx *cc = user_data;
    gtk_widget_destroy(cc->window);
}

static void
on_send_clicked(GtkButton *btn, gpointer user_data)
{
    (void) btn;
    ComposeCtx *cc = user_data;
    AppConfig *cfg = cc->ctx->cfg;

    if (!cfg->smtp_host || !*cfg->smtp_host || !cfg->from_email || !*cfg->from_email) {
        gtk_label_set_text(GTK_LABEL(cc->status_label),
            "Set up your SMTP server and email address in Settings first.");
        return;
    }

    GtkTextBuffer *buf = gtk_text_view_get_buffer(cc->body_view);
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(buf, &start, &end);
    gchar *body_text = gtk_text_buffer_get_text(buf, &start, &end, FALSE);
    const gchar *subject = gtk_entry_get_text(GTK_ENTRY(cc->subject_entry));
    const gchar *cc_addr = gtk_entry_get_text(GTK_ENTRY(cc->cc_entry));

    gchar *to = NULL;
    gchar *full_body = NULL;

    if (cc->is_new_bug) {
        const gchar *package = gtk_combo_box_text_get_active_text(
            GTK_COMBO_BOX_TEXT(cc->package_combo));
        const gchar *version = gtk_entry_get_text(GTK_ENTRY(cc->version_entry));
        const gchar *severity = gtk_combo_box_text_get_active_text(
            GTK_COMBO_BOX_TEXT(cc->severity_combo));

        if (!package || !*package) {
            gtk_label_set_text(GTK_LABEL(cc->status_label), "Please choose a package.");
            g_free(body_text);
            return;
        }
        if (!subject || !*subject) {
            gtk_label_set_text(GTK_LABEL(cc->status_label), "Please give the bug a title.");
            g_free(body_text);
            return;
        }

        GString *pseudo = g_string_new(NULL);
        g_string_append_printf(pseudo, "Package: %s\n", package);
        if (version && *version) g_string_append_printf(pseudo, "Version: %s\n", version);
        g_string_append_printf(pseudo, "Severity: %s\n", severity ? severity : "normal");
        g_string_append(pseudo, "\n");
        g_string_append(pseudo, body_text);

        full_body = g_string_free(pseudo, FALSE);
        to = g_strdup(BTS_SUBMIT_ADDRESS);
    } else {
        to = g_strdup_printf("%d@%s", cc->bug_number, BTS_BUG_DOMAIN);
        full_body = g_strdup(body_text);
    }

    GError *error = NULL;
    gboolean ok = mailer_send(cfg, to, (cc_addr && *cc_addr) ? cc_addr : NULL,
                               subject, full_body, NULL, &error);

    if (ok) {
        gtk_widget_destroy(cc->window);
    } else {
        gchar *msg = g_strdup_printf("Send failed: %s", error ? error->message : "unknown error");
        gtk_label_set_text(GTK_LABEL(cc->status_label), msg);
        g_free(msg);
        if (error) g_error_free(error);
    }

    g_free(to);
    g_free(full_body);
    g_free(body_text);
}

static void
compose_window_open(AppContext *ctx, gboolean is_new_bug, gint bug_number, const gchar *subject_hint)
{
    ComposeCtx *cc = g_new0(ComposeCtx, 1);
    cc->ctx = ctx;
    cc->is_new_bug = is_new_bug;
    cc->bug_number = bug_number;

    cc->window = gtk_application_window_new(ctx->app);
    gtk_window_set_title(GTK_WINDOW(cc->window),
        is_new_bug ? "File a New Bug" : "Reply to Bug");
    gtk_window_set_default_size(GTK_WINDOW(cc->window), 620, 520);
    gtk_window_set_transient_for(GTK_WINDOW(cc->window), GTK_WINDOW(ctx->main_window));
    g_object_set_data_full(G_OBJECT(cc->window), "compose-ctx", cc, g_free);

    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_container_set_border_width(GTK_CONTAINER(root), 12);
    gtk_container_add(GTK_CONTAINER(cc->window), root);

    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 6);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 8);
    gtk_box_pack_start(GTK_BOX(root), grid, FALSE, FALSE, 0);

    gint row = 0;

    if (is_new_bug) {
        GtkWidget *pkg_label = gtk_label_new("Package");
        gtk_label_set_xalign(GTK_LABEL(pkg_label), 0.0);
        gtk_grid_attach(GTK_GRID(grid), pkg_label, 0, row, 1, 1);

        GtkWidget *pkg_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
        cc->package_combo = packages_create_combo();
        gtk_widget_set_hexpand(cc->package_combo, TRUE);
        GtkWidget *info_btn = gtk_button_new_from_icon_name("dialog-information-symbolic", GTK_ICON_SIZE_BUTTON);
        gtk_widget_set_tooltip_text(info_btn, "What are pseudo-packages? (opens debian.org)");
        g_signal_connect(info_btn, "clicked", G_CALLBACK(on_info_clicked), cc);
        gtk_box_pack_start(GTK_BOX(pkg_box), cc->package_combo, TRUE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(pkg_box), info_btn, FALSE, FALSE, 0);
        gtk_grid_attach(GTK_GRID(grid), pkg_box, 1, row, 1, 1);
        row++;

        GtkWidget *ver_label = gtk_label_new("Version");
        gtk_label_set_xalign(GTK_LABEL(ver_label), 0.0);
        gtk_grid_attach(GTK_GRID(grid), ver_label, 0, row, 1, 1);
        cc->version_entry = gtk_entry_new();
        gtk_entry_set_placeholder_text(GTK_ENTRY(cc->version_entry), "e.g. 1.2.3-1 (optional)");
        gtk_widget_set_hexpand(cc->version_entry, TRUE);
        gtk_grid_attach(GTK_GRID(grid), cc->version_entry, 1, row, 1, 1);
        row++;

        GtkWidget *sev_label = gtk_label_new("Severity");
        gtk_label_set_xalign(GTK_LABEL(sev_label), 0.0);
        gtk_grid_attach(GTK_GRID(grid), sev_label, 0, row, 1, 1);
        cc->severity_combo = gtk_combo_box_text_new();
        for (int i = 0; severities[i]; i++)
            gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cc->severity_combo), severities[i]);
        gtk_combo_box_set_active(GTK_COMBO_BOX(cc->severity_combo), 4); /* normal */
        gtk_grid_attach(GTK_GRID(grid), cc->severity_combo, 1, row, 1, 1);
        row++;
    }

    GtkWidget *subj_label = gtk_label_new(is_new_bug ? "Title" : "Subject");
    gtk_label_set_xalign(GTK_LABEL(subj_label), 0.0);
    gtk_grid_attach(GTK_GRID(grid), subj_label, 0, row, 1, 1);
    cc->subject_entry = gtk_entry_new();
    gtk_widget_set_hexpand(cc->subject_entry, TRUE);
    if (subject_hint) gtk_entry_set_text(GTK_ENTRY(cc->subject_entry), subject_hint);
    gtk_grid_attach(GTK_GRID(grid), cc->subject_entry, 1, row, 1, 1);
    row++;

    GtkWidget *cc_label = gtk_label_new("Cc (optional)");
    gtk_label_set_xalign(GTK_LABEL(cc_label), 0.0);
    gtk_grid_attach(GTK_GRID(grid), cc_label, 0, row, 1, 1);
    cc->cc_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(cc->cc_entry), "extra-recipient@example.com");
    gtk_grid_attach(GTK_GRID(grid), cc->cc_entry, 1, row, 1, 1);
    row++;

    if (!is_new_bug) {
        gchar *to_hint = g_strdup_printf("Sending to Bug #%d (%d@%s)", bug_number, bug_number, BTS_BUG_DOMAIN);
        GtkWidget *to_label = gtk_label_new(to_hint);
        gtk_label_set_xalign(GTK_LABEL(to_label), 0.0);
        gtk_style_context_add_class(gtk_widget_get_style_context(to_label), "bts-command-help");
        gtk_grid_attach(GTK_GRID(grid), to_label, 0, row, 2, 1);
        g_free(to_hint);
        row++;
    }

    GtkWidget *scroller = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_vexpand(scroller, TRUE);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroller),
        GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    GtkWidget *textview = gtk_text_view_new();
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(textview), GTK_WRAP_WORD);
    gtk_container_add(GTK_CONTAINER(scroller), textview);
    cc->body_view = GTK_TEXT_VIEW(textview);
    gtk_box_pack_start(GTK_BOX(root), scroller, TRUE, TRUE, 0);

    cc->status_label = gtk_label_new("");
    gtk_label_set_xalign(GTK_LABEL(cc->status_label), 0.0);
    gtk_style_context_add_class(gtk_widget_get_style_context(cc->status_label), "bts-command-help");
    gtk_box_pack_start(GTK_BOX(root), cc->status_label, FALSE, FALSE, 0);

    GtkWidget *btn_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_widget_set_halign(btn_box, GTK_ALIGN_END);
    GtkWidget *cancel_btn = gtk_button_new_with_label("Cancel");
    g_signal_connect(cancel_btn, "clicked", G_CALLBACK(on_cancel_clicked), cc);
    GtkWidget *send_btn = gtk_button_new_with_label(is_new_bug ? "Submit Bug" : "Send Reply");
    gtk_style_context_add_class(gtk_widget_get_style_context(send_btn), "suggested-action");
    g_signal_connect(send_btn, "clicked", G_CALLBACK(on_send_clicked), cc);
    gtk_box_pack_start(GTK_BOX(btn_box), cancel_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(btn_box), send_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(root), btn_box, FALSE, FALSE, 0);

    gtk_widget_show_all(cc->window);
}

void
compose_window_new_bug(AppContext *ctx)
{
    compose_window_open(ctx, TRUE, 0, NULL);
}

void
compose_window_reply(AppContext *ctx, gint bug_number, const gchar *subject_hint)
{
    compose_window_open(ctx, FALSE, bug_number, subject_hint);
}
