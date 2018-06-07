/* ftime.c (emx+gcc) -- Copyright (c) 1993-1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <sys/timeb.h>
#include <time.h>
#include <emx/time.h>
#include <emx/syscalls.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_TIME
#include <InnoTekLIBC/logstrict.h>

void _STD(ftime)(struct timeb *ptr)
{
    LIBCLOG_ENTER("ptr=%p\n", (void *)ptr);

    if (!_tzset_flag)
        tzset();

    __ftime(ptr);
    time_t t_loc;
    t_loc = ptr->time;
    ptr->dstflag = _loc2gmt(&ptr->time, -1);
    ptr->timezone = _tzi.tz / 60;

    LIBCLOG_RETURN_MSG_VOID("ptr=%p:{.time=%ld, .millitm=%u, .timezone=%d, .dstflag=%d}\n",
                            (void *)ptr, (long)ptr->time, ptr->millitm, ptr->timezone, ptr->dstflag);
}

