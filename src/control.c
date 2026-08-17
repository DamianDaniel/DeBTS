#include "control.h"
#include <string.h>

const ControlCommandDef bts_control_commands[] = {
    { "retitle", "Retitle bug",
      { "Bug number", "New title", NULL, NULL }, 2,
      "Changes the bug's title." },

    { "reassign", "Reassign to package",
      { "Bug number", "Package", "Version (optional)", NULL }, 3,
      "Moves the bug to a different package, optionally recording the package version." },

    { "severity", "Change severity",
      { "Bug number", "Severity (critical/grave/serious/important/normal/minor/wishlist)", NULL, NULL }, 2,
      "Sets the bug's severity level." },

    { "tags", "Add/remove tags",
      { "Bug number", "+ / - / = (add / remove / set)", "Tag(s), space separated", NULL }, 3,
      "Adds, removes, or replaces the bug's tags, e.g. patch, moreinfo, wontfix, confirmed." },

    { "usertags", "Add/remove user tags",
      { "Bug number", "+ / - / = (add / remove / set)", "Usertag(s), space separated", NULL }, 3,
      "Like tags, but namespaced to the 'user' identity set with the user command." },

    { "user", "Set user tag namespace",
      { "Email address to use as usertag namespace", NULL, NULL, NULL }, 1,
      "Sets whose usertag namespace subsequent 'usertags' commands apply to." },

    { "found", "Mark found in version",
      { "Bug number", "Version (optional)", NULL, NULL }, 2,
      "Records that the bug is present in the given version; reopens it if it was closed." },

    { "notfound", "Remove found version",
      { "Bug number", "Version", NULL, NULL }, 2,
      "Removes a previously recorded 'found' version." },

    { "fixed", "Mark fixed in version",
      { "Bug number", "Version", NULL, NULL }, 2,
      "Records that the bug is fixed in the given version, without closing it." },

    { "notfixed", "Remove fixed version",
      { "Bug number", "Version", NULL, NULL }, 2,
      "Removes a previously recorded 'fixed' version." },

    { "close", "Close bug",
      { "Bug number", "Version (optional)", NULL, NULL }, 2,
      "Closes the bug, optionally recording the version it was fixed in." },

    { "reopen", "Reopen bug",
      { "Bug number", "Submitter override (optional)", NULL, NULL }, 2,
      "Reopens a closed bug." },

    { "merge", "Merge bugs",
      { "Bug number", "Other bug number(s), space separated", NULL, NULL }, 2,
      "Merges two or more bugs that describe the same problem (must already share title/package/severity)." },

    { "forcemerge", "Force-merge bugs",
      { "Master bug number", "Other bug number(s), space separated", NULL, NULL }, 2,
      "Merges bugs like 'merge', but copies the master bug's details onto the others first." },

    { "unmerge", "Unmerge bug",
      { "Bug number", NULL, NULL, NULL }, 1,
      "Splits a bug out of a merged group again." },

    { "clone", "Clone bug",
      { "Bug number", "New bug id(s), e.g. -1 -2", NULL, NULL }, 2,
      "Creates new bug report(s) that are clones of an existing one, for splitting up a single report." },

    { "block", "Mark blocked by",
      { "Bug number", "Blocking bug number(s), space separated", NULL, NULL }, 2,
      "Records that this bug cannot be fixed until the listed bug(s) are." },

    { "unblock", "Remove blocked-by",
      { "Bug number", "Bug number(s) to remove, space separated", NULL, NULL }, 2,
      "Removes a previously recorded blocking relationship." },

    { "affects", "Mark package as affected",
      { "Bug number", "+ / - / = (add / remove / set)", "Package(s), space separated", NULL }, 3,
      "Flags other packages as affected by this bug without reassigning it." },

    { "forwarded", "Set forwarded-to URL",
      { "Bug number", "URL or email of upstream report", NULL, NULL }, 2,
      "Records where the bug has been forwarded upstream." },

    { "notforwarded", "Clear forwarded-to",
      { "Bug number", NULL, NULL, NULL }, 1,
      "Clears a previously recorded forwarded-to address." },

    { "owner", "Set owner",
      { "Bug number", "Owner name <email>", NULL, NULL }, 2,
      "Assigns a specific person to own/handle this bug." },

    { "noowner", "Clear owner",
      { "Bug number", NULL, NULL, NULL }, 1,
      "Removes the bug's owner." },

    { "submitter", "Change submitter",
      { "Bug number", "New submitter email, or ! for yourself", NULL, NULL }, 2,
      "Corrects the recorded submitter address of the bug." },

    { "archive", "Archive bug",
      { "Bug number", NULL, NULL, NULL }, 1,
      "Moves an old, closed bug into the archive." },

    { "unarchive", "Unarchive bug",
      { "Bug number", NULL, NULL, NULL }, 1,
      "Restores a bug from the archive." },
};

const gint bts_control_commands_count =
    (gint)(sizeof(bts_control_commands) / sizeof(bts_control_commands[0]));

gchar *
control_command_format(const ControlCommandDef *def, gchar **params)
{
    GString *line = g_string_new(def->key);
    for (gint i = 0; i < def->param_count; i++) {
        const gchar *val = (params && params[i]) ? params[i] : "";
        if (val[0] == '\0') continue; /* skip empty optional params */
        g_string_append_c(line, ' ');
        g_string_append(line, val);
    }
    return g_string_free(line, FALSE);
}
