/* $Id: b_processSetPriority.c 2254 2005-07-17 12:25:44Z bird $ */
/** @file
 *
 * LIBC SYS Backend - setpriority().
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
#include "backend.h"
#include <sys/time.h>
#include <sys/resource.h>
#include <errno.h>
#include <InnoTekLIBC/backend.h>
#include <InnoTekLIBC/sharedpm.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_BACK_PROCESS
#include <InnoTekLIBC/logstrict.h>

#define INCL_DOSPROCESS
#define INCL_FSMACROS
#define INCL_ERRORS
#include <os2emx.h>


/*******************************************************************************
*   Structures and Typedefs                                                    *
*******************************************************************************/
/**
 * Arguments to the callback.
 */
typedef struct SETPRIORITYARGS
{
    unsigned    uClass;
    unsigned    uLevel;
    int         iNice;
    int         rc;
} SETPRIORITYARGS, *PSETPRIORITYARGS;


/*******************************************************************************
*   Internal Functions                                                         *
*******************************************************************************/
static int setPriority(__LIBC_PSPMPROCESS pProcess, void *pvUser);


/**
 * Sets the priority of a process, a group of processes
 * or all processed owned by a user.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   iWhich      PRIO_PROCESS, PRIO_PGRP, or PRIO_USER.
 * @param   idWho       Id of the type specified by iWhich. 0 means the current process/pgrp/user.
 * @param   iPrio       The new priority.
 */
int __libc_Back_processSetPriority(int iWhich, id_t idWho, int iPrio)
{
    LIBCLOG_ENTER("iWhich=%d idWho=%lld iPrio=%d\n", iWhich, idWho, iPrio);

    /*
     * Adjust the priority.
     */
    if (iPrio > PRIO_MAX)
        iPrio = PRIO_MAX;
    if (iPrio < PRIO_MIN)
        iPrio = PRIO_MIN;
    unsigned uPriority = __libc_back_priorityOS2FromUnix(iPrio);

    /*
     * Enumerate the processes and apply the new priority to them.
     */
    FS_VAR_SAVE_LOAD();
    SETPRIORITYARGS Args;
    Args.uClass = uPriority >> 8;
    Args.uLevel = uPriority & 0xff;
    Args.iNice  = iPrio;
    Args.rc = 0;
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
                    setPriority(pProcess, &Args);
                    __libc_spmRelease(pProcess);
                }
                else
                {
                    rc = DosSetPriority(PRTYS_PROCESS, Args.uClass, Args.uLevel, pid);
                    if (rc)
                        rc = __libc_native2errno(rc);
                }
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
                rc = __libc_spmEnumProcessesByPGrp(pgrp, setPriority, &Args);
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
                rc = __libc_spmEnumProcessesByUser(uid, setPriority, &Args);
            }
            else
                rc = -EINVAL;
            break;
        }

        default:
            rc = -EINVAL;
            break;
    }
    FS_RESTORE();

    /*
     * Process error codes.
     */
    if (!rc)
        rc = Args.rc;

    if (!rc)
        LIBCLOG_RETURN_INT(rc);
    LIBCLOG_ERROR_RETURN_INT(rc);
}


/**
 * Callback function.
 *
 * @returns 0 (never stops the enumeration).
 * @param   pProcess    Current process.
 * @param   pvUser      Pointer to the argument struct.
 */
static int setPriority(__LIBC_PSPMPROCESS pProcess, void *pvUser)
{
    if (__libc_spmCanSee(pProcess) >= 0)
    {
        int rc;
        PSETPRIORITYARGS pArgs = (PSETPRIORITYARGS)pvUser;
        if (    pArgs->iNice >= pProcess->iNice
            ||  __libc_spmIsSuperUser() >= 0)
        {
            if (__libc_spmCanModify(pProcess) >= 0)
            {
                rc = DosSetPriority(PRTYS_PROCESS, pArgs->uClass, pArgs->uLevel, pProcess->pid);
                if (!rc)
                {
                    LIBCLOG_MSG2("Changed priority of process %d from %d to %d\n", pProcess->pid, pProcess->iNice, pArgs->iNice);
                    pProcess->iNice = pArgs->iNice;
                    return 0;
                }

                if (rc == ERROR_NOT_DESCENDANT)
                    rc = -EPERM;
                else
                    rc = -__libc_native2errno(rc);
            }
            else
                rc = -EPERM;
        }
        else
            rc = -EACCES;
        if (!pArgs->rc)
            pArgs->rc = rc;
        LIBCLOG_MSG2("Failed to changed priority of process %d, rc=%d\n", pProcess->pid, rc);
    }

    return 0;
}

