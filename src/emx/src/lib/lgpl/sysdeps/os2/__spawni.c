/* $Id: __spawni.c 1055 2004-01-25 05:34:10Z bird $ */
/** @file
 *
 * System dependent worker for POSIX spawn.
 *
 * Copyright (c) 2004 knut st. osmundsen <bird-srcspam@anduin.net>
 *
 *
 * This file is part of Innotek LIBC.
 *
 * Innotek LIBC is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Innotek LIBC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Innotek LIBC; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */


/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include <stdlib.h>
#include <string.h>
#include <spawn.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <process.h>
#include <emx/io.h>
#include <emx/umalloc.h>
#ifdef DEBUG
#include <assert.h>
#endif
#include "../../posix/spawn_int.h"

/*******************************************************************************
*   Structures and Typedefs                                                    *
*******************************************************************************/
typedef struct RestoreAction
{
    int fdClose;                        /* handle to close before restoring fdRestore. */
    int fdRestore;                      /* fd which fdSaved is a saved version of. */
    int fdSaved;                        /* close-on-exec dup of fdRestore. */
    int fSavedCoE;                      /* the original close-on-exec flag. */
} RESTOREACTION, *PRESTOREACTION;

/**
 * Save the handle fdToSave save it's Close-on-Exec flag in *pfCoE and the
 * duplicate save handle in *pfdSaved.
 *
 * @returns 0 on success.
 * @returns -1 on failure. No handles were duplicated.
 * @param   fdToSave    Handle to save.
 * @param   pfdMin      First usable handle for the duplicate save handle.
 * @param   pfdSaved    Where to store the saved handle.
 *                      Set to -1 on failure.
 * @param   pfCoE       Where to store the close-on-exec flag.
 *                      Set to -1 on failure.
 */
static int save_handle(int fdToSave, int *pfdMin, PRESTOREACTION pRAction)
{
    int fdSaved;
    int fSavedCoE;

    pRAction->fdRestore = - 1;
    pRAction->fdSaved = -1;
    pRAction->fSavedCoE = -1;
    if (!__libc_FH(fdToSave))
        return 0;                       /* no such handle. */

    /* Get close on exec flag. */
    fSavedCoE = fcntl(fdToSave, F_GETFD);
    if (fSavedCoE < 0)
        return -1;

    /* find free handle and duplicate fdToSave onto it */
    while (__libc_FH(*pfdMin))
        (*pfdMin)++;
    if (*pfdMin >= 0x10000)
        return -1;
    fdSaved = dup2(fdToSave, *pfdMin);
    if (fdSaved < 0)
        return -1;

    /* marke it close on exit. */
    if (    !(fSavedCoE & FD_CLOEXEC)
        &&  fcntl(fdSaved, F_SETFD, FD_CLOEXEC) < 0)
    {
        /* shit! cleanup */
        close(fdSaved);
        return -1;
    }
    pRAction->fdRestore = fdToSave;
    pRAction->fdSaved   = fdSaved;
    pRAction->fSavedCoE = fSavedCoE;
    return 0;
}


/**
 * Restore a handle saved with save_handle().
 *
 * @param   fdToRestore The original handle which is to be restored.
 * @param   pRAction    Restore action structure.
 */
static void restore_handle(PRESTOREACTION pRAction)
{
    int saved_errno = errno;

    if (pRAction->fdClose >= 0)
    {
#ifndef DEBUG
        close(pRAction->fdClose);
#else
        if (close(pRAction->fdClose))
            assert(!"__spawni: restore close() failed");
#endif
        pRAction->fdClose = -1;
    }

    if (pRAction->fdSaved >= 0)
    {
#ifndef DEBUG
        dup2(pRAction->fdSaved, pRAction->fdRestore);
        close(pRAction->fdSaved);
        fcntl(pRAction->fdRestore, F_SETFD, pRAction->fSavedCoE);
#else
        if (dup2(pRAction->fdSaved, pRAction->fdRestore) < 0)
            assert(!"__spawni: restore dup2() failed");
        if (close(pRAction->fdSaved) < 0)
            assert(!"__spawni: restore close(saved) failed");
        if (fcntl(pRAction->fdRestore, F_SETFD, pRAction->fSavedCoE) < 0)
            assert(!"__spawni: restore fcntl() failed");
#endif
        pRAction->fdSaved = -1;
        pRAction->fSavedCoE = -1;
    }

    errno = saved_errno;
}


