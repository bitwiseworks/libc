/* $Id: filehandles.c 3373 2007-05-27 11:03:27Z bird $ */
/** @file
 *
 * LIBC File Handles.
 *
 * Copyright (c) 2003-2004 knut st. osmundsen <bird-srcspam@anduin.net>
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
*   Defined Constants And Macros                                               *
*******************************************************************************/
/** Maximum number of file handles. */
#define __LIBC_MAX_FHS      10000
/** Number of file handles to increase the max value with. */
#define __LIBC_INC_FHS      64


/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#define INCL_BASE
#define INCL_ERRORS
#define INCL_FSMACROS
#include <os2.h>
#include "libc-alias.h"
#include <malloc.h>
#include <string.h>
#include <sys/fcntl.h>
#include <errno.h>
#include <InnoTekLIBC/fork.h>
#include <InnoTekLIBC/sharedpm.h>
#include <InnoTekLIBC/tcpip.h>
#include <emx/io.h>
#include <emx/umalloc.h>
#include "syscalls.h"
#include "b_fs.h"
#include "b_dir.h"

#include <InnoTekLIBC/backend.h>
#define __LIBC_LOG_GROUP    __LIBC_LOG_GRP_INITTERM
#include <InnoTekLIBC/logstrict.h>


/*******************************************************************************
*   Global Variables                                                           *
*******************************************************************************/
/** Mutex semaphor protecting all the global data.
 * @todo This should've been a read/write lock (multiple read, exclusive write).
 */
static _fmutex              gmtx;
/** Array of file handle pointers. */
static __LIBC_PFH          *gpapFHs;
/** Number of entires in the file handle table. */
static unsigned             gcFHs;

/** Array of preallocated handles - normal files only! */
static LIBCFH               gaPreAllocated[40];
/** Number of free handles in the preallocated array. */
static unsigned             gcPreAllocatedAvail;

/** Indicator whether or not inherit flags and other stuff have been cleaned
 * up after fork. */
static int                  gfForkCleanupDone;

extern int _fmode_bin;


/*******************************************************************************
*   Internal Functions                                                         *
*******************************************************************************/
static int      fhMoreHandles(unsigned cMin);
static int      fhAllocate(int fh, unsigned fFlags, int cb, __LIBC_PCFHOPS pOps, __LIBC_PFH *ppFH, int *pfh, int fOwnSem);
static void     fhFreeHandle(__LIBC_PFH pFH);
static __LIBC_PFH  fhGet(int fh);
static int      fhForkParent1(__LIBC_PFORKHANDLE pForkHandle, __LIBC_FORKOP enmOperation);
static int      fhForkChild1(__LIBC_PFORKHANDLE pForkHandle, __LIBC_FORKOP enmOperation);
static void     fhForkCompletion(void *pvArg, int rc, __LIBC_FORKCTX enmCtx);


/**
 * Initiates the filehandle structures.
 *
 * @returns 0 on success.
 * @returns -1 on failure.
 */
int __libc_fhInit(void)
{
    __LIBC_PSPMINHERIT  pInherit;
    ULONG               cMaxFHs = 0;
    LONG                lDeltaFHs = 0;
    int                 rc;
    int                 i;

    /*
     * Only once.
     */
    if (gcFHs)
    {
        LIBC_ASSERTM_FAILED("already initialized!\n");
        return 0;
    }

    /*
     * Init the mutex which should've been a read write lock.
     */
    rc = _fmutex_create2(&gmtx, _FMC_MUST_COMPLETE, "LIBC SYS Filehandle Mutex");
    if (rc)
        return rc;

    /*
     * Figure out the size of the pointer array and initiatlize it.
     */
    rc = DosSetRelMaxFH(&lDeltaFHs, &cMaxFHs);
    if (rc)
    {
        LIBC_ASSERTM_FAILED("DosSetRelMaxFH failed with rc=%d when querying current values\n", rc);
        cMaxFHs = 20;
    }
    gcFHs = (cMaxFHs + 3) & ~3;     /* round up to nearest 4. */
    gpapFHs = _hcalloc(gcFHs, sizeof(gpapFHs[0]));
    if (!gpapFHs)
        return -1;

    gcPreAllocatedAvail = sizeof(gaPreAllocated) / sizeof(gaPreAllocated[0]);

    /*
     * Check if we inherited flags and stuff from parent.
     */
    pInherit = __libc_spmInheritRequest();
    if (!pInherit || !pInherit->pFHBundles)
    {
        /*
         * Work thru the handles checking if they are in use.
         * Was previously done in init_files() in startup.c.
         *      We need to allocate the handles and initialize them accordingly.
         * Bird Feb 12 2004 9:12pm: We do not have to do more than the first 3 handles.
         *      Other open files will be automatically added when __libc_FH() is called
         *      for usage validation in any of LIBC apis.
         */
        if (pInherit)
            __libc_spmInheritRelease();
        if (cMaxFHs > 3)
            cMaxFHs = 3;
        for (i = 0; i < cMaxFHs; i++)
            fhGet(i);
    }
    else
    {
        /*
         * Process the inherit data passed to us.
         */
        union
        {
            __LIBC_PSPMINHFHBHDR    pHdr;
            __LIBC_PSPMINHFHBSTD    pStds;
            __LIBC_PSPMINHFHBDIR    pDirs;
            __LIBC_PSPMINHFHBSOCK   pSockets;
            uintptr_t               u;
            void                   *pv;
        } u;
        u.pHdr = pInherit->pFHBundles;
        while (u.pHdr->uchType != __LIBC_SPM_INH_FHB_TYPE_END)
        {
            unsigned    i = 0;
            unsigned    c = u.pHdr->cHandles;
            unsigned    iFH = u.pHdr->StartFH;
            switch (u.pHdr->uchType)
            {
                case __LIBC_SPM_INH_FHB_TYPE_STANDARD:
                    for (i = 0; i < c; i++, iFH++)
                    {
                        __LIBC_PFH pFH;
                        if (fhAllocate(iFH, u.pStds->aHandles[i].fFlags, sizeof(__LIBC_FH), NULL, &pFH, NULL, 0))
                        {
                            __libc_Back_panic(0, NULL, "Failed to allocated inherited file handle! iFH=%d\n", iFH);
                            __libc_spmInheritRelease();
                            return -1;
                        }
                        pFH->Inode = u.pStds->aHandles[i].Inode;
                        pFH->Dev = u.pStds->aHandles[i].Dev;
                        pFH->pFsInfo = __libc_back_fsInfoObjByDev(pFH->Dev);
                        pFH->pszNativePath = u.pStds->aHandles[i].offNativePath
                            ? _hstrdup(pInherit->pszStrings + u.pStds->aHandles[i].offNativePath) : NULL;
                    }
                    u.pv = &u.pStds->aHandles[c];
                    break;

                case __LIBC_SPM_INH_FHB_TYPE_DIRECTORY:
                    for (i = 0; i < c; i++, iFH++)
                    {
                        const char *pszPath = u.pDirs->aHandles[i].offNativePath ? pInherit->pszStrings + u.pDirs->aHandles[i].offNativePath : NULL;
                        int rc = __libc_back_dirInherit(iFH, pszPath, u.pDirs->aHandles[i].fInUnixTree,
                                                        u.pDirs->aHandles[i].fFlags, u.pDirs->aHandles[i].Inode,
                                                        u.pDirs->aHandles[i].Dev, u.pDirs->aHandles[i].uCurEntry);
                        if (rc)
                        {
                            __libc_Back_panic(0, NULL, "Failed to allocated inherited directory handle! rc=%d iFH=%d path=%s\n", rc, iFH, pszPath);
                            __libc_spmInheritRelease();
                            return -1;
                        }
                    }
                    u.pv = &u.pDirs->aHandles[c];
                    break;

                case __LIBC_SPM_INH_FHB_TYPE_SOCKET_44:
                    for (i = 0; i < c; i++, iFH++)
                    {
                        int rc = TCPNAMEG44(AllocFHEx)(iFH, u.pSockets->aHandles[i].usSocket, u.pSockets->aHandles[i].fFlags, 0, NULL, NULL);
                        if (rc)
                        {
                            __libc_Back_panic(0, NULL, "Failed to allocated inherited socket (4.4) handle! rc=%d iFH=%d iSocket=%d\n",
                                              rc, iFH, u.pSockets->aHandles[i].usSocket);
                            __libc_spmInheritRelease();
                            return -1;
                        }
                    }
                    u.pv = &u.pSockets->aHandles[c];
                    break;

                case __LIBC_SPM_INH_FHB_TYPE_SOCKET_43:
                    for (i = 0; i < c; i++, iFH++)
                    {
                        int rc = TCPNAMEG43(AllocFHEx)(iFH, u.pSockets->aHandles[i].usSocket, u.pSockets->aHandles[i].fFlags, 0, NULL, NULL);
                        if (rc)
                        {
                            __libc_Back_panic(0, NULL, "Failed to allocated inherited socket (4.3) handle! rc=%d iFH=%d iSocket=%d\n",
                                              rc, iFH, u.pSockets->aHandles[i].usSocket);
                            __libc_spmInheritRelease();
                            return -1;
                        }
                    }
                    u.pv = &u.pSockets->aHandles[c];
                    break;

                /*
                 * Unknown - skip it.
                 */
                default:
                {
                    size_t cb = __LIBC_SPM_INH_FHB_SIZE(u.pHdr->uchType);
                    cb *= u.pHdr->cHandles;
                    u.u += cb + sizeof(*u.pHdr);
                    break;
                }

            }
        }
        __libc_spmInheritRelease();
    }

    return 0;
}

