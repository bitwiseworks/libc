/* $Id: tls.c 2260 2005-07-18 01:15:18Z bird $ */
/** @file
 *
 * InnoTek LIBC - Thread Local Storage Implementation.
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
#include <sys/builtin.h>
#include <errno.h>
#include <sys/limits.h>
#include <sys/smutex.h>
#include <InnoTekLIBC/thread.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_THREAD
#include <InnoTekLIBC/logstrict.h>


#if ULONG_MAX == UINT_MAX
    /* 32 bits */
    #define ENTRY_BITS      32
    #define ENTRY_BYTES     4
    #define INDEX_SHIFT     5
    #define BIT_MASK        0x0000001f
#else
    /* 64 bits */
    #define ENTRY_BITS      64
    #define ENTRY_BYTES     8
    #define INDEX_SHIFT     6
    #define BIT_MASK        0x000000000000003f
#endif

/*******************************************************************************
*   Internal Functions                                                         *
*******************************************************************************/
static void __libc_tlsThreadDestructor(__LIBC_PTHREADTERMCBREGREC pRegRec, unsigned fFlags);


/*******************************************************************************
*   Global Variables                                                           *
*******************************************************************************/
/** Number of allocated TLS items.
 * Updated atomically. Update before allocation and after free. */
static unsigned gcTLSAllocated;

/** TLS allocation bitmap map.
 * Set means allocated, clear means free. Updated atomically. */
static unsigned gauBitmap[(__LIBC_TLS_MAX + ENTRY_BITS - 1) / ENTRY_BITS];

/** TLS Per Thread Destructors. */
static   void (*gapfnDestructors[__LIBC_TLS_MAX])(void *, int, unsigned);

/** Thread Termination Registration Record. */
static __LIBC_THREADTERMCBREGREC    gThrdTermRegRec =
{
    NULL, 0, __libc_tlsThreadDestructor
};


int     __libc_TLSAlloc(void)
{
    LIBCLOG_ENTER("\n");
    unsigned        cTries;

    /*
     * Space left?
     */
    if (__atomic_increment_max(&gcTLSAllocated, __LIBC_TLS_MAX))
    {
        errno -ENOMEM;
        LIBCLOG_ERROR_RETURN(-1, "ret -1 - Out of TLS entries! cur=%d max=%d\n", gcTLSAllocated, __LIBC_TLS_MAX);
    }

    /*
     * Find a free entry (bit).
     * We'll scan the bitmap 32 times before we give up.
     */
    for (cTries = 32; cTries > 0; cTries--)
    {
        /*
         * Scan the bitmap int by int.
         */
        unsigned *pu = &gauBitmap[0];
        while (pu < &gauBitmap[sizeof(gauBitmap) / sizeof(gauBitmap[0])])
        {
            if (*pu != ~0)
            {
                /*
                 * Look for free bit.
                 */
                do
                {
                    unsigned uBit;
                    for (uBit = 0; uBit < ENTRY_BITS; uBit++)
                    {
                        if (!__atomic_set_bit(pu, uBit))
                        {
                            int iBitRet = (pu - &gauBitmap[0]) * ENTRY_BITS + uBit;
                            if (iBitRet < __LIBC_TLS_MAX)
                            {
                                gapfnDestructors[iBitRet] = NULL;
                                LIBCLOG_RETURN_INT(iBitRet);
                            }
                        }
                    }
                } while (*pu != ~0);
            }

            /* next entry */
            pu++;
        }
    }

    LIBCLOG_ERROR_RETURN_MSG(-1, "ret -1 - We're giving up finding a free enter!!! cur=%d max=%d\n", gcTLSAllocated, __LIBC_TLS_MAX);
}


int     __libc_TLSFree(int iIndex)
{
    LIBCLOG_ENTER("iIndex=%d\n", iIndex);
    /*
     * Validate index
     */
    if (    iIndex < 0
        ||  iIndex >= __LIBC_TLS_MAX
        ||  !__atomic_test_bit(&gauBitmap[0], iIndex)
            )
    {
        errno = EINVAL;
        LIBCLOG_ERROR_RETURN(-1, "ret -1 - Bad index %d. (max=%d)\n", iIndex, __LIBC_TLS_MAX);
    }

    /*
     * Clear the bitmap bit.
     */
    gapfnDestructors[iIndex] = NULL;
    /* @todo do serialized NULLing of all threads! */
    __atomic_clear_bit(&gauBitmap[0], iIndex);

    /*
     * Decrement the allocation count.
     */
    LIBC_ASSERT(gcTLSAllocated > 0);
    __atomic_decrement(&gcTLSAllocated);
    LIBCLOG_RETURN_INT(0);
}


void *  __libc_TLSGet(int iIndex)
{
    LIBCLOG_ENTER("iIndex=%d\n", iIndex);
    /*
     * Validate index
     */
    if (    iIndex < 0
        ||  iIndex >= __LIBC_TLS_MAX
        ||  !__atomic_test_bit(&gauBitmap[0], iIndex)
            )
    {
        errno = EINVAL;
        LIBCLOG_ERROR_RETURN(NULL, "ret NULL - Bad index %d. (max=%d)\n", iIndex, __LIBC_TLS_MAX);
    }

    /*
     * Get it.
     */
    LIBCLOG_RETURN_P(__libc_threadCurrent()->apvTLS[iIndex]);
}


