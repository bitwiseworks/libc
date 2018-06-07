/* $Id: b_timeHighResNano.c 1902 2005-04-24 09:55:59Z bird $ */
/** @file
 *
 * LIBC SYS Backend - gethrtime().
 *
 * Copyright (c) 2005 knut st. osmundsen <bird@anduin.net>
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
#define  INCL_FSMACROS
#define  INCL_BASE
#include <os2emx.h>
#include <InnoTekLIBC/backend.h>


/**
 * Gets the current high-resolution timestamp as nanoseconds.
 *
 * @returns nanosecond timestamp.
 */
hrtime_t __libc_Back_timeHighResNano(void)
{
    /*
     * Calc factor the first time.
     */
    static ULONG        ulFreq;
#ifdef USE_INTEGER
    static unsigned     uMult;
#endif
    if (!ulFreq)
    {
        int rc = DosTmrQueryFreq(&ulFreq);
        if (rc)
            return HRTIME_INFINITY;
#ifdef USE_INTEGER /* quick approximation, very assumpive. */
        ulFreq  /=    1000;
        uMult    = 1000000;
#endif
    }

    /*
     * Get time and recalc it to nanoseconds.
     */
    unsigned long long  ullCurrent = 0;
    int rc = DosTmrQueryTime((void *)&ullCurrent);
    if (!rc)
#ifdef USE_INTEGER
        return (ullCurrent * uMult) / ulFreq;
#else
    {
        long double lrd = ullCurrent;
        return lrd * 1000000000U / ulFreq;
    }
#endif

    /* failure - should not happen! */
    return HRTIME_INFINITY;
}

