/* $Id: _hstrdup.c 2312 2005-08-28 06:14:50Z bird $ */
/** @file
 *
 * LIBC - High Memory Heap - strdup().
 *
 * Copyright (c) 2005 knut st. osmundsen <bird@anduin.net>
 *
 *
 * This file is part of InnoTek LIBC.
 *
 * InnoTek LIBC is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * InnoTek LIBC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with InnoTek LIBC; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "libc-alias.h"
#include <emx/umalloc.h>
#include <string.h>


/**
 * Duplicates the specified string in a heapblock
 * from the high heap.
 *
 * @returns Pointer to heap block with a copy of the string.
 * @param   psz     Pointer to the string to duplicate.
 */
char *  _hstrdup(const char *psz)
{
    if (__predict_true(psz != NULL))
    {
        size_t cch = strlen(psz) + 1;
        char *pszCopy = _hmalloc(cch);
        if (__predict_true(pszCopy != NULL))
            return (char *)memcpy(pszCopy, psz, cch);
    }
    return NULL;
}

