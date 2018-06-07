/* $Id: thread_internals.c 2798 2006-08-28 02:01:31Z bird $ */
/** @file
 *
 * LIBC - Thread internals.
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
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_THREAD
#include "libc-alias.h"
#undef NDEBUG
#include <sys/builtin.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <sys/smutex.h>
#include <sys/builtin.h>
#include <sys/fmutex.h>
#include <emx/umalloc.h>
#include <emx/syscalls.h>
#include <InnoTekLIBC/thread.h>
#include <InnoTekLIBC/backend.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_THREAD
#include <InnoTekLIBC/logstrict.h>


/*******************************************************************************
*   Global Variables                                                           *
*******************************************************************************/
/** Preallocated thread structure. */
static __LIBC_THREAD    gPreAllocThrd;
/** Flags whether or not gPreAllocThrd is used. */
static int              gfPreAllocThrd;

/** Mutex for the thread database. */
static _fmutex          gmtxThrdDB;
/** Thread database. (Protected gmtxThrdDB.) */
static __LIBC_PTHREAD   gpThrdDB;
/** Number of threads in the thread database. (Protected gmtxThrdDB.) */
static unsigned         gcThrdDBEntries;
/** Zombie thread database. (Protected gmtxThrdDB.) */
static __LIBC_PTHREAD   gpThrdDBZombies;
/** Number of threads in the zombie thread database. (Protected gmtxThrdDB.) */
static unsigned         gcThrdDBZombies;
/** Fork cleanup indicator. */
static unsigned         gfForkCleanupDone;

/** Head of the thread termination callback list. */
static __LIBC_PTHREADTERMCBREGREC   gpTermHead;
/** Tail of the thread termination callback list. */
static __LIBC_PTHREADTERMCBREGREC   gpTermTail;
/** List protection semaphore (spinlock sort). */
static _smutex                      gsmtxTerm;



/**
 * Initialize a thread structure.
 *
 * @param   pThrd       Pointer to the thread structure which is to be initialized.
 * @param   pParentThrd Pointer to the thread structure for the parent thread.
 *                      If NULL and thread id is 1 then inherit from parent process.
 *                      If NULL and thread is not null or no record of parent then
 *                      use defaults.
 */
static void threadInit(__LIBC_PTHREAD pThrd, const __LIBC_PTHREAD pParentThrd)
{
    bzero(pThrd, sizeof(*pThrd));
    pThrd->cRefs = 1;
    __libc_Back_threadInit(pThrd, pParentThrd);
}


void __libc_threadUse(__LIBC_PTHREAD pThrd)
{
    LIBCLOG_ENTER("pThrd=%p\n", (void *)pThrd);
    int     rc;

    /*
     * Check that pThrd isn't already in use and set gpTLS to point at pThrd.
     * (*gpTLS might already be pointing to a temporary structure.)
     */
    assert(!pThrd->pNext && !pThrd->tid);
    *__libc_gpTLS = pThrd;

    /*
     * Set the thread id of this thread.
     */
    pThrd->tid = _gettid();

    /*
     * Insert this thread into the thread database.
     * Note that there might be a dead thread by the same id in the database.
     * This have to be removed of course.
     */
    /** @todo rewrite to use a read/write semaphore, not mutex. */
    if (gmtxThrdDB.fs == _FMS_UNINIT)
        _fmutex_create2(&gmtxThrdDB, 0, "LIBC Thread DB Mutex");

    rc = _fmutex_request(&gmtxThrdDB, 0);
    if (!rc)
    {
        /* Remove any zombie threads. */
        __LIBC_PTHREAD pPrev = NULL;
        __LIBC_PTHREAD p = gpThrdDB;
        while (p)
        {
            if (p->tid == pThrd->tid)
            {
                /* remove it and place it in the zombie list */
                __LIBC_PTHREAD pNext = p->pNext;
                if (pPrev)
                    pPrev->pNext = pNext;
                else
                    gpThrdDB = pNext;
                gcThrdDBEntries--;

                p->pNext = gpThrdDBZombies;
                gpThrdDBZombies = p;
                gcThrdDBZombies++;
                p = pNext;
                /* paranoid as always, we continue scanning the entire list. */
            }
            else
            {
                /* next */
                pPrev = p;
                p = p->pNext;
            }
        }

        /* Insert it */
        pThrd->pNext = gpThrdDB;
        gpThrdDB = pThrd;
        gcThrdDBEntries++;

        _fmutex_release(&gmtxThrdDB);
        LIBCLOG_RETURN_VOID();
    }

    LIBCLOG_ERROR_RETURN_VOID();
}


