/** @file
 *
 * LIBC Next statistics command line interface.
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

#define INCL_DOSERRORS
#define INCL_DOSMODULEMGR
#include <os2emx.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <emx/startup.h>
#include <libcn/version.h>
#include <InnoTekLIBC/sharedpm.h>
#include <InnoTekLIBC/errno.h>
#include "defs.h"


struct __libcn_version version = { sizeof(version) };

static void help()
{
    printf(
"libc-stats " VERSION VERSION_DETAILS "\n" VERSION_COPYRIGHT "\n"
"\n"
"Print runtime LIBC version information and statistics\n"
"\n"
"Usage:\n"
"  %s [OPTIONS]\n"
"\n"
"Options:\n"
"  --version    Print LIBC version\n"
"  --signature  Print LIBC DLL BLDLEVEL signature\n"
"  --hmod       Print LIBC DLL module handle\n"
"  --path       Print LIBC DLL full path\n"
"  --spm-dump   Print LIBC SPM dump\n"
"  --spm-check  Check LIBC SPM state\n"
,
getprogname()
);
    exit(0);
}

static void usage()
{
    fprintf(stderr, "Usage: %s --help | [OPTIONS]\n", getprogname());
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

static void file_error(FILE *f, const char *opname, const char *path)
{
    error("%s(%s): %s\n", opname, path, !f || ferror(f) ? strerror(errno) : "Premature EOF");
}

static char description[256 + 1 /* \0 */];

static void get_description(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f)
        file_error(NULL, "fopen", path);

    struct exe1p2_header hdr;
    if (fread(&hdr, sizeof(hdr), 1, f) != 1)
        file_error(f, "fread", path);
    if (hdr.e_magic != EXE_MAGIC_MZ)
        hdr.e_lfanew = 0; /* LXLITE may strip MZ header, check for LX */

    struct os2_header lx;
    if (fseek(f, hdr.e_lfanew, SEEK_SET))
        file_error(NULL, "fseek", path);
    if (fread(&lx, sizeof(lx), 1, f) != 1)
        file_error(f, "fread", path);
    if (lx.magic != EXE_MAGIC_LX)
        error("%s: Invalid LX header at offset 0x%08X: 0x%02X\n", path, hdr.e_lfanew, lx.magic);

    unsigned char len = 0;
    if (lx.nonresname_size > 0)
    {
        if (fseek(f, lx.nonresname_offset, SEEK_SET))
            file_error(NULL, "fseek", path);
        if (fread(&len, sizeof(len), 1, f) != 1 || (len && fread(description, len, 1, f) != 1))
            file_error(f, "fread", path);
    }
    description[len] = '\0';

    fclose(f);
}

int main(int argc, char **argv)
{
    static const struct option long_options[] =
    {
        { "version", no_argument, NULL, 'V' },
        { "signature", no_argument, NULL, 'S' },
        { "hmod", no_argument, NULL, 'H' },
        { "path", no_argument, NULL, 'P' },
        { "spm-dump", no_argument, NULL, 'D' },
        { "spm-check", no_argument, NULL, 'C' },
        { "help", no_argument, NULL, 'h' },
        { NULL, 0, NULL, 0 }
    };

    int c, rc, ver_rc, ver_errno;
    HMODULE hmod;
    char szModName[260];

    rc = DosQueryModFromEIP(&hmod, NULL, sizeof(szModName), szModName, NULL, (ULONG)_CRT_init);
    if (rc)
        error("DosQueryModFromEIP: %s\n", strerror(__libc_native2errno(rc)));

    rc = DosQueryModuleName(hmod, sizeof(szModName), szModName);
    if (rc)
        error("DosQueryModuleName: %s\n", strerror(__libc_native2errno(rc)));
    if (!realpath(szModName, szModName))
        error("realpath: %s\n", strerror(errno));
    unslashify(szModName);

    get_description(szModName);

    ver_rc = __libcn_query_version(&version);
    ver_errno = errno;

    while ((c = getopt_long(argc, argv, "", long_options, NULL)) != -1)
    {
        switch (c)
        {
            case 'V':
                if (ver_rc)
                {
                    if (ver_errno == ENOSYS)
                        error("Unknown LIBC version\n");
                    error("__libcn_query_version: %s\n", strerror(ver_errno));
                }
                printf("Next %u.%u.%u\n", version.major, version.minor, version.build);
                exit(0);
            case 'H':
                printf("0x%04lX\n", hmod);
                exit(0);
            case 'S':
                printf("%s\n", description);
                exit(0);
            case 'P':
                printf("%s\n", szModName);
                exit(0);
            case 'D':
            case 'C':
                rc = __libc_SpmCheck(0, c == 'C' ? 0 : -1 /* stdout */);
                if (rc < 0)
                    error("__libc_SpmCheck: %s", strerror(errno));
                if (c == 'D')
                {
                    if (ver_rc) /* Older LIBC doesn't support fVerbose = -1 */
                    {
                        const char *output = getenv("LIBC_LOGGING_OUTPUT");
                        if (!output || (stricmp(output, "stdout") && stricmp(output, "stderr")))
                        {
                            if (output && !stricmp(output, "curdir"))
                                printf("%s: Look for SPM dump in current directory", getprogname());
                            else
                                printf("%s: Look for SPM dump in %%LOGFILES%%\\app, /var/log/app, or similar", getprogname());
                        }
                    }
                    exit(0);
                }
                printf("SPM errors: %d\n", rc);
                exit(rc ? 1 : 0);
            case 'h':
                help();
                break;
            default:
                usage();
        }
    }

    if (optind < argc)
        usage();

    if (!ver_rc)
        printf("LIBC version:    Next %u.%u.%u (struct size: %u bytes)\n",
               version.major, version.minor, version.build, version.cb);
    else
        /* TODO: detect older versions */
        printf("LIBC version:    Unknown\n");

    int fParsed = 0;
    if (*description)
    {
        char vendor[33], revision[32], host[12], build[32];
        char date_time[27] = {0};
        char descr[256];
        int fields = sscanf(description, "@#%32[^:]:%31[^#]#@##1##%26c%11[^:]::::%31[^:]::@@%255[^\r\n]",
            vendor, revision, date_time, host, build, descr);
        if (fields == 6)
        {
            fParsed = 1;
            char *dt = date_time + sizeof(date_time) - 2;
            while (dt > date_time && *dt == ' ')
                *dt-- = '\0';
            dt = date_time;
            while (*dt == ' ')
                ++dt;
            printf("LIBC signature:\n"
                   "  Vendor:        %s\n"
                   "  Version:       %s.%s\n"
                   "  Date/time:     %s\n"
                   "  Build host:    %s\n"
                   "  Description:   %s\n",
                   vendor, revision, build, dt, host, descr);
        }
    }
    if (!fParsed)
        printf("LIBC signature:  %s\n", description);

    printf("LIBC module:     %s (0x%04lX)\n", szModName, hmod);

    return 0;
}
