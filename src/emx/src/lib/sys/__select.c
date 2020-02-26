/* $Id: __select.c 1631 2004-11-14 19:03:24Z bird $ */
/** @file
 *
 * __select().
 *
 * Copyright (c) 2003 knut st. osmundsen <bird-srcspam@anduin.net>
 *
 *
 * This file is part of Innotek LIBC.
 *
 * Innotek LIBC is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Innotek LIBC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Innotek LIBC; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "libc-alias.h"
#define INCL_FSMACROS
#define INCL_DOSSEMAPHORES
#define INCL_ERRORS
#include <os2emx.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/select.h>
#include <emx/io.h>
#include <emx/syscalls.h>
#include "syscalls.h"
#include "backend.h"


static int __select_wait(struct timeval *tv)
{
    /*
     * Wait.
     */
    FS_VAR();
    FS_SAVE_LOAD();
    int rc;
    if (!tv)
        rc = DosWaitEventSem(__libc_back_ghevWait, SEM_INDEFINITE_WAIT);
    else
    {
        ULONG cMillies = tv->tv_sec * 1000 + tv->tv_usec / 1000;
        if (!cMillies && tv->tv_usec)
            cMillies = 1;
        rc = DosWaitEventSem(__libc_back_ghevWait, cMillies);
    }
    FS_RESTORE();

    /*
     * Return.
     */
    if (!rc || rc == ERROR_TIMEOUT || rc == ERROR_SEM_TIMEOUT)
        return 0;
    errno = __libc_native2errno(rc);
    return -1;
}

int __select(int nfds, struct fd_set *readfds, struct fd_set *writefds,
             struct fd_set *exceptfds, struct timeval *tv)
{
    struct fd_set  *pLookAhead = NULL;
    int             cLookAhead = 0;
    int   (*pfnSelect)(int cFHs, struct fd_set *pRead, struct fd_set *pWrite, struct fd_set *pExcept, struct timeval *tv, int *prc)
        = NULL;
    int rc;
    int rc2 = -1;
    int i;

    /*
     * Wait operation?
     */
    if (nfds == 0 || (!readfds && !writefds && !exceptfds))
        return __select_wait(tv);


    /*
     * Iterate thru the bitmaps checking the handles.
     */
    for (i = 0; i < nfds; i++)
    {
        if (    (readfds   && FD_ISSET(i, readfds))
            ||  (writefds  && FD_ISSET(i, writefds))
            ||  (exceptfds && FD_ISSET(i, exceptfds)))
        {
            PLIBCFH pFH = __libc_FH(i);
            if (!pFH)
            {
                errno = EBADF;
                return -1;
            }

            /*
             * We don't like OS/2 handles, nor handles with differnet select routines.
             */
            if (    !pFH->pOps
                ||  (pfnSelect && pFH->pOps->pfnSelect != pfnSelect))
            {
                errno = EINVAL;
                return -1;
            }
            if (!pfnSelect)
                pfnSelect = pFH->pOps->pfnSelect;

            /*
             * Check lookahead.
             */
            if (pFH->iLookAhead >= 0 && readfds)
            {
                if (!pLookAhead)
                {
                    /* allocate set. */
                    int cb = (nfds + 31) / 8;
                    pLookAhead = alloca(cb);
                    if (!pLookAhead)
                    {
                        errno = ENOMEM;
                        return -1;
                    }
                    bzero(pLookAhead, cb);
                }
                FD_SET(i, pLookAhead);
                cLookAhead++;
            }
        }
    }

    if (pLookAhead)
    {
        /*
         * Do a select an merge the ready lookahead stuff with
         * the result.
         */
        struct timeval  tvzero = {0,0};
        rc = pfnSelect(nfds, readfds, writefds, exceptfds, &tvzero, &rc2);
        if (!rc)
        {
            if (rc2 > 0)
            {   /* merge */
                for (i = 0; i < nfds; i++)
                {
                    if (    FD_ISSET(i, pLookAhead)
                        && !FD_ISSET(i, readfds))
                    {
                        if (    (!writefds  || !FD_ISSET(i, writefds))
                            &&  (!exceptfds || !FD_ISSET(i, exceptfds)))
                            rc++;
                        FD_SET(i, readfds);
                    }
                }
            }
            else if (rc2 == 0)
            {   /* copy */
                memcpy(readfds, pLookAhead, (nfds + 7) / 8);
                rc = cLookAhead;
            }
        }
    }
    else if (pfnSelect)
    {
        /*
         * Do a straight select.
         */
        rc = pfnSelect(nfds, readfds, writefds, exceptfds, tv, &rc2);
    }
    else
    {
        /* @todo check specs!!! */
        errno = EINVAL;
        return -1;
    }


    /* set errno */
    if (rc)
    {
        if (rc > 0)
            _sys_set_errno(rc);
        else
            errno = -rc;
    }

    return rc2;
}
