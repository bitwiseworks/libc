/* $Id: SafeDosPeekQueue.c 3943 2014-12-28 23:31:07Z bird $ */
/** @file
 * kLibC - Safe DosPeekQueue().
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

ULONG APIENTRY SafeDosPeekQueue(HQUEUE hq, PREQUESTDATA pRequest, PULONG pcbData, PPVOID ppvData, PULONG pulElement,
                                BOOL32 fNowait, PBYTE pbPriority, HEV hsem);
ULONG APIENTRY SafeDosPeekQueue(HQUEUE hq, PREQUESTDATA pRequest, PULONG pcbData, PPVOID ppvData, PULONG pulElement,
                                BOOL32 fNowait, PBYTE pbPriority, HEV hsem)
{
    APIRET rc;
    SAFE_INOUTTYPE(pRequest, REQUESTDATA);
    SAFE_INOUTTYPE(pcbData, ULONG);
    SAFE_INOUTTYPE(ppvData, PVOID);
    SAFE_INOUTTYPE(pulElement, ULONG);
    SAFE_INOUTTYPE(pbPriority, BYTE);

    rc = DosPeekQueue(hq, SAFE_INOUTTYPE_USE(pRequest), SAFE_INOUTTYPE_USE(pcbData), SAFE_INOUTTYPE_USE(ppvData),
                      SAFE_INOUTTYPE_USE(pulElement), fNowait, SAFE_INOUTTYPE_USE(pbPriority), hsem);

    SAFE_INOUTTYPE_DONE(pbPriority);
    SAFE_INOUTTYPE_DONE(pulElement);
    SAFE_INOUTTYPE_DONE(ppvData);
    SAFE_INOUTTYPE_DONE(pcbData);
    SAFE_INOUTTYPE_DONE(pRequest);
    return rc;
}

