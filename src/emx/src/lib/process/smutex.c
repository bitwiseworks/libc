/* smutex.c (emx+gcc) */

#include "libc-alias.h"
#define INCL_DOSPROCESS
#define INCL_FSMACROS
#include <os2.h>
#include <sys/builtin.h>
#include <sys/smutex.h>
#include <InnoTekLIBC/FastInfoBlocks.h>
#include <InnoTekLIBC/backend.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_MUTEX
#include <InnoTekLIBC/logstrict.h>


void __smutex_request_internal(volatile _smutex *sem)
{
    LIBCLOG_ENTER("sem=%p\n", sem);
    FS_VAR();
    FS_SAVE_LOAD();
    DosSleep(1);
    while (__cxchg(sem, 1) != 0)
    {
        if (fibIsInExit())
        {
            __libc_Back_panic(0, NULL, "smutex deadlock: %p\n", sem);
            break;
        }
        DosSleep(1);
    }
    FS_RESTORE();
    LIBCLOG_RETURN_VOID();
}
