/*
    sem_init.c
    Copyright (c) 2020 bww bitwise works GmbH
*/

#include "libc-alias.h"
#include "sys/_sem.h"
#include <errno.h>
#include <InnoTekLIBC/errno.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_PROCESS
#include <InnoTekLIBC/logstrict.h>


int _STD(sem_init)(sem_t *sem, int pshared, unsigned int value)
{
    LIBCLOG_ENTER("sem=%p pshared=%d value=%u\n", sem, pshared, value);

    __sem_t *s = (__sem_t *)sem;

    if (value > SEM_VALUE_MAX)
    {
        errno = EINVAL;
        LIBCLOG_ERROR_RETURN_INT(-1);
    }

    s->magic = (uintptr_t)&DosCreateEventSem;
    s->count = value;

    ULONG flags = DCE_POSTONE;
    if (pshared)
        flags |= DC_SEM_SHARED;

    APIRET arc = DosCreateEventSem(NULL, &s->hev, flags, FALSE);
    if (arc)
    {
        LIBCLOG_MSG("DosCreateEventSem rc=%lu\n", arc);
        __libc_native2errno(arc);
        LIBCLOG_ERROR_RETURN_INT(-1);
    }

    LIBCLOG_RETURN_INT(0);
}
