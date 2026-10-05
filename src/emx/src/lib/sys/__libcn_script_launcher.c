/** @file
 *
 * __libcn_script_launcher().
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


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <process.h>
#include <features.h>


#define EXE_EXT ".exe"
#define EXE_EXT_LEN (sizeof(EXE_EXT) - 1)

/* This function is exported but not public (not in headers). */
int __libcn_script_launcher(int argc, char **argv);

#define ERROR(fmt, ...) do { \
    fprintf(stderr, "%s: " fmt, getprogname(), __VA_ARGS__); \
    return 128; \
} while (0)


/**
 * Calls spawnv on a file named as the current executable with the trailing .exe
 * removed.
 *
 * This is an internal function used by
 * src/libc-script-launcher/libc-script-launcher.s.
 *
 * @param argc Executable's argc.
 * @param argv Executable's argv.
 * @return Child exit code or 128 on pre-condition errors.
 */
int __libcn_script_launcher(int argc, char **argv)
{
    char path[_MAX_PATH];
    if (_execname(path, sizeof(path)) || !_realrealpath(path, path, sizeof(path)))
        ERROR("Cannot get current executable file name: %s\n", strerror(errno));

    size_t len = strlen(path);
    if (len < EXE_EXT_LEN || stricmp(&path[len - EXE_EXT_LEN], EXE_EXT))
        ERROR("'%s' does not have '" EXE_EXT "' extension\n", path);
    path[len - EXE_EXT_LEN] = '\0';

    char *name = _getname(path);
    if (!*name)
        ERROR("'%s' has empty filename part\n", path);

    char *executable = path;
    if (*argv)
    {
        /*
         * If argv[0] without .exe resolves to the same file, use it as the
         * executable to retain the original argv[0] inside shebang scripts.
         * Note that if it doesn't, we can't use it because it must be a valid
         * file that the interpreter will receive as the first argument inside
         * __spawnve after reading it and confirming it's a script, not argv[0]
         * (which is ignored in this case).
         *
         * NOTE: Before trimming .exe, trim trailing spaces: they are ignored by
         * OS/2 in file open calls and should not prevent .exe removal. See also
         * __spawnve that does the same for our launcher.
         */
        char *argv0 = *argv;
        len = strlen(argv0);
        while (len && argv0[len - 1] == ' ')
            argv0[--len] = '\0';
        if (len > EXE_EXT_LEN && !stricmp(&argv0[len - EXE_EXT_LEN], EXE_EXT))
            argv0[len - EXE_EXT_LEN] = '\0';
        char *argv0real = _realrealpath(argv0, NULL, 0);
        if (argv0real)
        {
            if (!strcmp(argv0real, path))
                executable = argv0;
            free(argv0real);
        }
    }
    int rc = spawnv(P_OVERLAY | P_NODEFEXT, executable, argv);
    if (rc == -1)
        ERROR("spawnv(%s): %s\n", executable, strerror(errno));

    /* Should not get here after P_OVERLAY */
    ERROR("spawnv(P_OVERLAY, %s): Unexpected return (%d)\n", executable, rc);
}
