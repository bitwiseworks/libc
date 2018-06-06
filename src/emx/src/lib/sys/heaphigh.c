/* $Id: heaphigh.c 2254 2005-07-17 12:25:44Z bird $ */
/** @file
 *
 * LIBC Base Functions for high memory heap.
 * Note. High memory heap does not mess around with sbrk().
 *
 * Copyright (c) 2003 knut st. osmundsen <bird-srcspam@anduin.net>
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with This program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */


/*******************************************************************************
*   Defined Constants And Macros                                               *
*******************************************************************************/
/** The size of a chunk descriptor pool (64KB). */
#define HIMEM_POOL_SIZE         (64*1024)
/** Default chunk size (16MB). */
#define HIMEM_CHUNK_SIZE        (16*1024*1024)
/** Minimum chunk size (64KB). */
#define HIMEM_CHUNK_SIZE_MIN    (64*1024)
/** Default commit size (256KB). */
#define HIMEM_COMMIT_SIZE       HIMEM_CHUNK_SIZE_MIN

/** Round chunk size. */
#define HIMEM_ROUND_SIZE(cb, align) ( ((cb) + (align) - 1) & ~((align) - 1) )



/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include "libc-alias.h"
#define INCL_DOSMEMMGR
#define INCL_EXAPIS
#define INCL_ERRORS
#include <os2emx.h>
#include <string.h>
#include <emx/umalloc.h>
#include "syscalls.h"
#define  __LIBC_LOG_GROUP __LIBC_LOG_GRP_HEAP
#include <InnoTekLIBC/logstrict.h>


/*******************************************************************************
*   Structures and Typedefs                                                    *
*******************************************************************************/
/**
 * Descriptor for a high memory chunk.
 */
typedef struct _HighChunk
{
    /** Pointer to the next one. */
    struct _HighChunk  *pNext;
    /** Pointer to the chunk. */
    void               *pv;
    /** Size of the chunk. */
    size_t              cb;
    /** Size of the un committed memory. */
    size_t              cbUncommitted;
} HIGHCHUNK;
/** Pointer to a descriptor for a high memory chunk. */
typedef HIGHCHUNK *PHIGHCHUNK;

/**
 * A pool of chunk descriptors.
 */
typedef struct _HighChunkPool
{
    /** Pointer to the next pool in the list.*/
    struct _HighChunkPool  *pNext;
    /** List of free chunks. */
    PHIGHCHUNK              pFreeHead;
    /** Index to the next uninitialized chunk.
     * ~0 if all are initialized. */
    unsigned                iUnitialized;
    /** Size of the pool. */
    unsigned                cChunks;
    /** Array of cChunks entires. */
    HIGHCHUNK               aChunks[1];
} HIGHCHUNKPOOL;
/** Pointer to a pool of chunk descriptors. */
typedef HIGHCHUNKPOOL *PHIGHCHUNKPOOL;


/*******************************************************************************
*   Global Variables                                                           *
*******************************************************************************/
/** List of chuncks.
 * Protected by _sys_gmtxHimem. */
static PHIGHCHUNK       gpHimemHead;
/** Search hint containing NULL or the last used chunk.
 * Protected by _sys_gmtxHimem. */
static PHIGHCHUNK       gpHimemHint;
/** List of chunk pools.
 * Protected by _sys_gmtxHimem. */
static PHIGHCHUNKPOOL   gpHimemPoolHead;



/*******************************************************************************
*   Internal Functions                                                         *
*******************************************************************************/
static PHIGHCHUNK himemAllocChunk(void);
static void himemFreeChunk(PHIGHCHUNK pChunk);


/**
 * Allocates a chunk descriptor.
 * Caller must not own semaphore.
 *
 * @returns Pointer to chunk descriptor.
 *          Owner of the semaphore. Caller must free it.
 * @returns NULL on failure.
 */
