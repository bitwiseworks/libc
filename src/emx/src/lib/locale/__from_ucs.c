/* $Id: __from_ucs.c 1519 2004-09-27 02:15:07Z bird $ */
/** @file
 *
 * Locale - Convert a char from unicode to mbcs.
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

#include <InnoTekLIBC/locale.h>
#define INCL_FSMACROS
#include <os2emx.h>

/** Convert a Unicode character to MBCS. */
extern int  __libc_ucs2From(UconvObject uobj, UniChar c, unsigned char *sbcs, size_t len)
{
    UniChar *inbuf = &c;
    void *outbuf = sbcs;
    size_t nonid, in_left = 1, out_left = len;
    int rc;
    FS_VAR();

    FS_SAVE_LOAD();
    rc = UniUconvFromUcs(uobj, &inbuf, &in_left, &outbuf, &out_left, &nonid);
    FS_RESTORE();
    if (rc || nonid || in_left)
        return 0;

    return len - out_left;
}
