/*
    clock_gettime.c
    Copyright (c) 2020 bww bitwiseworks GmbH
*/

#include "libc-alias.h"
#include <time.h>
#include <sys/time.h>
#include <errno.h>
#include <InnoTekLIBC/backend.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_TIME
#include <InnoTekLIBC/logstrict.h>

int _STD(clock_gettime)(clockid_t clock_id, struct timespec *tp)
{
    LIBCLOG_ENTER("clock_id=%d tp=%p\n", clock_id, tp);

    switch (clock_id)
    {
    case CLOCK_REALTIME:
        if (tp)
        {
            struct timeval tv;
            int rc = gettimeofday(&tv, NULL);
            if (rc)
                LIBCLOG_ERROR_RETURN_INT(-1);
            tp->tv_sec = tv.tv_sec;
            tp->tv_nsec = tv.tv_usec * 1000;
        }
        LIBCLOG_RETURN_INT(0);

    case CLOCK_MONOTONIC:
        if (tp)
        {
            hrtime_t hrt = __libc_Back_timeHighResNano();
            tp->tv_sec = hrt / 1000000000LL;
            tp->tv_nsec = hrt % 1000000000LL;
        }
        LIBCLOG_RETURN_INT(0);
    }

    errno = EINVAL;
    LIBCLOG_ERROR_RETURN_INT(-1);
}
