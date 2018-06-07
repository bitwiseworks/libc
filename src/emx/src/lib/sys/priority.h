/* $Id: priority.h 1910 2005-04-25 03:58:57Z bird $ */
/** @file
 *
 * LIBC SYS Backend - Priority Conversion.
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

#ifndef __priority_h__
#define __priority_h__

#include <sys/cdefs.h>

__BEGIN_DECLS

/**
 * Converts from OS/2 to Unix priority.
 * @returns Unix priority.
 * @param   iPrio       The OS/2 priority. (High byte is class, low byte is level.)
 */
int __libc_back_priorityUnixFromOS2(int iPrio);

/**
 * Converts from Unix to OS/2 priority.
 * @returns OS/2 priority. (High byte is class, low byte is level.)
 * @param   iNice       The Unix priority.
 */
int __libc_back_priorityOS2FromUnix(int iNice);

__END_DECLS

#endif
