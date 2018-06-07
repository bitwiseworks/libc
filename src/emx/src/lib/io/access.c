/* access.c (emx+gcc) -- Copyright (c) 1990-1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <string.h>
#include <io.h>
#include <errno.h>
#include <sys/stat.h>
#include <emx/io.h>
#include <emx/syscalls.h>


/** @todo Reimplement access and eaccess using the actual UID/GID stuff from the file and SPM. */
int _STD(access) (const char *name, int mode)
{
    struct stat s;
    if (stat(name, &s))
        return -1;

    /* Check if directory spec, don't trust stat to do that right yet. */
    char *psz = strchr(name, 0);
    if (    (psz[-1] == '/' || psz[-1] == '\\')
        &&  !(s.st_attr & _A_SUBDIR))
    {
        errno = ENOTDIR;
        return -1;
    }

    /* Volume IDs are not accessible. */
    if (s.st_attr & _A_VOLID)
    {
        errno = ENOENT;
        return -1;
    }

    /* When testing for write permission, check the read-only bit. */
    if ((mode & 2) && (s.st_attr & _A_RDONLY))
    {
        errno = EACCES;
        return -1;
    }
    return 0;
}
