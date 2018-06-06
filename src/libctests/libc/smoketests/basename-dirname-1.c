/* $Id: $ */
/** @file
 *
 * Smoke test case for dirname and basename.
 *
 * Copyright (c) 2006 knut st. osmundsen <bird@anduin.net>
 *
 *
 * This file is part of InnoTek LIBC.
 *
 * InnoTek LIBC is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * InnoTek LIBC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with InnoTek LIBC; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <libgen.h>
#include <stdio.h>
#include <limits.h>

#if defined(__OS2__) || defined(__WIN32__) || defined(__WIN64__)
# define HAVE_DRIVE_LETTERS
# define HAVE_DOS_SLASH
#endif

int main()
{
    int rcRet = 0;
    static const struct
    {
        const char *pszIn;
        const char *pszDirname;
        const char *pszBasename;
    } s_aTests[] =
    {  /* in */              /* dir */                   /* base */
        { NULL,                 ".",                        "." },
        { "",                   ".",                        "." },
        { ".",                  ".",                        "." },
        { "..",                 ".",                        ".." },
        { "/",                  "/",                        "/" },
        { "/usr",               "/",                        "usr" },
        { "/usr/",              "/",                        "usr" },
        { "/usr/bin",           "/usr",                     "bin" },
        { "/usr//lib",          "/usr",                     "lib" },
        { "/usr//lib//",        "/usr",                     "lib" },
#ifdef HAVE_DOS_SLASH
        { "/usr\\\\lib//",      "/usr",                     "lib" },
        { "/usr/lib\\\\",       "/usr",                     "lib" },
#endif
#ifdef HAVE_DRIVE_LETTERS
        { "d:/",                "d:/",                      "d:/" },
        { "d:/usr",             "d:/",                      "usr" },
        { "d:/usr/",            "d:/",                      "usr" },
        { "d:/usr/bin",         "d:/usr",                   "bin" },
        { "d:/usr//lib",        "d:/usr",                   "lib" },
        { "d:/usr//lib//",      "d:/usr",                   "lib" },
        { "d:" ,                "d:",                       "d:" },
        { "d:usr",              "d:",                       "usr" },
        { "d:usr//lib//",       "d:usr",                    "lib" },
#ifdef HAVE_DOS_SLASH
        { "d:\\",               "d:\\",                     "d:\\" },
        { "d:\\usr",            "d:\\",                     "usr" },
        { "d:\\usr\\\\lib",     "d:\\usr",                  "lib" },
#endif
#endif
    };


    int i;
    for (i = 0; i < sizeof(s_aTests) / sizeof(s_aTests[0]); i++)
    {
        char sz[PATH_MAX];
        char *psz;

        psz = basename(s_aTests[i].pszIn ? strncpy(sz, s_aTests[i].pszIn, sizeof(sz)) : NULL);
        if (!psz || strcmp(psz, s_aTests[i].pszBasename))
        {
            printf("basename-dirname-1: FAILURE - basename('%s') -> '%s' expected '%s' (#%d)\n",
                   s_aTests[i].pszIn, psz, s_aTests[i].pszBasename, i);
            rcRet++;
        }

        psz = dirname(s_aTests[i].pszIn ? strncpy(sz, s_aTests[i].pszIn, sizeof(sz)) : NULL);
        if (!psz || strcmp(psz, s_aTests[i].pszDirname))
        {
            printf("basename-dirname-1: FAILURE - dirname('%s') -> '%s' expected '%s' (#%d)\n",
                   s_aTests[i].pszIn, psz, s_aTests[i].pszDirname, i);
            rcRet++;
        }
    }

    if (!rcRet)
        printf("basename-dirname-1: SUCCESS\n");
    else
        printf("basename-dirname-1: FAILURE - %d errors\n", rcRet);
    return !!rcRet;
}

