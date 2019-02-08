/* $Id: SafeDosQueryCp.c 3942 2014-12-26 19:15:42Z bird $ */
/** @file
 * kLibC - Safe SafeDosQueryCp().
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

ULONG APIENTRY SafeDosQueryCp(ULONG cbCodePages, PULONG paCodePages, PULONG pcbReturned);
ULONG APIENTRY SafeDosQueryCp(ULONG cbCodePages, PULONG paCodePages, PULONG pcbReturned)
{
    APIRET rc;
    SAFE_INOUTBUF(paCodePages, cbCodePages, ULONG);
    SAFE_INOUTTYPE(pcbReturned, ULONG);

    rc = DosQueryCp(cbCodePages, SAFE_INOUTBUF_USE(paCodePages), SAFE_INOUTTYPE_USE(pcbReturned));

    SAFE_INOUTTYPE_DONE(pcbReturned);
    SAFE_INOUTBUF_DONE(paCodePages, cbCodePages);
    return rc;
}

