/* sys/pipe.c (emx+gcc) -- Copyright (c) 1992-1996 by Eberhard Mattes
                        -- Copyright (c) 2003 by Knut St. Osmunden */

#include "libc-alias.h"
#define INCL_ERRORS
#define INCL_DOSQUEUES
#define INCL_FSMACROS
#include <os2safe.h>
#include <os2emx.h>
#include <sys/fcntl.h>
#include <emx/syscalls.h>
#include <emx/io.h>
#include "syscalls.h"
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_IO
#include <InnoTekLIBC/logstrict.h>

int __pipe(int *two_handles, int pipe_size, PLIBCFH *ppFHRead, PLIBCFH *ppFHWrite)
{
    LIBCLOG_ENTER("two_handles=%p pipe_size=%d ppFHRead=%p ppFHWrite=%p\n", two_handles, pipe_size, ppFHRead, ppFHWrite);
    ULONG   rc;
    int     cExpandRetries;
    FS_VAR();

    /*
     * Create the pipe
     */
    FS_SAVE_LOAD();
    for (cExpandRetries = 0;;)
    {
        rc = DosCreatePipe((PHFILE)&two_handles[0], (PHFILE)&two_handles[1], pipe_size);
        if (rc != ERROR_TOO_MANY_OPEN_FILES)
            break;
        if (cExpandRetries++ >= 3)
            break;
        /* auto increment */
        __libc_FHMoreHandles();
    }   /* ... retry 3 times ... */

    if (!rc)
    {
        /*
         * Register the handle.
         */
        rc = __libc_FHAllocate((HFILE)two_handles[0], F_PIPE | O_RDONLY, sizeof(LIBCFH), NULL, ppFHRead, NULL);
        if (!rc)
            rc = __libc_FHAllocate((HFILE)two_handles[1], F_PIPE | O_WRONLY, sizeof(LIBCFH), NULL, ppFHWrite, NULL);
        if (rc)
        {
            DosClose((HFILE)two_handles[0]);
            DosClose((HFILE)two_handles[1]);
        }
    }
    FS_RESTORE();

    /*
     * Handle error.
     */
    if (rc)
    {
        _sys_set_errno(rc);
        LIBCLOG_ERROR_RETURN_INT(-1);
    }
    LIBCLOG_RETURN_INT(0);
}
