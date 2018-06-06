/* $Id: b_process.h 3373 2007-05-27 11:03:27Z bird $ */
/** @file
 *
 * LIBC Backend - Process Internals.
 *
 * Copyright (c) 2004-2007 knut st. osmundsen <bird-srcspam@anduin.net>
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

#ifndef __b_process_h__
#define __b_process_h__

#include <sys/cdefs.h>
#include <signal.h>
#include <InnoTekLIBC/libc.h>
#include <InnoTekLIBC/backend.h>


__BEGIN_DECLS

void __libc_back_processWaitNotifyTerm(void);
int  __libc_back_processWaitNotifyAlreadyStarted(void);
void __libc_back_processWaitNotifyExec(pid_t pid);
void __libc_back_processWaitNotifyChild(siginfo_t *pSigInfo);
void __libc_back_processWaitNotifyNoWait(int fNoWaitStatus);

__END_DECLS

#endif
