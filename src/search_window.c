#include "search_window.h"
#include "btsweb.h"
#include "main_window.h"

typedef struct {
    AppContext *ctx;
    GtkWidget *window;
    GtkWidget *package_entry, *src_entry, *maint_entry, *submitter_entry;
    GtkWidget *severity_combo, *status_combo, *tag_entry, *owner_entry;
} SearchCtx;

static void
on_search_clicked(GtkButton *btn, gpointer user_data)
{
    (void) btn;
    SearchCtx *sc = user_data;
    BtsSearchQuery q = {0};

    q.package = (gchar *) gtk_entry_get_text(GTK_ENTRY(sc->package_entry));
    q.src = (gchar *) gtk_entry_get_text(GTK_ENTRY(sc->src_entry));
    q.maintainer = (gchar *) gtk_entry_get_text(GTK_ENTRY(sc->maint_entry));
    q.submitter = (gchar *) gtk_entry_get_text(GTK_ENTRY(sc->submitter_entry));
    q.tag = (gchar *) gtk_entry_get_text(GTK_ENTRY(sc->tag_entry));
    q.owner = (gchar *) gtk_entry_get_text(GTK_ENTRY(sc->owner_entry));
    q.severity = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(sc->severity_combo));
    q.status = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(sc->status_combo));

    if (q.severity && g_strcmp0(q.severity, "any") == 0) q.severity = NULL;
    if (q.status && g_strcmp0(q.status, "any") == 0) q.status = NULL;

    gchar *url = btsweb_build_search_url(&q);
    if (!url) {
        return; /* nothing entered, ignore */
    }

    main_window_browse_url(sc->ctx, url);
    g_free(url);
    gtk_widget_destroy(sc->window);
}

static GtkWidget *
entry_row(GtkGrid *grid, gint row, const gchar *label, GtkWidget **out_entry)
{
    GtkWidget *l = gtk_label_new(label);
    gtk_label_set_xalign(GTK_LABEL(l), 0.0);
    gtk_grid_attach(grid, l, 0, row, 1, 1);
    *out_entry = gtk_entry_new();
    gtk_widget_set_hexpand(*out_entry, TRUE);
    gtk_grid_attach(grid, *out_entry, 1, row, 1, 1);
    return *out_entry;
}

void
search_window_show(AppContext *ctx)
{
    SearchCtx *sc = g_new0(SearchCtx, 1);
    sc->ctx = ctx;

    sc->window = gtk_application_window_new(ctx->app);
    gtk_window_set_title(GTK_WINDOW(sc->window), "Search the Debian BTS");
    gtk_window_set_default_size(GTK_WINDOW(sc->window), 420, 380);
    gtk_window_set_transient_for(GTK_WINDOW(sc->window), GTK_WINDOW(ctx->main_window));
    g_object_set_data_full(G_OBJECT(sc->window), "search-ctx", sc, g_free);

    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_container_set_border_width(GTK_CONTAINER(root), 12);
    gtk_container_add(GTK_CONTAINER(sc->window), root);

    GtkWidget *intro = gtk_label_new(
        "Opens real bugs.debian.org search results in the Browse tab -\n"
        "no scraping, just the site itself.");
    gtk_label_set_xalign(GTK_LABEL(intro), 0.0);
    gtk_style_context_add_class(gtk_widget_get_style_context(intro), "bts-command-help");
    gtk_box_pack_start(GTK_BOX(root), intro, FALSE, FALSE, 0);

    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 6);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    gtk_box_pack_start(GTK_BOX(root), grid, FALSE, FALSE, 0);

    entry_row(GTK_GRID(grid), 0, "Package", &sc->package_entry);
    entry_row(GTK_GRID(grid), 1, "Source package", &sc->src_entry);
    entry_row(GTK_GRID(grid), 2, "Maintainer email", &sc->maint_entry);
    entry_row(GTK_GRID(grid), 3, "Submitter email", &sc->submitter_entry);
    if (ctx->cfg->from_email && *ctx->cfg->from_email)
        gtk_entry_set_text(GTK_ENTRY(sc->submitter_entry), ctx->cfg->from_email);
    entry_row(GTK_GRID(grid), 6, "Tag", &sc->tag_entry);
    entry_row(GTK_GRID(grid), 7, "Owner email", &sc->owner_entry);

    GtkWidget *sev_label = gtk_label_new("Severity");
    gtk_label_set_xalign(GTK_LABEL(sev_label), 0.0);
    gtk_grid_attach(GTK_GRID(grid), sev_label, 0, 4, 1, 1);
    sc->severity_combo = gtk_combo_box_text_new();
    const gchar *sevs[] = { "any", "critical", "grave", "serious", "important",
                             "normal", "minor", "wishlist", NULL };
    for (int i = 0; sevs[i]; i++) gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(sc->severity_combo), sevs[i]);
    gtk_combo_box_set_active(GTK_COMBO_BOX(sc->severity_combo), 0);
    gtk_grid_attach(GTK_GRID(grid), sc->severity_combo, 1, 4, 1, 1);

    GtkWidget *status_label_w = gtk_label_new("Status");
    gtk_label_set_xalign(GTK_LABEL(status_label_w), 0.0);
    gtk_grid_attach(GTK_GRID(grid), status_label_w, 0, 5, 1, 1);
    sc->status_combo = gtk_combo_box_text_new();
    const gchar *statuses[] = { "any", "open", "done", "forwarded", NULL };
    for (int i = 0; statuses[i]; i++) gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(sc->status_combo), statuses[i]);
    gtk_combo_box_set_active(GTK_COMBO_BOX(sc->status_combo), 0);
    gtk_grid_attach(GTK_GRID(grid), sc->status_combo, 1, 5, 1, 1);

    GtkWidget *search_btn = gtk_button_new_with_label("Search");
    gtk_style_context_add_class(gtk_widget_get_style_context(search_btn), "suggested-action");
    gtk_widget_set_halign(search_btn, GTK_ALIGN_END);
    g_signal_connect(search_btn, "clicked", G_CALLBACK(on_search_clicked), sc);
    gtk_box_pack_start(GTK_BOX(root), search_btn, FALSE, FALSE, 0);

    gtk_widget_show_all(sc->window);
}
