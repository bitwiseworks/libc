/* $Id: DosEx.c 3805 2014-02-06 11:37:29Z ydario $ */
/** @file
 *
 * Dos API Extension Fundament.
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

/** @todo
 * There are a few optimizations which can be excercised here, it mostly comes
 * down to sorting things when allocating them in the child. Let's see how
 * slow it is first, then we'll consider making it more snappy.
 */


/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include "libc-alias.h"
#define INCL_ERRORS
#define INCL_DOS
#define INCL_FSMACROS
#define INCL_EXAPIS
#include <os2emx.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <emx/startup.h>
#include <386/builtin.h>
#define __LIBC_LOG_GROUP    __LIBC_LOG_GRP_HEAP /* fixme */
#include <InnoTekLIBC/logstrict.h>
#include <InnotekLIBC/fork.h>
#include "DosEx.h"
#include "syscalls.h"


/*******************************************************************************
*   Defined Constants And Macros                                               *
*******************************************************************************/
/** Size of a block in the pool. */
#define DOSEX_BLOCK_SIZE    sizeof(DOSEX)
/** The size of individual pools - 64KB. */
#define DOSEX_POOL_SIZE     (64 * 1024)


/*******************************************************************************
*   Structures and Typedefs                                                    *
*******************************************************************************/
/**
 * Each pool have a header like this.
 */
typedef struct _DOSEXHDR
{
    /** List heads for all types. */
    PDOSEX              apHeads[DOSEX_TYPE_MAX];
    /** List tails for all types. */
    PDOSEX              apTails[DOSEX_TYPE_MAX];
    /** The size of this pool */
    size_t              cb;
    /** Pointer to free space. */
    void               *pvFreeSpace;
    /** Size of freespace. */
    size_t              cbFreeSpace;
    /** Pointer to the next pool. */
    struct _DOSEXHDR   *pNext;
    /** alignement. */
    char                achReserved[8];
} DOSEXHDR;
/** Pointer to pool header. */
typedef DOSEXHDR *PDOSEXHDR;


/**
 * The argument given to dosexForkChildAlloc().
 */
typedef struct _DOSEXFORKCHILDALLOC
{
    /** Pool Address. */
    void       *pv;
    /** Pool Size. */
    size_t      cb;
} DOSEXFORKCHILDALLOC, *PDOSEXFORKCHILDALLOC;


/**
 * Handle bundle used when allocating a given semaphore handle.
 */
typedef struct DOSEXHANDLEBUNDLE
{
    /** Pointer to the next bundle. */
    struct DOSEXHANDLEBUNDLE   *pNext;
    /** Number of entries in the array (ah) which are currently used. */
    unsigned                    cUsed;
    /** Array of handles. */
    LHANDLE                     ah[1022];
} DOSEXHANDLEBUNDLE, *PDOSEXHANDLEBUNDLE;


/*******************************************************************************
*   Global Variables                                                           *
*******************************************************************************/
/** Flags whether or not we're initialized yet. */
static int          gfInited;
/** Flags that we're forking and need releasing the mutex. */
static int          gfFork;
/** Semaphore protecting the pools. */
static HMTX         gmtxPools;
/** Head of the pool list. */
static PDOSEXHDR    gpPoolsHead;
/** Tail of the pool list. */
static PDOSEXHDR    gpPoolsTail;
/** The size of the pools. */
static size_t       gcbPools;
/** The current size of allocated private memory. */
size_t              __libc_gcbDosExMemAlloc = 0;


/*******************************************************************************
*   Internal Functions                                                         *
*******************************************************************************/
static void dosexInit(void);
static int  dosexRequestMutex(void);
static void dosexReleaseMutex(void);
static int  dosexAllocPool(void);
#if 0
static void dosexFreePool(PDOSEXHDR pPool);
static void dosexCompactPools(void);
#endif
static int  dosexFreeEntry(DOSEXTYPE enmType, PDOSEX pCur, PDOSEX pPrev, PDOSEXHDR pPool);

static int  dosexForkParent(__LIBC_PFORKHANDLE pForkHandle, __LIBC_FORKOP enmOperation);
static int  dosexForkOpExecParent(__LIBC_PFORKHANDLE pForkHandle);
static int  dosexForkOpForkParent(__LIBC_PFORKHANDLE pForkHandle);
static int  dosexForkChildAlloc(__LIBC_PFORKHANDLE pForkHandle, void *pvArg, size_t cbArg);
static int  dosexForkChildProcess(__LIBC_PFORKHANDLE pForkHandle, void *pvArg, size_t cbArg);
static int  dosexForkPreMemAlloc(PDOSEX pEntry);
static int  dosexForkPreMemOpen(PDOSEX pEntry);
static int  dosexForkPreMutexCreate(PDOSEX pEntry);
static int  dosexForkPreMutexOpen(PDOSEX pEntry);
static int  dosexForkPreEventCreate(PDOSEX pEntry);
static int  dosexForkPreEventOpen(PDOSEX pEntry);
static int  dosexForkPreLoadModule(PDOSEX pEntry);
static int  dosexForkChild(__LIBC_PFORKHANDLE pForkHandle, __LIBC_FORKOP enmOperation);
static void dosexForkCleanup(void *pvArg, int rc, __LIBC_FORKCTX enmCtx);


/*
 * Put it in the init list for the usual paranoid reasons.
 * All facts speaks for it being lazyingly initialized during loading.
 */
_CRT_INIT1(dosexInit)

