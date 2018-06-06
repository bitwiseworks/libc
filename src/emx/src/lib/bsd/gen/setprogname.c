#include "namespace.h" /* bird */
#include <sys/cdefs.h>
__FBSDID("$FreeBSD: src/lib/libc/gen/setprogname.c,v 1.8 2002/03/29 22:43:41 markm Exp $");

#include <stdlib.h>
#include <string.h>

#include "libc_private.h"

void
setprogname(const char *progname)
{
    const char *p;
#ifdef __EMX__
    p = strlen(progname) + progname;
    while (p >= progname)
    {
        if (*p == '\\' || *p == '/' || *p == ':')
        {
            __progname = p + 1;
            return;
        }
        p--;
    }
    __progname = progname;
    return;
#else
	p = strrchr(progname, '/');
	if (p != NULL)
		__progname = p + 1;
	else
		__progname = progname;
#endif
}
