/* stat.c (emx+gcc) -- Copyright (c) 1990-1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <emx/time.h>
#include <InnoTekLIBC/backend.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_MISC
#include <InnoTekLIBC/logstrict.h>

int _STD(stat)(const char *name, struct stat *buffer)
{
    LIBCLOG_ENTER("name=%p:{%s} buffer=%p\n", (void *)name, name, (void *)buffer);
    int rc = __libc_Back_fsFileStat(name, buffer);
    if (!rc)
    {
        if (!_tzset_flag)
            tzset();
        _loc2gmt(&buffer->st_atime, -1);
        _loc2gmt(&buffer->st_mtime, -1);
        _loc2gmt(&buffer->st_ctime, -1);
        _loc2gmt(&buffer->st_birthtime, -1);
        LIBCLOG_RETURN_INT(rc);
    }

    errno = -rc;
    LIBCLOG_ERROR_RETURN_INT(-1);
}