__LIBC_PTHREAD __libc_threadCurrentSlow(void)
{
    LIBCLOG_ENTER("\n");
    if (!*__libc_gpTLS)
    {
        __LIBC_PTHREAD  pThrd;
        /*
         * Setup a temporary thread block on the stack so _hmalloc()
         * can't end up calling us recursivly if something goes wrong.
         */
        __LIBC_THREAD   Thrd;
        threadInit(&Thrd, NULL);
        *__libc_gpTLS   = &Thrd;

        if (!__lxchg(&gfPreAllocThrd, 1))
            pThrd = &gPreAllocThrd;
        else
        {
            pThrd = _hmalloc(sizeof(__LIBC_THREAD));
            assert(pThrd); /* deep, deep, deep, shit. abort the process in a controlled manner... */
        }
        *pThrd = Thrd;

        __libc_threadUse(pThrd);
        LIBCLOG_MSG("Created thread block %p\n", (void*)pThrd);
    }

    LIBCLOG_RETURN_P(*__libc_gpTLS);
}


__LIBC_PTHREAD __libc_threadAlloc(void)
{
    LIBCLOG_ENTER("\n");
    /*
     * No need to use the pre allocated here since the current thread will
     * most likely be using that one!
     */
    __LIBC_PTHREAD pThrd = _hmalloc(sizeof(__LIBC_THREAD));
    if (pThrd)
    {
        threadInit(pThrd, __libc_threadCurrentNoAuto());
        LIBCLOG_RETURN_P(pThrd);
    }
    LIBCLOG_ERROR_RETURN_P(pThrd);
}


void __libc_threadDereference(__LIBC_PTHREAD pThrd)
{
    LIBCLOG_ENTER("pThrd=%p (tid=%d)\n", (void *)pThrd, pThrd->tid);

    /*
     * Take owner ship of the DB semaphore.
     */
    _fmutex_request(&gmtxThrdDB, 0);
    if (pThrd->cRefs)
        pThrd->cRefs--;
    if (pThrd->cRefs)
    {
#ifdef DEBUG_LOGGING
        unsigned cRefs = pThrd->cRefs;
#endif
        _fmutex_release(&gmtxThrdDB);
        LIBCLOG_RETURN_MSG_VOID("ret void. cRefs=%d\n", cRefs);
    }
    LIBCLOG_MSG("unlinking and disposing the thread.\n");

    /*
     * Thread is dead, unlink it.
     */
    if (pThrd->tid)
    {
        if (pThrd == gpThrdDB)
        {
            gpThrdDB = pThrd->pNext;
            gcThrdDBEntries--;
        }
        else
        {
            __LIBC_PTHREAD p;
            for (p = gpThrdDB; p->pNext; p = p->pNext)
                if (p->pNext == pThrd)
                {
                    p->pNext = pThrd->pNext;
                    gcThrdDBEntries--;
                    p = NULL;
                    break;
                }

            /*
             * Not found? search zombie DB.
             */
            if (p)
            {
                if (gpThrdDBZombies == pThrd)
                {
                    gpThrdDBZombies = pThrd->pNext;
                    gcThrdDBZombies--;
                    p = NULL;
                }
                else
                {
                    for (p = gpThrdDBZombies; p->pNext; p = p->pNext)
                        if (p->pNext == pThrd)
                        {
                            p->pNext = pThrd->pNext;
                            gcThrdDBZombies--;
                            p = NULL;
                            break;
                        }
                }
            }

            /*
             * Did we succeed in finding it? If not don't dispose of the the thread!
             */
            if (p)
            {
                assert(!p);
                pThrd = NULL;
            }
        }
    }

    _fmutex_release(&gmtxThrdDB);


    /*
     * Dispose of the thread
     */
    if (pThrd)
    {
        /*
         * Clean up members.
         */
        __libc_Back_threadCleanup(pThrd);

        /*
         * Clear the TLS if it's for the current thread.
         */
        if (*__libc_gpTLS == pThrd)
            *__libc_gpTLS = NULL;

        /*
         * Release storage.
         */
        if (pThrd != &gPreAllocThrd)
            free(pThrd);
        else
            __lxchg(&gfPreAllocThrd, 0);
        LIBCLOG_RETURN_MSG_VOID("ret (gcThrdDBEntires=%d gcThrdDBZombies=%d)\n", gcThrdDBEntries, gcThrdDBZombies);
    }
    LIBCLOG_ERROR_RETURN_MSG_VOID("ret (gcThrdDBEntires=%d gcThrdDBZombies=%d)\n", gcThrdDBEntries, gcThrdDBZombies);
}


