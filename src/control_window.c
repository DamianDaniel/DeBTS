#include "control_window.h"
#include "control.h"
#include "mailer.h"

typedef struct {
    AppContext *ctx;
    GtkWidget *window;
    GtkWidget *command_combo;
    GtkWidget *params_box;
    GtkWidget *params_entries[4];
    gint params_entry_count;
    GtkWidget *help_label;
    GtkTextView *batch_view;
    GtkWidget *status_label;
} ControlCtx;

static const ControlCommandDef *
selected_def(ControlCtx *cw)
{
    gint idx = gtk_combo_box_get_active(GTK_COMBO_BOX(cw->command_combo));
    if (idx < 0 || idx >= bts_control_commands_count) return NULL;
    return &bts_control_commands[idx];
}

static void
destroy_child(GtkWidget *child, gpointer user_data)
{
    (void) user_data;
    gtk_widget_destroy(child);
}

static void
rebuild_params(ControlCtx *cw)
{
    gtk_container_foreach(GTK_CONTAINER(cw->params_box), destroy_child, NULL);
    cw->params_entry_count = 0;

    const ControlCommandDef *def = selected_def(cw);
    if (!def) return;

    for (gint i = 0; i < def->param_count; i++) {
        GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
        GtkWidget *label = gtk_label_new(def->param_labels[i]);
        gtk_label_set_xalign(GTK_LABEL(label), 0.0);
        gtk_widget_set_size_request(label, 210, -1);
        GtkWidget *entry = gtk_entry_new();
        gtk_widget_set_hexpand(entry, TRUE);
        gtk_box_pack_start(GTK_BOX(row), label, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(row), entry, TRUE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(cw->params_box), row, FALSE, FALSE, 0);
        cw->params_entries[cw->params_entry_count++] = entry;
    }
    gtk_label_set_text(GTK_LABEL(cw->help_label), def->help);
    gtk_widget_show_all(cw->params_box);
}

static void
on_command_changed(GtkComboBox *combo, gpointer user_data)
{
    (void) combo;
    rebuild_params((ControlCtx *) user_data);
}

static void
append_line_to_batch(ControlCtx *cw, const gchar *line)
{
    GtkTextBuffer *buf = gtk_text_view_get_buffer(cw->batch_view);
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(buf, &end);
    if (gtk_text_buffer_get_char_count(buf) > 0)
        gtk_text_buffer_insert(buf, &end, "\n", -1);
    gtk_text_buffer_get_end_iter(buf, &end);
    gtk_text_buffer_insert(buf, &end, line, -1);
}

static void
on_add_clicked(GtkButton *btn, gpointer user_data)
{
    (void) btn;
    ControlCtx *cw = user_data;
    const ControlCommandDef *def = selected_def(cw);
    if (!def) return;

    gchar *params[4] = {0};
    for (gint i = 0; i < cw->params_entry_count; i++) {
        params[i] = g_strdup(gtk_entry_get_text(GTK_ENTRY(cw->params_entries[i])));
    }
    gchar *line = control_command_format(def, params);
    append_line_to_batch(cw, line);
    g_free(line);
    for (gint i = 0; i < cw->params_entry_count; i++) g_free(params[i]);

    gtk_label_set_text(GTK_LABEL(cw->status_label), "");
}

static void
on_send_clicked(GtkButton *btn, gpointer user_data)
{
    (void) btn;
    ControlCtx *cw = user_data;
    AppConfig *cfg = cw->ctx->cfg;

    if (!cfg->smtp_host || !*cfg->smtp_host || !cfg->from_email || !*cfg->from_email) {
        gtk_label_set_text(GTK_LABEL(cw->status_label),
            "Set up your SMTP server and email address in Settings first.");
        return;
    }

    GtkTextBuffer *buf = gtk_text_view_get_buffer(cw->batch_view);
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(buf, &start, &end);
    gchar *commands = gtk_text_buffer_get_text(buf, &start, &end, FALSE);
    g_strstrip(commands);

    if (!*commands) {
        gtk_label_set_text(GTK_LABEL(cw->status_label), "Add at least one command first.");
        g_free(commands);
        return;
    }

    /* The BTS control bot reads commands until it hits a line that says
     * "thanks", "quit", "stop", or "--". Make sure we terminate cleanly. */
    gchar *lower = g_ascii_strdown(commands, -1);
    gboolean terminated = g_str_has_suffix(lower, "thanks") ||
                           g_str_has_suffix(lower, "quit") ||
                           g_str_has_suffix(lower, "stop");
    g_free(lower);

    gchar *body = terminated ? g_strdup(commands)
                              : g_strdup_printf("%s\nthanks", commands);

    GError *error = NULL;
    gboolean ok = mailer_send(cfg, BTS_CONTROL_ADDRESS, NULL,
                               "control message for Debian BTS", body, NULL, &error);

    if (ok) {
        gtk_label_set_text(GTK_LABEL(cw->status_label), "Sent to control@bugs.debian.org.");
        gtk_text_buffer_set_text(buf, "", -1);
    } else {
        gchar *msg = g_strdup_printf("Send failed: %s", error ? error->message : "unknown error");
        gtk_label_set_text(GTK_LABEL(cw->status_label), msg);
        g_free(msg);
        if (error) g_error_free(error);
    }

    g_free(body);
    g_free(commands);
}

