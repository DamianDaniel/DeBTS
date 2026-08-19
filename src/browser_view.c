#include "browser_view.h"
#include "compose_window.h"
#include "control_window.h"
#include <webkit2/webkit2.h>
#include <stdlib.h>

#define BTS_HOME "https://bugs.debian.org/"

typedef struct {
    AppContext *ctx;
    WebKitWebView *webview;
    GtkWidget *bug_toolbar;
    GtkWidget *bug_label;
    GtkWidget *url_label;
    gint current_bug; /* 0 if none detected */
} BrowserCtx;

/* pulls a bug number out of a bugs.debian.org URL, 0 if none */
static gint
bug_number_from_uri(const gchar *uri)
{
    if (!uri) return 0;
    GRegex *re1 = g_regex_new("[?&;]bug=(\\d+)", 0, 0, NULL);
    GRegex *re2 = g_regex_new("bugs\\.debian\\.org/(\\d{3,7})(?:[/?#]|$)", 0, 0, NULL);
    GMatchInfo *mi = NULL;
    gint number = 0;

    if (g_regex_match(re1, uri, 0, &mi)) {
        gchar *s = g_match_info_fetch(mi, 1);
        number = atoi(s);
        g_free(s);
    }
    if (mi) g_match_info_free(mi);

    if (number == 0 && g_regex_match(re2, uri, 0, &mi)) {
        gchar *s = g_match_info_fetch(mi, 1);
        number = atoi(s);
        g_free(s);
    }
    if (mi) g_match_info_free(mi);

    g_regex_unref(re1);
    g_regex_unref(re2);
    return number;
}

/* grabs the bug title out of the page's <title>, if there is one */
static gchar *
title_hint_from_page(WebKitWebView *webview, gint bug_number)
{
    const gchar *title = webkit_web_view_get_title(webview);
    if (!title) return g_strdup_printf("Bug#%d:", bug_number);

    GRegex *re = g_regex_new("^#\\d+\\s*-\\s*(.+?)\\s*-\\s*Debian Bug", G_REGEX_CASELESS, 0, NULL);
    GMatchInfo *mi = NULL;
    gchar *result = NULL;
    if (re && g_regex_match(re, title, 0, &mi)) {
        gchar *t = g_match_info_fetch(mi, 1);
        result = g_strdup_printf("Bug#%d: %s", bug_number, t);
        g_free(t);
    }
    if (mi) g_match_info_free(mi);
    if (re) g_regex_unref(re);

    return result ? result : g_strdup_printf("Bug#%d:", bug_number);
}

static void
on_reply_clicked(GtkButton *btn, gpointer user_data)
{
    (void) btn;
    BrowserCtx *bc = user_data;
    if (bc->current_bug <= 0) return;
    gchar *hint = title_hint_from_page(bc->webview, bc->current_bug);
    compose_window_reply(bc->ctx, bc->current_bug, hint);
    g_free(hint);
}

static void
on_retitle_clicked(GtkButton *btn, gpointer user_data)
{
    (void) btn;
    BrowserCtx *bc = user_data;
    if (bc->current_bug <= 0) return;
    control_window_show_for_bug(bc->ctx, bc->current_bug, "retitle");
}

static void
on_severity_clicked(GtkButton *btn, gpointer user_data)
{
    (void) btn;
    BrowserCtx *bc = user_data;
    if (bc->current_bug <= 0) return;
    control_window_show_for_bug(bc->ctx, bc->current_bug, "severity");
}

static void
on_more_commands_clicked(GtkButton *btn, gpointer user_data)
{
    (void) btn;
    BrowserCtx *bc = user_data;
    if (bc->current_bug <= 0) return;
    control_window_show_for_bug(bc->ctx, bc->current_bug, NULL);
}