__LIBC_PTHREAD __libc_threadLookup(unsigned tid)
{
    LIBCLOG_ENTER("tid=%d\n", tid);
    int             rc;
    __LIBC_PTHREAD  pThrd;

    /* can't search something which isn't there. */
    if (gmtxThrdDB.fs == _FMS_UNINIT)
        LIBCLOG_ERROR_RETURN_P(NULL);

    rc = _fmutex_request(&gmtxThrdDB, 0);
    if (rc)
        LIBCLOG_ERROR_RETURN(NULL, "ret NULL - fmutex f**ked. rc=%d\n", rc);

    for (pThrd = gpThrdDB; pThrd; pThrd = pThrd->pNext)
        if (pThrd->tid == tid)
        {
            pThrd->cRefs++;
            break;
        }

    _fmutex_release(&gmtxThrdDB);
    if (pThrd)
        LIBCLOG_RETURN_P(pThrd);
    LIBCLOG_ERROR_RETURN_P(pThrd);
}


__LIBC_PTHREAD __libc_threadLookup2(int (pfnCallback)(__LIBC_PTHREAD pCur, __LIBC_PTHREAD pBest, void *pvParam), void *pvParam)
{
    LIBCLOG_ENTER("pfnCallback=%p pvParam=%p\n", (void *)pfnCallback, pvParam);
    int             rc;
    __LIBC_PTHREAD  pThrd;
    __LIBC_PTHREAD  pBest = NULL;

    /* can't search something which isn't there. */
    if (gmtxThrdDB.fs == _FMS_UNINIT)
        LIBCLOG_ERROR_RETURN_P(NULL);

    rc = _fmutex_request(&gmtxThrdDB, 0);
    if (rc)
        LIBCLOG_ERROR_RETURN(NULL, "ret NULL - fmutex f**ked. rc=%d\n", rc);

    for (pThrd = gpThrdDB; pThrd; pThrd = pThrd->pNext)
    {
        rc = pfnCallback(pThrd, pBest, pvParam);
        if (rc == 0)
            continue;
        else if (rc == 1)
        {
            pBest = pThrd;
            continue;
        }
        else if (rc == 2)
        {
            pBest = pThrd;
            break;
        }
        else if (rc == -1)
            break;
        else
        {
            LIBC_ASSERTM_FAILED("Callback returned %d, allowed values are 2, 1, 0 and -1!\n", rc);
            break;
        }
    }

    if (pBest)
        pBest->cRefs++;

    _fmutex_release(&gmtxThrdDB);

    if (pBest)
        LIBCLOG_RETURN_MSG(pBest, "ret %p (tid=%d)\n", (void *)pBest, pBest->tid);
    LIBCLOG_ERROR_RETURN(pBest, "ret NULL\n");
}



int         __libc_threadEnum(int (pfnCallback)(__LIBC_PTHREAD pCur, void *pvParam), void *pvParam)
{
    LIBCLOG_ENTER("pfnCallback=%p pvParam=%p\n", (void *)pfnCallback, pvParam);
    int             rc;
    __LIBC_PTHREAD  pThrd;

    /* can't search something which isn't there. */
    if (gmtxThrdDB.fs == _FMS_UNINIT)
        LIBCLOG_ERROR_RETURN_P(NULL);

    rc = _fmutex_request(&gmtxThrdDB, 0);
    if (rc)
        LIBCLOG_ERROR_RETURN(NULL, "ret NULL - fmutex f**ked. rc=%d\n", rc);

    for (pThrd = gpThrdDB; pThrd; pThrd = pThrd->pNext)
    {
        rc = pfnCallback(pThrd, pvParam);
        if (rc == 0)
            continue;
        else if (rc == -1)
            break;
        else
        {
            LIBC_ASSERTM_FAILED("Callback returned %d, allowed values are 0 and -1!\n", rc);
            break;
        }
    }

    _fmutex_release(&gmtxThrdDB);

    LIBCLOG_RETURN_INT(rc);
}

