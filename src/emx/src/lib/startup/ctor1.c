/* ctor1.c (emx+gcc) -- Copyright (c) 1993 by Eberhard Mattes */

#include <stdlib.h>
#include <emx/startup.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_INITTERM
#include <InnoTekLIBC/logstrict.h>

void __ctordtorInit1 (int *ptr)
{
    LIBCLOG_ENTER("ptr=%p:{%d}\n", (void *)ptr, *ptr);
    int n;
    void (**ppf)();

    if (*ptr == -2)                 /* emxomf */
        for (ppf = (void (**)()) (ptr + 1); *ppf != NULL; ++ppf)
            (**ppf)();
    else
    {
        if (*ptr == -1) --ptr;      /* Fix GNU ld bug */
        n = *ptr++;                 /* Get size of vector */
        if (*ptr == -1)             /* First element must be -1, see crt0.s */
            for (ppf = (void (**)()) (ptr + 1); n > 1; ++ppf, --n)
                (**ppf)();
    }
    LIBCLOG_RETURN_VOID();
}