/**
 * Initialize the semaphore.
 */
CRT_DATA_USED
void dosexInit(void)
{
    LIBCLOG_ENTER("\n");
    FS_VAR()
    if (gfInited)
        LIBCLOG_RETURN_VOID();
    /*
     * Create mutex.
     */
    FS_SAVE_LOAD();
    if (!DosCreateMutexSem(NULL, &gmtxPools, 0, FALSE))
        gfInited = 1;
    FS_RESTORE();
    LIBCLOG_RETURN_VOID();
}

/**
 * Request the DosEx pool semaphore.
 * @returns 0 on success.
 * @returns OS/2 error code on failure.
 */
static int  dosexRequestMutex(void)
{
    LIBCLOG_ENTER("\n");
    int     rc;
    FS_VAR();

    /*
     * Lazy init.
     */
    if (!gfInited)
    {
        dosexInit();
        if (!gfInited)
            LIBCLOG_ERROR_RETURN_INT(ERROR_NOT_SUPPORTED);
    }

    FS_SAVE_LOAD();
    rc = DosRequestMutexSem(gmtxPools, 10*60*1000 /* 10 minuttes */);
    if (!rc)
    {
        ULONG ul;
        DosEnterMustComplete(&ul);
        FS_RESTORE();
        LIBCLOG_RETURN_INT(0);
    }

    LIBC_ASSERTM_FAILED("DosRequestMutexSem(%#lx, 10min) -> %d\n", gmtxPools, rc);
    FS_RESTORE();
    LIBCLOG_ERROR_RETURN_INT(rc);
}


/**
 * Release mutex succesfully obtained by dosexRequestMutex().
 */
static void dosexReleaseMutex(void)
{
    LIBCLOG_ENTER("\n");
    int     rc;
    FS_VAR();

    FS_SAVE_LOAD();
    rc = DosReleaseMutexSem(gmtxPools);
    if (!rc)
    {
        ULONG ul;
        DosExitMustComplete(&ul);
        FS_RESTORE();
        LIBCLOG_RETURN_VOID();
    }

    LIBC_ASSERTM_FAILED("DosReleaseMutexSem(%#lx) -> %d\n", gmtxPools, rc);
    FS_RESTORE();
    LIBCLOG_ERROR_RETURN_VOID();
}


/**
 * Allocates and adds another pool to the list.
 * @returns 0 on success.
 * @returns OS/2 error code.
 * @remark  Caller owns mutex.
 */
static int dosexAllocPool(void)
{
    LIBCLOG_ENTER("\n");
    PVOID       pv;                     /* only to shut up -pedantic! */
    PDOSEXHDR   pPool;
    size_t      cb;
    int         rc;
    FS_VAR();

    /*
     * Try allocate the a new pool.
     */
    FS_SAVE_LOAD();
    cb = DOSEX_POOL_SIZE * 2;
    rc = DosAllocMem(&pv, cb, PAG_READ | PAG_WRITE | PAG_COMMIT | OBJ_ANY);
    if (rc)
    {
        cb = DOSEX_POOL_SIZE;
        rc = DosAllocMem(&pv, cb, PAG_READ | PAG_WRITE | PAG_COMMIT);
    }
    FS_RESTORE();

    if (rc)
        LIBCLOG_ERROR_RETURN_INT(rc);
    pPool = (PDOSEXHDR)pv;

    /*
     * Initialize the pool.
     */
    LIBCLOG_MSG("Adding a pool of %d bytes.\n", cb);
    pPool->cb           = cb;
    pPool->cbFreeSpace  = ((cb - sizeof(*pPool)) / DOSEX_BLOCK_SIZE) * DOSEX_BLOCK_SIZE;
    pPool->pvFreeSpace  = pPool + 1;

    /*
     * Link it into the pool list.
     */
    if (gpPoolsTail)
    {
        gpPoolsTail->pNext = pPool;
        gpPoolsTail = pPool;
    }
    else
        gpPoolsTail = gpPoolsHead = pPool;
    gcbPools       += cb;

    LIBCLOG_RETURN_INT(0);
}


#if 0
/**
 * Frees a pool.
 * @param   pPool
 */
void dosexFreePool(PDOSEXHDR pPool)
{
    /*
     * Unlink the pool
     */
    if (pPool == gpPoolsHead)
    {
        gpPoolsHead = pPool->pNext;
        if (!gpPoolsHead)
            gpPoolsTail = NULL;
    }
    else
    {
        PDOSEXHDR pPrev = gpPoolsHead;
        while (pPrev->pNext != pPool)
            pPrev = pPrev->pNext;
        if (pPrev)
        {
            pPrev->pNext = pPool->pNext;
            if (!pPrev->pNext)
                gpPoolsTail = pPrev;
        }
    }

    /*
     * Update statistics.
     */
    gcbPools -= pPool->cb;

    /*
     * Free the memory.
     */
    DosFreeMem(pPool);
}

/**
 * Todo (not sure if required), if no free goes too.
 */
void dosexCompactPools(void)
{
    return;
}
#endif


/**
 * Allocate an entry.
 * @returns Pointer to allocated entry.
 * @returns NULL on memory shortage.
 * @param   enmType     Entry type.
 */
