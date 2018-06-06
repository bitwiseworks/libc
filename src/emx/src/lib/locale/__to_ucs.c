/* $Id: __to_ucs.c 1519 2004-09-27 02:15:07Z bird $ */
/** @file
 *
 * Locale - Confer from MBCS to Unicode.
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


/** Convert a MBCS character to Unicode; returns number of bytes in MBCS char. */
int  __libc_ucs2To(UconvObject uobj, const unsigned char *sbcs, size_t len, UniChar *ucs)
{
    void *inbuf = (void *)sbcs;
    UniChar *outbuf = ucs;
    size_t nonid, in_left = len, out_left = 1;
    int ret;
    FS_VAR();

    FS_SAVE_LOAD();
    ret = UniUconvToUcs(uobj, &inbuf, &in_left, &outbuf, &out_left, &nonid);
    FS_RESTORE();

    if ((ret && (ret != ULS_BUFFERFULL)) || nonid || out_left)
        return 0;

    return len - in_left;
}
