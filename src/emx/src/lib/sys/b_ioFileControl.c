/* $Id: $ */
/** @file
 *
 * LIBC SYS Backend - File Control.
 *
 * Copyright (c) 2003-2005 knut st. osmundsen <bird@innotek.de>
 *
 *
 * This file is part of InnoTek LIBC.
 *
 * InnoTek LIBC is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * InnoTek LIBC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with InnoTek LIBC; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include "libc-alias.h"

#define INCL_ERRORS
#define INCL_FSMACROS
#include <os2emx.h>

#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <io.h>
#include <string.h>
#include <sys/builtin.h>
#include "b_fs.h"
#include <InnoTekLIBC/backend.h>
#define __LIBC_LOG_GROUP    __LIBC_LOG_GRP_IO
#include <InnoTekLIBC/logstrict.h>


/*******************************************************************************
*   Internal Functions                                                         *
*******************************************************************************/
static int __fcntl_getfd(__LIBC_PFH pFH, int fh, int *pfFlags);
static int __fcntl_setfd(__LIBC_PFH pFH, int fh, int iArg);
static int __fcntl_locking(int fh, int iRequest, struct flock *pFlock);


/**
 * File Control.
 * 
 * Deals with file descriptor flags, file descriptor duplication and locking.
 * 
 * @returns 0 on success and *piRet set.
 * @returns Negated errno on failure and *piRet set to -1.
 * @param   fh          File handle (descriptor).
 * @param   iRequest    Which file file descriptior request to perform.
 * @param   iArg        Argument which content is specific to each
 *                      iRequest operation.
 * @param   prc         Where to store the value which upon success is
 *                      returned to the caller.
 */
int __libc_Back_ioFileControl(int fh, int iRequest, intptr_t iArg, int *prc)
{
    LIBCLOG_ENTER("fh=%d iRequest=%#x iArg=%#x prc=%p\n", fh, iRequest, iArg, (void *)prc);

    /*
     * Get the file fh data.
     */
    __LIBC_PFH  pFH;
    int rc = __libc_FHEx(fh, &pFH);
    if (!rc)
    {
        *prc = 0;
        if (!pFH->pOps)
        {
            /*
             * Standard OS/2 handle.
             */
            rc = __libc_Back_ioFileControlStandard(pFH, fh, iRequest, iArg, prc);
        }
        else
        {
            /*
             * Non-standard fh - call registered method.
             */
            rc = pFH->pOps->pfnFileControl(pFH, fh, iRequest, (int)iArg, prc); /** @todo fix iArg */
        }
        if (!rc)
            LIBCLOG_RETURN_INT(rc);
    }

    /* failure! */
    if (rc > 0)
        rc = -__libc_native2errno(rc);
    *prc = -1;
    LIBCLOG_ERROR_RETURN_INT(rc);
}


/**
 * File Control operation - OS/2 standard handle.
 * 
 * @returns 0 on success.
 * @returns OS/2 error code or negated errno on failure.
 * 
 * @param   pFH         Pointer to the handle structure to operate on.
 * @param   fh          It's associated filehandle.
 * @param   iRequest    Which file file descriptior request to perform.
 * @param   iArg        Argument which content is specific to each
 *                      iRequest operation.
 * @param   prc         Where to store the value which upon success is
 *                      returned to the caller.
 */
int __libc_Back_ioFileControlStandard(__LIBC_PFH pFH, int fh, int iRequest, intptr_t iArg, int *prc)
{
    LIBCLOG_ENTER("pFH=%p fh=%d iRequest=%#x iArg=%#x prc=%p\n", (void *)pFH, fh, iRequest, iArg, (void *)prc);

    switch (iRequest)
    {
        /*
         * Get file status flags and access modes.
         */
        case F_GETFL:
        {
            const unsigned fFlags = pFH->fFlags & __LIBC_FH_GETFL_MASK;
            *prc = fFlags;
            LIBCLOG_RETURN_INT(0);
        }

        /*
         * Set file status flags.
         */
        case F_SETFL:
        {
            *prc = 0;
            unsigned fFlags = pFH->fFlags & ~__LIBC_FH_SETFL_MASK;
            fFlags |= ((int)iArg & __LIBC_FH_SETFL_MASK);
            int rc = __libc_FHSetFlags(pFH, fh, fFlags);
            LIBCLOG_MIX0_RETURN_INT(rc);
        }

        /*
         * Get file descriptor flags.
         */
        case F_GETFD:
        {
            int rc = __fcntl_getfd(pFH, fh, prc);
            LIBCLOG_MIX0_RETURN_INT(rc);
        }

        /*
         * Set file descriptor flags.
         */
        case F_SETFD:
        {
            *prc = 0;
            int rc = __fcntl_setfd(pFH, fh, (int)iArg);
            LIBCLOG_MIX0_RETURN_INT(rc);
        }

        /*
         * File locking.
         */
        case F_GETLK:   /* get record locking information */
        case F_SETLK:   /* set record locking information */
        case F_SETLKW:  /* F_SETLK; wait if blocked */
        {
            *prc = 0;
            int rc = __fcntl_locking(fh, iRequest, (struct flock*)iArg);
            LIBCLOG_MIX0_RETURN_INT(rc);
        }

        default:
            LIBCLOG_ERROR_RETURN(-EINVAL, "ret -EINVAL - Invalid iRequest %#x\n", iRequest);
    }
}


