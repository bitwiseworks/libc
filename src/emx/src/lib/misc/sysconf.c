/* sysconf.c (emx+gcc) -- Copyright (c) 1994-1995 by Eberhard Mattes */

#include "libc-alias.h"
#include <unistd.h>
#include <limits.h>
#include <time.h>
#include <errno.h>
#include <sys/param.h>
#include <InnoTekLIBC/errno.h>

#define INCL_BASE
#define INCL_FSMACROS
#include <os2emx.h>

/* Return the POSIX.1  minimum values, for now. */

long _STD(sysconf) (int name)
{
    switch (name)
    {
        case _SC_ARG_MAX:
            return ARG_MAX;

        case _SC_CHILD_MAX:
            return _POSIX_CHILD_MAX;

        case _SC_CLK_TCK:
            /* On emx, CLK_TCK is a constant.  On other systems, it may a
               macro which calls sysconf (_SC_CLK_TCK). */

            return CLK_TCK;

        case _SC_NGROUPS_MAX:
            return _POSIX_NGROUPS_MAX;

        case _SC_OPEN_MAX:
            return OPEN_MAX;

        case _SC_STREAM_MAX:
            return _POSIX_STREAM_MAX;

        case _SC_TZNAME_MAX:
            return _POSIX_TZNAME_MAX;

        case _SC_JOB_CONTROL:
            return -1;

        case _SC_SAVED_IDS:
            return -1;

        case _SC_VERSION:
            return _POSIX_VERSION;

        case _SC_SIGQUEUE_MAX:
            return _POSIX_SIGQUEUE_MAX;

        case _SC_PAGESIZE:
            return PAGE_SIZE;

        case _SC_NPROCESSORS_ONLN:
        {
            static long cCpus = 0;
            if (!cCpus)
            {
                ULONG ul;
                FS_VAR_SAVE_LOAD();
                if (DosQuerySysInfo(QSV_NUMPROCESSORS, QSV_NUMPROCESSORS, &ul, sizeof(ul)))
                    ul = 1;
                cCpus = ul;
                FS_RESTORE();
            }
            return cCpus;
        }

        case _SC_PHYS_PAGES:
        {
            static long cPhysPages = 0;
            if (!cPhysPages)
            {
                APIRET rc;
                ULONG ul;
                FS_VAR_SAVE_LOAD();
                rc = DosQuerySysInfo(QSV_TOTPHYSMEM, QSV_TOTPHYSMEM, &ul, sizeof(ul));
                FS_RESTORE();
                if (rc)
                {
                    errno = __libc_native2errno(rc);
                    return -1;
                }
                cPhysPages = ul / PAGE_SIZE;
            }
            return cPhysPages;
        }

        case _SC_AVPHYS_PAGES:
        {
            APIRET rc;
            ULONG ul;
            FS_VAR_SAVE_LOAD();
            rc = DosQuerySysInfo(QSV_TOTAVAILMEM, QSV_TOTAVAILMEM, &ul, sizeof(ul));
            FS_RESTORE();
            if (rc)
            {
                errno = __libc_native2errno(rc);
                return -1;
            }
            return ul / PAGE_SIZE;
        }

        default:
            errno = EINVAL;
            return -1;
    }
}