#if 0 // not used
/**
 * Terminate the file handles.
 *
 * @returns 0 on success
 * @returns -1 on failure (what ever that means).
 */
int     _sys_term_filehandles(void)
{
    if (!gcFHs)
    {
        LIBC_ASSERTM_FAILED("gcFHs is zero - we've already been terminated!\n");
        return 0;
    }
    gcFHs = 0;

    /*
     * Don't take the mutex as we might be in an crashing/hung process.
     * no need to make matters worse by potentially waiting for ever here.
     */
    _fmutex_close(&gmtx);
    bzero(&gmtx, sizeof(gmtx));

    free(gpapFHs);
    gpapFHs = NULL;

    return 0;
}
#endif


/**
 * Adds a string to the string table.
 */
static inline unsigned fhAddString(const char *pszString, char **ppszStrings, size_t *pcbAlloc, size_t *pcb)
{
    if (!pszString)
        return 0;

    size_t      cb  = strlen(pszString) + 1;
    unsigned    off = *pcb;

    /* need more space? */
    if (cb + off > *pcbAlloc)
    {
        size_t cbAlloc = (cb + off + !off + 0xfff) & ~0xfff;
        void *pv = _hrealloc(*ppszStrings, cbAlloc);
        if (!pv)
            return 0;
        *ppszStrings = (char *)pv;
        *pcbAlloc = cbAlloc;
        if (!off)
        {
            *pcb = off = 1;
            **ppszStrings = '\0';
        }
    }

    /* copy */
    memcpy(*ppszStrings + off, pszString, cb);
    *pcb += cb;
    return off;
}


/**
 * Pack down file handles for exec.
 *
 * @returns Pointer to a high heap buffer of the size indicated in *pch containing
 *          the bundles for all the current file handles. The filehandle semaphore
 *          is also owned. Called __libc_fhInheritDone() to release the semaphore.
 * @returns NULL and errno on failure.
 * @param   pcb     Where to store the size of the returned data.
 */
