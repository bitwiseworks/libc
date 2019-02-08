/* $Id: SafeDosQueryCtryInfo.c 3942 2014-12-26 19:15:42Z bird $ */
/** @file
 * kLibC - Safe SafeDosQueryCtryInfo().
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

ULONG APIENTRY SafeDosQueryCtryInfo(ULONG cbInfo, PCOUNTRYCODE pCountryCode, PCOUNTRYINFO pCountryInfo, PULONG pcbReturned);
ULONG APIENTRY SafeDosQueryCtryInfo(ULONG cbInfo, PCOUNTRYCODE pCountryCode, PCOUNTRYINFO pCountryInfo, PULONG pcbReturned)
{
    APIRET rc;
    SAFE_INOUTBUF(pCountryInfo, cbInfo, COUNTRYINFO);
    SAFE_INOUTTYPE(pcbReturned, ULONG);
    SAFE_INTYPE(pCountryCode, COUNTRYCODE);

    rc = DosQueryCtryInfo(cbInfo, SAFE_INTYPE_USE(pCountryCode), SAFE_INOUTBUF_USE(pCountryInfo), SAFE_INOUTTYPE_USE(pcbReturned));

    SAFE_INTYPE_DONE(pCountryCode);
    SAFE_INOUTTYPE_DONE(pcbReturned);
    SAFE_INOUTBUF_DONE(pCountryInfo, cbInfo);
    return rc;
}

