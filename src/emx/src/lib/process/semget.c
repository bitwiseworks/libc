/* $Id: semget.c 2254 2005-07-17 12:25:44Z bird $ */
/** @file
 *
 * LIBC - semget.
 *
 * Copyright (c) 2005 knut st. osmundsen <bird@anduin.net>
 *
 *
 * This file is part of kBuild.
 *
 * kBuild is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * kBuild is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with kBuild; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */


/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include "libc-alias.h"
#include <sys/types.h>
#include <sys/sem.h>
#include <errno.h>
#include <InnoTekLIBC/backend.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_PROCESS
#include <InnoTekLIBC/logstrict.h>


/**
 * sysget.
 */
int _STD(semget)(key_t key, int nsems, int semflg)
{
    LIBCLOG_ENTER("key=%#lx nsems=%d semflg=%#x\n", key, nsems, semflg);
    int rc = __libc_Back_sysvSemGet(key, nsems, semflg);
    if (rc >= 0)
        LIBCLOG_RETURN_INT(rc);
    errno = -rc;
    LIBCLOG_ERROR_RETURN_INT(-1);
}