__LIBC_PSPMINHFHBHDR    __libc_fhInheritPack(size_t *pcb, char **ppszStrings, size_t *pcbStrings)
{
    LIBCLOG_ENTER("pcb=%p\n", (void *)pcb);
    size_t                  cbStringAlloc = *pcbStrings;
    size_t                  cb = 0x2000;
    unsigned                iFH;
    unsigned                cFHs;
    int                     rc;
    __LIBC_PSPMINHFHBHDR    pRet;
    union
    {
        __LIBC_PSPMINHFHBHDR    pHdr;
        __LIBC_PSPMINHFHBDIR    pDirs;
        __LIBC_PSPMINHFHBSTD    pStds;
        __LIBC_PSPMINHFHBSOCK   pSockets;
        uintptr_t               u;
        void                   *pv;
    } u;

    /*
     * Allocate buffers.
     */
    LIBC_ASSERT(cb > sizeof(u.pDirs->aHandles[0]) * 256 + sizeof(*u.pDirs));
    pRet = _hmalloc(cb);
    if (!pRet)
        LIBCLOG_ERROR_RETURN_P(NULL);

    /*
     * Lock the filehandle array.
     */
    rc = _fmutex_request(&gmtx, 0);
    if (rc)
    {
        _sys_set_errno(rc);
        free(pRet);
        LIBCLOG_ERROR_RETURN_P(NULL);
    }

    /*
     * Enumerate the handles.
     * UGLY!!!!!
     */
    for (cFHs = gcFHs, iFH = 0, u.pHdr = pRet; iFH < cFHs;)
    {
        __LIBC_FHTYPE   enmType;
        unsigned        i;

        /*
         * Skip unused and close-on-exec ones.
         */
        while (     iFH < cFHs
               &&   (   !gpapFHs[iFH]
                     || (gpapFHs[iFH]->fFlags & ((FD_CLOEXEC << __LIBC_FH_FDFLAGS_SHIFT) | O_NOINHERIT)) ) )
            iFH++;
        if (iFH >= cFHs)
            break;

        /*
         * Create new bundle.
         */
        /* Ensure space for max sized bundle and termination record. */
        if ( ((uintptr_t)&u.pDirs->aHandles[256] + sizeof(__LIBC_SPMINHFHBHDR) - (uintptr_t)pRet) > cb )
        {
            void *pv = realloc(pRet, cb + 0x2000);
            if (__predict_false(!pv))
            {
                _fmutex_release(&gmtx);
                free(pRet);
                LIBCLOG_ERROR_RETURN_P(NULL);
            }
            u.u += (uintptr_t)pv - (uintptr_t)pv;
            pRet = pv;
            cb += 0x2000;
        }
        /* init the bundle. */
        enmType = gpapFHs[iFH]->pOps ? gpapFHs[iFH]->pOps->enmType : enmFH_File;
        switch (enmType)
        {
            case enmFH_File:
            {
                u.pStds->Hdr.uchType = __LIBC_SPM_INH_FHB_TYPE_STANDARD;
                u.pStds->Hdr.StartFH = iFH;
                /* walk file handles. */
                for (i = 0; ; )
                {
                    u.pStds->aHandles[i].fFlags = gpapFHs[iFH]->fFlags;
                    u.pStds->aHandles[i].Inode  = gpapFHs[iFH]->Inode;
                    u.pStds->aHandles[i].Dev    = gpapFHs[iFH]->Dev;
                    u.pStds->aHandles[i].offNativePath = fhAddString(gpapFHs[iFH]->pszNativePath, ppszStrings, &cbStringAlloc, pcbStrings);
                    /* next */
                    i++; iFH++;
                    if (    i >= 255
                        ||  iFH >= cFHs
                        ||  !gpapFHs[iFH]
                        ||  gpapFHs[iFH]->pOps
                        || (gpapFHs[iFH]->fFlags & ((FD_CLOEXEC << __LIBC_FH_FDFLAGS_SHIFT) | O_NOINHERIT)) )
                        break;
                }
                /* commit the bundle. */
                u.pStds->Hdr.cHandles = i;
                u.pv = &u.pStds->aHandles[i];
                break;
            }

            case enmFH_Directory:
            {
                u.pDirs->Hdr.uchType = __LIBC_SPM_INH_FHB_TYPE_DIRECTORY;
                u.pDirs->Hdr.StartFH = iFH;
                /* walk file handles. */
                for (i = 0; ; )
                {
                    u.pDirs->aHandles[i].fFlags        = gpapFHs[iFH]->fFlags;
                    u.pDirs->aHandles[i].Inode         = gpapFHs[iFH]->Inode;
                    u.pDirs->aHandles[i].Dev           = gpapFHs[iFH]->Dev;
                    LIBC_ASSERT(gpapFHs[iFH]->pszNativePath && *gpapFHs[iFH]->pszNativePath);
                    u.pDirs->aHandles[i].offNativePath = fhAddString(gpapFHs[iFH]->pszNativePath, ppszStrings, &cbStringAlloc, pcbStrings);
                    if (__predict_false(!u.pDirs->aHandles[i].offNativePath))
                    {
                        _fmutex_release(&gmtx);
                        free(pRet);
                        LIBCLOG_ERROR_RETURN_P(NULL);
                    }
                    u.pDirs->aHandles[i].fInUnixTree   = !!((__LIBC_PFHDIR)gpapFHs[iFH])->fInUnixTree;
                    u.pDirs->aHandles[i].uCurEntry     = ((__LIBC_PFHDIR)gpapFHs[iFH])->uCurEntry;
                    /* next */
                    i++; iFH++;
                    if (    i >= 255
                        ||  iFH >= cFHs
                        ||  !gpapFHs[iFH]
                        ||  !gpapFHs[iFH]->pOps
                        ||  gpapFHs[iFH]->pOps->enmType != enmType
                        ||  (gpapFHs[iFH]->fFlags & ((FD_CLOEXEC << __LIBC_FH_FDFLAGS_SHIFT) | O_NOINHERIT)) )
                        break;
                }
                /* commit the bundle. */
                u.pStds->Hdr.cHandles = i;
                u.pv = &u.pDirs->aHandles[i];
                break;
            }

            case enmFH_Socket43:
            case enmFH_Socket44:
            {
                u.pSockets->Hdr.uchType = enmType == enmFH_Socket44
                    ? __LIBC_SPM_INH_FHB_TYPE_SOCKET_44 : __LIBC_SPM_INH_FHB_TYPE_SOCKET_43;
                u.pSockets->Hdr.StartFH = iFH;
                /* walk file handles. */
                for (i = 0; ; )
                {
                    u.pSockets->aHandles[i].fFlags   = gpapFHs[iFH]->fFlags;
                    u.pSockets->aHandles[i].usSocket = ((PLIBCSOCKETFH)gpapFHs[iFH])->iSocket;
                    /* next */
                    i++; iFH++;
                    if (    i >= 255
                        ||  iFH >= cFHs
                        ||  !gpapFHs[iFH]
                        ||  !gpapFHs[iFH]->pOps
                        ||  gpapFHs[iFH]->pOps->enmType != enmType
                        ||  (gpapFHs[iFH]->fFlags & ((FD_CLOEXEC << __LIBC_FH_FDFLAGS_SHIFT) | O_NOINHERIT)) )
                        break;
                }
                /* commit the bundle. */
                u.pSockets->Hdr.cHandles = i;
                u.pv = &u.pSockets->aHandles[i];
                break;
            }

            default:
            {   /* skip */
                for (;;)
                {
                    /* next */
                    iFH++;
                    if (    iFH >= cFHs
                        ||  !gpapFHs[iFH]
                        ||  !gpapFHs[iFH]->pOps
                        ||  (gpapFHs[iFH]->pOps->enmType != enmType))
                        break;
                }
                break;
            }

        } /* switch handle type. */

    } /* outer loop */

    /*
     * Done. Let's add a termination bundle and calc the actual size.
     */
    u.pHdr->uchType  = __LIBC_SPM_INH_FHB_TYPE_END;
    u.pHdr->cHandles = 0;
    u.pHdr->StartFH  = ~0;
    cb = (uintptr_t)(u.pHdr + 1) - (uintptr_t)pRet;
    *pcb = (cb + 3) & ~3;               /* (This is safe.) */
    LIBCLOG_RETURN_MSG(pRet, "ret %p *pcb=%d\n", (void *)pRet, *pcb);
}


/**
 * Called as a response to a successful __libc_fhInheritPack() to
 * release the file handle mutex.
 */
void        __libc_fhInheritDone(void)
{
    _fmutex_release(&gmtx);
}


/**
 * Called as a response to a successful exec so we can close
 * all open files.
 */
