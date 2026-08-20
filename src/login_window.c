#include "login_window.h"
#include "settings_form.h"

typedef struct {
    AppContext *ctx;
    GtkWidget *window;
    void (*on_done)(AppContext *);
    SettingsWidgets fields;
} LoginCtx;

static void
finish(LoginCtx *lc)
{
    void (*cb)(AppContext *) = lc->on_done;
    AppContext *ctx = lc->ctx;
    gtk_widget_destroy(lc->window);
    g_free(lc);
    cb(ctx);
}

static void
on_guest_clicked(GtkButton *btn, gpointer user_data)
{
    (void) btn;
    LoginCtx *lc = user_data;
    lc->ctx->cfg->setup_done = TRUE;
    config_save(lc->ctx->cfg, NULL);
    finish(lc);
}

static void
on_login_save_clicked(GtkButton *btn, gpointer user_data)
{
    (void) btn;
    LoginCtx *lc = user_data;
    settings_form_apply(&lc->fields, lc->ctx->cfg);
    lc->ctx->cfg->setup_done = TRUE;
    config_save(lc->ctx->cfg, NULL);
    finish(lc);
}

static void
show_login_form(GtkWidget *stack, LoginCtx *lc)
{
    GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_container_set_border_width(GTK_CONTAINER(page), 12);

    GtkWidget *notebook = gtk_notebook_new();
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
        settings_form_build_imap_tab(&lc->fields, lc->ctx->cfg), gtk_label_new("Incoming"));
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
        settings_form_build_smtp_tab(&lc->fields, lc->ctx->cfg), gtk_label_new("Outgoing"));
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
        settings_form_build_identity_tab(&lc->fields, lc->ctx->cfg), gtk_label_new("Identity"));
    gtk_box_pack_start(GTK_BOX(page), notebook, TRUE, TRUE, 0);

    GtkWidget *btn_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_widget_set_halign(btn_box, GTK_ALIGN_END);
    GtkWidget *save_btn = gtk_button_new_with_label("Log In & Continue");
    gtk_style_context_add_class(gtk_widget_get_style_context(save_btn), "suggested-action");
    g_signal_connect(save_btn, "clicked", G_CALLBACK(on_login_save_clicked), lc);
    gtk_box_pack_start(GTK_BOX(btn_box), save_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(page), btn_box, FALSE, FALSE, 0);

    gtk_widget_show_all(page);
    gtk_container_add(GTK_CONTAINER(stack), page);
    gtk_stack_set_visible_child(GTK_STACK(stack), page);
}

static void
on_login_clicked(GtkButton *btn, gpointer user_data)
{
    (void) btn;
    LoginCtx *lc = user_data;
    GtkWidget *stack = g_object_get_data(G_OBJECT(lc->window), "stack");
    show_login_form(stack, lc);
}

void
login_window_show(AppContext *ctx, void (*on_done)(AppContext *))
{
    LoginCtx *lc = g_new0(LoginCtx, 1);
    lc->ctx = ctx;
    lc->on_done = on_done;

    GtkWidget *window = gtk_application_window_new(ctx->app);
    lc->window = window;
    gtk_window_set_title(GTK_WINDOW(window), "Debian BTS Client");
    gtk_window_set_default_size(GTK_WINDOW(window), 480, 420);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);

    GtkWidget *stack = gtk_stack_new();
    g_object_set_data(G_OBJECT(window), "stack", stack);
    gtk_container_add(GTK_CONTAINER(window), stack);

    GtkWidget *welcome = gtk_box_new(GTK_ORIENTATION_VERTICAL, 16);
    gtk_container_set_border_width(GTK_CONTAINER(welcome), 24);
    gtk_widget_set_valign(welcome, GTK_ALIGN_CENTER);

    GtkWidget *title = gtk_label_new("Debian BTS Client");
    gtk_style_context_add_class(gtk_widget_get_style_context(title), "debts-panel-title");
    gtk_widget_set_halign(title, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(welcome), title, FALSE, FALSE, 0);

    GtkWidget *blurb = gtk_label_new(
        "Log in with your mail account to send bug reports and control\n"
        "commands, and to see bugs tied to your email. You can also just\n"
        "browse and search public bug reports as a guest.");
    gtk_label_set_justify(GTK_LABEL(blurb), GTK_JUSTIFY_CENTER);
    gtk_widget_set_halign(blurb, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(welcome), blurb, FALSE, FALSE, 0);

    GtkWidget *login_btn = gtk_button_new_with_label("Log In With Email");
    gtk_style_context_add_class(gtk_widget_get_style_context(login_btn), "suggested-action");
    gtk_widget_set_halign(login_btn, GTK_ALIGN_CENTER);
    g_signal_connect(login_btn, "clicked", G_CALLBACK(on_login_clicked), lc);
    gtk_box_pack_start(GTK_BOX(welcome), login_btn, FALSE, FALSE, 0);

    GtkWidget *guest_btn = gtk_button_new_with_label("Continue as Guest");
    gtk_widget_set_halign(guest_btn, GTK_ALIGN_CENTER);
    g_signal_connect(guest_btn, "clicked", G_CALLBACK(on_guest_clicked), lc);
    gtk_box_pack_start(GTK_BOX(welcome), guest_btn, FALSE, FALSE, 0);

    gtk_container_add(GTK_CONTAINER(stack), welcome);
    gtk_stack_set_visible_child(GTK_STACK(stack), welcome);

    gtk_widget_show_all(window);
}
