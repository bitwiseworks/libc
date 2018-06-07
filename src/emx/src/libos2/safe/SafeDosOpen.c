/* $Id: SafeDosOpen.c 3926 2014-10-26 01:40:43Z bird $ */
/** @file
 * kLibC - Safe SafeDosOpen().
 *
 * @copyright   Copyright (C) 2003-2014 knut st. osmundsen <bird-klibc-spam-xiv@anduin.net>
 * @licenses    MIT, BSD2, BSD3, BSD4, LGPLv2.1, LGPLv3, LGPLvFuture.
 */


/*******************************************************************************
* Header Files                                                                 *
*******************************************************************************/
#define INCL_BASE
#include <os2.h>
#include "safe.h"

ULONG APIENTRY SafeDosOpen(PCSZ pszFileName, PHFILE phFile, PULONG pulAction,
    ULONG ulFileSize, ULONG ulAttribute, ULONG ulOpenFlags, ULONG ulOpenMode,
    PEAOP2 pEABuf);

ULONG APIENTRY SafeDosOpen(PCSZ pszFileName, PHFILE phFile, PULONG pulAction,
    ULONG ulFileSize, ULONG ulAttribute, ULONG ulOpenFlags, ULONG ulOpenMode,
    PEAOP2 pEABuf)
{
    /** @todo pEABuf */
    ULONG   rc, ul1;
    PULONG  pul1 = NULL;
    HFILE   hf1;
    PHFILE  phf1 = NULL;
    SAFE_PCSZ(pszFileName);

    if (phFile)
    {
        hf1 = *phFile;
        phf1 = &hf1;
    }
    if (pulAction)
    {
        ul1 = *pulAction;
        pul1 = &ul1;
    }

    rc = DosOpen(SAFE_PCSZ_USE(pszFileName), phf1, pul1, ulFileSize, ulAttribute,
                 ulOpenFlags, ulOpenMode, pEABuf);

    if (phFile)
        *phFile = hf1;
    if (pulAction)
        *pulAction = ul1;

    SAFE_PCSZ_DONE(pszFileName);
    SAFE_DOS_FAILURE();
    return rc;
}

