#include "templates.h"

const BugTemplate bts_templates[] = {
    { "Blank", NULL, NULL, "" },

    { "Bug report", NULL, "normal",
      "* What led up to the situation?\n"
      "\n"
      "* What exactly did you do (or not do) that was effective\n"
      "  (or ineffective)?\n"
      "\n"
      "* What was the outcome of this action?\n"
      "\n"
      "* What outcome did you expect instead?\n"
      "\n"
      "*** End of the template - remove these template lines ***\n" },

    { "Feature request", NULL, "wishlist",
      "I would like to request the following feature:\n"
      "\n"
      "\n"
      "This would help because:\n"
      "\n" },

    { "Package won't build (FTBFS)", NULL, "serious",
      "The package fails to build from source.\n"
      "\n"
      "Build log (or the relevant part of it):\n"
      "\n" },

    { "ITP - Intent to package", "wnpp", "wishlist",
      "* Package name    : \n"
      "  Version         : \n"
      "  Upstream Author : \n"
      "* URL             : \n"
      "* License         : \n"
      "  Programming Lang: \n"
      "  Description     : \n" },

    { "RFP - Request for packaging", "wnpp", "wishlist",
      "* Package name    : \n"
      "  Version         : \n"
      "  Upstream Author : \n"
      "* URL             : \n"
      "* License         : \n"
      "  Description     : \n" },

    { "O - Orphaning a package", "wnpp", "normal",
      "I am orphaning this package, since I no longer have the\n"
      "time to maintain it.\n" },
};

const gint bts_templates_count =
    (gint)(sizeof(bts_templates) / sizeof(bts_templates[0]));
