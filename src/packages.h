#ifndef BTS_PACKAGES_H
#define BTS_PACKAGES_H

#include <gtk/gtk.h>

/* URL of Debian's official explanation of pseudo-packages, opened by the
 * little (i) info button next to the package chooser. */
#define BTS_PSEUDO_PACKAGES_URL "https://www.debian.org/Bugs/pseudo-packages"

/* Builds an editable GtkComboBoxText pre-populated with the common Debian
 * pseudo-packages (wnpp, general, ftp.debian.org, ...). The user may type
 * any real package name too, since the widget has an entry. */
GtkWidget *packages_create_combo(void);

/* Opens BTS_PSEUDO_PACKAGES_URL in the user's default browser. */
void packages_open_info_page(GtkWindow *parent);

#endif /* BTS_PACKAGES_H */
