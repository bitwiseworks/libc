/* $Id: highmemsocket.c 3746 2012-03-04 18:41:00Z bird $ */
/** @file
 * libsocket - high memory safe support routines.
 */
  
/*
 * Copyright (c) 2011 knut st. osmundsen <bird-src-spam@anduin.net>
 *
 *
 * This file is part of kLIBC.
 *
 * kLIBC is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * kLIBC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with kLIBC; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#define INCL_BASE
#define INCL_FSMACROS
#include <os2emx.h>

#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <errno.h>

#include "socket.h"


/*******************************************************************************
*   Structures and Typedefs                                                    *
*******************************************************************************/
/** 
 * Extended exception registration record for performing a longjmp on fault.
 */
typedef struct MYEXCPTREGREC
{
    EXCEPTIONREGISTRATIONRECORD Core;
    jmp_buf                     JmpBuf;
} MYXCPTREGREC;


static ULONG APIENTRY __libcsocket_safe_copy_xcpt(PEXCEPTIONREPORTRECORD pReport, PEXCEPTIONREGISTRATIONRECORD pRegistration,
                                                  PCONTEXTRECORD pContext, PVOID pWhatever)
{
    if (pReport->ExceptionNum == XCPT_ACCESS_VIOLATION)
    {
        MYXCPTREGREC *pXcptRegRec = (MYXCPTREGREC *)pRegistration;
        longjmp(pXcptRegRec->JmpBuf, 2);
    }

    return XCPT_CONTINUE_SEARCH;
}

    
/**
 * Safe copy.
 *
 * @returns 0 on success, EFAULT if the input or output buffer is invalid.
 * @param   pvDst           The output buffer.
 * @param   pvSrc           The input buffer
 * @param   cbCopy          The number of bytes to copy.
 */
int __libsocket_safe_copy(void *pvDst, void const *pvSrc, size_t cbCopy)
{
    MYXCPTREGREC    XcptRegRec;
    int             rc;
    FS_VAR_SAVE_LOAD();

    XcptRegRec.Core.ExceptionHandler = __libcsocket_safe_copy_xcpt;
    XcptRegRec.Core.prev_structure   = END_OF_CHAIN;

    DosSetExceptionHandler(&XcptRegRec.Core);
    if (setjmp(XcptRegRec.JmpBuf) == 0)
    {
        memcpy(pvDst, pvSrc, cbCopy);
        rc = 0;
    }
    else
        rc = EFAULT;    
    DosUnsetExceptionHandler(&XcptRegRec.Core);

    FS_RESTORE();
    return rc;
}


/** Check if ptr is in high memory or not. */
#define __LIBSOCKET_SAFE_IS_HIGH(ptr)   ((unsigned)(ptr) >= 512*1024*1024)

void *  _lmalloc(size_t);


int __libsocket_safe_addr_pre(const struct sockaddr *pAddr, const int *pcbAddr, __LIBSOCKET_SAFEADDR *pSafeAddr)
{
    pSafeAddr->pAddr   = (struct sockaddr *)pAddr;
    pSafeAddr->pcbAddr = (int *)pcbAddr;
    pSafeAddr->cbAddr  = 0;
    pSafeAddr->cbAlloc = 0;
    pSafeAddr->pvFree  = NULL;

    if (!__LIBSOCKET_SAFE_IS_HIGH(pAddr) && !__LIBSOCKET_SAFE_IS_HIGH(pcbAddr))
        return 0;

    if (__libsocket_safe_copy(&pSafeAddr->cbAddr, pcbAddr, sizeof(*pcbAddr)) == 0)
    {
        pSafeAddr->cbAlloc = pSafeAddr->cbAddr;
        pSafeAddr->pvFree = _lmalloc(pSafeAddr->cbAlloc);
        if (pSafeAddr->pvFree)
        {
            if (__libsocket_safe_copy(pSafeAddr->pvFree, pAddr, pSafeAddr->cbAlloc) == 0)
            {
                pSafeAddr->pAddr   = (struct sockaddr *)pSafeAddr->pvFree;
                pSafeAddr->pcbAddr = &pSafeAddr->cbAddr;
                return 0;
            }

            __libc_TcpipSetErrno(EFAULT);
            free(pSafeAddr->pvFree);
        }
        else
            __libc_TcpipSetErrno(ENOMEM);
    }
    else
        __libc_TcpipSetErrno(EFAULT);
    return -1;
}


/**
 * Undo the bounce buffering of the socket address after the kernel call.
 *
 * @returns 0 if ok, -1 and errno (if fSuccess is set) on error.
 * @param   pAddr       The user specified address buffer.
 * @param   pcbAddr     The user specified address length.
 * @param   pSafeAddr   Pointer to the safe socket address state.
 * @param   fSuccess    Non-zero if the kernel call succeeded.
 */
int __libsocket_safe_addr_post(struct sockaddr *pAddr, int *pcbAddr, __LIBSOCKET_SAFEADDR *pSafeAddr, int fSuccess)
{
    if (!pSafeAddr->pvFree)
        return 0;

    if (   __libsocket_safe_copy(pcbAddr, &pSafeAddr->cbAddr, sizeof(*pcbAddr)) == 0
        && __libsocket_safe_copy(pAddr, pSafeAddr->pvFree, pSafeAddr->cbAlloc) == 0)
    {
        free(pSafeAddr->pvFree);
        return 0;
    }
    
    free(pSafeAddr->pvFree);
    if (fSuccess)
        __libc_TcpipSetErrno(EFAULT);
    return -1;
}


/**
 * Free the bounce buffer if enagaged.
 *
 * Used instead of __libsocket_safe_addr_post for input only addresses.
 *
 * @param   pSafeAddr   Pointer to the safe socket address state.
 */
void __libsocket_safe_addr_free(__LIBSOCKET_SAFEADDR *pSafeAddr)
{
    if (pSafeAddr->pvFree)
        free(pSafeAddr->pvFree);
}
