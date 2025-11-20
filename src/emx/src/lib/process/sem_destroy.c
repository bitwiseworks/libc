/*
    sem_destroy.c
    Copyright (c) 2020 bww bitwise works GmbH
*/

#include "libc-alias.h"
#include "sys/_sem.h"
#include <errno.h>
#include <InnoTekLIBC/errno.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_PROCESS
#include <InnoTekLIBC/logstrict.h>


int _STD(sem_destroy)(sem_t *sem)
{
    LIBCLOG_ENTER("sem=%p\n", sem);

    __sem_t *s = (__sem_t *)sem;

    if (s->magic != (uintptr_t)&DosCreateEventSem)
    {
        errno = EINVAL;
        LIBCLOG_ERROR_RETURN_INT(-1);
    }

    APIRET arc = DosCloseEventSem(s->hev);
    if (arc)
    {
        LIBCLOG_MSG("DosCloseEventSem rc=%lu\n", arc);
        __libc_native2errno(arc);
        LIBCLOG_ERROR_RETURN_INT(-1);
    }

    LIBCLOG_RETURN_INT(0);
}
