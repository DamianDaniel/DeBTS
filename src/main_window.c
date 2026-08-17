#include "main_window.h"
#include "settings_window.h"
#include "compose_window.h"
#include "control_window.h"
#include "bug_detail_window.h"
#include "mailer.h"
#include "bug.h"

#define DEBTS_APP_NAME "Debian BTS Client"

enum {
    COL_NUMBER = 0,
    COL_PACKAGE,
    COL_SEVERITY,
    COL_STATUS,
    COL_TITLE,
    COL_BUG_PTR,
    N_COLS
};

typedef struct {
    AppContext *ctx;
    GtkWidget *treeview;
    GtkTreeModelFilter *filtered;
    GtkWidget *status_bar;
    GtkWidget *refresh_btn;
    GtkWidget *jump_entry;
    gint filter_mode; /* 0 = all, 1 = open, 2 = done */
} MainUi;

typedef struct {
    AppContext *ctx;
    MainUi *ui;
    GList *headers;
    GError *error;
} FetchResult;

static gboolean apply_fetch_result_idle(gpointer data);

static gpointer
fetch_thread_func(gpointer data)
{
    FetchResult *res = data;
    res->headers = mailer_list_headers(res->ctx->cfg, &res->error);
    g_idle_add(apply_fetch_result_idle, res);
    return NULL;
}

static void
populate_store_from_bugs(GtkListStore *store, GList *bugs)
{
    gtk_list_store_clear(store);
    for (GList *l = bugs; l; l = l->next) {
        Bug *b = l->data;
        gchar *num_str = g_strdup_printf("#%d", b->number);
        GtkTreeIter iter;
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter,
            COL_NUMBER, num_str,
            COL_PACKAGE, b->package ? b->package : "?",
            COL_SEVERITY, b->severity ? b->severity : "?",
            COL_STATUS, b->status ? b->status : "?",
            COL_TITLE, b->title ? b->title : "(unknown title)",
            COL_BUG_PTR, b,
            -1);
        g_free(num_str);
    }
}

static gboolean
apply_fetch_result_idle(gpointer data)
{
    FetchResult *res = data;
    AppContext *ctx = res->ctx;
    MainUi *ui = res->ui;

    gtk_widget_set_sensitive(ui->refresh_btn, TRUE);

    if (res->error) {
        gchar *msg = g_strdup_printf("Refresh failed: %s", res->error->message);
        gtk_label_set_text(GTK_LABEL(ui->status_bar), msg);
        g_free(msg);
        g_error_free(res->error);
    } else {
        /* replace cached headers/bugs */
        g_list_free_full(ctx->headers, (GDestroyNotify) mail_header_free);
        ctx->headers = res->headers;

        g_list_free_full(ctx->bugs, (GDestroyNotify) bug_free);
        ctx->bugs = bug_group_from_headers(ctx->headers);

        GtkTreeModel *base = gtk_tree_model_filter_get_model(ui->filtered);
        populate_store_from_bugs(GTK_LIST_STORE(base), ctx->bugs);

        gchar *msg = g_strdup_printf("%d bug thread(s) found in \xE2\x80\x9C%s\xE2\x80\x9D.",
            g_list_length(ctx->bugs), ctx->cfg->imap_folder);
        gtk_label_set_text(GTK_LABEL(ui->status_bar), msg);
        g_free(msg);
    }

    g_free(res);
    return G_SOURCE_REMOVE;
}

static void
on_refresh_clicked(GtkButton *btn, gpointer user_data)
{
    (void) btn;
    MainUi *ui = user_data;
    AppConfig *cfg = ui->ctx->cfg;

    if (!cfg->imap_host || !*cfg->imap_host || !cfg->imap_user || !*cfg->imap_user) {
        gtk_label_set_text(GTK_LABEL(ui->status_bar),
            "Set up your IMAP account in Settings first.");
        return;
    }

    gtk_widget_set_sensitive(ui->refresh_btn, FALSE);
    gtk_label_set_text(GTK_LABEL(ui->status_bar), "Fetching mail\xE2\x80\xA6");

    FetchResult *res = g_new0(FetchResult, 1);
    res->ctx = ui->ctx;
    res->ui = ui;
    GThread *t = g_thread_new("debts-fetch", fetch_thread_func, res);
    g_thread_unref(t);
}

