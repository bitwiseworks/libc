/* $Id: sysctl_os2.c 2064 2005-06-23 06:02:57Z bird $ */
/** @file
 *
 * LIBC - OS/2 specific sysctl stuff.
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

#define INCL_BASE
#define INCL_ERRORS
#define INCL_FSMACROS
#include <os2emx.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <InnoTekLIBC/FastInfoBlocks.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_MISC
#include <InnoTekLIBC/logstrict.h>
#define _KERNEL
#include <sys/sysctl.h>

/* This is the only symbol we actually export, we exploit this in sysctl_mib to drag
 * this module into the link. (bad hack!) */
SYSCTL_NODE(, CTL_OS2, os2, CTLFLAG_RW, 0, "os2");


/**
 * System bootup time.
 */
static int sysctl_kern_boottime(SYSCTL_HANDLER_ARGS)
{
    static struct timeval tv;                  /* The boot time, only calc this once! */
    if (!tv.tv_sec)
    {
        /*
         * Use the highres timer to estimate the wrap-arounds.
         * Then use that with the millisecond timer to get a
         * stable number.
         */
        unsigned cWraps = 0;
        union
        {
            QWORD  qw;
            unsigned long long ull;
        } u;
        ULONG ul;
        FS_VAR_SAVE_LOAD();
        if (   !DosTmrQueryFreq(&ul)
            && !DosTmrQueryTime(&u.qw))
        {
            long double lrd = u.ull;
            lrd *= 1000;
            lrd /= ul;
            unsigned long long ull = 0x100000000ULL;
            cWraps = lrd / ull;
        }
        FS_RESTORE();

        gettimeofday(&tv, NULL);
        gettimeofday(&tv, NULL);
        ul = fibGetMsCount();
        unsigned cSecs = (ul % 1000) * 1000;
        tv.tv_usec -= cSecs;
        tv.tv_sec -= ul / 1000;
        for (;;)
        {
            if (tv.tv_usec < 0)
            {
                tv.tv_sec--;
                tv.tv_usec += 1000000;
            }
            if (cWraps-- <= 0)
                break;
            tv.tv_sec  -= 4294967;
            tv.tv_usec -= 296000;
        }
        LIBC_ASSERT(tv.tv_sec > 0);
        LIBC_ASSERT(tv.tv_usec >= 0 && tv.tv_usec < 1000000);
    }

    return (sysctl_handle_opaque(oidp, &tv, sizeof(tv), req));
}

SYSCTL_PROC(_kern, KERN_BOOTTIME, boottime, CTLFLAG_RD,
	0, 0, sysctl_kern_boottime, "S,timeval", "System boottime");

/**
 * System bootup time.
 */
static int sysctl_kern_bootfile(SYSCTL_HANDLER_ARGS)
{
    static char szBootFile[] = "?:/OS2KRNL";
    if (szBootFile[0] == '?')
        szBootFile[0] = __libc_GpFIBGIS->SIS_BootDrv + 'A' - 1;

    return (sysctl_handle_string(oidp, szBootFile, sizeof(szBootFile), req));
}

SYSCTL_PROC(_kern, KERN_BOOTFILE, bootfile, CTLFLAG_RD,
	0, 0, sysctl_kern_bootfile, "A", "Kernel file");


/*
 * HW
 */

SYSCTL_STRING(_hw, HW_MACHINE, machine, CTLFLAG_RD,
	"i386", sizeof("i386"), "Machine");

/**
 * HW_MODEL
 */
static int sysctl_hw_model(SYSCTL_HANDLER_ARGS)
{
    static union
    {
        char        szModel[17];
        uint32_t    uRegs[4];
    } u;
    if (!u.szModel[0])
    {
        int i;
        __asm__ ("cpuid\n"
                 : "=a" (i),
                   "=b" (u.uRegs[0]),
                   "=c" (u.uRegs[2]),
                   "=d" (u.uRegs[1])
                 : "a" (0)
                 );
    }

    return (sysctl_handle_string(oidp, u.szModel, sizeof(u.szModel), req));
}

SYSCTL_PROC(_hw, HW_MODEL, hw_model, CTLFLAG_RD,
	0, 0, sysctl_hw_model, "A", "Process model");

/**
 * Number of CPUs.
 */
static int sysctl_hw_ncpus(SYSCTL_HANDLER_ARGS)
{
    static u_int cCpus = 0;
    if (!cCpus)
        cCpus = sysconf(_SC_NPROCESSORS_ONLN);

    return (sysctl_handle_int(oidp, &cCpus, 0, req));
}

SYSCTL_PROC(_hw, HW_NCPU, ncpus, CTLFLAG_RD,
	0, 0, sysctl_hw_ncpus, "UI", "number of cpus");


SYSCTL_STRING(_hw, HW_MACHINE_ARCH, machine_arch, CTLFLAG_RD,
	"i386", sizeof("i386"), "Machine arch");

