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

/** Touch each page in a range of addresses.
 *
 * This is required because DosRead and other OS/2 APIs seem not to be reentrant
 * enough -- if it's used to read data into a page to be loaded from the .EXE
 * file (dumped heap), it's recursively called by the guard pageexception
 * handler. This call seems to disturb the first call, which will return a
 * strange `error code' (ESP plus some constant).
 *
 * @param base Start address.
 * @param count Number of bytes.
 */
extern void __libc_touch(void *base, unsigned long count);

/**
 * Raise an EXCEPTQ debug exception to generate a debug report and continue execution.
 *
 * The debug report may contain an optional message specified in pszFormat with a reduced set of
 * printf-like format specifiers followed by up to 3 format arguments (the rest of arguments is
 * ignored). If pszFormat is NULL and there are no additional arguments, `<no message>` is used. If
 * pszFormat is a pointer that equals to 1, 2 or 3 (should be casted to const char* to avoid
 * warnings), the specified number of arguments following it is printed as 32-bit hex integers.
 * Alternatively, the __libc_debug_report_n macro may be used in this case to avoid casting the
 * number of arguments passed in pszFormat to const char*.
 *  *
 * @param   pszFormat   User message which may contain %s and %x or NULL.
 * @param   ...         String pointers and unsigned intergers as specified by the %s and %x in pszFormat.
 */
extern void __libc_debug_report(const char *pszFormat, ...);

/**
 * A convenience version of __libc_debug_report that casts nArgs to const char * and uses it as
 * pszFormat (note that nArgs must be <= 3, nArgs = 0 is equivalent to pszFormat = NULL).
 */
#define __libc_debug_report_n(nArgs, ...) __libc_debug_report((const char *)nArgs, __VA_ARGS__)

__END_DECLS

#endif