void        __libc_fhExecDone(void)
{
    LIBCLOG_ENTER("");

    /*
     * This isn't thread safe, but there shouldn't be any threads
     * around so it'll have to do for now.
     */
    unsigned iFH;
    for (iFH = 0; iFH < gcFHs; iFH++)
    {
        __LIBC_PFH pFH = gpapFHs[iFH];
        if (pFH)
            __libc_FHClose(iFH);
    }

    LIBCLOG_RETURN_VOID();
}



#undef  __LIBC_LOG_GROUP
#define __LIBC_LOG_GROUP    __LIBC_LOG_GRP_BACK_IO


/**
 * Opens the dummy device for non OS/2 file handles.
 *
 * @returns 0 on success.
 * @returns OS/2 error on failure.
 * @param   fh  Filehandle to open the dummy device for.
 *              Use -1 if no special handle is requested.
 * @param   pfh Where to store the opened fake handle.
 */
static int fhOpenDummy(int fh, int *pfh)
{
    int     rc;
    ULONG   ulAction;
    HFILE   hFile = fh;
    FS_VAR();

    /*
     * Open the NUL device.
     * If we're out of handle space, try increase it.
     */
    FS_SAVE_LOAD();
    for (;;)
    {
        rc = DosOpen((PCSZ)"\\DEV\\NUL", &hFile, &ulAction, 0, FILE_NORMAL,
                     OPEN_ACTION_FAIL_IF_NEW | OPEN_ACTION_OPEN_IF_EXISTS,
                     OPEN_SHARE_DENYNONE | OPEN_ACCESS_WRITEONLY,
                     NULL);
        if (rc != ERROR_TOO_MANY_OPEN_FILES)
            break;
        rc = fhMoreHandles(0);
        if (rc)
            break;
    }
    if (!rc)
    {
        /*
         * If the request was for a specific handle, we might have to dup it.
         */
        if (fh != -1 && hFile != fh)
        {
            HFILE hDup = fh;
            rc = DosDupHandle(hFile, &hDup);
            if (!rc)
                *pfh = hDup;
            DosClose(hFile);
        }
        else
            *pfh = hFile;
    }
    FS_RESTORE();
    return rc;
}


/**
 * Frees handle data.
 *
 * @param   pFH Pointer to the handle to be freed.
 * @param   fh  Handle number which the data used to be associated with.
 *              Use -1 if it doesn't apply.
 * @remark  Must own the mutex upon entry!
 */
static void fhFreeHandle(__LIBC_PFH pFH)
{
    /*
     * Free and zero resources referenced by the handle.
     */
    pFH->fFlags     = 0;
    pFH->iLookAhead = 0;
    pFH->pOps       = NULL;
    pFH->Dev        = 0;
    pFH->Inode      = 0;
    if (pFH->pFsInfo)
    {
        __libc_back_fsInfoObjRelease(pFH->pFsInfo);
        pFH->pFsInfo = NULL;
    }
    if (pFH->pszNativePath)
    {
        free(pFH->pszNativePath);
        pFH->pszNativePath = NULL;
    }

    /*
     * Free the handle it self.
     */
    if (    pFH >= &gaPreAllocated[0]
        &&  pFH <  &gaPreAllocated[sizeof(gaPreAllocated) / sizeof(gaPreAllocated[0])])
        gcPreAllocatedAvail++;
    else
        free(pFH);
}


/**
 * Try make room for more handles.
 *
 * @returns 0 on success.
 * @returns OS/2 error code on failure.
 * @param   cMin    Minimum number for the new max handle count.
 * @remark  Lock must be owned upon entry!
 */
static int fhMoreHandles(unsigned cMin)
{
    int     rc;
    ULONG   cNewMaxFHs = gcFHs;
    LONG    lDeltaFHs;
    FS_VAR();

    /*
     * How many handles?
     * Now, we must be somewhat sensibel about incrementing this number
     * into unreasonable values. If an application requires __LIBC_MAX_FHS+
     * files it must take steps to tell that to OS/2. If he has we'll receive
     * cMin > __LIBC_MAX_FHS, else keep __LIBC_MAX_FHS as a max limit for
     * auto increase.
     */
    lDeltaFHs = 0;
    cNewMaxFHs = 0;
    FS_SAVE_LOAD();
    if (    DosSetRelMaxFH(&lDeltaFHs, &cNewMaxFHs)
        ||  cNewMaxFHs < gcFHs)
        cNewMaxFHs = gcFHs;

    if (cMin < cNewMaxFHs)
    {   /* auto increment. */
        cMin = cNewMaxFHs + __LIBC_INC_FHS;
        if (cMin > __LIBC_MAX_FHS)
        {
            if (gcFHs >= __LIBC_MAX_FHS)
            {
                FS_RESTORE();
                return ERROR_TOO_MANY_OPEN_FILES;
            }
            cMin = __LIBC_MAX_FHS;
        }
    }
    lDeltaFHs = cMin - cNewMaxFHs;

    /*
     * Now to the actual increment.
     * cMax will at this point hold the disired new max file handle count.
     * Only we must take precautions in case someone have secretly done
     * this without our knowledge.
     */
    rc = DosSetRelMaxFH(&lDeltaFHs, &cNewMaxFHs);
    if (!rc && cMin > cNewMaxFHs)
        rc = DosSetMaxFH((cNewMaxFHs = cMin));
    FS_RESTORE();
    if (!rc)
    {
        /*
         * Reallocate the array of handle pointers.
         */
        __LIBC_PFH *papNewFHs = _hrealloc(gpapFHs, cNewMaxFHs * sizeof(gpapFHs[0]));
        if (papNewFHs)
        {
            gpapFHs = papNewFHs;
            bzero(&gpapFHs[gcFHs], (cNewMaxFHs - gcFHs) * sizeof(gpapFHs[0]));
            gcFHs = cNewMaxFHs;
        }
    }

    return rc;
}


/**
 * Ensures space for the given handle.
 *
 * @returns 0 on success.
 * @returns OS/2 error code on failure.
 * @param   fh      Filehandle which must fit.
 */
int __libc_FHEnsureHandles(int fh)
{
    int rc;
    if (fh < (int)gcFHs)
    {
        /*
         * This holds as long as no fool have been changing the max FH
         * outside our control. And if someone does, they have themselves
         * to blame for any traps in DosDupHandle().
         */
        return 0;
    }

    if (_fmutex_request(&gmtx, 0))
        return -1;

    rc = fhMoreHandles(fh + 1);

    _fmutex_release(&gmtx);
    return rc;
}


/**
 * Auto increase the MAX FH limit for this process.
 *
 * @returns 0 on success.
 * @returns -1 on failure.
 */
int __libc_FHMoreHandles(void)
{
    int rc;
    if (_fmutex_request(&gmtx, 0))
        return -1;

    rc = fhMoreHandles(0);

    _fmutex_release(&gmtx);
    return rc;
}




