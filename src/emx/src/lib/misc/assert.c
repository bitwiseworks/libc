/* assert.c (emx+gcc) -- Copyright (c) 1990-1995 by Eberhard Mattes */

#undef NDEBUG

#include "libc-alias.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_SIGNAL
#include <InnoTekLIBC/logstrict.h>

void _assert(const char *string, const char *fname, unsigned int line)
{
    LIBCLOG_ENTER("string=%p:{%s} fname=%p:{%s} line=%d\n", (void *)string, string, (void *)fname, fname, line);
    fprintf(stderr, "Assertion failed: %s, file %s, line %u\n",
            string, fname, line);
    fflush(stderr);
    abort();
    LIBCLOG_RETURN_VOID(); /* shut up */
}
