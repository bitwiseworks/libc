/* $Id: getloadavg.c 2254 2005-07-17 12:25:44Z bird $ */
/** @file
 *
 * getloadavg - BSD Interface.
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
#include "libc-alias.h"
#include <stdlib.h>
#include <InnoTekLIBC/backend.h>
#define __LIBC_LOG_GROUP    __LIBC_LOG_GRP_MISC
#include <InnoTekLIBC/logstrict.h>


/**
 *
 * The getloadavg() function returns the number of processes in the system
 * run queue averaged over various periods of time.  The system imposes a
 * maximum of 3 samples, representing averages over the last 1, 5, and 15
 * minutes, respectively.
 *
 * @returns 0 on success.
 * @returns -1 and errno on failure.
 * @param   loadavg     Where to store the load average samples.
 * @param   nelem       Number of samples to retrieve.
 *
 * @remark
 * Of course on OS/2 we have no such averages calculated by the kernel so
 * we must make our own approximations. Since it's a rather costy affair to
 * get the number of ready and running threads and we have no service suitable
 * for doing this in anyway, we'll have to resort to less accurate means.
 * The current approach is to recalc the average everytime getloadavg()
 * is called, but never do it more frequently than every 5 seconds.
 *
 * @remark  The getloadavg() function appeared in 4.3BSD-Reno.
 */
int _STD(getloadavg)(double loadavg[], int nelem)
{
    LIBCLOG_ENTER("loadavg=%p nelem=%d\n", (void *)loadavg, nelem);
    int rc = __libc_Back_miscLoadAvg(loadavg, nelem);
    if (!rc)
        LIBCLOG_RETURN_INT(0);
    errno = -rc;
    LIBCLOG_ERROR_RETURN_INT(-1);
}