PDOSEX  __libc_dosexAlloc(DOSEXTYPE enmType)
{
    LIBCLOG_ENTER("enmType=%d\n", enmType);
    PDOSEXHDR   pPool;
    PDOSEX      pRet = NULL;

    /*
     * Get mutex.
     */
    if (dosexRequestMutex())
        LIBCLOG_ERROR_RETURN_P(NULL);

    /*
     * Find pool with free memory.
     */
    pPool = gpPoolsHead;
    while (pPool && !pPool->apHeads[DOSEX_TYPE_FREE] && !pPool->cbFreeSpace)
        pPool = pPool->pNext;
    if (!pPool)
    {
        /*
         * Add a new pool.
         */
        if (!dosexAllocPool())
            pPool = gpPoolsTail;
    }

    if (pPool)
    {
        /*
         * Get free node.
         */
        if (pPool->apHeads[DOSEX_TYPE_FREE])
        {
            pRet = pPool->apHeads[DOSEX_TYPE_FREE];
            pPool->apHeads[DOSEX_TYPE_FREE] = pRet->pNext;
            if (!pRet->pNext)
                pPool->apTails[DOSEX_TYPE_FREE] = NULL;
        }
        else
        {
            pRet = (PDOSEX)pPool->pvFreeSpace;
            pPool->pvFreeSpace = (char *)pPool->pvFreeSpace + DOSEX_BLOCK_SIZE;
            pPool->cbFreeSpace -= DOSEX_BLOCK_SIZE;
        }

        /*
         * Initialize it and put it in the right list.
         */
        pRet->pNext     = NULL;
        bzero(&pRet->u, sizeof(pRet->u));

        if (pPool->apTails[enmType])
            pPool->apTails[enmType]->pNext = pRet;
        else
            pPool->apHeads[enmType] = pRet;
        pPool->apTails[enmType] = pRet;
    }

    /*
     * Release the mutex and return.
     */
    dosexReleaseMutex();
    if (pRet)
        LIBCLOG_RETURN_P(pRet);
    LIBCLOG_ERROR_RETURN_P(pRet);
}


/**
 * Free entry.
 * @returns 0 on success.
 * @returns -1 if not found.
 * @returns OS/2 error code on failure.
 * @param   enmType     Enter type.
 * @param   uKey        They entry key.
 * @remark  Caller is responsible for saving/loading/restoring FS.
 */
int     __libc_dosexFree(DOSEXTYPE enmType, unsigned uKey)
{
    LIBCLOG_ENTER("enmType=%d uKey=%u\n", enmType, uKey);
    int         rc;
    PDOSEXHDR   pPool;

    /*
     * Get mutex.
     */
    rc = dosexRequestMutex();
    if (rc)
        LIBCLOG_ERROR_RETURN_INT(rc);

    /*
     * Enumerate the pools looking for the entry.
     */
    pPool = gpPoolsHead;
    while (pPool)
    {
        PDOSEX  pPrev = NULL;
        PDOSEX  pCur = pPool->apHeads[enmType];
        while (pCur)
        {
            if (pCur->u.uKey == uKey)
            {
                /*
                 * Found it. Do the free operation and get out of here.
                 */
                rc = dosexFreeEntry(enmType, pCur, pPrev, pPool);

                /*
                 * Release the mutex and return.
                 */
                dosexReleaseMutex();
                if (!rc)
                    LIBCLOG_RETURN_INT(rc);
                LIBCLOG_ERROR_RETURN_INT(rc);
            }
            /* next */
            pPrev = pCur;
            pCur = pCur->pNext;
        }

        /* next */
        pPool = pPool->pNext;
    }
    /* not found */

    /*
     * Release the mutex and return.
     */
    dosexReleaseMutex();
    LIBCLOG_ERROR_RETURN_INT(-1);
}

/**
 * Preforms a free operation on the given entry (pCur).
 * This may cause the entry to be moved to the free list.
 *
 */
static int dosexFreeEntry(DOSEXTYPE enmType, PDOSEX pCur, PDOSEX pPrev, PDOSEXHDR pPool)
{
    int         rc;
    unsigned    fUnlink = 1;

    /*
     * Take action based on type.
     */
    switch (enmType)
    {
        case DOSEX_TYPE_MEM_ALLOC:
            rc = DosFreeMem(pCur->u.MemAlloc.pv);
            __atomic_sub(&__libc_gcbDosExMemAlloc, (pCur->u.MemAlloc.cb + 0xfff) & ~0xfff);
            break;

        case DOSEX_TYPE_MEM_OPEN:
            rc = DosFreeMem(pCur->u.MemOpen.pv);
            break;

        case DOSEX_TYPE_MUTEX_CREATE:
            rc = DosCloseMutexSem(pCur->u.MutexCreate.hmtx);
            if (rc == ERROR_SEM_BUSY)
                fUnlink = 0;
            break;

        case DOSEX_TYPE_MUTEX_OPEN:
            rc = DosCloseMutexSem(pCur->u.MutexOpen.hmtx);
            if (rc && rc != ERROR_INVALID_HANDLE)
                fUnlink = 0;
#ifdef PER_PROCESS_OPEN_COUNTS
            else
            {
                pCur->u.MutexOpen.cOpens--;
                if (pCur->u.MutexOpen.cOpens > 0)
                    fUnlink = 0;
            }
#endif
            break;

        case DOSEX_TYPE_EVENT_CREATE:
            rc = DosCloseEventSem(pCur->u.MutexCreate.hmtx);
            if (rc == ERROR_SEM_BUSY)
                fUnlink = 0;
            break;

        case DOSEX_TYPE_EVENT_OPEN:
            rc = DosCloseEventSem(pCur->u.EventOpen.hev);
            if (rc && rc != ERROR_INVALID_HANDLE)
                fUnlink = 0;
#ifdef PER_PROCESS_OPEN_COUNTS
            else
            {
                pCur->u.EventOpen.cOpens--;
                if (pCur->u.EventOpen.cOpens > 0)
                    fUnlink = 0;
            }
#endif
            break;

        case DOSEX_TYPE_LOAD_MODULE:
            rc = DosFreeModule(pCur->u.LoadModule.hmte);
            if (rc && rc != ERROR_INVALID_HANDLE)
                fUnlink = 0;
            else
            {
                pCur->u.LoadModule.cLoads--;
                if (pCur->u.LoadModule.cLoads > 0)
                    fUnlink = 0;
            }
            break;

        default:
            rc = ERROR_INVALID_PARAMETER;
            break;
    }

    /*
     * Unlink and put the free list it if requested.
     */
    if (fUnlink)
    {
        if (pPrev)
        {
            pPrev->pNext = pCur->pNext;
            if (!pPrev->pNext)
                pPool->apTails[enmType] = pPrev;
        }
        else
        {
            pPool->apHeads[enmType] = pCur->pNext;
            if (!pCur->pNext)
                pPool->apTails[enmType] = NULL;
        }

        pCur->pNext = pPool->apHeads[DOSEX_TYPE_FREE];
        pPool->apHeads[DOSEX_TYPE_FREE] = pCur;
    }

    return rc;
}

