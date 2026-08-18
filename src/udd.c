#include "udd.h"
#include <libpq-fe.h>
#include <gio/gio.h>
#include <stdlib.h>

#define UDD_CONNINFO "postgresql://udd-mirror:udd-mirror@udd-mirror.debian.net/udd?connect_timeout=15"

GList *
udd_search(const BtsSearchQuery *q, GError **error)
{
    GPtrArray *conditions = g_ptr_array_new_with_free_func(g_free);
    GPtrArray *values = g_ptr_array_new();

    #define ADD_COND(col, val) \
        if (val && *val) { \
            gchar *cond = g_strdup_printf("%s = $%d", col, values->len + 1); \
            g_ptr_array_add(conditions, cond); \
            g_ptr_array_add(values, (gpointer) val); \
        }

    ADD_COND("package", q->package);
    ADD_COND("source", q->src);
    ADD_COND("owner_email", q->owner);
    ADD_COND("submitter_email", q->submitter);
    ADD_COND("severity", q->severity);
    #undef ADD_COND

    if (conditions->len == 0) {
        g_ptr_array_free(conditions, TRUE);
        g_ptr_array_free(values, TRUE);
        return NULL; /* nothing to filter on */
    }

    PGconn *conn = PQconnectdb(UDD_CONNINFO);
    if (PQstatus(conn) != CONNECTION_OK) {
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_FAILED,
                    "Could not reach the UDD mirror: %s", PQerrorMessage(conn));
        PQfinish(conn);
        g_ptr_array_free(conditions, TRUE);
        g_ptr_array_free(values, TRUE);
        return NULL;
    }

    GString *sql = g_string_new(
        "SELECT id, package, severity, status, title FROM bugs WHERE ");
    for (guint i = 0; i < conditions->len; i++) {
        if (i > 0) g_string_append(sql, " AND ");
        g_string_append(sql, (gchar *) conditions->pdata[i]);
    }
    g_string_append(sql, " ORDER BY id DESC LIMIT 200");

    PGresult *res = PQexecParams(conn, sql->str, values->len, NULL,
        (const char * const *) values->pdata, NULL, NULL, 0);

    GList *results = NULL;
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_FAILED,
                    "UDD query failed: %s", PQresultErrorMessage(res));
    } else {
        int n = PQntuples(res);
        for (int i = 0; i < n; i++) {
            Bug *b = g_new0(Bug, 1);
            b->number = atoi(PQgetvalue(res, i, 0));
            b->package = PQgetisnull(res, i, 1) ? NULL : g_strdup(PQgetvalue(res, i, 1));
            b->severity = PQgetisnull(res, i, 2) ? NULL : g_strdup(PQgetvalue(res, i, 2));
            const gchar *status_raw = PQgetisnull(res, i, 3) ? NULL : PQgetvalue(res, i, 3);
            b->status = g_strdup(g_strcmp0(status_raw, "done") == 0 ? "done" : "open");
            b->title = PQgetisnull(res, i, 4) ? NULL : g_strdup(PQgetvalue(res, i, 4));
            results = g_list_prepend(results, b);
        }
        results = g_list_reverse(results);
    }

    PQclear(res);
    PQfinish(conn);
    g_string_free(sql, TRUE);
    g_ptr_array_free(conditions, TRUE);
    g_ptr_array_free(values, TRUE);

    return results;
}
