/* times.c (emx+gcc) -- Copyright (c) 1990-1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <time.h>
#include <sys/time.h>
#include <sys/times.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_TIME
#include <InnoTekLIBC/logstrict.h>

/* Note: return value overflows */

clock_t _STD(times)(struct tms *buffer)
{
    LIBCLOG_ENTER("buffer=%p\n", (void *)buffer);
    struct timeval tv;
    if (buffer)
    {
        buffer->tms_utime = clock();  /* clock () * HZ / CLOCKS_PER_SEC */
        buffer->tms_stime = 0;
        buffer->tms_cutime = 0;
        buffer->tms_cstime = 0;
    }
    if (gettimeofday(&tv, NULL) != 0)
        LIBCLOG_ERROR_RETURN_INT(-1);
    clock_t rc = CLK_TCK * tv.tv_sec + (CLK_TCK * tv.tv_usec) / 1000000;
    if (buffer)
        LIBCLOG_RETURN_MSG(rc, "ret %d (%#x) - buffer={.tms_utime=%d, .tms_stime=%d, .tms_cutime=%d, .tms_cstime=%d}\n",
                           (int)rc, (int)rc, (int)buffer->tms_utime, (int)buffer->tms_stime, (int)buffer->tms_cutime,
                           (int)buffer->tms_cstime);
    LIBCLOG_RETURN_MSG(rc, "ret %d (%#x)\n", (int)rc, (int)rc);
}
