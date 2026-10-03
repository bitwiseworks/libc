/** @file
 *
 * _realrealpath() command line interface.
 *
 * Copyright (c) 2026 Dmitrii Kuminov <coding@dmik.org>
 *
 *
 * This file is part of LIBC Next.
 *
 * LIBC Next is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * LIBC Next is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with LIBC Next; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */


#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <getopt.h>
#include <sys/syslimits.h>
#include <InnoTekLIBC/backend.h>


static int require_existing = 0;
static int forward_slashes = 0;

static void help()
{
    printf(
"realrealpath " VERSION VERSION_DETAILS "\n" VERSION_COPYRIGHT "\n"
"\n"
"Print the resolved absolute native file name;\n"
"all but the last component must exist\n"
"\n"
"Usage:\n"
"  %s [-eU] FILE...\n"
"\n"
"Options:\n"
"  -e  all components of the path must exist\n"
"  -U  convert backslashes to forward slashes on output\n",
getprogname()
);
    exit(0);
}

static void usage()
{
    fprintf(stderr, "Usage: %s --help | [-eU] FILE...\n", getprogname());
    exit(1);
}

static void error(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    fprintf(stderr, "%s: ", getprogname());
    vfprintf(stderr, format, args);
    va_end(args);
    exit(1);
}

static void unslashify(char *path)
{
    char *p = path;
    while ((p = strchr(p, '/')) != NULL)
        *p++ = '\\';
}

static void realrealpath(const char *path)
{
    /*
     * NOTE: use __libc_Back_fsPathResolve directly instead of _realrealpath to
     * account for require_existing.
     */

    char resolved[PATH_MAX];
    int f = __LIBC_BACKFS_FLAGS_RESOLVE_NATIVE;
    if (require_existing)
        f |= __LIBC_BACKFS_FLAGS_RESOLVE_FULL;
    else
        f |= __LIBC_BACKFS_FLAGS_RESOLVE_FULL_MAYBE;
    int rc = __libc_Back_fsPathResolve(path, resolved, PATH_MAX, f);
    if (rc < 0)
        error("%s: %s\n", path, strerror(-rc));

    /* We're built with Unix mode on and resolved has forward slashes */
    if (!forward_slashes)
        unslashify(resolved);

    printf("%s\n", resolved);
}

int main(int argc, char **argv)
{
    int c;
    static const struct option long_options[] =
    {
        { "help", no_argument, NULL, 'h' },
        { NULL, 0, NULL, 0 }
    };

    while ((c = getopt_long(argc, argv, "eU", long_options, NULL)) != -1)
    {
        switch (c)
        {
            case 'e':
                require_existing = 1;
                break;
            case 'U':
                forward_slashes = 1;
                break;
            case 'h':
                help();
                break;
            default:
                usage();
        }
    }

    if (optind == argc)
        usage();

    while (optind < argc)
        realrealpath(argv[optind++]);

    return 0;
}