static PHIGHCHUNK himemAllocChunk(void)
{
    LIBCLOG_ENTER("\n");
    PHIGHCHUNK      pChunk;
    PHIGHCHUNKPOOL  pPool;
    PVOID           pv;
    int             rc;

    /*
     * Take semaphore.
     */
    if (_fmutex_request(&_sys_gmtxHimem, _FMR_IGNINT))
        LIBCLOG_RETURN_P(NULL);

    /*
     * Walk the pool list and look for free chunks.
     */
    pChunk = NULL;
    for (pPool = gpHimemPoolHead; pPool; pPool = pPool->pNext)
    {
        if (pPool->pFreeHead)
        {
            /* Unlink free chunk. */
            pChunk = pPool->pFreeHead;
            pPool->pFreeHead = pChunk->pNext;
            pChunk->pNext = NULL;
            LIBCLOG_RETURN_P(pChunk);
        }
        if (pPool->iUnitialized != ~0)
        {
            /* We commit page by page to make fork() as fast as possible. */
            if ((((uintptr_t)&pPool->aChunks[pPool->iUnitialized]) & 0xfff) == 0)
            {
                rc = DosSetMem(&pPool->aChunks[pPool->iUnitialized], 0x1000, PAG_DEFAULT | PAG_COMMIT);
                LIBC_ASSERTM(rc, "DosSetMem(%p, 0x1000,) -> rc=%d\n", (void *)&pPool->aChunks[pPool->iUnitialized], rc);
                if (rc)
                    continue;
            }
            /* initialize a new chunk and return it. */
            pChunk = &pPool->aChunks[pPool->iUnitialized++];
            if (pPool->iUnitialized >= pPool->cChunks)
                pPool->iUnitialized = ~0;
            LIBCLOG_RETURN_P(pChunk);
        }
    }

    /*
     * We're out of chunk descriptors.
     * Allocate another pool.
     */
    rc = DosAllocMemEx(&pv, HIMEM_POOL_SIZE, PAG_WRITE  | PAG_READ | OBJ_ANY | OBJ_FORK);
    if (rc)
    {
        LIBC_ASSERTM_FAILED("Failed to allocate more chunks. rc=%d\n", rc);
        _fmutex_release(&_sys_gmtxHimem);
        LIBCLOG_ERROR_RETURN_P(NULL);
    }
    /* Commit the first page. */
    rc = DosSetMem(pv, 0x1000, PAG_DEFAULT | PAG_COMMIT);
    if (rc)
    {
        LIBC_ASSERTM_FAILED("DosSetMem(%p, 0x1000,) -> rc=%d\n", pv, rc);
        DosFreeMemEx(pv);
        _fmutex_release(&_sys_gmtxHimem);
        LIBCLOG_ERROR_RETURN_P(NULL);
    }

    /*
     * Initialize the pool, allocate the first chunk and put the pool into the lifo.
     */
    pPool = (PHIGHCHUNKPOOL)pv;
    pPool->cChunks = (HIMEM_POOL_SIZE - sizeof(HIGHCHUNKPOOL)) / sizeof(HIGHCHUNK);

    pPool->iUnitialized = 1;
    pChunk = &pPool->aChunks[0];

    pPool->pNext = gpHimemPoolHead;
    gpHimemPoolHead = pPool;

    LIBCLOG_RETURN_P(pChunk);
}


/**
 * Frees a chunk.
 * The caller must own the semaphore.
 *
 * @param   pChunk  The chunk to free.
 */
static void himemFreeChunk(PHIGHCHUNK pChunk)
{
    LIBCLOG_ENTER("pChunk=%p:{pv=%p cb=%#x}\n", (void *)pChunk, pChunk->pv, pChunk->cb);
    PHIGHCHUNKPOOL  pPool;

    /*
     * Walk the pool list and look for free chunks.
     */
    for (pPool = gpHimemPoolHead; pPool; pPool = pPool->pNext)
    {
        if (    pChunk >= &pPool->aChunks[0]
            &&  pChunk < &pPool->aChunks[pPool->cChunks])
        {
            pChunk->pv = NULL;
            pChunk->cb = 0;
            pChunk->cbUncommitted = 0;

            pChunk->pNext = pPool->pFreeHead;
            pPool->pFreeHead = pChunk;
            LIBCLOG_RETURN_VOID();
        }
    }

    LIBC_ASSERTM_FAILED("couldn't find pool which chunk %p belongs in!\n", (void *)pChunk);
    LIBCLOG_ERROR_RETURN_VOID();
}


