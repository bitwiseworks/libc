/* $Id: setitimer.c 2254 2005-07-17 12:25:44Z bird $ */
/** @file
 *
 * LIBC - setitimer().
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
 * Queries and/or starts/stops a timer.
 *
 * @returns 0 on success.
 * @returns -1 and errno on failure.
 * @param   iWhich      Which timer to get, any of the ITIMER_* #defines.
 *                      OS/2 only supports ITIMER_REAL.
 * @param   pValue      Where to store the value.
 *                      Optional. If NULL pOldValue must not be NULL.
 * @param   pOldValue   Where to store the old value.
 *                      Optional. If NULL pValue must not be NULL.
 */
int _STD(setitimer)(int iWhich, const struct itimerval *pValue, struct itimerval *pOldValue)
{
    LIBCLOG_ENTER("iWhich=%d pValue=%p{.ti_value={.tv_sec=%d, .tv_usec=%ld}, ti_interval={.tv_sec=%d, .tv_usec=%ld}} pOldValue=%p\n",
                  iWhich, (void *)pValue,
                  pValue ? pValue->it_value.tv_sec : -1,
                  pValue ? pValue->it_value.tv_usec : -1,
                  pValue ? pValue->it_interval.tv_sec : -1,
                  pValue ? pValue->it_interval.tv_usec : -1,
                  (void *)pOldValue);
    int rc = __libc_Back_signalTimer(iWhich, pValue, pOldValue);
    if (!rc)
        LIBCLOG_RETURN_MSG(0, "ret 0 *pOldValue={.ti_value={.tv_sec=%d, .tv_usec=%ld}, ti_interval={.tv_sec=%d, .tv_usec=%ld}}\n",
                           pOldValue ? pOldValue->it_value.tv_sec : -1,
                           pOldValue ? pOldValue->it_value.tv_usec : -1,
                           pOldValue ? pOldValue->it_interval.tv_sec : -1,
                           pOldValue ? pOldValue->it_interval.tv_usec : -1);
    errno = -rc;
    LIBCLOG_ERROR_RETURN_INT(-1);
}

