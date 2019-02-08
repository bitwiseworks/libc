/* $Id: SafeDosCreateQueue.c 3943 2014-12-28 23:31:07Z bird $ */
/** @file
 * kLibC - Safe DosCreateQueue().
 *
 * @copyright   Copyright (C) 2003-2015 knut st. osmundsen <bird-klibc-spam-xiv@anduin.net>
 * @licenses    MIT, BSD2, BSD3, BSD4, LGPLv2.1, LGPLv3, LGPLvFuture.
 */


/*******************************************************************************
* Header Files                                                                 *
*******************************************************************************/
#define INCL_BASE
#include <os2.h>
#include "safe.h"

ULONG APIENTRY SafeDosCreateQueue(PHQUEUE phq, ULONG ulPriority, PCSZ pszName);
ULONG APIENTRY SafeDosCreateQueue(PHQUEUE phq, ULONG ulPriority, PCSZ pszName)
{
    APIRET rc;
    SAFE_PCSZ(pszName);
    SAFE_INOUTTYPE(phq, HQUEUE);

    rc = DosCreateQueue(SAFE_INOUTTYPE_USE(phq), ulPriority, SAFE_PCSZ_USE(pszName));

    SAFE_INOUTTYPE_DONE(phq);
    SAFE_PCSZ_DONE(pszName);
    return rc;
}

