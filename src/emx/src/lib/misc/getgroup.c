/* getgroup.c (emx+gcc) -- Copyright (c) 1994-1995 by Eberhard Mattes */

/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include "libc-alias.h"
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_MISC
#include <InnoTekLIBC/logstrict.h>

/*******************************************************************************
*   Global Variables                                                           *
*******************************************************************************/
/** Pointer to array of groups. */
static gid_t   *gpaGroups;
/** Count of groups in array. */
static int      gcGroups;

int _STD(getgroups) (int gidsetlen, gid_t grouplist [])
{
    LIBCLOG_ENTER("gidsetlen=%d grouplist=%p\n", gidsetlen, (void *)grouplist);
    if (gcGroups)
    {
        int c = gidsetlen >= gcGroups ? gcGroups : gidsetlen;
        memcpy(grouplist, gpaGroups, c * sizeof(gid_t));
        LIBCLOG_RETURN_INT(c);
    }

    LIBCLOG_ERROR_RETURN_INT(0);
}


int _STD(setgroups)(int ngroups, const gid_t *grouplist)
{
    LIBCLOG_ENTER("ngroups=%d grouplist=%p\n", ngroups, (void *)grouplist);

    void *pv = realloc(gpaGroups, (ngroups + 1) * sizeof(gid_t));
    if (!pv)
        LIBCLOG_ERROR_RETURN_INT(-1);

    gpaGroups = memcpy(pv, grouplist, ngroups * sizeof(gid_t));
    gpaGroups[ngroups] = -1;
    gcGroups = ngroups;

    LIBCLOG_ERROR_RETURN_INT(0);
}
