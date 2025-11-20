/*
    clock_getres.c
    Copyright (c) 2020 bww bitwise works GmbH
*/

#include "libc-alias.h"
#include <time.h>
#include <sys/time.h>
#include <errno.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_TIME
#include <InnoTekLIBC/logstrict.h>

int _STD(clock_getres)(clockid_t clock_id, struct timespec *res)
{
    LIBCLOG_ENTER("clock_id=%d tp=%p\n", clock_id, res);

    switch (clock_id)
    {
    case CLOCK_REALTIME:
        if (res)
        {
            /*
             * The actual QSV_MS_COUNT resolution is 31 ms but the precision
             * of returned snapshots is 1 ms (due to rounding error and such).
             */
            res->tv_sec = 0;
            res->tv_nsec = 1000000;
        }
        LIBCLOG_RETURN_INT(0);

    case CLOCK_MONOTONIC:
        if (res)
        {
            /*
             * __libc_Back_timeHighResNano returns snapshots with a precision
             * of 1 ns.
             */
            res->tv_sec = 0;
            res->tv_nsec = 1;
        }
        LIBCLOG_RETURN_INT(0);
    }

    errno = EINVAL;
    LIBCLOG_ERROR_RETURN_INT(-1);
}
