/* dtor1.c (emx+gcc) -- Copyright (c) 1993 by Eberhard Mattes */

#include <stdlib.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_INITTERM
#include <InnoTekLIBC/logstrict.h>
#include <emx/startup.h>

void __ctordtorTerm1(int *ptr)
{
    LIBCLOG_ENTER("ptr=%p:{%d}\n", (void *)ptr, *ptr);
    int n;
    void (**ppf)();

    if (*ptr == -2)                 /* emxomf */
    {
        for (ppf = (void (**)()) (ptr + 1); *ppf != NULL; ++ppf)
            ;
        for (--ppf; ppf > (void (**)()) ptr; --ppf)
            (**ppf)();
    }
    else
    {
        if (*ptr == -1) --ptr;      /* Fix GNU ld bug */
        n = *ptr++ - 1;             /* Get size of vector */
        if (*ptr == -1)             /* First element must be -1, see crt0.s */
            for (ppf = (void (**)()) ptr; n > 0; --n)
                (*ppf[n])();
    }
    LIBCLOG_RETURN_VOID();
}