static void
on_new_bug_clicked(GtkButton *btn, gpointer user_data)
{
    (void) btn;
    MainUi *ui = user_data;
    compose_window_new_bug(ui->ctx);
}

static void
on_control_clicked(GtkButton *btn, gpointer user_data)
{
    (void) btn;
    MainUi *ui = user_data;
    control_window_show(ui->ctx);
}

static void
on_settings_clicked(GtkButton *btn, gpointer user_data)
{
    (void) btn;
    MainUi *ui = user_data;
    settings_window_show(ui->ctx);
}

static void
on_about_clicked(GtkButton *btn, gpointer user_data)
{
    (void) btn;
    MainUi *ui = user_data;
    const gchar *authors[] = { "You", NULL };
    gtk_show_about_dialog(GTK_WINDOW(ui->ctx->main_window),
        "program-name", DEBTS_APP_NAME,
        "version", "0.1",
        "comments", "A friendly desktop client for the Debian Bug Tracking System,\n"
                    "talking to it entirely over your own IMAP/SMTP mail account -\n"
                    "no other mail client required.",
        "website", "https://www.debian.org/Bugs/",
        "website-label", "Debian BTS documentation",
        "authors", authors,
        "license-type", GTK_LICENSE_GPL_3_0,
        NULL);
}

static void
on_jump_go_clicked(GtkButton *btn, gpointer user_data)
{
    (void) btn;
    MainUi *ui = user_data;
    const gchar *text = gtk_entry_get_text(GTK_ENTRY(ui->jump_entry));
    if (!text || !*text) return;

    /* Accept "123456" or "#123456" or "Bug#123456". */
    while (*text && !g_ascii_isdigit(*text)) text++;
    gint number = atoi(text);
    if (number <= 0) {
        gtk_label_set_text(GTK_LABEL(ui->status_bar), "Enter a bug number to jump to.");
        return;
    }

    for (GList *l = ui->ctx->bugs; l; l = l->next) {
        Bug *b = l->data;
        if (b->number == number) {
            bug_detail_window_show(ui->ctx, b);
            return;
        }
    }
    gchar *msg = g_strdup_printf(
        "Bug #%d isn't in your loaded mail - it may not be in \xE2\x80\x9C%s\xE2\x80\x9D, "
        "or you need to hit Refresh.", number, ui->ctx->cfg->imap_folder);
    gtk_label_set_text(GTK_LABEL(ui->status_bar), msg);
    g_free(msg);
}

static void
on_row_activated(GtkTreeView *tv, GtkTreePath *path, GtkTreeViewColumn *col, gpointer user_data)
{
    (void) tv; (void) col;
    MainUi *ui = user_data;
    GtkTreeIter filtered_iter, child_iter;

    if (!gtk_tree_model_get_iter(GTK_TREE_MODEL(ui->filtered), &filtered_iter, path)) return;
    gtk_tree_model_filter_convert_iter_to_child_iter(ui->filtered, &child_iter, &filtered_iter);

    GtkTreeModel *base = gtk_tree_model_filter_get_model(ui->filtered);
    Bug *b = NULL;
    gtk_tree_model_get(base, &child_iter, COL_BUG_PTR, &b, -1);
    if (b) bug_detail_window_show(ui->ctx, b);
}

static gboolean
filter_visible_func(GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
    MainUi *ui = data;
    if (ui->filter_mode == 0) return TRUE;

    gchar *status = NULL;
    gtk_tree_model_get(model, iter, COL_STATUS, &status, -1);
    gboolean visible = TRUE;
    if (status) {
        if (ui->filter_mode == 1) visible = (g_strcmp0(status, "open") == 0);
        else if (ui->filter_mode == 2) visible = (g_strcmp0(status, "done") == 0);
        g_free(status);
    }
    return visible;
}

static void
on_sidebar_row_selected(GtkListBox *box, GtkListBoxRow *row, gpointer user_data)
{
    (void) box;
    MainUi *ui = user_data;
    if (!row || !ui->filtered) return;
    ui->filter_mode = gtk_list_box_row_get_index(row);
    gtk_tree_model_filter_refilter(ui->filtered);
}

