/* $Id: $ */
/** @file
 *
 * kUNIX - Termination of a DLL.
 *
 * Copyright (c) 2006 knut st. osmundsen <bird@innotek.de>
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


/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include <InnoTekLIBC/backend.h>
#include <InnoTekLIBC/atexit.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_INITTERM
#include <InnoTekLIBC/logstrict.h>



/**
 * Called by dll0.s when a dll is being unloaded.
 *
 * @param   fFlags  Bit 0: If set the application is open to put the default heap in high memory.
 *                         If clear the application veto against putting the default heap in high memory.
 *                  Bit 1: If set some of the unixness of LIBC is disabled.
 *                         If clear all unix like features are enabled.
 *                  Bit 2: If set the dll is registered for forking.
 *                         If clear the dll isn't registered for forking.
 *
 * @param   hmod    The module handle.
 */
/** @todo 0.7: #defines for the flags! klibc/startup.h? */
void __libc_Back_termDll(unsigned fFlags, uintptr_t hmod)
{
    LIBCLOG_ENTER("fFlag=%#x hmod=%x\n", fFlags, hmod);
    __libc_atexit_unload(hmod);
    LIBCLOG_RETURN_VOID();
}


