#include "config.h"
#include <stdio.h>
#include <sys/stat.h>

static gchar *
config_path(void)
{
    return g_build_filename(g_get_user_config_dir(), "debts", "config.ini", NULL);
}

static gchar *
dup_or_default(GKeyFile *kf, const gchar *group, const gchar *key, const gchar *def)
{
    GError *err = NULL;
    gchar *val = g_key_file_get_string(kf, group, key, &err);
    if (err) {
        g_clear_error(&err);
        return g_strdup(def ? def : "");
    }
    return val;
}

AppConfig *
config_load(void)
{
    AppConfig *cfg = g_new0(AppConfig, 1);
    gchar *path = config_path();
    GKeyFile *kf = g_key_file_new();
    GError *err = NULL;

    gboolean have_file = g_key_file_load_from_file(kf, path, G_KEY_FILE_NONE, &err);
    if (!have_file) {
        g_clear_error(&err);
    }

    cfg->imap_host   = dup_or_default(kf, "IMAP", "host", "");
    cfg->imap_port   = have_file ? g_key_file_get_integer(kf, "IMAP", "port", NULL) : 993;
    if (cfg->imap_port == 0) cfg->imap_port = 993;
    cfg->imap_ssl    = have_file ? g_key_file_get_boolean(kf, "IMAP", "ssl", NULL) : TRUE;
    cfg->imap_user   = dup_or_default(kf, "IMAP", "user", "");
    cfg->imap_pass   = dup_or_default(kf, "IMAP", "pass", "");
    cfg->imap_folder = dup_or_default(kf, "IMAP", "folder", "INBOX");

    cfg->smtp_host   = dup_or_default(kf, "SMTP", "host", "");
    cfg->smtp_port   = have_file ? g_key_file_get_integer(kf, "SMTP", "port", NULL) : 587;
    if (cfg->smtp_port == 0) cfg->smtp_port = 587;
    cfg->smtp_ssl    = have_file ? g_key_file_get_boolean(kf, "SMTP", "ssl", NULL) : TRUE;
    cfg->smtp_user   = dup_or_default(kf, "SMTP", "user", "");
    cfg->smtp_pass   = dup_or_default(kf, "SMTP", "pass", "");

    cfg->from_name   = dup_or_default(kf, "Identity", "name", "");
    cfg->from_email  = dup_or_default(kf, "Identity", "email", "");

    g_key_file_free(kf);
    g_free(path);
    return cfg;
}

gboolean
config_save(AppConfig *cfg, GError **error)
{
    gchar *dir = g_build_filename(g_get_user_config_dir(), "debts", NULL);
    if (g_mkdir_with_parents(dir, 0700) != 0) {
        g_set_error(error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                    "Could not create config directory %s", dir);
        g_free(dir);
        return FALSE;
    }
    g_free(dir);

    GKeyFile *kf = g_key_file_new();

    g_key_file_set_string(kf, "IMAP", "host", cfg->imap_host);
    g_key_file_set_integer(kf, "IMAP", "port", cfg->imap_port);
    g_key_file_set_boolean(kf, "IMAP", "ssl", cfg->imap_ssl);
    g_key_file_set_string(kf, "IMAP", "user", cfg->imap_user);
    g_key_file_set_string(kf, "IMAP", "pass", cfg->imap_pass);
    g_key_file_set_string(kf, "IMAP", "folder", cfg->imap_folder);

    g_key_file_set_string(kf, "SMTP", "host", cfg->smtp_host);
    g_key_file_set_integer(kf, "SMTP", "port", cfg->smtp_port);
    g_key_file_set_boolean(kf, "SMTP", "ssl", cfg->smtp_ssl);
    g_key_file_set_string(kf, "SMTP", "user", cfg->smtp_user);
    g_key_file_set_string(kf, "SMTP", "pass", cfg->smtp_pass);

    g_key_file_set_string(kf, "Identity", "name", cfg->from_name);
    g_key_file_set_string(kf, "Identity", "email", cfg->from_email);

    gchar *path = config_path();
    gboolean ok = g_key_file_save_to_file(kf, path, error);
    if (ok) {
        /* config contains credentials in plaintext - restrict permissions */
        chmod(path, S_IRUSR | S_IWUSR);
    }
    g_free(path);
    g_key_file_free(kf);
    return ok;
}

void
config_free(AppConfig *cfg)
{
    if (!cfg) return;
    g_free(cfg->imap_host);
    g_free(cfg->imap_user);
    g_free(cfg->imap_pass);
    g_free(cfg->imap_folder);
    g_free(cfg->smtp_host);
    g_free(cfg->smtp_user);
    g_free(cfg->smtp_pass);
    g_free(cfg->from_name);
    g_free(cfg->from_email);
    g_free(cfg);
}
