/*
    _sem.h -- Internal definitions for POSIX semaphores
    Copyright (c) 2020 bww bitwiseworks GmbH
*/

#ifndef __SYS__SEM_H__
#define __SYS__SEM_H__

#define INCL_DOSSEMAPHORES
#define INCL_DOSERRORS
#include <os2.h>

#include <semaphore.h>
#include <sys/types.h>

struct __sem
{
    union
    {
        struct
        {
            uintptr_t magic;
            volatile int count;
            HEV hev;
        };
        struct _sem dummy;
    };
};

typedef struct __sem __sem_t;

#endif /* __SYS__SEM_H__ */
