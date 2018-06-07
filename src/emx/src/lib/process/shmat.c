/* $Id: shmat.c 2432 2005-11-13 02:41:06Z bird $ */
/** @file
 *
 * LIBC - shmat.
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
#include <sys/shm.h>
#include <stdarg.h>
#include <errno.h>
#include <InnoTekLIBC/backend.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_PROCESS
#include <InnoTekLIBC/logstrict.h>


/**
 * shmat.
 */
void *_STD(shmat)(int shmid, const void *shmaddr, int shmflg)
{
    LIBCLOG_ENTER("shmid=%#x shmaddr=%p shmflg=%#x\n", shmid, shmaddr, shmflg);
    void *pvActual = NULL;
    int rc = __libc_Back_sysvShmAt(shmid, shmaddr, shmflg, &pvActual);
    if (rc >= 0)
        LIBCLOG_RETURN_P(pvActual);
    errno = -rc;
    LIBCLOG_ERROR_RETURN_P((void *)-1);
}

