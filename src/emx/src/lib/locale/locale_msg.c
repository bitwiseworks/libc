/* $Id: locale_msg.c 2161 2005-07-03 00:31:40Z bird $ */
/** @file
 *
 * LIBC Locale - LC_MESSAGE data.
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

#include <InnoTekLIBC/locale.h>

/** Message locale information. */
__LIBC_LOCALEMSG            __libc_gLocaleMsg =
{
    .pszYesExpr = "^[yY]",
    .pszNoExpr = "^[nN]",
    .pszYesStr = "",
    .pszNoStr = "",
    .fConsts = 1
};

/** Message locale information for the 'C'/'POSIX' locale. */
const __LIBC_LOCALEMSG      __libc_gLocaleMsgDefault =
{
    .pszYesExpr = "^[yY]",
    .pszNoExpr = "^[nN]",
    .pszYesStr = "",
    .pszNoStr = "",
    .fConsts = 1
};
