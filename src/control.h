#ifndef BTS_CONTROL_H
#define BTS_CONTROL_H

#include <glib.h>

#define BTS_CONTROL_ADDRESS "control@bugs.debian.org"
#define BTS_SUBMIT_ADDRESS  "submit@bugs.debian.org"
/* bugnumber@bugs.debian.org is used for replying/following up on a bug */
#define BTS_BUG_DOMAIN      "bugs.debian.org"

typedef struct {
    const gchar *key;              /* internal id, also the literal command word */
    const gchar *label;            /* human readable name shown in the dropdown */
    const gchar *param_labels[4];  /* labels for up to 4 free-text parameters */
    gint param_count;
    const gchar *help;             /* one-line explanation shown under the form */
} ControlCommandDef;

/* Every command below maps directly onto a line understood by
 * control@bugs.debian.org, see https://www.debian.org/Bugs/server-control */
extern const ControlCommandDef bts_control_commands[];
extern const gint bts_control_commands_count;

/* Builds the literal command line to send (WITHOUT trailing newline) from
 * user-supplied parameter strings. `params` must have at least
 * def->param_count entries. Returns a newly allocated string. */
gchar *control_command_format(const ControlCommandDef *def, gchar **params);

#endif /* BTS_CONTROL_H */
