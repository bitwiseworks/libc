/* getcwd1.c (emx+gcc) -- Copyright (c) 1992-1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdlib.h>
#include <string.h>
#include <emx/syscalls.h>
#include <InnoTekLIBC/backend.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_MISC
#include <InnoTekLIBC/logstrict.h>

int _getcwd1(char *buffer, char drive)
{
    LIBCLOG_ENTER("buffer=%p drive=%c\n", (void *)buffer, drive);

    int rc = __libc_Back_fsDirCurrentGet(buffer, PATH_MAX, drive, __LIBC_BACK_FSCWD_NO_DRIVE);
    if (!rc)
        LIBCLOG_RETURN_MSG(0, "ret 0 buffer=:{%s}\n", buffer);

    errno = -rc;
    LIBCLOG_ERROR_RETURN_INT(-1);
}
