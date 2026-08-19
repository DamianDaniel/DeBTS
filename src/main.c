#include <gtk/gtk.h>
#include <curl/curl.h>
#include "app_context.h"
#include "config.h"
#include "main_window.h"
#include "login_window.h"
#include "bug.h"
#include "mailer.h"

static void
load_css(void)
{
    GtkCssProvider *provider = gtk_css_provider_new();
    GError *error = NULL;

    /* checks the installed path, then the build tree */
    const gchar *candidates[] = {
        "/usr/share/debts/style.css",
        "data/style.css",
        "../data/style.css",
        NULL
    };
    for (int i = 0; candidates[i]; i++) {
        if (g_file_test(candidates[i], G_FILE_TEST_EXISTS)) {
            gtk_css_provider_load_from_path(provider, candidates[i], &error);
            break;
        }
    }
    if (error) {
        g_warning("Could not load stylesheet: %s", error->message);
        g_error_free(error);
    } else {
        gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
            GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    }
    g_object_unref(provider);
}

static void
show_main_window(AppContext *ctx)
{
    main_window_show(ctx);
}

static void
on_activate(GtkApplication *app, gpointer user_data)
{
    AppContext *ctx = user_data;
    ctx->app = app;

    if (!ctx->cfg->setup_done) {
        login_window_show(ctx, show_main_window);
    } else {
        show_main_window(ctx);
    }
}

int
main(int argc, char **argv)
{
    /* GPU compositing crashes on some systems, keep it off */
    g_setenv("WEBKIT_DISABLE_COMPOSITING_MODE", "1", FALSE);
    g_setenv("WEBKIT_DISABLE_DMABUF_RENDERER", "1", FALSE);
    g_setenv("LIBGL_ALWAYS_SOFTWARE", "1", FALSE);

    curl_global_init(CURL_GLOBAL_DEFAULT);

    AppContext ctx = {0};
    ctx.cfg = config_load();

#if GLIB_CHECK_VERSION(2, 74, 0)
    GtkApplication *app = gtk_application_new("org.debian.debts", G_APPLICATION_DEFAULT_FLAGS);
#else
    GtkApplication *app = gtk_application_new("org.debian.debts", G_APPLICATION_FLAGS_NONE);
#endif
    g_signal_connect(app, "activate", G_CALLBACK(on_activate), &ctx);
    g_signal_connect(app, "startup", G_CALLBACK(load_css), NULL);

    int status = g_application_run(G_APPLICATION(app), argc, argv);

    g_object_unref(app);
    g_list_free_full(ctx.bugs, (GDestroyNotify) bug_free);
    g_list_free_full(ctx.headers, (GDestroyNotify) mail_header_free);
    config_free(ctx.cfg);
    curl_global_cleanup();

    return status;
}
