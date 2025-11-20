/*
    sem_timedwait.c
    Copyright (c) 2020 bww bitwise works GmbH
*/

#include "libc-alias.h"
#include "sys/_sem.h"
#include <errno.h>
#include <sys/time.h>
#include <sys/limits.h>
#include <InnoTekLIBC/errno.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_PROCESS
#include <InnoTekLIBC/logstrict.h>


int _STD(sem_timedwait)(sem_t *sem, const struct timespec *abs_timeout)
{
    LIBCLOG_ENTER("sem=%p abs_timeout=%p\n", sem, abs_timeout);

    __sem_t *s = (__sem_t *)sem;

    if (s->magic != (uintptr_t)&DosCreateEventSem ||
        abs_timeout->tv_nsec < 0 || abs_timeout->tv_nsec >= 1000000000L)
    {
        errno = EINVAL;
        LIBCLOG_ERROR_RETURN_INT(-1);
    }

    int val = __atomic_fetch_sub(&s->count, 1, __ATOMIC_RELAXED);
    LIBCLOG_MSG("val=%d\n", val);

    if (val <= 0)
    {
        /*
         * The semaphore is locked, wati for sem_post for the given time.
         * Note that DosWait granularity is ms, so we truncate times to ms.
         */
        struct timeval now;
        gettimeofday(&now, NULL);

        unsigned long long current, desired;
        current = (unsigned)now.tv_sec * 1000 + now.tv_usec / 1000;
        desired = (unsigned)abs_timeout->tv_sec * 1000 + abs_timeout->tv_nsec / 1000000;
        LIBCLOG_MSG("current=%llu desired=%llu\n", current, desired);

        if (current >= desired)
        {
            errno = ETIMEDOUT;
            LIBCLOG_ERROR_RETURN_INT(-1);
        }

        desired -= current;
        while (1)
        {
            ULONG timeout = desired > ULONG_MAX - 1 ? ULONG_MAX - 1 : desired;
            LIBCLOG_MSG("timeout rc=%lu\n", timeout);

            APIRET arc = DosWaitEventSem(s->hev, timeout);
            if (!arc)
                break;

            if (arc != ERROR_TIMEOUT)
            {
                LIBCLOG_MSG("DosWaitEventSem rc=%lu\n", arc);
                __libc_native2errno(arc);
                LIBCLOG_ERROR_RETURN_INT(-1);
            }

            desired -= timeout;
            if (!desired)
            {
                errno = ETIMEDOUT;
                LIBCLOG_ERROR_RETURN_INT(-1);
            }
        }
    }

    LIBCLOG_RETURN_INT(0);
}