int     __libc_ThreadRegisterTermCallback(__LIBC_PTHREADTERMCBREGREC pRegRec)
{
    LIBCLOG_ENTER("pRegRec=%p:{.pNext=%p, .fFlags=%u, .pfnCallback=%p}\n", (void *)pRegRec,
                  pRegRec ? (void *)pRegRec->pNext : NULL,
                  pRegRec ? pRegRec->fFlags : 0,
                  pRegRec ? (void *)pRegRec->pfnCallback : NULL);

    /*
     * Validate input.
     */
    if (pRegRec->pNext)
    {
        errno = EINVAL;
        LIBCLOG_ERROR_RETURN(-1, "ret -1 - pNext must be NULL not %p\n", (void * )pRegRec->pNext);
    }
    if (!pRegRec->pfnCallback)
    {
        errno = EINVAL;
        LIBCLOG_ERROR_RETURN(-1, "ret -1 - pfnCallback not be NULL\n");
    }
    if (pRegRec->fFlags)
    {
        errno = EINVAL;
        LIBCLOG_ERROR_RETURN(-1, "ret -1 - fFlags must be ZERO not %u\n", pRegRec->fFlags);
    }

    /*
     * Insert into the LIFO.
     */
    _smutex_request(&gsmtxTerm);
    if (    !pRegRec->pNext
        &&  pRegRec != gpTermTail)
    {
        pRegRec->pNext = gpTermTail;
        gpTermTail = pRegRec;
        if (!gpTermHead)
            gpTermHead = pRegRec;
        _smutex_release(&gsmtxTerm);
        LIBCLOG_RETURN_INT(0);
    }
    else
    {
        _smutex_release(&gsmtxTerm);
        errno = EEXIST;
        LIBCLOG_ERROR_RETURN(-1, "ret -1 - Double registration of %p\n", (void *)pRegRec);
    }
}


void    __libc_threadTermination(unsigned fFlags)
{
    LIBCLOG_ENTER("fFlags=%u\n", fFlags);
    __LIBC_PTHREADTERMCBREGREC  pCur;
    __LIBC_PTHREADTERMCBREGREC  pTail;

    /*
     * We'll need to do this in a safe manner.
     *
     * ASSUME no removal of records.
     * Thus we just pick the head and tail record and walk them.
     */
    _smutex_request(&gsmtxTerm);
    pTail = gpTermTail;
    pCur = gpTermHead;
    _smutex_release(&gsmtxTerm);

    while (pCur)
    {
        /* call it */
        LIBCLOG_MSG("calling %p with %p\n", (void *)pCur->pfnCallback, (void *)pCur);
        pCur->pfnCallback(pCur, fFlags);

        /* next */
        if (pCur == pTail)
            break;
        pCur = pCur->pNext;
    }

    LIBCLOG_RETURN_VOID();
}


#undef  __LIBC_LOG_GROUP
#define __LIBC_LOG_GROUP    __LIBC_LOG_GRP_FORK


/**
 * Fork completion callback used to release the thread db lock.
 *
 * @param   pvArg   NULL.
 * @param   rc      The fork() result. Negative on failure.
 * @param   enmCtx  The calling context.
 */
static void threadForkCompletion(void *pvArg, int rc, __LIBC_FORKCTX enmCtx)
{
    LIBCLOG_ENTER("pvArg=%p rc=%d enmCtx=%d - gfForkCleanupDone=%d\n", pvArg, rc, enmCtx, gfForkCleanupDone);

    if (!gfForkCleanupDone)
    {
        LIBCLOG_MSG("Unlocking the thread DB.\n");
        if (enmCtx == __LIBC_FORK_CTX_PARENT)
            _fmutex_release(&gmtxThrdDB);
        else
            _fmutex_release_fork(&gmtxThrdDB);
        gfForkCleanupDone = 1;
    }
    LIBCLOG_RETURN_VOID();
}


/**
 * Parent fork callback for locking down the thread db while forking.
 *
 * @returns 0 on success.
 * @returns -errno on failure.
 * @param   pForkHandle     Pointer to fork handle.
 * @param   enmOperation    Fork operation.
 */
static int threadForkParent1(__LIBC_PFORKHANDLE pForkHandle, __LIBC_FORKOP enmOperation)
{
    LIBCLOG_ENTER("pForkHandle=%p enmOperation=%d\n", (void *)pForkHandle, enmOperation);

    if (enmOperation != __LIBC_FORK_OP_EXEC_PARENT)
        LIBCLOG_RETURN_INT(0);

    /*
     * Lock the thread database before fork() and schedule an unlocking completion callback.
     */
    LIBCLOG_MSG("Locking the thead DB.\n");
    if (_fmutex_request(&gmtxThrdDB, 0))
        LIBCLOG_ERROR_RETURN_INT(-EDEADLK);

    gfForkCleanupDone = 0;
    int rc = pForkHandle->pfnCompletionCallback(pForkHandle, threadForkCompletion, NULL, __LIBC_FORK_CTX_BOTH);
    if (rc >= 0)
        LIBCLOG_RETURN_INT(rc);

    _fmutex_release(&gmtxThrdDB);
    LIBCLOG_ERROR_RETURN_INT(rc);
}


_FORK_PARENT1(0xffffff02, threadForkParent1)

