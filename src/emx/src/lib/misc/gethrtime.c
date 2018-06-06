/* $Id: gethrtime.c 1902 2005-04-24 09:55:59Z bird $ */
/** @file
 *
 * LIBC - gethrtime().
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
#include <sys/time.h>
#include <InnoTekLIBC/backend.h>

/**
 * Get's get current high-resolution timer value as nanoseconds.
 *
 * @returns nanosecond timestamp.
 *
 * @remark SUN/HP/RTLinux extension, seems useful.
 */
hrtime_t _STD(gethrtime)(void)
{
    return __libc_Back_timeHighResNano();
}