static void
on_uri_changed(WebKitWebView *webview, GParamSpec *pspec, gpointer user_data)
{
    (void) pspec;
    BrowserCtx *bc = user_data;
    const gchar *uri = webkit_web_view_get_uri(webview);
    gtk_label_set_text(GTK_LABEL(bc->url_label), uri ? uri : "");

    bc->current_bug = bug_number_from_uri(uri);
    if (bc->current_bug > 0) {
        gchar *label = g_strdup_printf("Bug #%d", bc->current_bug);
        gtk_label_set_text(GTK_LABEL(bc->bug_label), label);
        g_free(label);
        gtk_widget_show_all(bc->bug_toolbar);
    } else {
        gtk_widget_hide(bc->bug_toolbar);
    }
}

static void
on_back_clicked(GtkButton *btn, gpointer user_data)
{
    (void) btn;
    BrowserCtx *bc = user_data;
    webkit_web_view_go_back(bc->webview);
}

static void
on_forward_clicked(GtkButton *btn, gpointer user_data)
{
    (void) btn;
    BrowserCtx *bc = user_data;
    webkit_web_view_go_forward(bc->webview);
}

static void
on_reload_clicked(GtkButton *btn, gpointer user_data)
{
    (void) btn;
    BrowserCtx *bc = user_data;
    webkit_web_view_reload(bc->webview);
}

static void
on_home_clicked(GtkButton *btn, gpointer user_data)
{
    (void) btn;
    BrowserCtx *bc = user_data;
    webkit_web_view_load_uri(bc->webview, BTS_HOME);
}

static void
on_open_external_clicked(GtkButton *btn, gpointer user_data)
{
    (void) btn;
    BrowserCtx *bc = user_data;
    const gchar *uri = webkit_web_view_get_uri(bc->webview);
    if (!uri) return;
    GtkWidget *toplevel = gtk_widget_get_toplevel(GTK_WIDGET(bc->webview));
    GError *error = NULL;
    gtk_show_uri_on_window(GTK_IS_WINDOW(toplevel) ? GTK_WINDOW(toplevel) : NULL,
        uri, GDK_CURRENT_TIME, &error);
    if (error) g_error_free(error); /* not fatal, just couldn't open */
}

void
browser_view_load(GtkWidget *view, const gchar *url)
{
    BrowserCtx *bc = g_object_get_data(G_OBJECT(view), "browser-ctx");
    if (bc) webkit_web_view_load_uri(bc->webview, url);
}

