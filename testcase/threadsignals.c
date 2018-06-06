/* $Id: threadsignals.c 1329 2004-03-23 00:17:37Z bird $ */
/** @file
 *
 * Experiment with sending other threads exceptions.
 *
 * Copyright (c) 2004 knut st. osmundsen <bird-srcspam@anduin.net>
 *
 *
 * This file is part of InnoTek LIBC.
 *
 * InnoTek LIBC is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * InnoTek LIBC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with InnoTek LIBC; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */



#define INCL_BASE
#include <os2.h>
#include <stdio.h>
#include <process.h>


ULONG _System exceptionhandler(PEXCEPTIONREPORTRECORD pXRepRec,
                               struct _EXCEPTIONREGISTRATIONRECORD *pXRegRec,
                               PCONTEXTRECORD pCtx, PVOID pv)
{
    printf("exception: %#lx info[0]=%#lx\n", pXRepRec->ExceptionNum, pXRepRec->ExceptionInfo[0]);
    return XCPT_CONTINUE_SEARCH;
}

void threadproc(void *arg)
{
    ULONG                       rc;
    EXCEPTIONREGISTRATIONRECORD XRegRec;

    XRegRec.ExceptionHandler = exceptionhandler;
    XRegRec.prev_structure   = (PVOID)-1;
    DosSetExceptionHandler(&XRegRec);

    rc = DosSleep(~0);
    printf("DosSleep returned rc=%lu\n", rc);

    DosUnsetExceptionHandler(&XRegRec);
}


int main()
{
    TID     tid;
    ULONG   rc;
    tid = _beginthread(threadproc, NULL, 0x10000, NULL);
    printf("started thread %ld\n", tid);
    DosSleep(100);
    rc = DosKillThread(tid);
    printf("DosKillThread returned %lu\n", rc);
    return 0;
}
