/* $Id: $ *//* $Id: $ */
/** @file
 *
 * LIBC SYS Backend - umask.
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

/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include "libc-alias.h"
#include "b_fs.h"
#include <386/builtin.h>
#include <sys/stat.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_BACK_IO
#include <InnoTekLIBC/logstrict.h>


/**
 * Sets the umask to mask & 0777.
 *
 * The umask is used by open(), creat(), mkdir(), mkfifo(),
 * mknod(), mq_open() and sem_open() to set the initial permissions
 * for an newly created object.
 *
 * @returns The old umask. (Will never fail!)
 * @param   mask    The new umask.
 */
mode_t _STD(umask)(mode_t mask)
{
    LIBCLOG_ENTER("mask=0%o\n", mask);
    mask &= 0777;
    mode_t rc = __atomic_xchg((volatile unsigned *)&__libc_gfsUMask, mask); /* (paranoia^2) */
    LIBCLOG_RETURN_MSG(rc, "ret 0%o (%d)\n", rc, rc);
}