/**
 * Finds a entry given by type and key.
 *
 * @returns Pointer to the entry on success.
 *          The caller _must_ call __libc_dosexRelease() with this pointer!
 * @returns NULL on failure.
 *
 * @param   enmType     Type of the entry to find.
 * @param   uKey        Entery key.
 */
PDOSEX  __libc_dosexFind(DOSEXTYPE enmType, unsigned uKey)
{
    LIBCLOG_ENTER("enmType=%d uKey=%u\n", enmType, uKey);
    PDOSEXHDR   pPool;

    /*
     * Get mutex.
     */
    if (dosexRequestMutex())
        LIBCLOG_ERROR_RETURN_P(NULL);

    /*
     * Enumerate the pools looking for the entry.
     */
    pPool = gpPoolsHead;
    while (pPool)
    {
        PDOSEX  pCur = pPool->apHeads[enmType];
        while (pCur)
        {
            if (pCur->u.uKey == uKey)
            {
                /*
                 * Found it - return!
                 */
                LIBCLOG_RETURN_P(pCur);
            }
            /* next */
            pCur = pCur->pNext;
        }

        /* next */
        pPool = pPool->pNext;
    }

    /*
     * Release the mutex and return.
     */
    dosexReleaseMutex();
    LIBCLOG_ERROR_RETURN_P(NULL);
}

/**
 * Releases an entry obtained by __libc_dosexFind().
 * @param   pEntry      Pointer to the entry to release.
 */
void    __libc_dosexRelease(PDOSEX pEntry)
{
    LIBCLOG_ENTER("pEntry=%p\n", (void *)pEntry);

    if (!pEntry)
        LIBCLOG_RETURN_VOID();

    dosexReleaseMutex();
    LIBCLOG_RETURN_VOID();
}


/**
 * Get the memory stats.
 *
 * This api is intended for fork when it's figuring out the minimum and maximum
 * sizes of the fork buffer.
 *
 * @param   pcbPools        Where to store the size of the pools.
 * @param   pcbMemAlloc     Where to store the size of the allocated private memory.
 *                          I.e. memory allocated by DosAllocMemEx(,,..|OBJ_FORK).
 */
void    __libc_dosexGetMemStats(size_t *pcbPools, size_t *pcbMemAlloc)
{
    *pcbPools    = gcbPools;
    *pcbMemAlloc = __libc_gcbDosExMemAlloc;
    LIBCLOG_MSG2("__libc_dosexGetMemStats: *pcbPools=%d *pcbMemAlloc=%d\n", *pcbPools, *pcbMemAlloc);
}




#undef  __LIBC_LOG_GROUP
#define __LIBC_LOG_GROUP    __LIBC_LOG_GRP_FORK
_FORK_PARENT1(0xffffffff, dosexForkParent)

/**
 * Fork callback which duplicates the pages in the memory objects
 * allocated by DosAllocMemEx(,,..|OBJ_FORK).
 *
 * At this point all the memory objects have been safely allocated.
 * Can also mention that the pages for the module have been copied too, so we
 * have the pool list back together with an fully initialized fmutex. Aint it sweet :-)
 *
 * @returns 0 on success.
 * @returns -errno on failure.
 * @param   pForkHandle     Pointer to fork handle.
 * @param   enmOperation    Fork operation.
 */
int dosexForkParent(__LIBC_PFORKHANDLE pForkHandle, __LIBC_FORKOP enmOperation)
{
    LIBCLOG_ENTER("pForkHandle=%p enmOperation=%d\n", (void *)pForkHandle, enmOperation);
    int     rc;
    switch (enmOperation)
    {
        case __LIBC_FORK_OP_EXEC_PARENT:
            rc = dosexForkOpExecParent(pForkHandle);
            break;

        case __LIBC_FORK_OP_FORK_PARENT:
            rc = dosexForkOpForkParent(pForkHandle);
            break;

        default:
            rc = 0;
            break;
    }

    LIBCLOG_RETURN_INT(rc);
}


/**
 * Set up the allocation and copying of the pools and
 *
 * @returns 0 on success.
 * @returns -errno on failure.
 * @param   pForkHandle     Pointer to the fork handle.
 */
