/* $Id: SafeDosQueryDBCSEnv.c 3942 2014-12-26 19:15:42Z bird $ */
/** @file
 * kLibC - Safe SafeDosQueryDBCSEnv().
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

ULONG APIENTRY SafeDosQueryDBCSEnv(ULONG cbBuf, PCOUNTRYCODE pCountryCode, PCHAR pchBuf);
ULONG APIENTRY SafeDosQueryDBCSEnv(ULONG cbBuf, PCOUNTRYCODE pCountryCode, PCHAR pchBuf)
{
    APIRET rc;
    SAFE_INOUTBUF(pchBuf, cbBuf, CHAR);
    SAFE_INTYPE(pCountryCode, COUNTRYCODE);

    rc = DosQueryDBCSEnv(cbBuf, SAFE_INTYPE_USE(pCountryCode), SAFE_INOUTBUF_USE(pchBuf));

    SAFE_INTYPE_DONE(pCountryCode);
    SAFE_INOUTBUF_DONE(pchBuf, cbBuf);
    return rc;
}

