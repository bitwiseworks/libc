/*
    sem_getvalue.c
    Copyright (c) 2020 bww bitwiseworks GmbH
*/

#include "libc-alias.h"
#include "sys/_sem.h"
#include <errno.h>
#include <InnoTekLIBC/errno.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_PROCESS
#include <InnoTekLIBC/logstrict.h>


int _STD(sem_getvalue)(sem_t *sem, int *sval)
{
    LIBCLOG_ENTER("sem=%p sval=%p\n", sem, sval);

    __sem_t *s = (__sem_t *)sem;

    if (s->magic != (uintptr_t)&DosCreateEventSem)
    {
        errno = EINVAL;
        LIBCLOG_ERROR_RETURN_INT(-1);
    }

    int val = __atomic_load_n(&s->count, __ATOMIC_RELAXED);
    LIBCLOG_MSG("val=%d\n", val);

    *sval = val;
    LIBCLOG_RETURN_INT(0);
}
