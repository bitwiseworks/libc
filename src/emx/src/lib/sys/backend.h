/* $Id: backend.h 2021 2005-06-13 02:16:10Z bird $ */
/** @file
 *
 * LIBC SYS Backend - internal header.
 *
 * Copyright (c) 2004 knut st. osmundsen <bird@innotek.de>
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

#ifndef __backend_h__
#define __backend_h__

#include <sys/cdefs.h>

__BEGIN_DECLS

/** Wait Event Semaphore.
 * This is an event semaphore which will never be posted (unless we wanna resolve
 * some nasty deadlock in the future) but is used to wait for a signal to arrive.
 * When a signal arrives the wait will be interrupted to allow for execution of
 * the exception and signal processing. DosWaitEventSem will return ERROR_INTERRUPT.
 */
extern unsigned long __libc_back_ghevWait;

/**
 * Converts native error code to errno error code.
 * @returns The errno value corresponding to the native error code.
 * @param   rc      Native error code.
 */
int __libc_native2errno(unsigned long rc);

/**
 * Initializes the various LIBC hooks using the given
 * specification string.
 */
void __libc_back_hooksInit(const char *pszSpec, unsigned long hmod);

/**
 * Internal _abspath entry with optional FS mutex locking.
 */
int __libc_abspath(char *pszDst, const char *pszSrc, int cbDst, int fLock);

__END_DECLS

#endif
