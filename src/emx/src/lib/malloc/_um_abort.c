/* $Id: _um_abort.c 2205 2005-07-04 02:01:50Z bird $ */
/** @file
 *
 * LIBC - _um_abort, internal heap abort routine.
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
#include <InnoTekLIBC/backend.h>

void _um_abort(const char *pszMsg, ...)
{
    va_list args;
    va_start(args, pszMsg);
    __libc_Back_panicV(0, NULL, pszMsg, args);
    va_end(args);
}

