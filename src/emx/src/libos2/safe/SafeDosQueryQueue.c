/* $Id: SafeDosQueryQueue.c 3943 2014-12-28 23:31:07Z bird $ */
/** @file
 * kLibC - Safe DosQueryQueue().
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

ULONG APIENTRY SafeDosQueryQueue(HQUEUE hq, PULONG pulCount);
ULONG APIENTRY SafeDosQueryQueue(HQUEUE hq, PULONG pulCount)
{
    APIRET rc;
    SAFE_INOUTTYPE(pulCount, ULONG);

    rc = DosQueryQueue(hq, SAFE_INOUTTYPE_USE(pulCount));

    SAFE_INOUTTYPE_DONE(pulCount);
    return rc;
}

