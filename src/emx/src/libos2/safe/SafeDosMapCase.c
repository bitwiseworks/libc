/* $Id: SafeDosMapCase.c 3942 2014-12-26 19:15:42Z bird $ */
/** @file
 * kLibC - Safe SafeDosMapCase().
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

ULONG APIENTRY SafeDosMapCase(ULONG ulLength, __const__ COUNTRYCODE *pCountryCode, PCHAR pchString);
ULONG APIENTRY SafeDosMapCase(ULONG ulLength, __const__ COUNTRYCODE *pCountryCode, PCHAR pchString)
{
    APIRET rc;
    SAFE_INOUTBUF(pchString, ulLength, CHAR);
    SAFE_INTYPE(pCountryCode, COUNTRYCODE);

    rc = DosMapCase(ulLength, SAFE_INTYPE_USE(pCountryCode), SAFE_INOUTBUF_USE(pchString));

    SAFE_INTYPE_DONE(pCountryCode);
    SAFE_INOUTBUF_DONE(pchString, ulLength);
    return rc;
}

