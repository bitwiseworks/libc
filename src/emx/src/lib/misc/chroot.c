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
 * On OS/2 chroot() is used to create the Unix apartment. We don't use the term
 * jail or prison since the prison guards have been on vacation since the late
 * '80 and there are locks in the doors (metaphorically speaking, of course).
 *
 * The Unix apartment is entered by the "/". It can be left by any driveletter.
 * The Unix apartment is inherited by child processes.
 *
 * After a successful creation, the Unix apartment can be destroyed (and left)
 * by passing NULL in pszNewRoot, restoring the process to a state it had before
 * the first chroot() call. Note that this is an OS/2-specific extension and it
 * is not portable (on Unix systems, doing so will normally result in EFAULT).
 *
 * Another OS/2 extension allows to set up a permanent pseudo-Unix apartment for
 * process by setting the UNIXROOT environment variable to a full directory path
 * including the drive letter before starting it up. This directory will serve
 * as virtual "/@unixroot" tree in all path-related operations, acting as a
 * built-in path rewrite rule. This pseudo-apartment cannot be entered or left:
 * "/" always keeps the native OS/2 meaning (the root of the current drive), and
 * getcwd() always returns full native paths. However, if UNIXROOT_CHROOTED is
 * also set at start-up, the pseudo-apartment is upgraded to the normal Unix
 * apartment as if chroot(getenv(UNIXROOT)) were called.
 *
 * In pseudo Unix-apartment mode, calling chroot() will replace the pseudo
 * Unix-apartment with the normal one that will act as described above. But if
 * pszNewRoot resolves to UNIXROOT, the call is equivalent to chroot(NULL) and
 * will restore the process state as described above. This special handling
 * allows to cancel the chroot() effect the Unix way via chroot("."), provided
 * that there was a chdir("/@unixroot") call (similar to chdir("/") on Unix)
 * before the first chroot().
 *
 * Note that in normal Unix apartment mode, the virtual "/@unixroot" tree is
 * always mapped to "/", regardless of UNIXROOT or UNIXROOT_CHROOTED presence.
 *
 * @returns 0 on success.
 * @returns -1 and errno on failure.
 * @param   pszNewRoot  Pointer to the new unix root directory or NULL
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
