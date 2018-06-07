/* sys/waitpid.c (emx+gcc) -- Copyright (c) 1993-1996 by Eberhard Mattes
                           -- Copyright (c) 2003 by Knut St. Osmundsen */

#include "libc-alias.h"
#define INCL_DOSPROCESS
#define INCL_ERRORS
#define INCL_FSMACROS
#include <os2emx.h>
#include <sys/wait.h>
#include <emx/syscalls.h>
#include "syscalls.h"

int __waitpid (int pid, int *status, int options)
{
    ULONG rc;
    RESULTCODES res;
    PID pid2;
    FS_VAR();

    if (pid == -1)
        pid = 0;
    FS_SAVE_LOAD();
    rc = DosWaitChild(DCWA_PROCESS,
                      options & WNOHANG ? DCWW_NOWAIT : DCWW_WAIT,
                      &res, &pid2, pid);
    FS_RESTORE();
    if (    rc == ERROR_CHILD_NOT_COMPLETE
        &&  (options & WNOHANG))
        return 0;
    if (rc != 0)
    {
        _sys_set_errno(rc);
        return -1;
    }
    *status = res.codeResult << 8;
    return pid2;
}