/**
 * Allocates a file handle.
 *
 * @returns 0 on success.
 * @returns OS/2 error code and errno set to the corresponding error number.
 * @param   fh      Number of the filehandle to allocate.
 *                  Will fail if the handle is in use.
 *                  Use -1 for any handle.
 * @param   fFlags  Initial flags.
 * @param   cb      Size of the file handle.
 *                  Must not be less than the mandatory size (sizeof(LIBCFH)).
 * @param   pOps    Value of the pOps field, NULL not allowed.
 * @param   ppFH    Where to store the allocated file handle struct pointer. (NULL allowed)
 * @param   pfh     Where to store the number of the filehandle allocated.
 * @param   fOwnSem Set if we should not take or release the semaphore.
 * @remark  The preallocated handles make this function somewhat big and messy.
 */
static int fhAllocate(int fh, unsigned fFlags, int cb, __LIBC_PCFHOPS pOps, __LIBC_PFH *ppFH, int *pfh, int fOwnSem)
{
    __LIBC_PFH  pFH;
    int         rc;
    FS_VAR();

    /*
     * If we know we cannot use the preallocated handles it's prefered to
     * do the allocation outside the mutex.
     */
    pFH = NULL;
    if (cb > sizeof(LIBCFH) || !gcPreAllocatedAvail)
    {
        pFH = _hmalloc(cb);
        if (!pFH)
            return ERROR_NOT_ENOUGH_MEMORY;
    }

    /*
     * Now take the lock.
     */
    if (!fOwnSem && _fmutex_request(&gmtx, 0))
        return -1;

    /*
     * Now is the time to try use the pre allocated handles.
     */
    if (!pFH)
    {
        if (cb == sizeof(LIBCFH) && gcPreAllocatedAvail)
        {
            __LIBC_PFH pFHSearch;
            for (pFHSearch = &gaPreAllocated[0];
                 pFH < &gaPreAllocated[sizeof(gaPreAllocated) / sizeof(gaPreAllocated[0])];
                 pFHSearch++)
                if (!pFHSearch->fFlags)
                {
                    gcPreAllocatedAvail--;
                    pFH = pFHSearch;
                    break;
                }
            LIBC_ASSERT(pFH);
        }

        /*
         * If the pre allocated handle table for some reason was
         * full, we'll allocate the handle here.
         */
        if (!pFH)
            pFH = _hmalloc(cb);
    }

    if (pFH)
    {
        /*
         * Initiate the handle data.
         */
        pFH->fFlags     = fFlags;
        if (fFlags & O_NOINHERIT)
            fFlags |= FD_CLOEXEC << __LIBC_FH_FDFLAGS_SHIFT;
        pFH->iLookAhead = -1;
        pFH->pOps       = pOps;
        pFH->Inode      = ++_sys_ino;
        if (!pFH->Inode)
            pFH->Inode  = ++_sys_ino;
        pFH->Dev        = 0;
        pFH->pFsInfo    = NULL;
        pFH->pszNativePath = NULL;

        /*
         * Insert into the handle table.
         */
        if (fh == -1)
        {
            /*
             * Non OS/2 filehandle.
             * Open dummy device to find a handle number.
             */
            LIBC_ASSERTM(pOps, "cannot open a non-standard handle without giving pOps!\n");
            rc = fhOpenDummy(-1, &fh);
            if (!rc)
            {
                /*
                 * Enough space in the handle table?
                 */
                if (fh >= gcFHs)
                    rc = fhMoreHandles(fh + 1);
                if (!rc)
                {
                    /*
                     * Free any old filehandle which death we didn't catch
                     * and insert the new one.
                     */
                    if (gpapFHs[fh])
                        fhFreeHandle(gpapFHs[fh]);
                    gpapFHs[fh] = pFH;
                }
                else
                {
                    /* failure: close dummy */
                    FS_SAVE_LOAD();
                    DosClose((HFILE)fh);
                    FS_RESTORE();
                    fh = -1;
                }
            }
        }
        else
        {
            /*
             * Specific handle request.
             * Make sure there are enough file handles available.
             */
            rc = 0;
            if (fh >= gcFHs)
                rc = fhMoreHandles(fh + 1);

            if (!rc)
            {
                /*
                 * Send the close call to any non-OS/2 handle.
                 */
                __LIBC_PFH  pFHOld = gpapFHs[fh];
                if (    pFHOld
                    &&  pFHOld->pOps
                    &&  pFHOld->pOps->pfnClose)
                    rc = pFHOld->pOps->pfnClose(pFHOld, fh);
                if (!rc)
                {
                    /*
                     * If not OS/2 filehandle, make a fake handle.
                     */
                    if (pOps)
                        rc = fhOpenDummy(fh, &fh);
                    if (!rc)
                    {
                        /*
                         * Free any old filehandle (replaced during dup2 or closed outside
                         * this libc) and insert the new one.
                         */
                        if (pFHOld)
                            fhFreeHandle(pFHOld);
                        gpapFHs[fh] = pFH;
                    }
                }
            }
        }
    }
    else
        rc = ERROR_NOT_ENOUGH_MEMORY;

    /*
     * On failure cleanup any mess!
     */
    if (rc)
    {
        _sys_set_errno(rc);
        if (pFH)
            fhFreeHandle(pFH);
        pFH = NULL;
        fh = -1;
    }

    /*
     * Return.
     */
    if (ppFH)
        *ppFH = pFH;
    if (pfh)
        *pfh = fh;

    if (!fOwnSem)
        _fmutex_release(&gmtx);
    return rc;
}

/**
 * Allocates a file handle.
 *
 * @returns 0 on success.
 * @returns OS/2 error code and errno set to the corresponding error number.
 * @param   fh      Number of the filehandle to allocate.
 *                  Will fail if the handle is in use.
 *                  Use -1 for any handle.
 * @param   fFlags  Initial flags.
 * @param   cb      Size of the file handle.
 *                  Must not be less than the mandatory size (sizeof(LIBCFH)).
 * @param   pOps    Value of the pOps field.
 * @param   ppFH    Where to store the allocated file handle struct pointer. (NULL allowed)
 * @param   pfh     Where to store the number of the filehandle allocated.
 * @remark  The preallocated handles make this function somewhat big and messy.
 */
int __libc_FHAllocate(int fh, unsigned fFlags, int cb, __LIBC_PCFHOPS pOps, __LIBC_PFH *ppFH, int *pfh)
{
    return fhAllocate(fh, fFlags, cb, pOps, ppFH, pfh, 0);
}


/**
 * Close (i.e. free) a file handle.
 *
 * @returns 0 on success.
 * @returns OS/2 error code on failure and errno set to corresponding error number.
 * @param   fh      Filehandle to close.
 */
