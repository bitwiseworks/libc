/* $Id: safe.h 810 2003-10-06 00:55:10Z bird $ */
/** @file
 *
 * Macros, prototypes and inline helpers.
 *
 * Copyright (c) 2003 knut st. osmundsen <bird-srcspam@anduin.net>
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with This program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#ifndef __safe_h__
#define __safe_h__


#include <stdlib.h>
#include <string.h>
void *  _lmalloc(unsigned);

/** Check if ptr is in high memory or not. */
#define SAFE_IS_HIGH(ptr)   ((unsigned)(ptr) >= 512*1024*1024)



/** Wrap a const string. */
#define SAFE_PCSZ(arg) \
    char * arg##_safe = (char*)arg;                 \
    if (SAFE_IS_HIGH(arg))                          \
    {                                               \
        int cch = strlen((char*)arg) + 1;           \
        arg##_safe = _lmalloc(cch);                 \
        if (!arg##_safe) goto safe_failure;         \
        memcpy(arg##_safe, (arg), cch);             \
    }                                               \
        {


/** Use the const string. */
#define SAFE_PCSZ_USE(arg) (PCSZ)arg##_safe

/** Cleanup a const string. */
#define SAFE_PCSZ_DONE(arg) \
        }                                           \
    if (arg##_safe != (char*)arg) free(arg##_safe)


/** Generic failure label for Dos API wappers.
 * It'll just return out of memory error. */
#define SAFE_DOS_FAILURE() \
    if (0)                                          \
    {                                               \
        safe_failure:                               \
            rc = 8;                                 \
    }


#endif
