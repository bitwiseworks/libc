/* $Id: nullstub.c 1972 2005-05-06 03:36:47Z bird $ */
/** @file
 *
 * LIBC - compatiblity stubs.
 *
 * Copyright (c) 2005 knut st. osmundsen <bird@anduin.net>
 *
 *
 * This file is part of kBuild.
 *
 * kBuild is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * kBuild is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with kBuild; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include <stdlib.h>
#include <unistd.h>


/*******************************************************************************
*   Global Variables                                                           *
*******************************************************************************/
char __nullstub_data[32] = {0};


int __nullstub_function(void);
int __nullstub_function(void)
{
    write(2, "\r\n__stub_function\r\n", sizeof("\r\n__stub_function\r\n") - 1);
    abort();
    return -1;
}