int __libc_FHClose(int fh)
{
    LIBCLOG_ENTER("fh=%d\n", fh);
    __LIBC_PFH  pFH;
    int         rc;
    FS_VAR();

    if (_fmutex_request(&gmtx, 0))
        LIBCLOG_ERROR_RETURN(-1, "ret -1 - fh=%d is not opened according to our table!\n", fh);

    /*
     * Validate input.
     */
    if (!fhGet(fh))
    {
        _fmutex_release(&gmtx);
        errno = EBADF;
        LIBCLOG_ERROR_RETURN(-1, "ret -1 - fh=%d is not opened according to our table!\n", fh);
    }

    pFH = gpapFHs[fh];

    /*
     * If this is an non OS/2 handle, let the subsystem clean it up first.
     */
    rc = 0;
    if (pFH->pOps && pFH->pOps->pfnClose)
        rc = pFH->pOps->pfnClose(pFH, fh);

    /*
     * We should continue closing the OS/2 handle if the previuos step
     * were successful or if the handle has become invalid.
     */
    if (    !rc
        ||  rc == ERROR_INVALID_HANDLE
        ||  rc == -EBADF
        ||  rc == -ENOTSOCK)
    {
        int rc2;

        /*
         * Close the OS/2 handle and remove the handle from the array
         * making the space available to others.
         * - This ain't the way which scales best, but it's the safe way.
         */
        FS_SAVE_LOAD();
        rc2 = DosClose(fh);
        FS_RESTORE();
        LIBC_ASSERTM(!rc2, "DosClose(%d) -> rc2=%d (rc=%d)\n", fh, rc2, rc);
        if (!pFH->pOps)
            rc = rc2;

        /*
         * If we successfully freed the handle, or it had become invalid
         * we will free it now.
         */
        if (    !rc2
            ||  rc2 == ERROR_INVALID_HANDLE
            ||  pFH->pOps)
        {
            gpapFHs[fh] = NULL;
            fhFreeHandle(pFH);
            /*
             * Reset the error as from the LIBC point of view it's a success
             * (the file hangle is gone).
             * Check https://github.com/bitwiseworks/libcx/issues/60 for a
             * consequence of not doing this.
             */
            rc = 0;
        }
    }

    _fmutex_release(&gmtx);

    if (!rc)
        LIBCLOG_RETURN_INT(0);
    if (rc > 0)
        _sys_set_errno(rc);
    else
        errno = -rc;
    LIBCLOG_ERROR_RETURN_INT(rc);
}


/**
 * Get the LIBC handle structure corresponding to a filehandle.
 *
 * @returns Pointer to handle structure on success.
 * @returns NULL on failure.
 * @param   fh  Handle to lookup.
 * @remark  Must own the mutex.
 * @internal
 */
static __LIBC_PFH fhGet(int fh)
{
    __LIBC_PFH pFH = NULL;

    /*
     * Someone might have opened this externally after
     * growing the filehandle range.
     */
    if (fh >= gcFHs && fh < 0x10000)
    {
        LONG    lDelta = 0;
        ULONG   cCur = 0;
        if (!DosSetRelMaxFH(&lDelta, &cCur))
            cCur = gcFHs;
        if (gcFHs != cCur)
            fhMoreHandles(cCur);
    }

    /*
     * Validate file handle range.
     */
    if (fh >= 0 && fh < gcFHs)
    {
        pFH = gpapFHs[fh];
        if (!pFH)
        {
            /*
             * Try import the handle.
             */
            ULONG       rc;
            ULONG       fulType;
            ULONG       fulDevFlags;
            ULONG       fulMode;
            unsigned    fLibc;
            dev_t       Dev = 0;

            /*
             * Is it in use and if so what kind of handle?
             */
            if (    (rc = DosQueryHType((HFILE)fh, &fulType, &fulDevFlags)) != NO_ERROR
                ||  (rc = DosQueryFHState((HFILE)fh, &fulMode)) != NO_ERROR)
            {
                errno = EBADF;
                return NULL;
            }

            /*
             * Determin initial flags.
             */
            switch (fulType & 0xff)
            {
                default: /* paranoia */
                case HANDTYPE_FILE:
                    fLibc = F_FILE;
                    break;
                case HANDTYPE_DEVICE:
                    fLibc = F_DEV;
                    /* @todo inherit O_NDELAY */
                    if (!(fulDevFlags & 0xf))
                        Dev = makedev('c', 0);
                    else if (fulDevFlags & 1 /*KBD*/)
                        Dev = makedev('c', 1);
                    else if (fulDevFlags & 2 /*SCR*/)
                        Dev = makedev('c', 2);
                    else if (fulDevFlags & 4 /*NUL*/)
                        Dev = makedev('c', 4);
                    else /*if (fulDevFlags & 8 / *CLK* /)*/
                        Dev = makedev('c', 8);
                    break;
                case HANDTYPE_PIPE:
                    fLibc = F_PIPE;
                    Dev = makedev('p', 0);
                    break;
            }

            /*
             * Read write flags.
             */
            switch (fulMode & (OPEN_ACCESS_READONLY | OPEN_ACCESS_WRITEONLY | OPEN_ACCESS_READWRITE))
            {
                case OPEN_ACCESS_READONLY:      fLibc |= O_RDONLY; break;
                case OPEN_ACCESS_WRITEONLY:     fLibc |= O_WRONLY; break;
                default: /* paranoia */
                case OPEN_ACCESS_READWRITE:     fLibc |= O_RDWR; break;
            }

            /*
             * Inherit flags.
             */
            if (fulMode & OPEN_FLAGS_NOINHERIT)
                fLibc |= O_NOINHERIT | (FD_CLOEXEC << __LIBC_FH_FDFLAGS_SHIFT);

            /*
             * Textflag.
             */
            if (!_fmode_bin)
                fLibc |= O_TEXT;


            /*
             * Allocate a new handle for this filehandle.
             */
            rc = fhAllocate(fh, fLibc, sizeof(LIBCFH), NULL, &pFH, NULL, 1);
            if (!rc)
                pFH->Dev = Dev;
            else
                pFH = NULL;
        }
    }
    else /* out of range: can't possibly be open. */
        errno = EBADF;

    return pFH;
}

/**
 * Get the LIBC handle structure corresponding to a filehandle.
 *
 * @returns Pointer to handle structure on success.
 * @returns NULL on failure.
 * @param   fh  Handle to lookup.
 * @deprecated
 */
__LIBC_PFH __libc_FH(int fh)
{
    __LIBC_PFH pFH;

    /** @todo shared access */
    if (_fmutex_request(&gmtx, 0))
        return NULL;

    pFH = fhGet(fh);

    _fmutex_release(&gmtx);
    return pFH;
}


/**
 * Get the LIBC handle structure corresponding to a filehandle.
 *
 * @returns Pointer to handle structure on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   fh      Handle to lookup.
 * @param   ppFH    Where to store the filehandle pointer
 */
