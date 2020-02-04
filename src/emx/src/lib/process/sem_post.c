/*
    sem_post.c
    Copyright (c) 2020 bww bitwiseworks GmbH
*/

#include "libc-alias.h"
#include "sys/_sem.h"
#include <errno.h>
#include <InnoTekLIBC/errno.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_PROCESS
#include <InnoTekLIBC/logstrict.h>


int _STD(sem_post)(sem_t *sem)
{
    LIBCLOG_ENTER("sem=%p\n", sem);

    __sem_t *s = (__sem_t *)sem;

    if (s->magic != (uintptr_t)&DosCreateEventSem)
    {
        errno = EINVAL;
        LIBCLOG_ERROR_RETURN_INT(-1);
    }

    int val = __atomic_fetch_add(&s->count, 1, __ATOMIC_RELAXED);
    LIBCLOG_MSG("val=%d\n", val);

    if (val < 0)
    {
        /*
         * There are threads waiting in sem_wait, post the semaphore (once)
         * to unblock one of them.
         */
        APIRET arc = DosPostEventSem(s->hev);
        if (arc)
        {
            LIBCLOG_MSG("DosPostEventSem rc=%lu\n", arc);
            __libc_native2errno(arc);
            LIBCLOG_ERROR_RETURN_INT(-1);
        }
    }

    LIBCLOG_RETURN_INT(0);
}
