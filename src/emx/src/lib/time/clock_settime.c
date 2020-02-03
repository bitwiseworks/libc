/*
    clock_settime.c
    Copyright (c) 2020 bww bitwiseworks GmbH
*/

#include "libc-alias.h"
#include <time.h>
#include <sys/time.h>
#include <errno.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_TIME
#include <InnoTekLIBC/logstrict.h>

int _STD(clock_settime)(clockid_t clock_id, const struct timespec *tp)
{
    LIBCLOG_ENTER("clock_id=%d tp=%p\n", clock_id, tp);

    switch (clock_id)
    {
    case CLOCK_REALTIME:
        if (tp)
        {
            struct timeval tv;
            tv.tv_sec = tp->tv_sec;
            tv.tv_usec = tp->tv_nsec / 1000;
            int rc = settimeofday(&tv, NULL);
            if (rc)
                LIBCLOG_ERROR_RETURN_INT(-1);
        }
        LIBCLOG_RETURN_INT(0);

    case CLOCK_MONOTONIC:
        /*
         * There is no way to change the high resolution timer's counter.
         */
        errno = EPERM;
        LIBCLOG_ERROR_RETURN_INT(-1);
    }

    errno = EINVAL;
    LIBCLOG_ERROR_RETURN_INT(-1);
}