/**
 * Heap callback function for allocating high memory.
 * We allocate a good deal of memory, but only commits the requested size.
 *
 * @returns Pointer to the allocated memory on success.
 * @returns NULL on failure.
 * @param   Heap        Heap in question.
 * @param   pcb         Input the requested block size.
 *                      Output the amount of memory actually allocated.
 * @param   pfClean     On output this will tell the call if the memory
 *                      pointed to by the returned address is clean
 *                      (i.e zeroed) or not.
 *
 * @remark  Assumes it serves one heap only ATM!
 */
void *__libc_HimemDefaultAlloc(Heap_t Heap, size_t *pcb, int *pfClean)
{
    LIBCLOG_ENTER("Heap=%p pcb=%p:{%#x} pfClean=%p\n", (void *)Heap, (void *)pcb, *pcb, (void *)pfClean);
    size_t      cbAlloc;
    size_t      cbCommit;
    PVOID       pv;
    int         rc;
    PHIGHCHUNK  pChunk;


    if (!gpHimemPoolHead)
    {
        /*
         * The first time we force the allocation of the chunk decriptor
         * pool before we allocate the actual chunk to optimize the address space.
         */
        pChunk = himemAllocChunk();
        if (pChunk)
        {
            himemFreeChunk(pChunk);
            _fmutex_release(&_sys_gmtxHimem);
        }
    }
    else
    {
        /*
         * Check if we have a chunk we can expand to satisfy the request.
         */
        if (_fmutex_request(&_sys_gmtxHimem, _FMR_IGNINT))
            LIBCLOG_RETURN_P(NULL);
        cbCommit = *pcb;
        cbCommit = HIMEM_ROUND_SIZE(cbCommit, HIMEM_COMMIT_SIZE);
        pChunk = gpHimemHint;
        if (!pChunk)
            pChunk = gpHimemHead;
        for (; pChunk; pChunk = pChunk->pNext)
        {
            if (pChunk->cbUncommitted >= cbCommit)
            {
                /* commit the rest if it's less than the minimum commit size. */
                if (pChunk->cbUncommitted - cbCommit < HIMEM_COMMIT_SIZE)
                    cbCommit = pChunk->cbUncommitted;

                /*
                 * commit the lump.
                 */
                pv = (char *)pChunk->pv + pChunk->cb - pChunk->cbUncommitted;
                rc = DosSetMem(pv, cbCommit, PAG_DEFAULT | PAG_COMMIT);
                if (rc)
                {   /* page by page */
                    void   *pvCom = pv;
                    int     cbCom = (int)cbCommit;
                    for (rc = 0; cbCom > 0; cbCom -= 0x1000, pv = (char *)pv + 0x1000)
                    {
                        int rc2 = DosSetMem(pvCom, 0x1000, PAG_DEFAULT | PAG_COMMIT);
                        LIBC_ASSERTM(!rc2, "DosSetMem(%p, 0x1000, commit) -> %d\n", pvCom, rc2);
                        if (rc2)
                            rc = rc2;
                    }
                }

                if (!rc)
                {
                    pChunk->cbUncommitted -= cbCommit;
                    gpHimemHint = pChunk->cbUncommitted ? pChunk : gpHimemHead;
                    _fmutex_release(&_sys_gmtxHimem);

                    /* return pv and commit size. The heap takes care of joining
                     * it with the earlier part of the block. ASSUMES ONE HEAP!!! */
                    *pcb = cbCommit;
                    LIBCLOG_RETURN_MSG(pv, "ret %p *pcb=%#x\n", pv, *pcb);
                }
                LIBC_ASSERTM_FAILED("DosSetMem(%p, %#x, commit) -> %d\n", pv, cbCommit, rc);
                continue;
            }
        }

        /* Out of luck, allocate a new chunk. */
        _fmutex_release(&_sys_gmtxHimem);
    }


    /*
     * Allocate a (rather big) memory block, there is generally speaking
     * more than enough high memory to allocate from. So, we'll round the
     * chunks sizes up quite a bit to keep the object count low and the
     * heap as flexible as possible.
     */
    cbAlloc = *pcb;
    cbAlloc = HIMEM_ROUND_SIZE(cbAlloc, HIMEM_CHUNK_SIZE);
    rc = DosAllocMemEx(&pv, cbAlloc, PAG_READ | PAG_WRITE | OBJ_ANY | OBJ_FORK);
    if (rc == ERROR_NOT_ENOUGH_MEMORY)
    {
        /*
         * That's odd, we're out of address space or something.
         * Try again with the minimum rounding.
         */
        cbAlloc = *pcb + sizeof(HIGHCHUNK);
        cbAlloc = HIMEM_ROUND_SIZE(cbAlloc, HIMEM_CHUNK_SIZE_MIN);
        rc = DosAllocMemEx(pv, cbAlloc, PAG_READ | PAG_WRITE | OBJ_ANY | OBJ_FORK);
    }
    if (rc)
    {
        LIBC_ASSERTM_FAILED("Failed to allocate chunk! rc=%d cbAlloc=%d *pcb=%d\n", rc, cbAlloc, *pcb);
        LIBCLOG_ERROR_RETURN_P(NULL);
    }

    /*
     * Commit the requested memory size.
     */
    cbCommit = *pcb;
    cbCommit = HIMEM_ROUND_SIZE(cbCommit, HIMEM_COMMIT_SIZE);
    rc = DosSetMem(pv, cbCommit, PAG_DEFAULT | PAG_COMMIT);
    if (!rc)
    {
        /*
         * Allocate a chunk descriptor taking in the heap semaphore in the same run.
         */
        pChunk = himemAllocChunk();
        if (pChunk)
        {
            /* init */
            pChunk->pv          = pv;
            pChunk->cb          = cbAlloc;
            pChunk->cbUncommitted = cbAlloc - cbCommit;
            /* link in to the list. */
            pChunk->pNext = gpHimemHead;
            gpHimemHead   = pChunk;
            gpHimemHint   = pChunk;

            /* release and return. */
            _fmutex_release(&_sys_gmtxHimem);
            *pcb = cbCommit;
            *pfClean = _BLOCK_CLEAN;
            LIBCLOG_RETURN_MSG(pv, "ret %p *pcb=%#x\n", pv, *pcb);
        }
    }
    else
        LIBC_ASSERTM_FAILED("DosSetMem(%p, %#x, PAG_DEFAULT | PAG_COMMIT) -> %d\n", pv, cbCommit, rc);

    DosFreeMemEx(pv);
    LIBCLOG_ERROR_RETURN_P(NULL);
}


