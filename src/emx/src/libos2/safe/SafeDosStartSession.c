/* $Id: SafeDosStartSession.c 3385 2007-06-10 11:34:17Z bird $ */
/** @file
 *
 * Safe DosStartSession.
 *
 * Copyright (c) 2004 Dmitry Froloff <froloff@os2.ru>
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
#include "safe.h"

APIRET APIENTRY  SafeDosStartSession(PSTARTDATA psd, PULONG pulIdSession, PPID ppid);
APIRET APIENTRY  SafeDosStartSession(PSTARTDATA psd, PULONG pulIdSession, PPID ppid)
{
    APIRET  rc;
    ULONG   ulIdSession;
    PID     pid;
    PBYTE   pchSafeMem = NULL;
    size_t  cbSafe = 0;
    /* It seems that DosStartSeesion doesn't have high memory disadvantages with
       STARTDATA struct itself  */
    PSZ     PgmTitle        = psd->PgmTitle;
    int     l0;
    PSZ     PgmName         = psd->PgmName;
    int     l1;
    PSZ     PgmInputs       = psd->PgmInputs;
    int     l2;
    PSZ     TermQ           = psd->TermQ;
    int     l3;
    PSZ     Environment     = psd->Environment;
    int     l4;
    PSZ     IconFile        = psd->IconFile;
    int     l5;
    PSZ     ObjectBuffer    = psd->ObjectBuffer;
    ULONG   ObjectBuffLen   = psd->ObjectBuffLen;

    if (SAFE_IS_HIGH(PgmTitle))
    {
        l0 = strlen((const char *)PgmTitle) + 1;
        cbSafe = l0;
    }
    else
        l0 = 0;
    if (SAFE_IS_HIGH(PgmName))
    {
        l1 = strlen((const char *)PgmName) + 1;
        cbSafe += l1;
    }
    else
        l1 = 0;
    if (SAFE_IS_HIGH(TermQ))
    {
        l2 = strlen((const char *)TermQ) + 1;
        cbSafe += l2;
    }
    else
        l2 = 0;
    if (SAFE_IS_HIGH(PgmInputs))
    {
        l3 = strlen((const char *)PgmInputs) + 1;
        cbSafe += l3;
    }
    else
        l3 = 0;
    if (SAFE_IS_HIGH(Environment))
    {
        const char *psz = Environment;
        while (*psz)
            psz = strchr(psz, '\0') + 1;
        l4 = (uintptr_t)psz - (uintptr_t)Environment + 1;
        cbSafe += l4;
    }
    else
        l4 = 0;
    if (SAFE_IS_HIGH(IconFile))
    {
        l5 = strlen((const char *)IconFile) + 1;
        cbSafe += l5;
    }
    else
        l5 = 0;
    if (SAFE_IS_HIGH(ObjectBuffer) && ObjectBuffLen)
        cbSafe += ObjectBuffLen;
    else
        ObjectBuffLen = 0;

    if (cbSafe)
    {
        PBYTE   pchSafe = pchSafeMem = _lmalloc(cbSafe);
        if (pchSafeMem == NULL)
            return ERROR_NOT_ENOUGH_MEMORY;

        if (l0)
        {
            memcpy(pchSafe, PgmTitle, l0);
            psd->PgmTitle = pchSafe;
            pchSafe += l0;
        }
        if (l1)
        {
            memcpy(pchSafe, PgmName, l1);
            psd->PgmName = pchSafe;
            pchSafe += l1;
        }
        if (l2)
        {
            memcpy(pchSafe, TermQ, l2);
            psd->TermQ = pchSafe;
            pchSafe += l2;
        }
        if (l3)
        {
            memcpy(pchSafe, PgmInputs, l3);
            psd->PgmInputs = pchSafe;
            pchSafe += l3;
        }
        if (l4)
        {
            memcpy(pchSafe, Environment, l4);
            psd->Environment = pchSafe;
            pchSafe += l4;
        }
        if (l5)
        {
            memcpy(pchSafe, IconFile, l5);
            psd->IconFile = pchSafe;
            pchSafe += l5;
        }
        if (ObjectBuffLen)
            psd->ObjectBuffer = pchSafe;
    }
    else
        pchSafeMem = NULL;

    rc = DosStartSession(psd,
                         pulIdSession ? &ulIdSession : NULL,
                         ppid ? &pid : NULL);

    /** @todo One should *NOT* modify the input structure but make a COPY!!! */
    /* Restore saved objects */
    psd->PgmTitle = PgmTitle;
    psd->PgmName = PgmName;
    psd->TermQ = TermQ;
    psd->PgmInputs = PgmInputs;
    psd->Environment = Environment;
    psd->IconFile = IconFile;

    /* Set return values */
    if (ObjectBuffLen)
        memcpy(ObjectBuffer, psd->ObjectBuffer, ObjectBuffLen);
    psd->ObjectBuffer = ObjectBuffer;
    if (pulIdSession)
        *pulIdSession = ulIdSession;
    if (ppid)
        *ppid = pid;

    /* cleanup and return. */
    if (pchSafeMem)
        free(pchSafeMem);
    return rc;

}

