/* $Id: b_threadInit.c 1661 2004-11-22 01:46:26Z bird $ */
/** @file
 *
 * LIBC SYS Backend - thread structure init.
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
#define INCL_DOSEXCEPTIONS
#define INCL_FSMACROS
#include <os2emx.h>
#include <sys/signal.h>
#include <emx/syscalls.h>
#include "syscalls.h"
#include "b_signal.h"
#include <InnoTekLIBC/thread.h>
#include <InnoTekLIBC/backend.h>


void __libc_Back_threadInit(__LIBC_PTHREAD pThrd, const __LIBC_PTHREAD pParentThrd)
{
    pThrd->b.sys.fd.hdir = HDIR_CREATE;
    pThrd->b.sys.fd.cFiles = 0;

    if (pParentThrd && pParentThrd->tid)
    {
        /*
         * We don't mess with the signal semaphore here because that *will*
         * cause deadlocks. So to protect ourselves against signals
         * we'll simply enter a must complete section while accessing the
         * variables. This naturally assumes that WE are the parent!
         */
        FS_VAR_SAVE_LOAD();
        ULONG ulNesting = 0;
        DosEnterMustComplete(&ulNesting);

        /*
         * Copy signal stuff.
         */
        pThrd->SigSetBlocked    = pParentThrd->SigSetBlocked;
        if (pParentThrd->fSigSetBlockedOld)
        {
            pThrd->SigSetBlockedOld     = pParentThrd->SigSetBlockedOld;
            pThrd->fSigSetBlockedOld    = pParentThrd->fSigSetBlockedOld;
        }

        /*
         * Exit must complete section.
         */
        DosExitMustComplete(&ulNesting);
        FS_RESTORE();
    }
}

