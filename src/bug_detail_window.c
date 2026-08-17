#include "bug_detail_window.h"
#include "compose_window.h"
#include "control_window.h"

typedef struct {
    AppContext *ctx;
    gint bug_number;
    gchar *subject_hint;
} DetailActionCtx;

static void
detail_action_ctx_free(gpointer data)
{
    DetailActionCtx *d = data;
    g_free(d->subject_hint);
    g_free(d);
}

static void
load_thread_into_view(GtkTextView *view, AppConfig *cfg, GList *headers)
{
    GtkTextBuffer *buf = gtk_text_view_get_buffer(view);
    GtkTextIter end;

    for (GList *l = headers; l; l = l->next) {
        MailHeader *h = (MailHeader *) l->data;
        GError *error = NULL;
        gchar *full = mailer_fetch_full(cfg, h->uid, &error);

        gtk_text_buffer_get_end_iter(buf, &end);
        gchar *divider = g_strdup_printf(
            "\n\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80"
            " %s \xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\xe2\x94\x80\n\n",
            h->date ? h->date : "");
        gtk_text_buffer_insert(buf, &end, divider, -1);
        g_free(divider);

        gtk_text_buffer_get_end_iter(buf, &end);
        if (full) {
            gtk_text_buffer_insert(buf, &end, full, -1);
            g_free(full);
        } else {
            gchar *err_text = g_strdup_printf("[could not fetch message: %s]\n",
                error ? error->message : "unknown error");
            gtk_text_buffer_insert(buf, &end, err_text, -1);
            g_free(err_text);
        }
        if (error) g_error_free(error);
    }
}

static void
on_reply_clicked(GtkButton *btn, gpointer user_data)
{
    (void) btn;
    DetailActionCtx *d = user_data;
    compose_window_reply(d->ctx, d->bug_number, d->subject_hint);
}

static void
on_control_clicked(GtkButton *btn, gpointer user_data)
{
    (void) btn;
    DetailActionCtx *d = user_data;
    control_window_show_for_bug(d->ctx, d->bug_number, "retitle");
}

void
bug_detail_window_show(AppContext *ctx, Bug *bug)
{
    GtkWidget *window = gtk_application_window_new(ctx->app);
    gchar *title = g_strdup_printf("Bug #%d", bug->number);
    gtk_window_set_title(GTK_WINDOW(window), title);
    g_free(title);
    gtk_window_set_default_size(GTK_WINDOW(window), 720, 600);
    gtk_window_set_transient_for(GTK_WINDOW(window), GTK_WINDOW(ctx->main_window));

    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_container_set_border_width(GTK_CONTAINER(root), 12);
    gtk_container_add(GTK_CONTAINER(window), root);

    gchar *summary = g_strdup_printf("#%d \xe2\x80\x94 %s\nPackage: %s   Severity: %s   Status: %s",
        bug->number,
        bug->title ? bug->title : "(no title known)",
        bug->package ? bug->package : "(unknown)",
        bug->severity ? bug->severity : "(unknown)",
        bug->status ? bug->status : "(unknown)");
    GtkWidget *summary_label = gtk_label_new(summary);
    gtk_label_set_xalign(GTK_LABEL(summary_label), 0.0);
    gtk_label_set_line_wrap(GTK_LABEL(summary_label), TRUE);
    gtk_box_pack_start(GTK_BOX(root), summary_label, FALSE, FALSE, 0);
    g_free(summary);

    GtkWidget *scroller = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_vexpand(scroller, TRUE);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroller),
        GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    GtkWidget *textview = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(textview), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(textview), GTK_WRAP_WORD_CHAR);
    gtk_style_context_add_class(gtk_widget_get_style_context(textview), "bts-message-view");
    gtk_container_add(GTK_CONTAINER(scroller), textview);
    gtk_box_pack_start(GTK_BOX(root), scroller, TRUE, TRUE, 0);

    GtkWidget *btn_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_widget_set_halign(btn_box, GTK_ALIGN_END);

    DetailActionCtx *d = g_new0(DetailActionCtx, 1);
    d->ctx = ctx;
    d->bug_number = bug->number;
    d->subject_hint = g_strdup_printf("Bug#%d: %s", bug->number, bug->title ? bug->title : "");
    /* Both buttons share the same context; free it once, when the window closes. */
    g_object_set_data_full(G_OBJECT(window), "detail-action-ctx", d, detail_action_ctx_free);

    GtkWidget *reply_btn = gtk_button_new_with_label("Reply\xe2\x80\xa6");
    g_signal_connect(reply_btn, "clicked", G_CALLBACK(on_reply_clicked), d);
    gtk_box_pack_start(GTK_BOX(btn_box), reply_btn, FALSE, FALSE, 0);

    GtkWidget *control_btn = gtk_button_new_with_label("Control Commands\xe2\x80\xa6");
    g_signal_connect(control_btn, "clicked", G_CALLBACK(on_control_clicked), d);
    gtk_box_pack_start(GTK_BOX(btn_box), control_btn, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(root), btn_box, FALSE, FALSE, 0);

    gtk_widget_show_all(window);
    load_thread_into_view(GTK_TEXT_VIEW(textview), ctx->cfg, bug->headers);
}
