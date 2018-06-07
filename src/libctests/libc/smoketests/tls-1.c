/* $Id: tls-1.c 2055 2005-06-19 08:04:00Z bird $ */
/** @file
 *
 * LIBC tls tests.
 *
 * Copyright (c) 2004 knut st. osmundsen <bird-srcspam@anduin.net>
 *
 *
 * This file is part of LIBC.
 *
 * LIBC is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * LIBC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with LIBC; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*******************************************************************************
*   Defined Constants And Macros                                               *
*******************************************************************************/
#define TEST_THREADS 10


/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include <InnoTekLIBC/thread.h>
#include <process.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <386/builtin.h>

/*******************************************************************************
*   Structures and Typedefs                                                    *
*******************************************************************************/
/** Generic argument structure for the thread functions. */
typedef struct ThreadArg
{
    int     iTLS;
    int     cErrors;
    int     tid;
    int     fDone;
} THREADARG, *PTHREADARG;

static void thread1(void *pvArg)
{
    PTHREADARG  pArg = (PTHREADARG)pvArg;
    int         i;

    if (__libc_TLSGet(pArg->iTLS) != NULL)
    {
        printf("tls: error: tid %d: initial get failed on index %d\n",
               pArg->tid, pArg->iTLS);
        pArg->cErrors++;
    }

    for (i = 0; i < 100; i++)
    {
        if (__libc_TLSSet(pArg->iTLS, pvArg))
        {
            printf("tls: error: tid %d: set failed on index %d, i=%d\n",
                   pArg->tid, pArg->iTLS, i);
            pArg->cErrors++;
        }

        if (__libc_TLSGet(pArg->iTLS) != pvArg)
        {
            printf("tls: error: tid %d: get failed on index %d, i=%d\n",
                   pArg->tid, pArg->iTLS, i);
            pArg->cErrors++;
        }
        usleep(1000);
    }
    pArg->fDone = 1;
}


/** allocations and deallocations thread function. */
static void thread2(void *pvArg)
{
    PTHREADARG pArg = (PTHREADARG)pvArg;
    int i;

    for (i = 0; i < 20000; i++)
    {
        int iTLS1;
        int iTLS2;
        iTLS1 = __libc_TLSAlloc();
        if (iTLS1 < 0)
        {
            printf("tls: error: tid %d: TLSAlloc (1) failed, i=%d\n", pArg->tid, i);
            pArg->cErrors++;
        }

        iTLS2 = __libc_TLSAlloc();
        if (iTLS2 < 0)
        {
            printf("tls: error: tid %d: TLSAlloc (2) failed, i=%d\n", pArg->tid, i);
            pArg->cErrors++;
        }

        if (__libc_TLSFree(iTLS1))
        {
            printf("tls: error: tid %d: TLSFree (1) failed, i=%d, iTLS1=%d\n", pArg->tid, i, iTLS1);
            pArg->cErrors++;
        }

        if (__libc_TLSFree(iTLS2))
        {
            printf("tls: error: tid %d: TLSFree (2) failed, i=%d, iTLS2=%d\n", pArg->tid, i, iTLS2);
            pArg->cErrors++;
        }
    }
    pArg->fDone = 1;
}


