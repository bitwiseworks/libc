#include <sys/cdefs.h>
__FBSDID("$FreeBSD: src/lib/libc/gen/getprogname.c,v 1.4 2002/03/29 22:43:41 markm Exp $");

#include "namespace.h"
#include <stdlib.h>
#include "un-namespace.h"

#include "libc_private.h"

#ifdef __EMX__
#define INCL_BASE
#define INCL_FSMACROS
#include <os2emx.h>
#include <string.h>
const char *__progname = NULL;
#endif

__weak_reference(_getprogname, getprogname);


const char *
_getprogname(void)
{
#ifdef __EMX__
    /*
     * On demand initiation.
     * THREADS: The assignment isn't safe.
     * THREADS: pib_pchcmd is pointing into environment space which in theory
     *          can be changed by any other thread while we're parsing it.
     */
    if (!__progname)
    {
        char *psz, *psz2;
        PTIB ptib;
        PPIB ppib;
        FS_VAR();

        FS_SAVE();
        DosGetInfoBlocks(&ptib, &ppib);
        FS_RESTORE();

        psz = ppib->pib_pchcmd;
        psz2 = strlen(psz) + psz;
        while (psz2 >= psz)
        {
            if (*psz2 == '\\' || *psz2 == '/' || *psz2 == ':')
                break;
            psz2--;
        }
        psz2++;
        /* This is in the volatile env. block - dupe it. */
        __progname = strdup(psz2);
    }
#endif
	return (__progname);
}
