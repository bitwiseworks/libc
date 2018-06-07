/* $Id: localeconv.c 2254 2005-07-17 12:25:44Z bird $ */
/** @file
 *
 * localeconv().
 *
 * Copyright (c) 2003 InnoTek Systemberatung GmbH
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

#define __INTERNAL_DEFS
#include "libc-alias.h"
#include <locale.h>
#include <InnoTekLIBC/locale.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_LOCALE
#include <InnoTekLIBC/logstrict.h>

/**
 * Return the address of the (static) locale information structure.
 */
struct lconv *_STD(localeconv)(void)
{
    LIBCLOG_ENTER("\n");
    LIBCLOG_RETURN_P(&__libc_gLocaleLconv.s);
}