static GtkWidget *
build_sidebar(MainUi *ui)
{
    GtkWidget *panel = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_style_context_add_class(gtk_widget_get_style_context(panel), "debts-panel");
    gtk_widget_set_size_request(panel, 160, -1);

    GtkWidget *title = gtk_label_new("folders");
    gtk_label_set_xalign(GTK_LABEL(title), 0.0);
    gtk_style_context_add_class(gtk_widget_get_style_context(title), "debts-panel-title");
    gtk_box_pack_start(GTK_BOX(panel), title, FALSE, FALSE, 0);

    GtkWidget *listbox = gtk_list_box_new();
    gtk_style_context_add_class(gtk_widget_get_style_context(listbox), "bts-sidebar");
    const gchar *labels[] = { "All Bugs", "Open", "Done" };
    for (int i = 0; i < 3; i++) {
        GtkWidget *label = gtk_label_new(labels[i]);
        gtk_label_set_xalign(GTK_LABEL(label), 0.0);
        gtk_list_box_insert(GTK_LIST_BOX(listbox), label, -1);
    }
    g_signal_connect(listbox, "row-selected", G_CALLBACK(on_sidebar_row_selected), ui);
    gtk_list_box_select_row(GTK_LIST_BOX(listbox),
        gtk_list_box_get_row_at_index(GTK_LIST_BOX(listbox), 0));

    GtkWidget *scroller = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_vexpand(scroller, TRUE);
    gtk_container_add(GTK_CONTAINER(scroller), listbox);
    gtk_box_pack_start(GTK_BOX(panel), scroller, TRUE, TRUE, 0);
    return panel;
}

static GtkWidget *
build_treeview(MainUi *ui)
{
    GtkListStore *store = gtk_list_store_new(N_COLS,
        G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER);

    GtkTreeModel *filtered = gtk_tree_model_filter_new(GTK_TREE_MODEL(store), NULL);
    gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(filtered),
        filter_visible_func, ui, NULL);
    ui->filtered = GTK_TREE_MODEL_FILTER(filtered);
    g_object_unref(store);

    GtkWidget *tv = gtk_tree_view_new_with_model(filtered);
    g_object_unref(filtered);
    ui->treeview = tv;

    struct { const gchar *title; gint col; gint width; } cols[] = {
        { "Bug#",     COL_NUMBER,   90  },
        { "Package",  COL_PACKAGE,  160 },
        { "Severity", COL_SEVERITY, 90  },
        { "Status",   COL_STATUS,   80  },
        { "Title",    COL_TITLE,    400 },
    };
    for (guint i = 0; i < G_N_ELEMENTS(cols); i++) {
        GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
        if (cols[i].col == COL_TITLE) g_object_set(renderer, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
        GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes(
            cols[i].title, renderer, "text", cols[i].col, NULL);
        gtk_tree_view_column_set_resizable(column, TRUE);
        gtk_tree_view_column_set_fixed_width(column, cols[i].width);
        gtk_tree_view_append_column(GTK_TREE_VIEW(tv), column);
    }
    gtk_tree_view_set_search_column(GTK_TREE_VIEW(tv), COL_TITLE);
    g_signal_connect(tv, "row-activated", G_CALLBACK(on_row_activated), ui);

    GtkWidget *panel = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_style_context_add_class(gtk_widget_get_style_context(panel), "debts-panel");
    gtk_widget_set_hexpand(panel, TRUE);

    GtkWidget *title = gtk_label_new("bugs");
    gtk_label_set_xalign(GTK_LABEL(title), 0.0);
    gtk_style_context_add_class(gtk_widget_get_style_context(title), "debts-panel-title");
    gtk_box_pack_start(GTK_BOX(panel), title, FALSE, FALSE, 0);

    GtkWidget *scroller = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_vexpand(scroller, TRUE);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroller),
        GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scroller), tv);
    gtk_box_pack_start(GTK_BOX(panel), scroller, TRUE, TRUE, 0);
    return panel;
}

