/* $Id: libc.h 1618 2004-11-07 14:19:42Z bird $ */
/** @file
 *
 * Global LIBC stuff.
 *
 * Copyright (c) 2004 knut st. osmundsen <bird-srcspam@anduin.net>
 *
 *
 * This file is part of InnoTek LIBC.
 *
 * InnoTek LIBC is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * InnoTek LIBC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with InnoTek LIBC; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef __InnoTekLIBC_libc_h__
#define __InnoTekLIBC_libc_h__

#include <sys/cdefs.h>
#include <sys/signal.h>

__BEGIN_DECLS

/** Disable (some) unix imitation features.
 *
 * If non-zero, '\\' is prefered over '/', and a bunch of other unix
 * imitations are dropped.
 *
 * Initiated by __init_dll().
 */
extern int __libc_gfNoUnix;

/** Signal set of the signals which will interrupt system call execution.
 * By default all signals will interrupt syscall execution, since OS/2 can't really
 * restart system calls easily.
 * Update is protected by the signal semaphore, however read access isn't.
 */
extern sigset_t __libc_gSignalRestartMask;

__END_DECLS

#endif
