/* $Id: b_processCredentials.c 3677 2011-03-14 17:51:00Z bird $ */
/** @file
 *
 * LIBC SYS Backend - [sg]et.*[ug]id().
 *
 * Copyright (c) 2004 knut st. osmundsen <bird-srcspam@anduin.net>
 *
 *
 * This file is part of InnoTek LIBC.
 *
 * InnoTek LIBC is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * InnoTek LIBC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with InnoTek LIBC; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */



/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include "libc-alias.h"
#include "syscalls.h"
#include <errno.h>
#include <InnoTekLIBC/sharedpm.h>
#include <InnoTekLIBC/backend.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_BACK_PROCESS
#include <InnoTekLIBC/logstrict.h>


/**
 * Gets the real user id of the current process.
 * @returns Real user id.
 */
uid_t __libc_Back_processGetUid(void)
{
    LIBCLOG_ENTER("\n");
    uid_t uid = __libc_spmGetId(__LIBC_SPMID_UID);
    LIBCLOG_RETURN_INT(uid);
}


/**
 * Gets the effective user id of the current process.
 * @returns Effective user id.
 */
uid_t __libc_Back_processGetEffUid(void)
{
    LIBCLOG_ENTER("\n");
    uid_t uid = __libc_spmGetId(__LIBC_SPMID_EUID);
    LIBCLOG_RETURN_INT(uid);
}


/**
 * Sets the effective user id of the current process.
 * If the caller is superuser real and saved user id are also set.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno) on failure.
 * @param   uid         New effective user id.
 *                      For superusers this is also the new real and saved user id.
 */
int __libc_Back_processSetUid(uid_t uid)
{
    LIBCLOG_ENTER("uid=%d (%#x)\n", uid, uid);
    int rc = __libc_spmSetUid(uid);
    if (!rc)
        LIBCLOG_RETURN_INT(rc);
    LIBCLOG_ERROR_RETURN_INT(rc);
}

/**
 * Sets the real, effective and saved user ids of the current process.
 * Unprivilegde users can only set them to the real user id, the
 * effective user id or the saved user id.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno) on failure.
 * @param   ruid    New real user id. Ignore if -1.
 * @param   euid    New effective user id. Ignore if -1.
 * @param   svuid   New Saved user id. Ignore if -1.
 */
int __libc_Back_processSetUidAll(uid_t ruid, uid_t euid, uid_t svuid)
{
    LIBCLOG_ENTER("ruid=%d (%#x) euid=%d (%#x) svuid=%d (%#x)\n", ruid, ruid, euid, euid, svuid, svuid);
    int rc = __libc_spmSetUidAll(ruid, euid, svuid);
    if (!rc)
        LIBCLOG_RETURN_INT(rc);
    LIBCLOG_ERROR_RETURN_INT(rc);
}


/**
 * Gets the real group id of the current process.
 * @returns Real group id.
 */
gid_t __libc_Back_processGetGid(void)
{
    LIBCLOG_ENTER("\n");
    gid_t gid = __libc_spmGetId(__LIBC_SPMID_GID);
    LIBCLOG_RETURN_INT(gid);
}


/**
 * Gets the effective group id of the current process.
 * @returns Effective group id.
 */
gid_t __libc_Back_processGetEffGid(void)
{
    LIBCLOG_ENTER("\n");
    gid_t gid = __libc_spmGetId(__LIBC_SPMID_EGID);
    LIBCLOG_RETURN_INT(gid);
}


/**
 * Sets the effective group id of the current process.
 * If the caller is superuser real and saved group id are also set.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno) on failure.
 */
int __libc_Back_processSetGid(gid_t gid)
{
    LIBCLOG_ENTER("gid=%d (%#x)\n", gid, gid);
    int rc = __libc_spmSetGid(gid);
    if (!rc)
        LIBCLOG_RETURN_INT(rc);
    LIBCLOG_ERROR_RETURN_INT(rc);
}

/**
 * Sets the real, effective and saved group ids of the current process.
 * Unprivilegde users can only set them to the real group id, the
 * effective group id or the saved group id.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno) on failure.
 * @param   rgid    New real group id. Ignore if -1.
 * @param   egid    New effective group id. Ignore if -1.
 * @param   svgid   New Saved group id. Ignore if -1.
 */
int __libc_Back_processSetGidAll(gid_t rgid, gid_t egid, gid_t svgid)
{
    LIBCLOG_ENTER("rgid=%d (%#x) egid=%d (%#x) svgid=%d (%#x)\n", rgid, rgid, egid, egid, svgid, svgid);
    int rc = __libc_spmSetGidAll(rgid, egid, svgid);
    if (!rc)
        LIBCLOG_RETURN_INT(rc);
    LIBCLOG_ERROR_RETURN_INT(rc);
}


/**
 * Gets the session id of the current process.
 * @returns Session id.
 * @returns Negated errno on failure.
 * @param   pid     Process to get the process group for.
 *                  Use 0 for the current process.
 */
pid_t __libc_Back_processGetSid(pid_t pid)
{
    LIBCLOG_ENTER("\n");
    pid_t sid;
    if (pid == 0 || pid == _sys_pid)
        sid = __libc_spmGetId(__LIBC_SPMID_SID);
    else
        sid = -EACCES;
    if (sid >= 0)
        LIBCLOG_RETURN_INT(sid);
    LIBCLOG_ERROR_RETURN_INT(sid);
}


/**
 * Gets the process group of the specfied process.
 * @returns Process group.
 * @returns Negated errno on failure.
 * @param   pid     Process to get the process group for.
 *                  Use 0 for the current process.
 */
pid_t __libc_Back_processGetPGrp(pid_t pid)
{
    LIBCLOG_ENTER("pid=%d\n", pid);
    pid_t pgrp;
    if (pid == 0 || pid == _sys_pid)
        pgrp = __libc_spmGetId(__LIBC_SPMID_PGRP);
    else
        pgrp = -EACCES;
    if (pgrp >= 0)
        LIBCLOG_RETURN_INT(pgrp);
    LIBCLOG_ERROR_RETURN_INT(pgrp);
}

