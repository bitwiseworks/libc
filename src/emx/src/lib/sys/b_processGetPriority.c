/* $Id: b_processGetPriority.c 2254 2005-07-17 12:25:44Z bird $ */
/** @file
 *
 * LIBC SYS Backend - getpriority().
 *
 * Copyright (c) 2005 knut st. osmundsen <bird@anduin.net>
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
#include "priority.h"
#include <sys/time.h>
#include <sys/resource.h>
#include <errno.h>
#include <InnoTekLIBC/backend.h>
#include <InnoTekLIBC/sharedpm.h>
#include <InnoTekLIBC/FastInfoBlocks.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_BACK_PROCESS
#include <InnoTekLIBC/logstrict.h>


/*******************************************************************************
*   Internal Functions                                                         *
*******************************************************************************/
static int getPriority(__LIBC_PSPMPROCESS pProcess, void *pvUser);


/**
 * Gets the most favourable priority of a process, group of processes
 * or all processed owned by a user.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   iWhich      PRIO_PROCESS, PRIO_PGRP, or PRIO_USER.
 * @param   idWho       Id of the type specified by iWhich. 0 means the current process/pgrp/user.
 * @param   piPrio      Where to store the priority.
 */
int __libc_Back_processGetPriority(int iWhich, id_t idWho, int *piPrio)
{
    LIBCLOG_ENTER("iWhich=%d idWho=%lld piPrio=%p\n", iWhich, idWho, (void *)piPrio);
    int iNice = PRIO_MAX + 1;
    int rc = 0;
    switch (iWhich)
    {
        /*
         * A specific process.
         */
        case PRIO_PROCESS:
        {
            pid_t pid = (pid_t)idWho;
            if ((id_t)pid == idWho)
            {
                __LIBC_PSPMPROCESS pProcess = __libc_spmQueryProcess(pid);
                if (pProcess)
                {
                    getPriority(pProcess, &iNice);
                    __libc_spmRelease(pProcess);
                }
                else
                    rc = -ESRCH;
            }
            else
                rc = -EINVAL;
            break;
        }

        /*
         * Progress group.
         */
        case PRIO_PGRP:
        {
            pid_t pgrp = (pid_t)idWho;
            if ((id_t)pgrp == idWho)
                rc = __libc_spmEnumProcessesByPGrp(pgrp, getPriority, &iNice);
            else
                rc = -EINVAL;
            break;
        }

        /*
         * All processes owned by a specific user.
         */
        case PRIO_USER:
        {
            uid_t uid = (uid_t)idWho;
            if ((id_t)uid == idWho)
            {
                if (!uid)
                    uid = __libc_spmGetId(__LIBC_SPMID_EUID);
                rc = __libc_spmEnumProcessesByUser(uid, getPriority, &iNice);
            }
            else
                rc = -EINVAL;
            break;
        }

        default:
            rc = -EINVAL;
            break;
    }

    /*
     * Found anything?
     */
    if (rc)
        LIBCLOG_ERROR_RETURN_INT(rc);
    if (iNice > PRIO_MAX)
        LIBCLOG_ERROR_RETURN_INT(-ESRCH);

    /*
     * Done.
     */
    *piPrio = iNice;
    LIBCLOG_RETURN_MSG(0, "ret 0 *piPrio=%d\n", *piPrio);
}


/**
 * Callback function.
 *
 * @returns 0 (never stops the enumeration).
 * @param   pProcess    Current process.
 * @param   pvUser      Pointer to the argument struct.
 */
static int getPriority(__LIBC_PSPMPROCESS pProcess, void *pvUser)
{
    if (__libc_spmCanSee(pProcess) >= 0)
    {
        int *piNice = (int *)pvUser;
        int iNice = pProcess->iNice;
        int iNiceCur;

        /* Update current process if default nice. */
        if (    pProcess->pid == fibGetPid()
            &&  (   iNice > (iNiceCur = __libc_back_priorityUnixFromOS2(__libc_GpFIBLIS->prtyCurrent)) /* highest priority thread */
                 || (iNice != iNiceCur && iNice == 0 && fibGetTid() == 1)                              /* or if possibly uninitialized (only thread 1) */
                )
            )
            pProcess->iNice = iNice = iNiceCur;

        /* min */
        if (iNice < *piNice)
            *piNice = iNice;
    }
    return 0;
}