void
main_window_show(AppContext *ctx)
{
    MainUi *ui = g_new0(MainUi, 1);
    ui->ctx = ctx;

    GtkWidget *window = gtk_application_window_new(ctx->app);
    ctx->main_window = window;
    gtk_window_set_title(GTK_WINDOW(window), "Debian BTS Client");
    gtk_window_set_default_size(GTK_WINDOW(window), 980, 620);
    g_object_set_data_full(G_OBJECT(window), "main-ui", ui, g_free);

    GtkWidget *header = gtk_header_bar_new();
    gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(header), TRUE);
    gtk_header_bar_set_title(GTK_HEADER_BAR(header), "Debian BTS Client");
    gtk_window_set_titlebar(GTK_WINDOW(window), header);

    ui->refresh_btn = gtk_button_new_from_icon_name("view-refresh-symbolic", GTK_ICON_SIZE_BUTTON);
    gtk_widget_set_tooltip_text(ui->refresh_btn, "Fetch new mail from your BTS folder");
    g_signal_connect(ui->refresh_btn, "clicked", G_CALLBACK(on_refresh_clicked), ui);
    gtk_header_bar_pack_start(GTK_HEADER_BAR(header), ui->refresh_btn);

    GtkWidget *new_bug_btn = gtk_button_new_with_label("New Bug\xE2\x80\xA6");
    g_signal_connect(new_bug_btn, "clicked", G_CALLBACK(on_new_bug_clicked), ui);
    gtk_header_bar_pack_start(GTK_HEADER_BAR(header), new_bug_btn);

    GtkWidget *control_btn = gtk_button_new_with_label("Control Commands\xE2\x80\xA6");
    g_signal_connect(control_btn, "clicked", G_CALLBACK(on_control_clicked), ui);
    gtk_header_bar_pack_start(GTK_HEADER_BAR(header), control_btn);

    GtkWidget *settings_btn = gtk_button_new_from_icon_name("preferences-system-symbolic", GTK_ICON_SIZE_BUTTON);
    gtk_widget_set_tooltip_text(settings_btn, "Account settings");
    g_signal_connect(settings_btn, "clicked", G_CALLBACK(on_settings_clicked), ui);
    gtk_header_bar_pack_end(GTK_HEADER_BAR(header), settings_btn);

    GtkWidget *about_btn = gtk_button_new_from_icon_name("help-about-symbolic", GTK_ICON_SIZE_BUTTON);
    gtk_widget_set_tooltip_text(about_btn, "About " DEBTS_APP_NAME);
    g_signal_connect(about_btn, "clicked", G_CALLBACK(on_about_clicked), ui);
    gtk_header_bar_pack_end(GTK_HEADER_BAR(header), about_btn);

    GtkWidget *jump_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_style_context_add_class(gtk_widget_get_style_context(jump_box), "debts-jumpbox");
    ui->jump_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(ui->jump_entry), "Jump to bug #\xE2\x80\xA6");
    gtk_entry_set_width_chars(GTK_ENTRY(ui->jump_entry), 16);
    g_signal_connect(ui->jump_entry, "activate", G_CALLBACK(on_jump_go_clicked), ui);
    GtkWidget *jump_go = gtk_button_new_with_label("Go");
    g_signal_connect(jump_go, "clicked", G_CALLBACK(on_jump_go_clicked), ui);
    gtk_box_pack_start(GTK_BOX(jump_box), ui->jump_entry, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(jump_box), jump_go, FALSE, FALSE, 0);
    gtk_header_bar_pack_end(GTK_HEADER_BAR(header), jump_box);

    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_style_context_add_class(gtk_widget_get_style_context(root), "debts-page");
    gtk_container_add(GTK_CONTAINER(window), root);

    GtkWidget *paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_widget_set_margin_start(paned, 10);
    gtk_widget_set_margin_end(paned, 10);
    gtk_widget_set_margin_top(paned, 10);
    gtk_paned_set_wide_handle(GTK_PANED(paned), TRUE);
    gtk_box_pack_start(GTK_BOX(root), paned, TRUE, TRUE, 0);
    GtkWidget *treeview_widget = build_treeview(ui);
    gtk_paned_pack1(GTK_PANED(paned), build_sidebar(ui), FALSE, FALSE);
    gtk_paned_pack2(GTK_PANED(paned), treeview_widget, TRUE, FALSE);

    ui->status_bar = gtk_label_new(
        "Set up your account in Settings, then hit Refresh to load bugs from your mail folder.");
    gtk_label_set_xalign(GTK_LABEL(ui->status_bar), 0.0);
    gtk_widget_set_margin_start(ui->status_bar, 8);
    gtk_widget_set_margin_end(ui->status_bar, 8);
    gtk_widget_set_margin_top(ui->status_bar, 4);
    gtk_widget_set_margin_bottom(ui->status_bar, 4);
    gtk_box_pack_start(GTK_BOX(root), ui->status_bar, FALSE, FALSE, 0);

    gtk_widget_show_all(window);

    if (ctx->cfg->imap_host && *ctx->cfg->imap_host) {
        on_refresh_clicked(NULL, ui);
    }
}
