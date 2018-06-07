/* $Id: b_time.c 3709 2011-03-17 01:00:53Z bird $ */
/** @file
 *
 * LIBC SYS Backend - times helpers.
 *
 * Copyright (c) 2005 knut st. osmundsen <bird@anduin.net>
 *
 *
 * This file is part of kLIBC.
 *
 * kLIBC is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * kLIBC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with kLIBC; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include "libc-alias.h"
#define INCL_FSMACROS
#include <os2emx.h>
#include "b_time.h"
#include <time.h>
#include <emx/time.h>


/**
 * Converts local unix time to file time.
 *
 * @param   Time    Local unix time (secs).
 * @param   pTime   Where to store the time.
 * @param   pDate   Where to store the date.
 */
void __libc_back_timeUnix2FileTime(time_t Time, PFTIME pTime, PFDATE pDate)
{
    struct tm Tm;
    
    if (!_tzset_flag)
        tzset();
    gmtime_r(&Time, &Tm);
    
    if (Tm.tm_year >= 80)
    {
        pTime->twosecs   = Tm.tm_sec / 2;
        pTime->minutes   = Tm.tm_min;
        pTime->hours     = Tm.tm_hour;
        pDate->day       = Tm.tm_mday;
        pDate->month     = Tm.tm_mon + 1;
        pDate->year      = Tm.tm_year - 1980 + 1900;
    }
    else
    {
        pTime->twosecs   = 0;
        pTime->minutes   = 0;
        pTime->hours     = 0;
        pDate->day       = 1;
        pDate->month     = 1;
        pDate->year      = 0;
    }
}