/*
 * Internal OS dependent worker.
 * Note that it returns 0 on success and errno on failure (as required by Posix
 * for posix_spawn, see #46 for details).
 */
int __spawni(
    pid_t *pid,
    const char *path,
    const posix_spawn_file_actions_t *file_actions,
    const posix_spawnattr_t *attrp,
    char *const argv[],
    char *const envp[],
    int use_path)
{
    int             iFileAction = 0;    /* where to start cleaning up file actions from.  */
    PRESTOREACTION  paRestore = NULL;
    int             rc;                 /* result. */
    pid_t           pidChild;
#if 0 /* requires some process management from LIBC */
    unsigned    fFlags = attrp ? attrp->__flags : 0;

    /*
     * Create child process.
     */
    pChild = __libc_PSExec();

    /*
     * Signal settings to be picked up by the child.
     */
    if (fFlags & POSIX_SPAWN_SETSIGMASK)
        pChild->SigMask = attrp->__ss;
    if (fFlags & POSIX_SPAWN_SETSIGDEF)
        pChild->fSigAction = LIBC_SETSIGDEF;

    /*
     * Scheduler fun - don't think we should try apply this
     * and let the child enherit it, it's better having the child setting these things.
     */
    if (fFlags & POSIX_SPAWN_SETSCHEDPARAM)
        pChild->SchedParam = attrp->__sp;
    if (fFlags & POSIX_SPAWN_SETSCHEDULER)
        pChild->SchedPolicy = attrp->__policy;
#endif
    /* Ignore POSIX_SPAWN_SETPGROUP & POSIX_SPAWN_RESETIDS  for now. */


    /*
     * Apply file actions - this is gonna be fun.
     *      - Dup2() with two file handle is ok.
     *      - If there is any dup2 with target as -1 or and any open we're
     *        in for some work.
     *        The reason is that we have to save any handles which we close
     *        with close() or dup2(), and these saved handles must not crash with
     *        any we might open thru open() or dup2(,-1).
     */
    rc = 0;
    if (file_actions)
    {
        int i;
        int fdMin;
        int cOpens;

        /* TODO: lock the file handle table for other threads. */

        /* Verify handles and predict what open() and dup2() will return so we can
           find the min save handle number.
           Note. we're counting too many opens here... but who cares. */
        for (i = cOpens = 0; !rc && i < file_actions->__used; ++i)
        {
            struct __spawn_action  *pAction = (struct __spawn_action *)&file_actions->__actions[i]; /* nasty - const */
            switch (pAction->tag)
            {
                case spawn_do_close:
                    if (!__libc_FH(pAction->action.close_action.fd))
                        rc = -1;
                    break;
                case spawn_do_open:
                    cOpens++;
                    break;
                case spawn_do_dup2:
                    if (!__libc_FH(pAction->action.dup2_action.fd))
                        rc = -1;
                    if (pAction->action.dup2_action.newfd == -1)
                        cOpens++;
                    break;
            }
        }
        if (rc)
        {
            /* TODO: release the file handle table lock. */
            errno = EINVAL;
            return errno;
        }

        paRestore = _hmalloc(sizeof(RESTOREACTION) * file_actions->__used);
        if (!paRestore)
        {
            /* TODO: release the file handle table lock. */
            return errno;
        }
        memset(paRestore, -1, sizeof(RESTOREACTION) * file_actions->__used);

        for (fdMin = 0; cOpens >= 0 && fdMin < 0x10000; fdMin++)
        {
            int fClosed;
            int fDuped;
            for (i = fClosed = fDuped = 0; i < file_actions->__used; ++i)
            {
                struct __spawn_action  *pAction = (struct __spawn_action *)&file_actions->__actions[i]; /* nasty - const */
                switch (pAction->tag)
                {
                    case spawn_do_close:
                        if (pAction->action.close_action.fd == fdMin)
                            fClosed = 1;
                        break;
                    case spawn_do_dup2:
                        if (pAction->action.dup2_action.newfd == fdMin)
                            fDuped = 1;
                        break;
                    case spawn_do_open:
                        if (pAction->action.open_action.fd == fdMin)
                            fDuped = 1;
                        break;
                }
            }

            /* if duped we cannot use it. */
            if (fDuped)
                continue;
            /* if not closed or not being freed we cannot use it. */
            if (!fClosed && __libc_FH(fdMin))
                continue;
            /* it's either closed or free */
            cOpens--;
        }

        /* Execute the actions. */
        for (i = rc = 0; !rc && i < file_actions->__used; ++i)
        {
            struct __spawn_action  *pAction = (struct __spawn_action *)&file_actions->__actions[i]; /* nasty - const */
            PRESTOREACTION          pRAction = &paRestore[i];

            switch (pAction->tag)
            {
                case spawn_do_close:
                    rc = save_handle(pAction->action.close_action.fd, &fdMin, pRAction);
                    if (rc)
                        break;
                    if (close(pAction->action.close_action.fd))
                    {
                        restore_handle(pRAction);
                        rc = -1;
                    }
                    break;

                case spawn_do_open:
                    pRAction->fdClose = open(pAction->action.open_action.path,
                                             pAction->action.open_action.oflag,
                                             pAction->action.open_action.mode);
                    if (pRAction->fdClose < 0)
                        rc = -1;
                    else if (   pRAction->fdClose != pAction->action.open_action.fd
                             && pAction->action.open_action.fd != -1   /* -1 is not standard */)
                    {
                        int fd = pRAction->fdClose;
                        pRAction->fdClose = -1;
                        rc = save_handle(pAction->action.open_action.fd, &fdMin, pRAction);
                        if (!rc)
                        {
                            if (dup2(fd, pAction->action.open_action.fd) < 0)
                            {
                                restore_handle(pRAction);
                                rc = -1;
                            }
                        }
                        close(fd);
                    }
                    break;

                case spawn_do_dup2:
                    if (pAction->action.dup2_action.newfd != -1)
                    {
                        rc = save_handle(pAction->action.dup2_action.newfd, &fdMin, pRAction);
                        if (rc)
                            break;
                    }

                    pRAction->fdClose = dup2(pAction->action.dup2_action.fd, pAction->action.dup2_action.newfd);
                    if (pRAction->fdClose < 0)
                    {
                        restore_handle(pRAction);
                        rc = -1;
                    }
                    break;
            }
        }
        iFileAction = i;                 /* restore loop starts by decrementing it. */
    } /* fileactions */


    /*
     * Spawn the child process (if all went well so far).
     */
    if (!rc)
    {
        if (use_path)
            pidChild = spawnve(P_NOWAIT, path, argv, envp);
        else
            pidChild = spawnvpe(P_NOWAIT, path, argv, envp);
        if (pidChild >= 0)
        {
            #if 0
            /* Update child */
            pChild->pid = pidChild;
            #endif
            /* Set return */
            *pid = pidChild;
        }
        else
            rc = errno;
    }
    else
    {
        errno = EINVAL;                 /* error in file_actions or attrp. */
        rc = errno;
    }

    /*
     * Restore the file handles.
     */
    if (paRestore)
    {
        int save_errno = errno;
        while (iFileAction-- > 0)
            restore_handle(&paRestore[iFileAction]);
        free(paRestore);
        errno = save_errno;
    }

    /* TODO: release file handle table. */

    return rc;
}
