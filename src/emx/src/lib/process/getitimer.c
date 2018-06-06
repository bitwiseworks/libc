/* $Id: getitimer.c 2254 2005-07-17 12:25:44Z bird $ */
/** @file
 *
 * LIBC - getitimer().
 *
 * Copyright (c) 2004 knut st. osmundsen <bird@innotek.de>
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
#include "libc-alias.h"
#include <sys/time.h>
#include <sys/errno.h>
#include <InnoTekLIBC/backend.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_SIGNAL
#include <InnoTekLIBC/logstrict.h>


/**
 * Gets the current values of a timer.
 *
 * @returns 0 on success.
 * @returns -1 and errno on failure.
 * @param   iWhich  Which timer to get, any of the ITIMER_* #defines.
 *                  OS/2 only supports ITIMER_REAL.
 * @param   pValue  Where to store the value.
 */
int _STD(getitimer)(int iWhich, struct itimerval *pValue)
{
    LIBCLOG_ENTER("iWhich=%d pValue=%p\n", iWhich, (void *)pValue);

    int rc = __libc_Back_signalTimer(iWhich, NULL, pValue);
    if (!rc)
        LIBCLOG_RETURN_MSG(0, "ret 0 *pValue={.ti_value={.ti_sec=%ld, .ti_usec=%ld}, ti_interval={.ti_sec=%ld, .ti_usec=%ld}}\n",
                           pValue->it_value.tv_sec, pValue->it_value.tv_usec, pValue->it_interval.tv_sec, pValue->it_interval.tv_usec);
    errno = -rc;
    LIBCLOG_ERROR_RETURN_INT(-1);
}