static int     dosexForkOpExecParent(__LIBC_PFORKHANDLE pForkHandle)
{
    LIBCLOG_ENTER("pForkHandle=%p\n", (void *)pForkHandle);
    int                     rc;
    PDOSEXHDR               pPool;
    PPIB                    pPib;
    PTIB                    pTib;
    TID                     tidCur;
    FS_VAR();

    /*
     * Figure out the current tid.
     */
    FS_SAVE_LOAD();
    DosGetInfoBlocks(&pTib, &pPib);
    tidCur = pTib->tib_ptib2->tib2_ultid;

    /*
     * Get mutex.
     */
    if (dosexRequestMutex())
    {
        FS_RESTORE();
        LIBCLOG_ERROR_RETURN_INT(-EBUSY);
    }

    /*
     * Register cleanup function.
     */
    rc = pForkHandle->pfnCompletionCallback(pForkHandle, dosexForkCleanup, NULL, __LIBC_FORK_CTX_PARENT);
    if (rc)
    {
        dosexReleaseMutex();
        FS_RESTORE();
        LIBCLOG_ERROR_RETURN_INT(rc);
    }
    gfFork = 1;


    /*
     * Duplicate the pools.
     */
    rc = 0;
    for (pPool = gpPoolsHead; pPool; pPool = pPool->pNext)
    {
        PDOSEX              pEntry;
        DOSEXFORKCHILDALLOC Arg;
        /*
         * Issue invoke call for allocating the memory.
         */
        Arg.pv = pPool;
        Arg.cb = pPool->cb;
        rc = pForkHandle->pfnInvoke(pForkHandle, dosexForkChildAlloc, &Arg, sizeof(Arg));
        if (rc)
            break;

        /*
         * Query states of event and mutex semaphores.
         * Mutexes not owned by this thread will be created with their initial state, the others will
         * be created with the current state.
         * Event semaphores will be created with the current post count.
         */
        for (pEntry = pPool->apHeads[DOSEX_TYPE_MUTEX_CREATE]; !rc && pEntry; pEntry = pEntry->pNext)
        {
            PID     pid;
            TID     tid;
            ULONG   cNestings;
            int rc = DosQueryMutexSem(pEntry->u.MutexCreate.hmtx, &pid, &tid, &cNestings);
            LIBC_ASSERTM(!rc, "DosQueryMutexSem(%#lx,) -> rc=%d\n", pEntry->u.MutexCreate.hmtx, rc);
            if (!rc && tid == tidCur)
                pEntry->u.MutexCreate.cCurState = (unsigned short)cNestings;
            else
                pEntry->u.MutexCreate.cCurState = pEntry->u.MutexCreate.fInitialState;
        }

        for (pEntry = pPool->apHeads[DOSEX_TYPE_EVENT_CREATE]; !rc && pEntry; pEntry = pEntry->pNext)
        {
            ULONG cPostings;
            int rc = DosQueryEventSem(pEntry->u.EventCreate.hev, &cPostings);
            LIBC_ASSERTM(!rc, "DosQueryEventSem(%#lx,) -> rc=%d\n", pEntry->u.EventCreate.hev, rc);
            if (!rc)
                pEntry->u.EventCreate.cCurState = (unsigned short)cPostings;
            else
                pEntry->u.EventCreate.cCurState = 0;
        }

        /*
         * Copy the pages.
         */
        rc = pForkHandle->pfnDuplicatePages(pForkHandle, pPool, (char *)pPool + pPool->cb, __LIBC_FORK_FLAGS_ALL);
        if (rc)
            break;
    }

    /*
     * Issue invoke call to process the pools and allocate the system resources.
     */
    if (!rc)
        rc = pForkHandle->pfnInvoke(pForkHandle, dosexForkChildProcess, &gpPoolsHead, sizeof(gpPoolsHead));

    /*
     * Release mutex and release.
     */
    if (rc)
    {
        dosexReleaseMutex();
        gfFork = 0;
    }
    LIBCLOG_RETURN_INT(rc);
}


/**
 * Set up the allocation and copying of the pools and
 *
 * @returns 0 on success.
 * @returns -errno on failure.
 * @param   pForkHandle     Pointer to the fork handle.
 */
static int  dosexForkOpForkParent(__LIBC_PFORKHANDLE pForkHandle)
{
    LIBCLOG_ENTER("pForkHandle=%p\n", (void *)pForkHandle);
    /*
     * Enumerate all the allocated memory objects
     * and duplicate their pages.
     */
    PDOSEXHDR   pPool = gpPoolsHead;
    while (pPool)
    {
        PDOSEX  pEntry = pPool->apHeads[DOSEX_TYPE_MEM_ALLOC];
        while (pEntry)
        {
            int rc = pForkHandle->pfnDuplicatePages(pForkHandle,
                                                    pEntry->u.MemAlloc.pv,
                                                    (char *)pEntry->u.MemAlloc.pv + pEntry->u.MemAlloc.cb,
                                                    __LIBC_FORK_FLAGS_ONLY_DIRTY | __LIBC_FORK_FLAGS_PAGE_ATTR
                                                    | __LIBC_FORK_FLAGS_ALLOC_FLAGS | (pEntry->u.MemAlloc.flFlags & __LIBC_FORK_FLAGS_ALLOC_MASK));
            if (rc)
            {
                rc = -__libc_native2errno(rc);
                LIBCLOG_ERROR_RETURN_INT(rc);
            }

            /* next entry */
            pEntry = pEntry->pNext;
        }

        /* next pool */
        pPool = pPool->pNext;
    }

    /*
     * We're done.
     */
    dosexReleaseMutex();
    gfFork = 0;

    LIBCLOG_RETURN_INT(0);
}



