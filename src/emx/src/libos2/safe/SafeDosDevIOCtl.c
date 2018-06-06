/* $Id: SafeDosDevIOCtl.c 3907 2014-10-24 00:50:45Z bird $ */
/** @file
 * kLibC - Safe SafeDosDevIOCtl().
 *
 * @copyright   Copyright (C) 2014 knut st. osmundsen <bird-klibc-spam-xiv@anduin.net>
 * @licenses    MIT, BSD2, BSD3, BSD4, LGPLv2.1, LGPLv3, LGPLvFuture.
 */


/*******************************************************************************
* Header Files                                                                 *
*******************************************************************************/
#define INCL_BASE
#include <os2.h>
#include "safe.h"
#include <sys/param.h>

APIRET APIENTRY  SafeDosDevIOCtl(HFILE hDevice, ULONG ulCategory, ULONG ulFunction,
                                 PVOID pvParams, ULONG cbParamsMax, PULONG pcbParams,
                                 PVOID pvData, ULONG cbDataMax, PULONG pcbData);


APIRET APIENTRY  SafeDosDevIOCtl(HFILE hDevice, ULONG ulCategory, ULONG ulFunction,
                                 PVOID pvParams, ULONG cbParamsMax, PULONG pcbParams,
                                 PVOID pvData, ULONG cbDataMax, PULONG pcbData)
{
    /*
     * Shadow user buffers residing in high memory.
     */
    ULONG  cbParamsSafe  = 0;
    PULONG pcbParamsSafe = pcbParams;
    if (SAFE_IS_HIGH(pcbParamsSafe))
    {
        cbParamsSafe  = *pcbParamsSafe;
        pcbParamsSafe = &cbParamsSafe;
    }

    ULONG  cbParamsAllocated = 0;
    PVOID  pvParamsSafe      = pvParams;
    if (SAFE_IS_HIGH(pvParamsSafe))
    {
        cbParamsAllocated = MAX(cbParamsMax, *pcbParamsSafe);
        pvParamsSafe = _lmalloc(cbParamsAllocated);
        if (!pvParamsSafe)
            return ERROR_NOT_ENOUGH_MEMORY;
        memcpy(pvParamsSafe, pvParams, cbParamsAllocated);
    }

    ULONG  cbDataSafe  = 0;
    PULONG pcbDataSafe = pcbData;
    if (SAFE_IS_HIGH(pcbDataSafe))
    {
        cbDataSafe  = *pcbDataSafe;
        pcbDataSafe = &cbDataSafe;
    }

    ULONG  cbDataAllocated = 0;
    PVOID  pvDataSafe      = pvData;
    if (SAFE_IS_HIGH(pvDataSafe))
    {
        cbDataAllocated = MAX(cbDataMax, *pcbDataSafe);
        pvDataSafe = _lmalloc(cbDataAllocated);
        if (!pvDataSafe)
        {
            if (cbParamsAllocated)
                free(pvParamsSafe);
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        memcpy(pvDataSafe, pvData, cbDataAllocated);
    }

    /*
     * Do the call with the safe buffers.
     */
    APIRET rc = DosDevIOCtl(hDevice, ulCategory, ulFunction,
                            pvParamsSafe, cbParamsMax, pcbParamsSafe,
                            pvDataSafe,   cbDataMax,   pcbDataSafe);

    /*
     * Copy shadow buffer content back to the caller.
     */
    if (pcbParamsSafe == &cbParamsSafe)
        *pcbParams = cbParamsSafe;

    if (pcbDataSafe == &cbDataSafe)
        *pcbParams = cbDataSafe;

    if (cbParamsAllocated)
    {
        if (memcmp(pvParamsSafe, pvParams, cbParamsAllocated) != 0)
            memcpy(pvParams, pvParamsSafe, cbParamsAllocated);
        free(pvParamsSafe);
    }

    if (cbDataAllocated)
    {
        if (memcmp(pvDataSafe, pvData, cbDataAllocated) != 0)
            memcpy(pvData, pvDataSafe, cbDataAllocated);
        free(pvDataSafe);
    }

    return rc;
}

