/* $Id: b_ioPathConf.c 3695 2011-03-15 23:30:51Z bird $ */
/** @file
 *
 * kNIX - fpathconf.
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

#include "syscalls.h"
#include <InnoTekLIBC/backend.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_BACK_FS
#include <InnoTekLIBC/logstrict.h>
#include "b_fs.h"

/**
 * Query filesystem configuration information by file handle.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   fh          The handle to query config info about.
 * @param   iName       Which path config variable to query.
 * @param   plValue     Where to return the configuration value.
 * @sa      __libc_Back_fsPathConf, fpathconf, pathconf, sysconf.
 */
int __libc_Back_ioPathConf(int fh, int iName, long *plValue)
{
    LIBCLOG_ENTER("fh=%d iName=%d pvValue=%p\n", fh, iName, plValue);

    /*
     * Resolve the file handle and let the FS code we have in common with
     * __libc_back_fsNativePathConf do the job.
     */
    __LIBC_PFH pFH;
    int rc = __libc_FHEx(fh, &pFH);
    if (rc == 0)
        rc = __libc_back_fsInfoPathConf(pFH->pFsInfo, iName, plValue);
    LIBCLOG_MIX_RETURN_INT(rc);
}