/**
 * Allocate memory for a pool on the child side.
 *
 * @returns 0 on success.
 * @returns -errno on failure.
 * @param   pForkHandle     Fork handle.
 * @param   pvArg           Pointer to a DOSEXFORKCHILDALLOC structure.
 * @param   cbArg           sizeof(DOSEXFORKCHILDALLOC).
 */
static int dosexForkChildAlloc(__LIBC_PFORKHANDLE pForkHandle, void *pvArg, size_t cbArg)
{
    LIBCLOG_ENTER("pForkHandle=%p pvArg=%p cbArg=%d\n", (void *)pForkHandle, pvArg, cbArg);
    int                     rc;
    PDOSEXFORKCHILDALLOC    pArg = (PDOSEXFORKCHILDALLOC)pvArg;
    LIBC_ASSERTM(cbArg == sizeof(DOSEXFORKCHILDALLOC), "cbArg=%d expected %d\n", cbArg, sizeof(DOSEXFORKCHILDALLOC));

    /*
     * Perform the allocation.
     */
    rc = DosAllocMemEx(&pArg->pv, pArg->cb, PAG_READ | PAG_WRITE | PAG_COMMIT | OBJ_LOCATION);
    if (rc)
        LIBCLOG_ERROR_RETURN_INT(-ENOMEM);
    LIBCLOG_RETURN_INT(0);
}


/**
 * Process the pools allocating all the system resources
 * recorded there.
 *
 * @returns 0 on success.
 * @returns -errno on failure.
 * @param   pForkHandle     Fork handle.
 * @param   pvArg           Pointer to the pointer to the head of the pools list.
 * @param   cbArg           sizeof(PDOSEXHDR).
 */
static int dosexForkChildProcess(__LIBC_PFORKHANDLE pForkHandle, void *pvArg, size_t cbArg)
{
    LIBCLOG_ENTER("pForkHandle=%p pvArg=%p:{%p} cbArg=%d\n", (void *)pForkHandle, pvArg, *(void **)pvArg, cbArg);
    int         rc = 0;
    PDOSEXHDR   pPoolsHead = *(PDOSEXHDR *)pvArg;
    PDOSEXHDR   pPool;
    LIBC_ASSERTM(cbArg == sizeof(PDOSEXHDR), "cbArg=%d expected %d\n", cbArg, sizeof(PDOSEXHDR));

    /*
     * Iterate the pools.
     */
    for (pPool = pPoolsHead; pPool; pPool = pPool->pNext)
    {
        PDOSEX  pEntry;
        /* Private memory. */
        for (pEntry = pPool->apHeads[DOSEX_TYPE_MEM_ALLOC];     !rc && pEntry; pEntry = pEntry->pNext)
            rc = dosexForkPreMemAlloc(pEntry);
        if (rc)
            break;

        /* Shared memory. */
        for (pEntry = pPool->apHeads[DOSEX_TYPE_MEM_OPEN];      !rc && pEntry; pEntry = pEntry->pNext)
            rc = dosexForkPreMemOpen(pEntry);
        if (rc)
            break;

        /* Mutex creation. */
        for (pEntry = pPool->apHeads[DOSEX_TYPE_MUTEX_CREATE];  !rc && pEntry; pEntry = pEntry->pNext)
            rc = dosexForkPreMutexCreate(pEntry);
        if (rc)
            break;

        /* Mutex opening. */
        for (pEntry = pPool->apHeads[DOSEX_TYPE_MUTEX_OPEN];    !rc && pEntry; pEntry = pEntry->pNext)
            rc = dosexForkPreMutexOpen(pEntry);
        if (rc)
            break;

        /* Event creation. */
        for (pEntry = pPool->apHeads[DOSEX_TYPE_EVENT_CREATE];  !rc && pEntry; pEntry = pEntry->pNext)
            rc = dosexForkPreEventCreate(pEntry);
        if (rc)
            break;

        /* Event opening. */
        for (pEntry = pPool->apHeads[DOSEX_TYPE_EVENT_OPEN];    !rc && pEntry; pEntry = pEntry->pNext)
            rc = dosexForkPreEventOpen(pEntry);
        if (rc)
            break;

        /* Module loading. */
        for (pEntry = pPool->apHeads[DOSEX_TYPE_LOAD_MODULE];   !rc && pEntry; pEntry = pEntry->pNext)
            rc = dosexForkPreLoadModule(pEntry);
        if (rc)
            break;
    }

    if (!rc)
        LIBCLOG_RETURN_INT(rc);
    LIBCLOG_ERROR_RETURN_INT(rc);
}

/**
 * Allocates the memory object described in the entry.
 *
 * @returns 0 on success.
 * @returns -errno on failure.
 * @param   pEntry  Entry containing a memory allocation.
 */
static int dosexForkPreMemAlloc(PDOSEX pEntry)
{
    int rc;
    rc = DosAllocMemEx(&pEntry->u.MemAlloc.pv, pEntry->u.MemAlloc.cb, (pEntry->u.MemAlloc.flFlags & ~OBJ_FORK) | OBJ_LOCATION);
    LIBC_ASSERTM(!rc, "DosAllocMemEx(={%p},%#lx,%#lx | OBJ_LOCATION) -> rc=%d\n",
                 pEntry->u.MemAlloc.pv, pEntry->u.MemAlloc.cb, pEntry->u.MemAlloc.flFlags, rc);
    if (rc)
        return -__libc_native2errno(rc);
    return 0;
}