int __libc_FHEx(int fh, __LIBC_PFH *ppFH)
{
    __LIBC_PFH pFH;

    /** @todo shared access */
    int rc = _fmutex_request(&gmtx, 0);
    if (rc)
        return -__libc_native2errno(rc);

    /** @todo rewrite fhGet() to avoid this errno saving and restoring. */
    int errno_saved = errno;
    pFH = fhGet(fh);
    rc = -errno;
    errno = errno_saved;

    _fmutex_release(&gmtx);
    if (pFH)
    {
        *ppFH = pFH;
        return 0;
    }
    return rc;
}


/**
 * Update the flags for the filehandle.
 *
 * This is a service routine for unifying the updating of the flags in
 * the __LIBC_FH structure. It will not call any FH operators, since
 * it assumes that the caller takes care of such things.
 *
 * @returns 0 on success.
 * @returns Negated errno on failure.
 * @param   pFH     The filehandle struct.
 * @param   fh      The filehandle.
 * @param   fFlags  The new flags.
 */
int __libc_FHSetFlags(__LIBC_PFH pFH, int fh, unsigned fFlags)
{
    LIBCLOG_ENTER("pFH=%p fh=%d fFlags=%#x\n", (void *)pFH, fh, fFlags);
    int     rc;
    ULONG   fulState;
    FS_VAR();
    LIBC_ASSERT(((fFlags & O_NOINHERIT) != 0) == ((fFlags & (FD_CLOEXEC << __LIBC_FH_FDFLAGS_SHIFT)) != 0));

    FS_SAVE_LOAD();
    rc = DosQueryFHState(fh, &fulState);
    if (!rc)
    {
        ULONG fulNewState;
        LIBC_ASSERTM(     ( (fulState & OPEN_FLAGS_NOINHERIT) != 0 )
                      ==  (   (pFH->fFlags & (O_NOINHERIT | (FD_CLOEXEC << __LIBC_FH_FDFLAGS_SHIFT)))
                           == (O_NOINHERIT | (FD_CLOEXEC << __LIBC_FH_FDFLAGS_SHIFT)) ),
                     "Inherit flags are out of sync for file hFile %d (%#x)! fulState=%08lx fFlags=%08x\n",
                     fh, fh, fulState, pFH->fFlags);
        if (fFlags & (FD_CLOEXEC << __LIBC_FH_FDFLAGS_SHIFT))
            fulNewState = fulState | OPEN_FLAGS_NOINHERIT;
        else
            fulNewState = fulState & ~OPEN_FLAGS_NOINHERIT;
        if (fulNewState != fulState)
        {
            fulNewState &= OPEN_FLAGS_WRITE_THROUGH | OPEN_FLAGS_FAIL_ON_ERROR | OPEN_FLAGS_NO_CACHE | OPEN_FLAGS_NOINHERIT; /* Turn off non-participating bits. */
            rc = DosSetFHState(fh, fulNewState);
        }
    }
    FS_RESTORE();

    if (!rc)
    {
        __atomic_xchg(&pFH->fFlags, fFlags);
        LIBCLOG_RETURN_INT(0);
    }

    rc = -__libc_native2errno(rc);
    LIBCLOG_ERROR_RETURN_INT(rc);
}




#undef  __LIBC_LOG_GROUP
#define __LIBC_LOG_GROUP    __LIBC_LOG_GRP_FORK

_FORK_PARENT1(0xfffff000, fhForkParent1)

/**
 * Parent fork callback.
 *
 * There are two purposes for this function. First, lock the filehandle
 * array while forking. Second, force all handles to tempoarily be inherited
 * by child process.
 *
 * @returns 0 on success.
 * @returns -errno on failure.
 * @param   pForkHandle     Pointer to fork handle.
 * @param   enmOperation    Fork operation.
 */
static int fhForkParent1(__LIBC_PFORKHANDLE pForkHandle, __LIBC_FORKOP enmOperation)
{
    LIBCLOG_ENTER("pForkHandle=%p enmOperation=%d\n", (void *)pForkHandle, enmOperation);
    int rc;

    switch (enmOperation)
    {
        /*
         * Take file handle semaphore.
         * Mark all handles temporarily for inheritance.
         */
        case __LIBC_FORK_OP_EXEC_PARENT:
        {
            /*
             * Acquire Semaphore and register completion handler for cleaning up.
             */
            rc = _fmutex_request(&gmtx, 0);
            if (!rc)
            {
                gfForkCleanupDone = 0;
                rc = pForkHandle->pfnCompletionCallback(pForkHandle, fhForkCompletion, NULL, __LIBC_FORK_CTX_BOTH);
                if (rc < 0)
                    _fmutex_release(&gmtx);
                else
                {
                    /*
                     * Iterate all file handles and do pre exec processing, i.e. mark as inherit.
                     */
                    unsigned iFH;
                    unsigned cFHs = gcFHs;
                    for (iFH = 0; iFH < cFHs; iFH++)
                    {
                        __LIBC_PFH pFH;
                        if ((pFH = gpapFHs[iFH]) != NULL)
                        {
                            if (pFH->pOps && pFH->pOps->pfnForkParent)
                            {
                                rc = pFH->pOps->pfnForkParent(pFH, iFH, pForkHandle, __LIBC_FORK_OP_EXEC_PARENT);
                                if (rc)
                                {
                                    LIBC_ASSERTM_FAILED("pfnForkParent(=%p) for handle %d failed with rc=%d\n",
                                                        (void *)pFH->pOps->pfnForkParent, iFH, rc);
                                    if (rc > 0)
                                        rc = -__libc_native2errno(rc);
                                    break;
                                }
                            }

                            /* mark as inherit. */
                            if (pFH->fFlags & (O_NOINHERIT | (FD_CLOEXEC << __LIBC_FH_FDFLAGS_SHIFT)))
                            {
                                ULONG   fulState;

                                rc = DosQueryFHState(iFH, &fulState);
                                if (!rc)
                                {
                                    fulState = fulState & ~OPEN_FLAGS_NOINHERIT;
                                    fulState &= OPEN_FLAGS_WRITE_THROUGH | OPEN_FLAGS_FAIL_ON_ERROR | OPEN_FLAGS_NO_CACHE | OPEN_FLAGS_NOINHERIT; /* Turn off non-participating bits. */
                                    rc = DosSetFHState(iFH, fulState);
                                }
                                if (rc)
                                {
                                    LIBC_ASSERTM_FAILED("DosSetFHState or DosQueryFHState failed with rc=%d for handle %d\n", rc, iFH);
                                    rc = -__libc_native2errno(rc);
                                    break;
                                }
                            }
                        }
                    } /* for */
                    /* No need to clean up on failure. the completion callback will do that. */
                }
            }
            else
            {
                LIBC_ASSERTM_FAILED("failed to get the filehandle mutex. rc=%d\n", rc);
                rc = -__libc_native2errno(rc);
            }
            break;
        }


        /*
         * Iterate all file handles and do fork exec processing.
         * This is only done for non standard file handles.
         */
        case __LIBC_FORK_OP_FORK_PARENT:
        {
            unsigned iFH;
            unsigned cFHs = gcFHs;
            for (iFH = 0, rc = 0; iFH < cFHs; iFH++)
            {
                __LIBC_PFH pFH;
                if ((pFH = gpapFHs[iFH]) != NULL)
                {
                    if (pFH->pOps && pFH->pOps->pfnForkParent)
                    {
                        rc = pFH->pOps->pfnForkParent(pFH, iFH, pForkHandle, __LIBC_FORK_OP_FORK_PARENT);
                        if (rc)
                        {
                            LIBC_ASSERTM_FAILED("pfnForkParent(=%p) for handle %d failed with rc=%d\n",
                                                (void *)pFH->pOps->pfnForkParent, iFH, rc);
                            if (rc > 0)
                                rc = -__libc_native2errno(rc);
                            break;
                        }
                    }
                }
            }
            break;
        }

        default:
            rc = 0;
            break;
    }

    LIBCLOG_RETURN_INT(rc);
}