int     __libc_TLSSet(int iIndex, void *pvValue)
{
    LIBCLOG_ENTER("iIndex=%d pvValue=%p\n", iIndex, pvValue);
    /*
     * Validate index
     */
    if (    iIndex < 0
        ||  iIndex >= __LIBC_TLS_MAX
        ||  !__atomic_test_bit(&gauBitmap[0], iIndex)
            )
    {
        errno = EINVAL;
        LIBCLOG_ERROR_RETURN(-1, "ret -1 - Bad index %d. (max=%d)\n", iIndex, __LIBC_TLS_MAX);
    }

    /*
     * Set it.
     */
    __libc_threadCurrent()->apvTLS[iIndex] = pvValue;
    LIBCLOG_RETURN_INT(0);
}



int     __libc_TLSDestructor(int iIndex, void (*pfnDestructor)(void *pvValue, int iIndex, unsigned fFlags), unsigned fFlags)
{
    LIBCLOG_ENTER("iIndex=%d pfnDestructor=%p\n", iIndex, (void *)pfnDestructor);
    static _smutex      smtxInit;
    static volatile int fInited;

    /*
     * Validate index
     */
    if (    iIndex < 0
        ||  iIndex >= __LIBC_TLS_MAX
        ||  !__atomic_test_bit(&gauBitmap[0], iIndex)
            )
    {
        errno = EINVAL;
        LIBCLOG_ERROR_RETURN(-1, "ret -1 - Bad index %d. (max=%d)\n", iIndex, __LIBC_TLS_MAX);
    }
    if (fFlags)
    {
        errno = EINVAL;
        LIBCLOG_ERROR_RETURN(-1, "ret -1 - fFlags must be zero not %x!\n", fFlags);
    }

    /*
     * Init the destructor if needed.
     */
    if (!fInited)
    {
        _smutex_request(&smtxInit);
        if (!fInited)
        {
            __libc_ThreadRegisterTermCallback(&gThrdTermRegRec);
            fInited = 1;
        }
        _smutex_release(&smtxInit);
    }

    /*
     * Update destructor index.
     */
    LIBCLOG_MSG("old=%p new=%p\n", (void *)gapfnDestructors[iIndex], (void *)pfnDestructor);
    gapfnDestructors[iIndex] = pfnDestructor;
    LIBCLOG_RETURN_INT(0);
}


/**
 * Callback function for thread termination.
 * See __LIBC_THREADTERMCBREGREC::pfnCallback for parameter details.
 *
 * This is registered by the first call to __libc_TLSDestructor() and will
 * be called back on normal thread terminations. It's function is to iterate
 * the registered TLS destructors and calling back the registered TLS
 * destructors so they can perform proper per thread cleanups.
 */
static void __libc_tlsThreadDestructor(__LIBC_PTHREADTERMCBREGREC pRegRec, unsigned fFlags)
{
    LIBCLOG_ENTER("pRegRec=%p, fFlags=%d\n", (void *)pRegRec, fFlags);

    /* get thread structure. */
    __LIBC_PTHREAD  pThrd = __libc_threadCurrentNoAuto();
    if (!pThrd)
        LIBCLOG_RETURN_MSG_VOID("ret void (thread struct not initialized)\n");


    /* iterate it word by word. */
    int i;
    unsigned *pu = (unsigned *)&gauBitmap[0];
    for (i = 0; i < sizeof(gauBitmap) / sizeof(unsigned); i++, pu++)
    {
         if (*pu)
         {
             /* iterate the word byte by byte. */
             int j;
             unsigned char *puch = (unsigned char *)&gauBitmap[0];
             for (j = 0; j < sizeof(unsigned char); j++, puch++)
             {
                if (*puch)
                {
                    /* iterate byte entry by entry. */
                    int k = i * sizeof(unsigned) * 8 + j * 8;
                    int kEnd = k + 8;
                    while (k < kEnd)
                    {
                        void (*pfnDestructor)(void *, int, unsigned);
                        void  *pvValue;
                        if (    __atomic_test_bit(&gauBitmap[0], k)
                            &&  k < __LIBC_TLS_MAX
                            &&  (pvValue = pThrd->apvTLS[k]) != NULL
                            &&  (pfnDestructor = gapfnDestructors[k]) != NULL)
                        {
                            LIBCLOG_MSG("tls %d: calling %p with %p\n", k, (void *)pfnDestructor, pThrd->apvTLS[k]);
                            pThrd->apvTLS[k] = NULL;
                            pfnDestructor(pvValue, k, fFlags);
                        }

                        /* next */
                        k++;
                    }
                }
             }
         }
    }
    LIBCLOG_RETURN_VOID();
}


void (*__libc_TLSGetDestructor(int iIndex, unsigned *pfFlags))(void *, int, unsigned)
{
    LIBCLOG_ENTER("iIndex=%d, pfFlags=%p\n", iIndex, (void *)pfFlags);
    void (*pfnDestructor)(void *, int, unsigned);

    /*
     * Validate index
     */
    if (    iIndex < 0
        ||  iIndex >= __LIBC_TLS_MAX
        ||  !__atomic_test_bit(&gauBitmap[0], iIndex)
            )
    {
        errno = EINVAL;
        LIBCLOG_ERROR_RETURN(NULL, "ret NULL - Bad index %d. (max=%d)\n", iIndex, __LIBC_TLS_MAX);
    }

    pfnDestructor = gapfnDestructors[iIndex];
    if (pfFlags)
        *pfFlags = 0;
    LIBCLOG_RETURN_P(pfnDestructor);
}