/**
 * Allocates the memory object described in the entry.
 *
 * @returns 0 on success.
 * @returns -errno on failure.
 * @param   pEntry  Entry containing a memory allocation.
 */
static int dosexForkPreMemOpen(PDOSEX pEntry)
{
#ifdef PER_PROCESS_OPEN_COUNTS
    unsigned    cOpens = pEntry->u.MemOpen.cOpens;
#else
    unsigned    cOpens = 1;
#endif
    while (cOpens-- > 0)
    {
        int rc = DosGetSharedMem(pEntry->u.MemOpen.pv, pEntry->u.MemOpen.flFlags);
        LIBC_ASSERTM(!rc, "DosGetSharedMem(%p,%#lx) -> rc=%d\n", pEntry->u.MemOpen.pv, pEntry->u.MemOpen.flFlags, rc);
        if (rc)
            return -__libc_native2errno(rc);
    }
    return 0;
}


/**
 * Creates the mutex semaphore described in the entry.
 *
 * @returns 0 on success.
 * @returns -errno on failure.
 * @param   pEntry  Entry containing a memory allocation.
 */
static int dosexForkPreMutexCreate(PDOSEX pEntry)
{
    int                 i;
    int                 rc;
    DOSEXHANDLEBUNDLE   Bundle;
    PDOSEXHANDLEBUNDLE  pBundle = &Bundle;
    HMTX                hmtx = pEntry->u.MutexCreate.hmtx;
    ULONG               flFlags = pEntry->u.MutexCreate.flFlags;
    Bundle.cUsed = 0;
    Bundle.pNext = NULL;

    /*
     * Loop allocating handles till we get or pass the right one.
     *
     * This ASSUMES that OS/2 allways will give use the lowes available
     * handle. This might not be true.. (we'll find out)
     */
    for (i = 0; ; i++)
    {
        HMTX    hmtxNew = NULLHANDLE;
        rc = DosCreateMutexSem(NULL, &hmtxNew, flFlags, 0);
        if (rc)
        {
            LIBC_ASSERTM_FAILED("DosCreateMutexSem(,,%#lx,0) -> rc=%d\n", flFlags, rc);
            pBundle->cUsed = i;
            rc = -__libc_native2errno(rc);
            break;
        }
        if (hmtxNew == hmtx)
        {
            unsigned    cNestings;
            for (cNestings = pEntry->u.MutexCreate.cCurState; cNestings > 0; cNestings--)
                DosRequestMutexSem(hmtx, 0);
            pBundle->cUsed = i;
            break;
        }

        if (i >= sizeof(pBundle->ah) / sizeof(pBundle->ah[0]))
        {
            pBundle->cUsed = i;
            pBundle = pBundle->pNext = alloca(sizeof(*pBundle));
            if (!pBundle)
            {
                DosCloseMutexSem(hmtxNew);
                rc = -ENOMEM;
                break;
            }
            i = 0;
        }
        pBundle->ah[i] = hmtxNew;
    }

    /*
     * Cleanup.
     */
    for (pBundle = &Bundle; pBundle; pBundle = pBundle->pNext)
    {
        i = pBundle->cUsed;
        while (i-- > 0)
        {
            int rc2 = DosCloseMutexSem((HMTX)pBundle->ah[i]);
            LIBC_ASSERTM(!rc2, "DosCloseMutexSem(%#lx) failed rc2=%d\n", pBundle->ah[i], rc2);
            rc2 = rc2;
        }
    }

    return rc;
}


/**
 * Opens the mutex semaphore described in the entry.
 *
 * @returns 0 on success.
 * @returns -errno on failure.
 * @param   pEntry  Entry containing a memory allocation.
 */
static int dosexForkPreMutexOpen(PDOSEX pEntry)
{
#ifdef PER_PROCESS_OPEN_COUNTS
    unsigned cOpens = pEntry->u.MutexOpen.cOpens;
#else
    unsigned cOpens = 1;
#endif
    while (cOpens-- > 0)
    {
        int rc = DosOpenMutexSem(NULL, &pEntry->u.MutexOpen.hmtx);
        LIBC_ASSERTM(!rc, "DosOpenMutexSem(,%#lx) -> rc=%d\n", pEntry->u.MutexOpen.hmtx, rc);
        if (rc)
            return -__libc_native2errno(rc);
    }
    return 0;
}


/**
 * Creates the event semaphore described in the entry.
 *
 * @returns 0 on success.
 * @returns -errno on failure.
 * @param   pEntry  Entry containing a memory allocation.
 */
