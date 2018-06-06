/* $Id: $ */
/** @file
 *
 * LIBC SYS Backend - ftime.
 *
 * InnoTek Systemberatung GmbH confidential
 *
 * Copyright (c) 2004 InnoTek Systemberatung GmbH
 * Author: knut st. osmundsen <bird-srcspam@anduin.net>
 *
 * All Rights Reserved
 *
 */

#define CALCDIFF


/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include "libc-alias.h"
#define INCL_BASE
#define INCL_DOSINFOSEG
#define INCL_FSMACROS
#include <os2emx.h>
#include <sys/timeb.h>
#include <time.h>
#include <emx/time.h>
#include <emx/syscalls.h>
#include <386/builtin.h>
#include "syscalls.h"

#ifdef CALCDIFF
/**
 * Calcs the uDiff in formula timeb.millitm = (msecs + uDiff) % 1000.
 */
static unsigned calcdiff(void)
{
    /*
     * We make a little effort to lower the chances of preempting
     * while reading the msecs and hundredths members of GIS.
     */
    #pragma pack(1)
    union combined2
    {
        volatile unsigned long long ull;
        struct s
        {
            ULONG   msecs;
            UCHAR   hour;
            UCHAR   minutes;
            UCHAR   seconds;
            UCHAR   hundredths;
        } s;
    };
    #pragma pack()
    unsigned            uDiff;
    PGINFOSEG           pGIS = GETGINFOSEG();
    union combined2    *pCombined = (union combined2 *)&pGIS->msecs;
    union combined2     Combined;

    /*
     * Get a usable data set.
     *
     * We try to skip any msecs which end with 0. The two PCs I've got
     * here indicates that this is a good idea for consistency in the result.
     */
    Combined.ull = pCombined->ull;
    if (!(Combined.s.msecs % 10))
    {
        int i = 0;
        FS_VAR();
        FS_SAVE_LOAD();
        do
        {
            DosSleep(1);
            Combined.ull = pCombined->ull;
        } while (!(Combined.s.msecs % 10) && ++i < 2);
        FS_RESTORE();
    }

    /*
     * Calc the diff value.
     */
    uDiff = Combined.s.hundredths * 10 - (Combined.s.msecs % 1000);
    if ((int)uDiff < 0)
        uDiff += 1000;                  /* must be positive for the rounding. */
    /* round up to closes 10. */
    uDiff += 9;
    uDiff /= 10;
    uDiff *= 10;
    return uDiff;
}
#endif /* CALCDIFF */


/**
 * Get the current local time.
 * @param   ptr     Where to store the result.
 *                  Note that only the members time and millitm are filled in,
 *                  the caller is expected to fill the remaining fields.
 */
void __ftime(struct timeb *ptr)
{
#pragma pack(1)
    union last
    {
        unsigned long long ull;
        struct
        {
            unsigned secs;
            unsigned msecs;
            short    milli;
        } s;
    };
    union combined
    {
        struct
        {
            ULONG       time;
            ULONG       msecs;
            UCHAR       _not_used__hour;
            UCHAR       _not_used__minutes;
            UCHAR       _not_used__seconds;
            UCHAR       hundredths;
        } s;
    };
    static union
    {
        PGINFOSEG       pGIS;
        volatile union combined *pCombined;
    } uGIS;
#pragma pack()
    static volatile union last LastStatic;
    static volatile unsigned uDiff;

    /*
     * Init.
     */
    if (!uGIS.pGIS)
    {
#ifdef CALCDIFF
        uDiff = calcdiff();
#endif
        uGIS.pGIS = GETGINFOSEG();
    }

    /*
     * Get the value and convert it to the return format.
     */
    /* read times */
    union combined  Combined = *uGIS.pCombined;
    union last      LastCopy = LastStatic;

    /* calc now */
    union last Now;
    Now.s.secs = Combined.s.time;
#ifdef CALCDIFF
    Now.s.milli = (Combined.s.msecs + uDiff) % 1000;
#else
    Now.s.milli = (Combined.s.hundredths * 10) | ((Combined.s.msecs + uDiff) % 10);
#endif
    Now.s.msecs = Combined.s.msecs;

    /* validate now */
    unsigned uDiffSecs = Now.s.secs - LastCopy.s.secs;
    if (!uDiffSecs)
    {
        int iDiffMilli = Now.s.milli - LastCopy.s.milli;
        if (iDiffMilli < 0)
        {
            uDiff -= -iDiffMilli + 1;
            Now = LastCopy;
        }
    }
    else if (uDiffSecs == 1)
    {
        unsigned    uDiffMsecs = Now.s.msecs - LastCopy.s.msecs;
        unsigned    uDiffMilli = Now.s.milli + 1000 - LastCopy.s.milli;
        if (uDiffMilli > uDiffMsecs + 500)
        {
            uDiff += 1000 - Now.s.milli;
            Now.s.milli = 0;
        }
    }

    /* update the last know ok value */
    LastStatic = Now; /** @todo 8byte + 4 byte exchange! */

    /* store the result in the user buffer */
    ptr->time       = Now.s.secs;
    ptr->millitm    = Now.s.milli;
    ptr->timezone   = 0;
    ptr->dstflag    = 0;
}
