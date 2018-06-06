/* $Id: errno.h 1454 2004-09-04 06:22:38Z bird $ */
/** @file
 *
 * Errno helpers.
 *
 * Copyright (c) 2004 knut st. osmundsen <bird@innotek.de>
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with This program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#ifndef __InnoTekLIBC_errno_h__
#define __InnoTekLIBC_errno_h__


#include <sys/cdefs.h>

__BEGIN_DECLS
int __libc_native2errno(unsigned long rc);
__END_DECLS


#endif