static int dosexForkPreEventCreate(PDOSEX pEntry)
{
    int                 i;
    int                 rc;
    DOSEXHANDLEBUNDLE   Bundle;
    PDOSEXHANDLEBUNDLE  pBundle = &Bundle;
    HEV                 hev = pEntry->u.EventCreate.hev;
    ULONG               flFlags = pEntry->u.EventCreate.flFlags;
    unsigned            cPostings  = pEntry->u.EventCreate.cCurState;
    Bundle.cUsed = 0;
    Bundle.pNext = NULL;

    /*
     * Loop allocating handles till we get or pass the right one.
     *
     * This ASSUMES that OS/2 allways will give use the lowes available
     * handle. This might not be true.. (we'll find out)
     */
    for (i = 0; ; i++)
    {
        HEV hevNew = NULLHANDLE;
        rc = DosCreateEventSem(NULL, &hevNew, flFlags, cPostings > 0);
        if (rc)
        {
            LIBC_ASSERTM_FAILED("DosCreateEventSem(,,%#lx,%d) -> rc=%d\n", flFlags, cPostings > 0, rc);
            pBundle->cUsed = i;
            rc = -__libc_native2errno(rc);
            break;
        }
        if (hevNew == hev)
        {
            for (; cPostings > 1; cPostings--)
                DosPostEventSem(hev);
            pBundle->cUsed = i;
            break;
        }

        if (i >= sizeof(pBundle->ah) / sizeof(pBundle->ah[0]))
        {
            pBundle->cUsed = i;
            pBundle = pBundle->pNext = alloca(sizeof(*pBundle));
            if (!pBundle)
            {
                DosCloseEventSem(hevNew);
                rc = -ENOMEM;
                break;
            }
            i = 0;
        }
        pBundle->ah[i] = hevNew;
    }

    /*
     * Cleanup.
     */
    for (pBundle = &Bundle; pBundle; pBundle = pBundle->pNext)
    {
        i = pBundle->cUsed;
        while (i-- > 0)
        {
            int rc2 = DosCloseEventSem((HEV)pBundle->ah[i]);
            LIBC_ASSERTM(!rc2, "DosCloseEventSem(%#lx) failed rc2=%d\n", pBundle->ah[i], rc2);
            rc2 = rc2;
        }
    }

    return rc;
}


/**
 * Opens the event semaphore described in the entry.
 *
 * @returns 0 on success.
 * @returns -errno on failure.
 * @param   pEntry  Entry containing a memory allocation.
 */
static int dosexForkPreEventOpen(PDOSEX pEntry)
{
#ifdef PER_PROCESS_OPEN_COUNTS
    unsigned cOpens = pEntry->u.EventOpen.cOpens;
#else
    unsigned cOpens = 1;
#endif
    while (cOpens-- > 0)
    {
        int rc = DosOpenEventSem(NULL, &pEntry->u.EventOpen.hev);
        LIBC_ASSERTM(!rc, "DosOpenEventSem(,%#lx) -> rc=%d\n", pEntry->u.EventOpen.hev, rc);
        if (rc)
            return -__libc_native2errno(rc);
    }
    return 0;
}


/**
 * Loads the module described in the entry.
 *
 * @returns 0 on success.
 * @returns -errno on failure.
 * @param   pEntry  Entry containing a memory allocation.
 */
static int dosexForkPreLoadModule(PDOSEX pEntry)
{
    char    szName[CCHMAXPATH];
    int     cLoads = pEntry->u.LoadModule.cLoads;

    /*
     * Figure out the name first.
     */
    int     rc = DosQueryModuleName(pEntry->u.LoadModule.hmte, sizeof(szName), szName);
    if (rc)
    {
        LIBC_ASSERTM(!rc, "DosQueryModuleName(%#lx,%d,%p) -> rc=%d \n", pEntry->u.LoadModule.hmte, sizeof(szName), szName, rc);
        if (rc)
            return -__libc_native2errno(rc);
    }

    /*
     * Perform the loads.
     */
    while (cLoads-- > 0)
    {
        HMODULE hmod = 0;
        rc = DosLoadModule(NULL, 0, (PCSZ)szName, &hmod);
        LIBC_ASSERTM(!rc, "DosLoadModule(,,'%s',) -> rc=%d (hmte=%#lx)\n", szName, rc, pEntry->u.LoadModule.hmte);
        if (rc)
            return -__libc_native2errno(rc);
    }
    return 0;
}



/**
 * Releases the mutex when fork() is done in the parent contex.
 *
 * @param   pvArg   NULL.
 * @param   rc      The fork result. This is 0 on success. On failure it is the
 *                  negative errno value.
 * @param   enmCtx  The context the completion callback function is called in.
 */
void dosexForkCleanup(void *pvArg, int rc, __LIBC_FORKCTX enmCtx)
{
    LIBCLOG_ENTER("pvArg=%p rc=%d enmCtx=%d\n", pvArg, rc, enmCtx);

    /*
     * Cleanup.
     */
    if (gfFork)
    {
        dosexReleaseMutex();
        gfFork = 0;
    }

    LIBCLOG_RETURN_VOID();
}


_FORK_CHILD1(0xffffffff, dosexForkChild)

/**
 * Fork callback which initializes DosEx in the child process.
 *
 * @returns 0 on success.
 * @returns -errno on failure.
 * @param   pForkHandle     Pointer to fork handle.
 * @param   enmOperation    Fork operation.
 */
int dosexForkChild(__LIBC_PFORKHANDLE pForkHandle, __LIBC_FORKOP enmOperation)
{
    LIBCLOG_ENTER("pForkHandle=%p enmOperation=%d\n", (void *)pForkHandle, enmOperation);
    int     rc;
    switch (enmOperation)
    {
        /*
         * After copying the data segment we must reinitialize
         * the semaphore protecting us.
         */
        case __LIBC_FORK_OP_FORK_CHILD:
            gfInited  = 0;
            gfFork    = 1;
            gmtxPools = 0;
            dosexInit();
            rc = 0;
            break;

        default:
            rc = 0;
            break;
    }

    LIBCLOG_RETURN_INT(rc);
}

