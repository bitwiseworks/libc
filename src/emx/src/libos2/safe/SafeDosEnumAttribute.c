/** @file
 * kLibC - Safe SafeDosEnumAttribute().
 *
 * @copyright   Copyright (C) 2019 bww bitwise works GmbH
 * @licenses    MIT, BSD2, BSD3, BSD4, LGPLv2.1, LGPLv3, LGPLvFuture.
 */


/*******************************************************************************
* Header Files                                                                 *
*******************************************************************************/
#define INCL_BASE
#include <os2.h>
#include "safe.h"
#include <sys/param.h>

APIRET APIENTRY  SafeDosEnumAttribute(ULONG ulRefType, CPVOID pvFile, ULONG ulEntry,
                                      PVOID pvBuf, ULONG cbBuf, PULONG pulCount, ULONG ulInfoLevel);


APIRET APIENTRY  SafeDosEnumAttribute(ULONG ulRefType, CPVOID pvFile, ULONG ulEntry,
                                      PVOID pvBuf, ULONG cbBuf, PULONG pulCount, ULONG ulInfoLevel)
{
    APIRET rc;

    ULONG ulFileLength = ulRefType == ENUMEA_REFTYPE_FHANDLE ? sizeof(HFILE) : strlen(pvFile) + 1;

    SAFE_INBUF(pvFile, ulFileLength, VOID);
    SAFE_INOUTBUF(pvBuf, cbBuf, VOID);
    SAFE_INOUTTYPE(pulCount, ULONG);

    rc = DosEnumAttribute(ulRefType, SAFE_INBUF_USE(pvFile), ulEntry, SAFE_INOUTBUF_USE(pvBuf), cbBuf, SAFE_INOUTTYPE_USE(pulCount), ulInfoLevel);

    SAFE_INOUTTYPE_DONE(pulCount);
    SAFE_INOUTBUF_DONE(pvBuf, cbBuf);
    SAFE_INBUF_DONE(pvFile, ulFileLength);

    return rc;
}
