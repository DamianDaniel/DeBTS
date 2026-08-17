#ifndef BTS_APP_CONTEXT_H
#define BTS_APP_CONTEXT_H

#include <gtk/gtk.h>
#include "config.h"
#include "mailer.h"
#include "bug.h"

typedef struct {
    GtkApplication *app;
    AppConfig      *cfg;
    GtkWidget      *main_window;
    GtkListStore   *bug_store;   /* columns: see mainwindow.c BugCol enum */
    GList          *headers;     /* GList<MailHeader*>, last fetch, owns memory */
    GList          *bugs;        /* GList<Bug*>, derived from `headers`, owns memory */
} AppContext;

#endif /* BTS_APP_CONTEXT_H */