_FORK_CHILD1(0xffffff00, fhForkChild1)

/**
 * Child fork callback.
 *
 * There are two purposes for this function. First, restore no-inherit handles
 * and call pfnForkChild(). Second, to release the file handle mutex.
 *
 * Note that only __LIBC_FORK_OP_FORK_CHILD is propagated to pfnForkChild().
 *
 * @returns 0 on success.
 * @returns -errno on failure.
 * @param   pForkHandle     Pointer to fork handle.
 * @param   enmOperation    Fork operation.
 */
static int fhForkChild1(__LIBC_PFORKHANDLE pForkHandle, __LIBC_FORKOP enmOperation)
{
    LIBCLOG_ENTER("pForkHandle=%p enmOperation=%d\n", (void *)pForkHandle, enmOperation);
    int rc;

    switch (enmOperation)
    {
        /*
         * Clear all handles which was temporarily marked for inheritance.
         * Release the file handle semaphore.
         */
        case __LIBC_FORK_OP_FORK_CHILD:
        {
            int         rc2;
            unsigned    iFH;
            unsigned    cFHs = gcFHs;
            for (iFH = 0, rc = 0; iFH < cFHs; iFH++)
            {
                __LIBC_PFH pFH;
                if ((pFH = gpapFHs[iFH]) != NULL)
                {
                    /* call pfnForkChild(). */
                    if (pFH->pOps && pFH->pOps->pfnForkChild)
                    {
                        rc = pFH->pOps->pfnForkChild(pFH, iFH, pForkHandle, __LIBC_FORK_OP_FORK_CHILD);
                        if (rc)
                        {
                            LIBC_ASSERTM_FAILED("pfnForkChild(=%p) for handle %d failed with rc=%d\n",
                                                (void *)pFH->pOps->pfnForkChild, iFH, rc);
                            if (rc > 0)
                                rc = -__libc_native2errno(rc);
                            break;
                        }
                    }

                    /* clear flag. */
                    if (pFH->fFlags & (O_NOINHERIT | (FD_CLOEXEC << __LIBC_FH_FDFLAGS_SHIFT)))
                    {
                        ULONG fulState;
                        rc = DosQueryFHState(iFH, &fulState);
                        if (!rc)
                        {
                            fulState |= OPEN_FLAGS_NOINHERIT;
                            fulState &= OPEN_FLAGS_WRITE_THROUGH | OPEN_FLAGS_FAIL_ON_ERROR | OPEN_FLAGS_NO_CACHE | OPEN_FLAGS_NOINHERIT; /* Turn off non-participating bits. */
                            rc = DosSetFHState(iFH, fulState);
                        }
                        LIBC_ASSERTM(!rc, "DosSetFHState or DosQueryFHState failed with rc=%d for handle %d\n", rc, iFH);
                    }
                }
            }

            /* release semaphore. */
            rc2 = _fmutex_release(&gmtx);
            if (rc2)
            {
                LIBC_ASSERTM_FAILED("_fmutex_release failed rc=%d\n", rc2);
                if (rc >= 0)
                    rc = -__libc_native2errno(rc2);
            }
            gfForkCleanupDone = 1;
            break;
        }

        default:
            rc = 0;
            break;
    }

    LIBCLOG_RETURN_INT(rc);
}


/**
 * Fork completion callback used to release the file handle semaphore
 * and set the noinherit flags.
 *
 * @param   pvArg   NULL.
 * @param   rc      The fork() result. Negative on failure.
 * @param   enmCtx  The calling context.
 */
static void fhForkCompletion(void *pvArg, int rc, __LIBC_FORKCTX enmCtx)
{
    LIBCLOG_ENTER("pvArg=%p rc=%d enmCtx=%d\n", pvArg, rc, enmCtx);
    rc = rc;
    pvArg = pvArg;

    if (    !gfForkCleanupDone
        && (    !rc
            ||  enmCtx == __LIBC_FORK_CTX_PARENT))
    {
        /*
         * Iterate handles and set no-inherit flags.
         *
         * We assume that the non standard handles will do any necessary
         * cleanup themselves. If they register completion callbacks they
         * have all be executed by now since we registered our before them
         * the the order is LIFO.
         */
        unsigned iFH;
        unsigned cFHs = gcFHs;
        for (iFH = 0, rc = 0; iFH < cFHs; iFH++)
        {
            __LIBC_PFH pFH;
            if ((pFH = gpapFHs[iFH]) != NULL)
            {
                if (pFH->fFlags & (O_NOINHERIT | (FD_CLOEXEC << __LIBC_FH_FDFLAGS_SHIFT)))
                {
                    ULONG fulState;
                    rc = DosQueryFHState(iFH, &fulState);
                    if (!rc)
                    {
                        fulState |= OPEN_FLAGS_NOINHERIT;
                        fulState &= OPEN_FLAGS_WRITE_THROUGH | OPEN_FLAGS_FAIL_ON_ERROR | OPEN_FLAGS_NO_CACHE | OPEN_FLAGS_NOINHERIT; /* Turn off non-participating bits. */
                        rc = DosSetFHState(iFH, fulState);
                    }
                    LIBC_ASSERTM(!rc, "DosSetFHState or DosQueryFHState failed with rc=%d for handle %d\n", rc, iFH);
                }
            }
        }
    }

    /*
     * Release the mutex.
     */
    if (!gfForkCleanupDone)
    {
        gfForkCleanupDone = 1;
        _fmutex_release(&gmtx);
    }

    LIBCLOG_RETURN_VOID();
}

