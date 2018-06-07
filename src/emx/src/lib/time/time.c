/* time.c (emx+gcc) -- Copyright (c) 1990-1995 by Eberhard Mattes */

#include "libc-alias.h"
#include <sys/timeb.h>
#include <time.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_TIME
#include <InnoTekLIBC/logstrict.h>

time_t _STD(time) (time_t *t)
{
    LIBCLOG_ENTER("t=%p\n", (void *)t);
    struct timeb tb;
    ftime(&tb);
    if (t != NULL)
        *t = tb.time;
    LIBCLOG_RETURN_MSG(tb.time, "ret %lld (%#llx)\n", (long long)tb.time, (long long)tb.time);
}