/**
 * Heap callback function for releaseing high memory.
 *
 * @param   Heap    Heap in question.
 * @param   pv      Pointer to block.
 * @param   cb      Size of block.
 */
void __libc_HimemDefaultRelease(Heap_t Heap, void *pv, size_t cb)
{
    LIBCLOG_ENTER("Heap=%p pv=%p cb=%#x\n", (void *)Heap, pv, cb);
    int         rc;
    PHIGHCHUNK  pChunk;
    PHIGHCHUNK  pPrevChunk;

    LIBC_ASSERT(cb);
    LIBC_ASSERT(!(cb & 0xfff));
    LIBC_ASSERT(pv);
    LIBC_ASSERT(!((uintptr_t)pv & 0xfff));


    /*
     * Take semaphore.
     */
    if (_fmutex_request(&_sys_gmtxHimem, _FMR_IGNINT) != 0)
        return;

    /*
     * Remove from top to bottom of the described block.
     *
     * We must (?) handle cases where the pv+cb describes a memory area
     * which covers several chunks. This is easier when done from the end.
     *
     * We ASSUME that the heap will not request areas which is not at the
     * end of the committed memory in a chunk.
     */
    do
    {
        void       *pvEnd = (char *)pv + cb;
        for (pChunk = gpHimemHead, pPrevChunk = NULL; pChunk; pPrevChunk = pChunk, pChunk = pChunk->pNext)
        {
            size_t  offEnd = (uintptr_t)pvEnd - (uintptr_t)pChunk->pv;
            if (offEnd <= pChunk->cb)
            {
                void   *pvFree;
                size_t  off = (uintptr_t)pv - (uintptr_t)pChunk->pv;
                if (off > pChunk->cb)
                    off = 0;

                /* check that it's at the end of the committed area. */
                if (offEnd != pChunk->cb - pChunk->cbUncommitted)
                {
                    LIBC_ASSERTM_FAILED("Bad high heap release!! pv=%p cb=%#x off=%#x offEnd=%#x; chunk pv=%p cb=%#x cbUncomitted=%#x\n",
                                        pv, cb, off, offEnd, pChunk->pv, pChunk->cb, pChunk->cbUncommitted);
                    _fmutex_release(&_sys_gmtxHimem);
                    LIBCLOG_ERROR_RETURN_VOID();
                }

                /*
                 * Decommit part of the chunk.
                 */
                if (off > 0)
                {
                    size_t  cbDecommit = offEnd - off;
                    rc = DosSetMem(pv, cbDecommit, PAG_DECOMMIT);
                    if (rc)
                    {
                        LIBC_ASSERTM_FAILED("DosSetMem(%p, %#x, decommit) -> %d\n", pv, cbDecommit, rc);
                        /* page by page */
                        for (; cbDecommit; cbDecommit -= 0x1000)
                            DosSetMem(pv, 0x1000, PAG_DECOMMIT);
                    }
                    pChunk->cbUncommitted += cbDecommit;

                    /* we're done. */
                    _fmutex_release(&_sys_gmtxHimem);
                    LIBCLOG_RETURN_VOID();
                }

                /*
                 * Free the chunk.
                 */
                pvFree = pChunk->pv;
                /* unlink and free the chunk descriptor */
                if (pPrevChunk)
                    pPrevChunk->pNext = pChunk->pNext;
                else
                    gpHimemHead = pChunk->pNext;
                if (gpHimemHint == pChunk)
                    gpHimemHint = gpHimemHead;
                himemFreeChunk(pChunk);

                /* free */
                rc = DosFreeMemEx(pvFree);
                LIBC_ASSERTM(!rc, "DosFreeMem(%p) -> %d\n", pvFree, rc);

                /* Update size and restart loop. */
                cb -= offEnd - off;
                break;
            }
        }

        LIBC_ASSERTM(pChunk, "Couldn't find any chunk containing the area pv=%p cb=%#x!\n", pv, cb);
    } while (cb && pChunk);

    _fmutex_release(&_sys_gmtxHimem);
    LIBCLOG_RETURN_VOID();
}


#if 0
int     __libc_HimemDefaultExpand(Heap_t Heap, void *pvBase, size_t cbOld, size_t *pcbNew, int *pfClean)
{
    LIBCLOG_ENTER("Heap=%p pvBase=%p cbOld=%#x pcbNew=%p:{%#x} pfClean=%p\n",
                  (void *)Heap, pvBase, cbOld, (void *)pcbNew, *pcbNew, (void *)pfClean);


    LIBCLOG_RETURN_INT(0);
}

void    __libc_HimemDefaultShrink(Heap_t Heap, void *pvBase, size_t cbOld, size_t *pcbNew)
{
    LIBCLOG_ENTER("Heap=%p pvBase=%p cbOld=%#x pcbNew=%p:{%#x} pfClean=%p\n",
                  (void *)Heap, pvBase, cbOld, (void *)pcbNew, *pcbNew, (void *)pfClean);

    LIBCLOG_RETURN_VOID(0);
}
#endif


/**
 * Query if the system have support for high memory.
 *
 * @returns 1 if the system have more than 512MB of user address space.
 * @returns 0 if the system only have 512MB of user address space.
 */
int     __libc_HasHighMem(void)
{
    return _sys_gcbVirtualAddressLimit > 512*1024*1024;
}

