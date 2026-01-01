/* $Id: resource.c 1519 2004-09-27 02:15:07Z bird $ */
/** @file
 *
 * This file contains the implementation of the functions in the
 * <sys/resource.h> ( $FreeBSD: src/sys/sys/resource.h,v 1.22 2004/04/07 04:19:49 imp Exp $ )
 *
 * Copyright (c) 2004 Bart van Leeuwen <bart at netlabs dot org>
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
#define INCL_FSMACROS
#define INCL_DOSFILEMGR
#define INCL_DOSPROCESS
#define INCL_DOSMISC
#define INCL_DOSERRORS
#include <os2.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/resource.h>
#include "syscalls.h"


/* getrlimit: Get resource limits *
 * for documentation see:
 * http://www.opengroup.org/onlinepubs/009695399/functions/getrlimit.html
 */

int	_STD(getrlimit)(int iResId, struct rlimit * pLimit)
{
    int rcRet = 0;

    switch (iResId)
    {
        /*
         * Max size of JFS is 2TB.
         */
        case RLIMIT_FSIZE:
            pLimit->rlim_cur = (1ULL << 41);
            pLimit->rlim_max = (1ULL << 41);
            break;

        /*
         * LIBC doesn't write cores.
         */
        case RLIMIT_CORE:
            pLimit->rlim_cur = 0;
            pLimit->rlim_max = 0;
            break;

        /*
         * Query max filehandles.
         */
        case RLIMIT_NOFILE:
        {
            ULONG     ulCurMax = 0;
            LONG      lReqCount = 0;
            APIRET    rc;
            FS_VAR();

            FS_SAVE_LOAD();
            rc = DosSetRelMaxFH(&lReqCount, &ulCurMax);
            FS_RESTORE();
            if (rc == NO_ERROR)
            {
                if (ulCurMax < 10000)
                    ulCurMax = 10000;
                pLimit->rlim_cur = ulCurMax;
                pLimit->rlim_max = ulCurMax;
            }
            else
            {
                _sys_set_errno(rc);
                rcRet = -1;
            }
            break;
        }

        case RLIMIT_CPU:
        case RLIMIT_SBSIZE:
        case RLIMIT_MEMLOCK:
            errno = ENOSYS;
            rcRet = -1;
            break;

        /*
         * Limits of the current stack.
         */
        case RLIMIT_STACK:
        {
            PTIB pTib;
            PPIB pPib;
            DosGetInfoBlocks(&pTib, &pPib);
            pLimit->rlim_cur = (uintptr_t)pTib->tib_pstacklimit - (uintptr_t)pTib->tib_pstack;
            pLimit->rlim_max = (uintptr_t)pTib->tib_pstacklimit - (uintptr_t)pTib->tib_pstack;
            break;
        }

        /*
         * Limit of the data segment.
         * In OS/2 limit of private memory.
         */
        case RLIMIT_DATA:
        {
            ULONG   cbPrivateLow;
            ULONG   cbPrivateHigh;
            if (DosQuerySysInfo(QSV_MAXPRMEM, QSV_MAXPRMEM, &cbPrivateLow, sizeof(cbPrivateLow)))
                cbPrivateLow = 64*1024*1024; /* 64MB is always secured by OS/2. */
            if (DosQuerySysInfo(QSV_MAXHPRMEM, QSV_MAXHPRMEM, &cbPrivateHigh, sizeof(cbPrivateHigh)))
                cbPrivateHigh = 0;
            pLimit->rlim_cur = cbPrivateLow + cbPrivateHigh;
            pLimit->rlim_max = pLimit->rlim_cur;
            break;
        }

        /*
         * Maximum number of processes.
         */
        case RLIMIT_NPROC:
        {
            ULONG   cMaxProcesses;
            if (DosQuerySysInfo(QSV_MAXPROCESSES, QSV_MAXPROCESSES, &cMaxProcesses, sizeof(cMaxProcesses)))
                cMaxProcesses = 256;
            pLimit->rlim_cur = cMaxProcesses;
            pLimit->rlim_max = cMaxProcesses;
            break;
        }

        /*
         * Virtual process size.
         */
        case RLIMIT_VMEM:
        {
            ULONG   cMBUserVirtualMem = 0;
            if (DosQuerySysInfo(QSV_VIRTUALADDRESSLIMIT, QSV_VIRTUALADDRESSLIMIT, &cMBUserVirtualMem, sizeof(cMBUserVirtualMem)))
                cMBUserVirtualMem = 512;
            pLimit->rlim_cur = cMBUserVirtualMem * (1 << 20);
            pLimit->rlim_max = pLimit->rlim_cur;
            break;
        }

        default:
            errno = EINVAL;
            rcRet = -1;
            break;
    };

    return rcRet;
}

int	_STD(setrlimit)(int iResId, const struct rlimit *pLimit)
{
    errno = ENOSYS;
    return -1;
}

/*
 * Taken from https://github.com/komh/os2compat/blob/master/process/getrusage.c.
 * Uses times API, so derives its limitations.
 */
int	_STD(getrusage)(int who, struct rusage *usage)
{
    struct tms time;

    if (who != RUSAGE_SELF && who != RUSAGE_CHILDREN)
    {
        errno = EINVAL;
        return -1;
    }

    /* Intialize members of struct rusage */
    memset(usage, 0, sizeof(*usage));

    if (times(&time) != (clock_t)-1)
    {
        clock_t u;
        clock_t s;

        if (who == RUSAGE_CHILDREN)
        {
            u = time.tms_cutime;
            s = time.tms_cstime;
        }
        else
        {
            u = time.tms_utime;
            s = time.tms_stime;
        }

        usage->ru_utime.tv_sec = u / CLK_TCK;
        usage->ru_utime.tv_usec = ( u % CLK_TCK ) * 1000000U / CLK_TCK;
        usage->ru_stime.tv_sec = s / CLK_TCK;
        usage->ru_stime.tv_usec = (s % CLK_TCK ) * 1000000U / CLK_TCK;
    }

    return 0;
}