/**
 * F_GETFD operation on standard OS/2 fh.
 * Gets file descriptor flags, which at the moment is limited to FD_CLOEXEC.
 *
 * @returns 0 on success.
 * @returns OS/2 error code or negated errno on failure.
 * @param   pFH     File handler structure.
 * @param   fh   File fh.
 * @param   pfFlags Where to store the flags on success.
 */
static int __fcntl_getfd(__LIBC_PFH pFH, int fh, int *pfFlags)
{
    LIBCLOG_ENTER("pFH=%p fh=%d\n", (void *)pFH, fh);

    FS_VAR_SAVE_LOAD();
    ULONG   fulState;
    int rc = DosQueryFHState(fh, &fulState);
    FS_RESTORE();
    if (!rc)
    {
        unsigned fFlags = pFH->fFlags;
        /* flags out of sync? */
        if (    ( (fulState & OPEN_FLAGS_NOINHERIT) != 0 )
            !=  (   (fFlags & (O_NOINHERIT | (FD_CLOEXEC << __LIBC_FH_FDFLAGS_SHIFT)))
                 == (O_NOINHERIT | (FD_CLOEXEC << __LIBC_FH_FDFLAGS_SHIFT)) ) )
        {
            LIBC_ASSERTM_FAILED("Inherit flags are out of sync for file fh %d (%#x)! fulState=%08lx fFlags=%08x\n",
                                fh, fh, fulState, fFlags);
            if (fulState & OPEN_FLAGS_NOINHERIT)
                fFlags |= O_NOINHERIT | (FD_CLOEXEC << __LIBC_FH_FDFLAGS_SHIFT);
            else
                fFlags &= ~(O_NOINHERIT | (FD_CLOEXEC << __LIBC_FH_FDFLAGS_SHIFT));
            __atomic_xchg(&pFH->fFlags, fFlags);
        }

        *pfFlags = fFlags >> __LIBC_FH_FDFLAGS_SHIFT;
        LIBCLOG_RETURN_INT(0);
    }
    LIBC_ASSERTM_FAILED("DosQueryFHState(%d,) failed rc=%d! was handle closed?\n", fh, rc);
    LIBCLOG_ERROR_RETURN_INT(rc);
}


/**
 * F_SETFD operation on standard OS/2 fh.
 * Sets file descriptor flags, which at the moment is limited to FD_CLOEXEC.
 *
 * @returns 0 on success.
 * @returns OS/2 error code or negated errno on failure.
 * @param   pFH     File handler structure.
 * @param   fh      File fh.
 * @param   iArg    New file descriptor flags.
 */
static int __fcntl_setfd(__LIBC_PFH pFH, int fh, int iArg)
{
    LIBCLOG_ENTER("pFH=%p fh=%d iArg=%#x\n", (void *)pFH, fh, iArg);

    /*
     * Calc new flags.
     */
    unsigned fFlags = pFH->fFlags;
    fFlags = (fFlags & ~__LIBC_FH_FDFLAGS_MASK) | (iArg << __LIBC_FH_FDFLAGS_SHIFT);
    if (iArg & FD_CLOEXEC)
        fFlags |= O_NOINHERIT;
    else
        fFlags &= ~O_NOINHERIT;

    /*
     * Update the flags.
     */
    int rc = __libc_FHSetFlags(pFH, fh, fFlags);
    LIBCLOG_MIX0_RETURN_INT(rc);
}


/**
 * Handle locking requests.
 * 
 * @returns 0 on success.
 * @returns OS/2 error code or negated errno on failure.
 * @param   fh       File fh.
 * @param   iRequest     Lock iRequest.
 * @param   pFlock      Pointer to flock structure.
 */
