/* $Id: SafeDosQueryCollate.c 3942 2014-12-26 19:15:42Z bird $ */
/** @file
 * kLibC - Safe SafeDosQueryCollate().
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

ULONG APIENTRY SafeDosQueryCollate(ULONG ulLength, __const__ COUNTRYCODE *pCountryCode, PCHAR pchBuffer, PULONG pulDataLength);
ULONG APIENTRY SafeDosQueryCollate(ULONG ulLength, __const__ COUNTRYCODE *pCountryCode, PCHAR pchBuffer, PULONG pulDataLength)
{
    APIRET rc;
    SAFE_INOUTBUF(pchBuffer, ulLength, CHAR);
    SAFE_INOUTTYPE(pulDataLength, ULONG);
    SAFE_INTYPE(pCountryCode, COUNTRYCODE);

    rc = DosQueryCollate(ulLength, SAFE_INTYPE_USE(pCountryCode), SAFE_INOUTBUF_USE(pchBuffer), SAFE_INOUTTYPE_USE(pulDataLength));

    SAFE_INTYPE_DONE(pCountryCode);
    SAFE_INOUTTYPE_DONE(pulDataLength);
    SAFE_INOUTBUF_DONE(pchBuffer, ulLength);
    return rc;
}

