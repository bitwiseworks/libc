/* $Id: b_fsInfoPathConf.c 3695 2011-03-15 23:30:51Z bird $ */
/** @file
 *
 * kNIX - fpathconf and pathconf.
 *
 * Copyright (c) 2011 knut st. osmundsen <bird-src-spam@anduin.net>
 *
 *
 * This file is part of kLIBC.
 *
 * kLIBC is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * kLIBC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with kLIBC; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include "syscalls.h"
#include "b_fs.h"
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_BACK_FS
#include <InnoTekLIBC/logstrict.h>

/**
 * Filesystem info to pathconf/fpathconf.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   pFsInfo     The file system info object. Can be NULL.
 * @param   iName       Which path config variable to query.
 * @param   plValue     Where to return the configuration value.
 */
int __libc_back_fsInfoPathConf(__LIBC_PFSINFO pFsInfo, int iName, long *plValue)
{
    LIBCLOG_ENTER("pFsInfo=%p iName=%d\n", pFsInfo, iName);

    switch (iName)
    {
        case _PC_FILESIZEBITS:
            *plValue = pFsInfo ? pFsInfo->cFileSizeBits   : 32;
            break;
        case _PC_LINK_MAX:
            *plValue = pFsInfo ? pFsInfo->cMaxLinks       : 1;
            break;
        case _PC_MAX_CANON:
            *plValue = pFsInfo ? pFsInfo->cchMaxTermCanon : MAX_CANON;
            break;
        case _PC_MAX_INPUT:
            *plValue = pFsInfo ? pFsInfo->cchMaxTermInput : MAX_INPUT;
            break;
        case _PC_NAME_MAX:
            *plValue = pFsInfo ? pFsInfo->cchMaxName      : _POSIX_NAME_MAX;
            break;
        case _PC_PATH_MAX:
            *plValue = pFsInfo ? pFsInfo->cchMaxPath      : _POSIX_NAME_MAX;
            break;
        case _PC_PIPE_BUF:
            *plValue = pFsInfo ? pFsInfo->cbPipeBuf       : _POSIX_PIPE_BUF;
            break;
        case _PC_ALLOC_SIZE_MIN:
            *plValue = pFsInfo ? pFsInfo->cbBlock         : 512;
            break;
        case _PC_REC_INCR_XFER_SIZE:
            *plValue = pFsInfo ? pFsInfo->cbXferIncr      : 512;
            break;
        case _PC_REC_MAX_XFER_SIZE:
            *plValue = pFsInfo ? pFsInfo->cbXferMax       : (LONG_MAX / 512 * 512);
            break;
        case _PC_REC_MIN_XFER_SIZE:
            *plValue = pFsInfo ? pFsInfo->cbXferMin       : 512;
            break;
        case _PC_REC_XFER_ALIGN:
            *plValue = pFsInfo ? pFsInfo->uXferAlign      : 4096;
            break;
        case _PC_SYMLINK_MAX:
            *plValue = pFsInfo ? pFsInfo->cchMaxSymlink   : _POSIX_SYMLINK_MAX;
            break;
        case _PC_CHOWN_RESTRICTED:
            *plValue = pFsInfo ? pFsInfo->fChOwnRestricted: 1;
            break;
        case _PC_NO_TRUNC:
            *plValue = pFsInfo ? pFsInfo->fNoNameTrunc    : 1;
            break;
        case _PC_VDISABLE:
            *plValue = _POSIX_VDISABLE;
            break;
        case _PC_CAP_PRESENT:
            *plValue = 0;
            break;
        case _PC_INF_PRESENT:
            *plValue = 0;
            break;
        case _PC_MAC_PRESENT:
            *plValue = 0;
            break;

        /* not implemented */
        case _PC_ACL_EXTENDED:
        case _PC_ACL_PATH_MAX:
        case _PC_ASYNC_IO:
        case _PC_PRIO_IO:
        case _PC_SYNC_IO:
        default:
            LIBCLOG_ERROR_RETURN_INT(-EINVAL);
    }

    LIBCLOG_RETURN_INT(0);
}