static int __fcntl_locking(int fh, int iRequest, struct flock *pFlock)
{
    LIBCLOG_ENTER("fh=%d iRequest=%d pFlock=%p\n", fh, iRequest, (void *)pFlock);

    /* 
     * Check input.
     */
    /** @todo: Implement F_GETLK */
    if (iRequest == F_GETLK)
        LIBCLOG_ERROR_RETURN_MSG(-EINVAL, "ret -EINVAL - F_GETLK is not implemented!\n");
    if (!pFlock)
        LIBCLOG_ERROR_RETURN_INT(-EINVAL);

    /* 
     * Check fh & get filesize. 
     */
    union
    {
        FILESTATUS3     fsts3;
        FILESTATUS3L    fsts3L;
    }       info;
    int rc;
    FS_VAR_SAVE_LOAD();
#if OFF_MAX > LONG_MAX
    int  fLarge = 0;
    if (__libc_gpfnDosOpenL)
    {
        rc = DosQueryFileInfo(fh, FIL_STANDARDL, &info, sizeof(info.fsts3L));
        fLarge = 1;
    }
    else
#endif
        rc = DosQueryFileInfo(fh, FIL_STANDARD, &info, sizeof(info.fsts3));
    FS_RESTORE();
    if (!rc)
    {
        ULONG       fAccess;
        int         fLock;
        ULONG       ulTimeout;
        off_t       cbFile;
        off_t       offStart;
        off_t       cbRange;
#if OFF_MAX > LONG_MAX
        if (fLarge)
            cbFile = info.fsts3L.cbFile;
        else
#endif
            cbFile = info.fsts3.cbFile;

        /* range */
        cbRange = pFlock->l_len ? pFlock->l_len : OFF_MAX;

        /* offset */
        switch (pFlock->l_whence)
        {
            case SEEK_SET:  offStart = pFlock->l_start; break;
            case SEEK_CUR:  offStart = tell(fh) + pFlock->l_start; break;
            case SEEK_END:  offStart = cbFile - pFlock->l_start; break;
            default:
                LIBCLOG_ERROR_RETURN_MSG(-EINVAL, "ret -EINVAL - Invalid l_whence=%d\n", pFlock->l_whence);
        }
        if (   offStart < 0
            || (   cbRange != OFF_MAX
                && cbRange + offStart < 0) )
            LIBCLOG_ERROR_RETURN_MSG(-EINVAL, "ret -EINVAL - Invalid offStart=%llx cbRange=%llx\n", offStart, cbRange);

        /* flags and order */
        fAccess = 0; /* exclusive */
        switch (pFlock->l_type)
        {
            case F_UNLCK:
                fLock = 0;
                break;

            case F_RDLCK:
                fAccess = 1; /* shared */
            case F_WRLCK:
                fLock = 1;
                break;

            default:
                LIBCLOG_ERROR_RETURN_MSG(-EINVAL, "ret -EINVAL - Invalid l_type=%d\n", pFlock->l_type);
        }

        /* timeout */
        if (iRequest == F_SETLKW)
            ulTimeout = SEM_INDEFINITE_WAIT;
        else
            ulTimeout = SEM_IMMEDIATE_RETURN;

        /* Do work. */
#if OFF_MAX > LONG_MAX
        rc = ERROR_INVALID_PARAMETER;
        if (__libc_gpfnDosSetFileLocksL)
        {
            FILELOCKL   aflock[2];
            bzero(&aflock[(fLock + 1) & 1], sizeof(aflock[0]));
            aflock[fLock].lOffset = offStart;
            aflock[fLock].lRange  = cbRange;
            FS_SAVE_LOAD();
            rc = __libc_gpfnDosSetFileLocksL(fh, &aflock[0], &aflock[1], ulTimeout, fAccess);
            FS_RESTORE();
        }
        /* 
         * There is/was a bug in the large API which make it fail on non JFS
         * disks with ERROR_INVALID_PARAMETER. We need to work around this. 
         */
        if (rc == ERROR_INVALID_PARAMETER)
#endif
        {
            FILELOCK    aflock[2];
#if OFF_MAX > LONG_MAX
            if (    offStart > LONG_MAX
                ||  (   cbRange != OFF_MAX
                     && (   cbRange > LONG_MAX
                         || offStart + cbRange > LONG_MAX)
                    )
               )
                LIBCLOG_ERROR_RETURN_MSG(-EOVERFLOW, "ret -EOVERFLOW\n");
#endif
            bzero(&aflock[(fLock + 1) & 1], sizeof(aflock[0]));
            aflock[fLock].lOffset = offStart;
            aflock[fLock].lRange  = cbRange;
            FS_SAVE_LOAD();
            rc = DosSetFileLocks(fh, &aflock[0], &aflock[1], ulTimeout, fAccess);
            FS_RESTORE();
        }
    }

    LIBCLOG_MIX0_RETURN_INT(rc);
}