GtkWidget *
browser_view_new(AppContext *ctx)
{
    BrowserCtx *bc = g_new0(BrowserCtx, 1);
    bc->ctx = ctx;

    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    /* plain nav bar - this is a real browser tab, not a fetch */
    GtkWidget *navbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_style_context_add_class(gtk_widget_get_style_context(navbar), "debts-navbar");
    gtk_container_set_border_width(GTK_CONTAINER(navbar), 4);

    GtkWidget *back_btn = gtk_button_new_from_icon_name("go-previous-symbolic", GTK_ICON_SIZE_BUTTON);
    g_signal_connect(back_btn, "clicked", G_CALLBACK(on_back_clicked), bc);
    GtkWidget *fwd_btn = gtk_button_new_from_icon_name("go-next-symbolic", GTK_ICON_SIZE_BUTTON);
    g_signal_connect(fwd_btn, "clicked", G_CALLBACK(on_forward_clicked), bc);
    GtkWidget *reload_btn = gtk_button_new_from_icon_name("view-refresh-symbolic", GTK_ICON_SIZE_BUTTON);
    g_signal_connect(reload_btn, "clicked", G_CALLBACK(on_reload_clicked), bc);
    GtkWidget *home_btn = gtk_button_new_from_icon_name("go-home-symbolic", GTK_ICON_SIZE_BUTTON);
    g_signal_connect(home_btn, "clicked", G_CALLBACK(on_home_clicked), bc);

    GtkWidget *external_btn = gtk_button_new_from_icon_name("web-browser-symbolic", GTK_ICON_SIZE_BUTTON);
    gtk_widget_set_tooltip_text(external_btn, "Open this page in your system browser");
    g_signal_connect(external_btn, "clicked", G_CALLBACK(on_open_external_clicked), bc);

    bc->url_label = gtk_label_new(BTS_HOME);
    gtk_label_set_xalign(GTK_LABEL(bc->url_label), 0.0);
    gtk_label_set_ellipsize(GTK_LABEL(bc->url_label), PANGO_ELLIPSIZE_MIDDLE);
    gtk_widget_set_hexpand(bc->url_label, TRUE);

    gtk_box_pack_start(GTK_BOX(navbar), back_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(navbar), fwd_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(navbar), reload_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(navbar), home_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(navbar), bc->url_label, TRUE, TRUE, 6);
    gtk_box_pack_start(GTK_BOX(navbar), external_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(root), navbar, FALSE, FALSE, 0);

    /* a plain bar above the page for bug pages - NOT a GtkOverlay.
     * compositing a floating child over a WebKitWebView's own draw
     * surface is what was corrupting the heap; a sibling bar just
     * pushes the page down instead, and never touches its draw path. */
    bc->bug_toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_style_context_add_class(gtk_widget_get_style_context(bc->bug_toolbar), "debts-bug-toolbar");
    gtk_widget_set_no_show_all(bc->bug_toolbar, TRUE); /* hidden until a bug shows up */

    bc->bug_label = gtk_label_new("");
    gtk_style_context_add_class(gtk_widget_get_style_context(bc->bug_label), "debts-bug-toolbar-label");
    gtk_box_pack_start(GTK_BOX(bc->bug_toolbar), bc->bug_label, FALSE, FALSE, 4);

    GtkWidget *reply_btn = gtk_button_new_with_label("Reply\xE2\x80\xA6");
    g_signal_connect(reply_btn, "clicked", G_CALLBACK(on_reply_clicked), bc);
    gtk_box_pack_start(GTK_BOX(bc->bug_toolbar), reply_btn, FALSE, FALSE, 0);

    GtkWidget *retitle_btn = gtk_button_new_with_label("Retitle\xE2\x80\xA6");
    g_signal_connect(retitle_btn, "clicked", G_CALLBACK(on_retitle_clicked), bc);
    gtk_box_pack_start(GTK_BOX(bc->bug_toolbar), retitle_btn, FALSE, FALSE, 0);

    GtkWidget *severity_btn = gtk_button_new_with_label("Severity\xE2\x80\xA6");
    g_signal_connect(severity_btn, "clicked", G_CALLBACK(on_severity_clicked), bc);
    gtk_box_pack_start(GTK_BOX(bc->bug_toolbar), severity_btn, FALSE, FALSE, 0);

    GtkWidget *more_btn = gtk_button_new_with_label("More Commands\xE2\x80\xA6");
    g_signal_connect(more_btn, "clicked", G_CALLBACK(on_more_commands_clicked), bc);
    gtk_box_pack_start(GTK_BOX(bc->bug_toolbar), more_btn, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(root), bc->bug_toolbar, FALSE, TRUE, 0);

    /* the real page - a plain sibling now, no overlay compositing over it */
    bc->webview = WEBKIT_WEB_VIEW(webkit_web_view_new());
    g_signal_connect(bc->webview, "notify::uri", G_CALLBACK(on_uri_changed), bc);

    /* GPU compositing crashes on some systems, software render is safer */
    WebKitSettings *settings = webkit_web_view_get_settings(bc->webview);
    webkit_settings_set_hardware_acceleration_policy(settings,
        WEBKIT_HARDWARE_ACCELERATION_POLICY_NEVER);

    gtk_box_pack_start(GTK_BOX(root), GTK_WIDGET(bc->webview), TRUE, TRUE, 0);

    g_object_set_data_full(G_OBJECT(root), "browser-ctx", bc, g_free);

    webkit_web_view_load_uri(bc->webview, BTS_HOME);
    return root;
}