int main()
{
    int         cErrors = 0;
    int         cTLSes = 0;
    static int  aiTLSes[1024];
    int         i;
    static THREADARG    aThreadArgs[1024];
    int         iTLS;

    /*
     * Allocate the maximum number of TLSes checking that
     * the limit checking works.
     */
    do
    {
        aiTLSes[cTLSes] = __libc_TLSAlloc();
        if (aiTLSes[cTLSes] < 0)
            break;
        cTLSes++;
    } while (cTLSes < 1024);
    printf("tls: info: max=%d\n", cTLSes);
    if (cTLSes < __LIBC_TLS_MAX)
    {
        printf("tls: error: cTLSes != max (%d != %d)\n", cTLSes, __LIBC_TLS_MAX);
        cErrors++;
    }
    /* check for duplicates */
    for (i = 0; i <  cTLSes; i++)
    {
        int j;
        for (j = 0; j < cTLSes; j++)
        {
            if (i != j && aiTLSes[i] == aiTLSes[j])
            {
                printf("tls: error: duplicate allocation. i=%d, j=%d, index=%d\n",
                       i, j, aiTLSes[i]);
                cErrors++;
            }
        }
    }

    /*
     * Set,get,set all entries.
     */
    i = cTLSes;
    while (i-- > 0)
    {
        if (__libc_TLSSet(aiTLSes[i], (void*)(0x80000000 | i)))
        {
            printf("tls: error: set failed on index %d (i=%d)\n",
                   aiTLSes[i], i);
            cErrors++;
        }

        if (__libc_TLSGet(aiTLSes[i]) != (void*)(0x80000000 | i))
        {
            printf("tls: error: get failed on index %d (i=%d)\n",
                   aiTLSes[i], i);
            cErrors++;
        }

        if (__libc_TLSSet(aiTLSes[i], (void*)(0x80004000 | i)))
        {
            printf("tls: error: set (2) failed on index %d (i=%d)\n",
                   aiTLSes[i], i);
            cErrors++;
        }
    }

    /*
     * Free the allocated TLSes.
     */
    for (i = 0; i < cTLSes; i++)
    {
        if (__libc_TLSGet(aiTLSes[i]) !=  (void*)(0x80004000 | i))
        {
            printf("tls: error: get (2) failed on index %d (i=%d)\n",
                   aiTLSes[i], i);
            cErrors++;
        }

        if (__libc_TLSFree(aiTLSes[i]))
        {
            printf("tls: error: failed to free index %d\n", aiTLSes[i]);
            cErrors++;
        }

        if (__libc_TLSGet(aiTLSes[i]) != NULL)
        {
            printf("tls: error: access possible after freed, index %d\n", aiTLSes[i]);
            cErrors++;
        }

    }


    /*
     * Set & Get threaded.
     */
    iTLS = __libc_TLSAlloc();
    if (iTLS >= 0)
    {
        /* spawn threads. */
        for (i = 0; i < TEST_THREADS; i++)
        {
            aThreadArgs[i].cErrors  = 0;
            aThreadArgs[i].iTLS     = iTLS;
            aThreadArgs[i].fDone    = 0;
            aThreadArgs[i].tid      = _beginthread(thread1, NULL, 0x10000, &aThreadArgs[i]);
            if (aThreadArgs[i].tid <= 0)
            {
                aThreadArgs[i].fDone    = 1;
                printf("tls: error: failed to create thread! i=%d\n", i);
                cErrors++;
            }
        }

        /* wait for the threads */
        i = 0;
        do
        {
            usleep(64000 + i);
            for (i = 0; i < TEST_THREADS; i++)
                if (!aThreadArgs[i].fDone)
                    break;
        } while (i < TEST_THREADS);

        for (i = 0; i < TEST_THREADS; i++)
            cErrors += aThreadArgs[i].cErrors;

        if (__libc_TLSFree(iTLS))
        {
            printf("tls: error: failed to free TLS entry %d (set/get)\n", iTLS);
            cErrors++;
        }
    }
    else
    {
        printf("tls: error: failed to allocated TLS entry for set/get threaded.\n");
        cErrors++;
    }

    /*
     * Threaded alloc + free.
     */
    /*
     * Set & Get threaded.
     */
    for (i = 0; i < TEST_THREADS; i++)
    {
        aThreadArgs[i].cErrors  = 0;
        aThreadArgs[i].iTLS     = iTLS;
        aThreadArgs[i].fDone    = 0;
        aThreadArgs[i].tid      = _beginthread(thread2, NULL, 0x10000, &aThreadArgs[i]);
        if (aThreadArgs[i].tid <= 0)
        {
            aThreadArgs[i].fDone    = 1;
            printf("tls: error: failed to create thread! i=%d\n", i);
            cErrors++;
        }
    }

    /* wait for the threads */
    i = 0;
    do
    {
        usleep(32000 + i);
        for (i = 0; i < TEST_THREADS; i++)
            if (!aThreadArgs[i].fDone)
                break;
    } while (i < TEST_THREADS);

    for (i = 0; i < TEST_THREADS; i++)
        cErrors += aThreadArgs[i].cErrors;


    /*
     * Test summary
     */
    if (cErrors)
        printf("tls: testcase failed with %d errors\n", cErrors);
    else
        printf("tls: testcase executed successfully\n");
    return cErrors;
}
