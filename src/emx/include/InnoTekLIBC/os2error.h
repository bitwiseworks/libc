/* $Id: os2error.h 1505 2004-09-12 19:40:29Z bird $ */
/** @file
 *
 * LIBC DosError() Wrapping.
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

#ifndef __InnoTekLIBC_os2error_h__
#define __InnoTekLIBC_os2error_h__

#include <sys/cdefs.h>

__BEGIN_DECLS

/**
 * Sets the error notifications.
 *
 * @returns See DosError().
 * @param   fError  DosError().
 */
int __libc_OS2ErrorSet(ULONG fError);

/**
 * Pushed (and sets) the error notifications setting.
 *
 * @returns See DosError().
 * @param   pfPushVar   Where to store the current value.
 * @param   fError      DosError().
 */
int __libc_OS2ErrorPush(PULONG pfPushVar, ULONG fError);

/**
 * Pop the error notifications setting.
 *
 * @returns See DosError().
 * @param   fPopVar     The value to pop.
 */
int __libc_OS2ErrorPop(ULONG fPopVar);

__END_DECLS

#endif
