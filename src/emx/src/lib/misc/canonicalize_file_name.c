/* $Id: canonicalize_file_name.c 1519 2004-09-27 02:15:07Z bird $ */
/** @file
 *
 * canonicalize_file_name() - GNU Extension.
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
#define _GNU_SOURCE
#include <stdlib.h>


/**
 * Get the canonicalized absolute path for a file.
 *
 * @returns Pointer to malloc'ed buffer containing the resolved path.
 *          Call is responsible for freeing it.
 * @returns NULL and errno on failure.
 * @param   path    Path to canonicalize.
 */
char *_STD(canonicalize_file_name)(const char *path)
{
    return realpath(path, NULL);
}

