/*
    sem_trywait.c
    Copyright (c) 2020 bww bitwiseworks GmbH
*/

#include "libc-alias.h"
#include "sys/_sem.h"
#include <errno.h>
#include <InnoTekLIBC/errno.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_PROCESS
#include <InnoTekLIBC/logstrict.h>


int _STD(sem_trywait)(sem_t *sem)
{
    LIBCLOG_ENTER("sem=%p\n", sem);

    __sem_t *s = (__sem_t *)sem;

    if (s->magic != (uintptr_t)&DosCreateEventSem)
    {
        errno = EINVAL;
        LIBCLOG_ERROR_RETURN_INT(-1);
    }

    int val = __atomic_load_n(&s->count, __ATOMIC_RELAXED);
    LIBCLOG_MSG("val=%d\n", val);

    if (val <= 0)
    {
        /*
         * The semaphore is locked.
         */
        errno = EAGAIN;
        LIBCLOG_ERROR_RETURN_INT(-1);
    }

    LIBCLOG_RETURN_INT(0);
}
