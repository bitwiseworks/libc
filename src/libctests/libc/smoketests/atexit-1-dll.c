/* $Id: $ */
/** @file
 *
 * A dll for the atexit-1 testcase, ticket #113.
 *
 * Copyright (c) 2006 knut st. osmundsen <bird@innotek.de>
 *
 *
 * This file is part of kLIBC.
 *
 * kLIBC is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * kLIBC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with kLIBC; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>


void atexit_callback(void)
{
    /* do nothing */
    abort();
}


void on_exit_callback(int rc, void *pvUser)
{
    /* do nothing */
    abort();
}


void __attribute__((__constructor__)) foo(void)
{
    if (atexit(atexit_callback))
    {
        printf("atexit failed: %d - %s\n", errno, strerror(errno));
        abort();
    }

    if (on_exit(on_exit_callback, (void *)42))
    {
        printf("on_exit failed: %d - %s\n", errno, strerror(errno));
        abort();
    }
    printf("atexit-1: successfully registered the atexit and on_exit callbacks.\n");
}


