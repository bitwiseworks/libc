/* $Id: chroot.c 2254 2005-07-17 12:25:44Z bird $ */
/** @file
 *
 * chroot().
 *
 * Copyright (c) 2004 knut st. osmundsen <bird-srcspam@anduin.net>
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
#include <unistd.h>
#include <errno.h>
#include <InnoTekLIBC/backend.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_MISC
#include <InnoTekLIBC/logstrict.h>


/**
 * Changes the root of the current process to pszNewRoot.
 *
 * If cwd is within pszNewRoot at the time of the call getcwd() will return a
 * path to the directory relative to pszNewRoot after the call. If cwd isn't
 * within pszNewRoot, getcwd() will remain unchanged. Change the cwd using a
 * path relative to "/" to make sure the process is inside the new root.
 *
 * On OS/2 chroot() is used to create an unix apartment. We don't use the term
 * jail or prison since the prison guards have been on vacation since the
 * late '80 and there are locks in the doors (metaphorically speaking, of course).
 *
 * The Unix compartment is entered by the "/". It can be left by any driveletter.
 * The Unix compartment is inherited by child processes.
 *
 * @returns 0 on success.
 * @returns -1 and errno on failure.
 * @param   pszNewRoot  Pointer to the new unix root directory
 */
int	 _STD(chroot)(const char *pszNewRoot)
{
    LIBCLOG_ENTER("pszNewRoot=%p:{%s}\n", (void *)pszNewRoot, pszNewRoot);
    int rc = __libc_Back_fsDirChangeRoot(pszNewRoot);
    if (!rc)
        LIBCLOG_RETURN_INT(0);
    errno = -rc;
    LIBCLOG_ERROR_RETURN_INT(-1);
}