static void
build_and_show(AppContext *ctx, gint prefill_bug, const gchar *prefill_command_key)
{
    ControlCtx *cw = g_new0(ControlCtx, 1);
    cw->ctx = ctx;

    cw->window = gtk_application_window_new(ctx->app);
    gtk_window_set_title(GTK_WINDOW(cw->window), "BTS Control Commands");
    gtk_window_set_default_size(GTK_WINDOW(cw->window), 640, 560);
    gtk_window_set_transient_for(GTK_WINDOW(cw->window), GTK_WINDOW(ctx->main_window));
    g_object_set_data_full(G_OBJECT(cw->window), "control-ctx", cw, g_free);

    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_container_set_border_width(GTK_CONTAINER(root), 12);
    gtk_container_add(GTK_CONTAINER(cw->window), root);

    GtkWidget *intro = gtk_label_new(
        "Build one or more commands, add each to the batch below, then send them\n"
        "all at once as a single email to control@bugs.debian.org.");
    gtk_label_set_xalign(GTK_LABEL(intro), 0.0);
    gtk_box_pack_start(GTK_BOX(root), intro, FALSE, FALSE, 0);

    GtkWidget *combo_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    GtkWidget *combo_label = gtk_label_new("Command");
    cw->command_combo = gtk_combo_box_text_new();
    gint default_idx = 0;
    for (gint i = 0; i < bts_control_commands_count; i++) {
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cw->command_combo),
            bts_control_commands[i].label);
        if (prefill_command_key && g_strcmp0(bts_control_commands[i].key, prefill_command_key) == 0)
            default_idx = i;
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(cw->command_combo), default_idx);
    g_signal_connect(cw->command_combo, "changed", G_CALLBACK(on_command_changed), cw);
    gtk_box_pack_start(GTK_BOX(combo_row), combo_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(combo_row), cw->command_combo, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(root), combo_row, FALSE, FALSE, 0);

    cw->help_label = gtk_label_new("");
    gtk_label_set_xalign(GTK_LABEL(cw->help_label), 0.0);
    gtk_label_set_line_wrap(GTK_LABEL(cw->help_label), TRUE);
    gtk_style_context_add_class(gtk_widget_get_style_context(cw->help_label), "bts-command-help");
    gtk_box_pack_start(GTK_BOX(root), cw->help_label, FALSE, FALSE, 0);

    cw->params_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_box_pack_start(GTK_BOX(root), cw->params_box, FALSE, FALSE, 0);
    rebuild_params(cw);

    if (prefill_bug > 0 && cw->params_entry_count > 0) {
        gchar *num_str = g_strdup_printf("%d", prefill_bug);
        gtk_entry_set_text(GTK_ENTRY(cw->params_entries[0]), num_str);
        g_free(num_str);
    }

    GtkWidget *add_btn = gtk_button_new_with_label("Add to Batch \xE2\x86\x93");
    gtk_widget_set_halign(add_btn, GTK_ALIGN_START);
    g_signal_connect(add_btn, "clicked", G_CALLBACK(on_add_clicked), cw);
    gtk_box_pack_start(GTK_BOX(root), add_btn, FALSE, FALSE, 0);

    GtkWidget *batch_label = gtk_label_new("Batch to send (editable):");
    gtk_label_set_xalign(GTK_LABEL(batch_label), 0.0);
    gtk_box_pack_start(GTK_BOX(root), batch_label, FALSE, FALSE, 0);

    GtkWidget *scroller = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_vexpand(scroller, TRUE);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroller),
        GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    GtkWidget *textview = gtk_text_view_new();
    gtk_style_context_add_class(gtk_widget_get_style_context(textview), "bts-message-view");
    gtk_container_add(GTK_CONTAINER(scroller), textview);
    cw->batch_view = GTK_TEXT_VIEW(textview);
    gtk_box_pack_start(GTK_BOX(root), scroller, TRUE, TRUE, 0);

    cw->status_label = gtk_label_new("");
    gtk_label_set_xalign(GTK_LABEL(cw->status_label), 0.0);
    gtk_style_context_add_class(gtk_widget_get_style_context(cw->status_label), "bts-command-help");
    gtk_box_pack_start(GTK_BOX(root), cw->status_label, FALSE, FALSE, 0);

    GtkWidget *btn_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_widget_set_halign(btn_box, GTK_ALIGN_END);
    GtkWidget *close_btn = gtk_button_new_with_label("Close");
    g_signal_connect_swapped(close_btn, "clicked", G_CALLBACK(gtk_widget_destroy), cw->window);
    GtkWidget *send_btn = gtk_button_new_with_label("Send Control Email");
    gtk_style_context_add_class(gtk_widget_get_style_context(send_btn), "suggested-action");
    g_signal_connect(send_btn, "clicked", G_CALLBACK(on_send_clicked), cw);
    gtk_box_pack_start(GTK_BOX(btn_box), close_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(btn_box), send_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(root), btn_box, FALSE, FALSE, 0);

    gtk_widget_show_all(cw->window);
}

void
control_window_show(AppContext *ctx)
{
    build_and_show(ctx, 0, NULL);
}

void
control_window_show_for_bug(AppContext *ctx, gint bug_number, const gchar *command_key)
{
    build_and_show(ctx, bug_number, command_key);
}
