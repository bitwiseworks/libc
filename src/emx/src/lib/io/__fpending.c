/* $Id: __fpending.c 1907 2005-04-24 12:40:24Z bird $ */
/** @file
 *
 * LIBC - __fpending().
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
#include <stdio_ext.h>
#include <emx/io.h>


/**
 * Return amount of output in bytes pending on a stream FP.
 *
 * @remark SUN extension.
 */
size_t __fpending(FILE *pStream)
{
    return (pStream->_flags & _IOWRT) && _bbuf(pStream)
        ? pStream->_ptr - pStream->_buffer
        : 0;
}

