/* $Id: __do_uni.c 1519 2004-09-27 02:15:07Z bird $ */
/** @file
 *
 * Locale - String processor.
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
#include <alloca.h>
#include <malloc.h>
#include <string.h>
#define INCL_FSMACROS
#include <os2emx.h>

/**
 * Utility routine that converts a string to Unicode, calls some
 * function and then converts the string back.
 * Used to handle MBCS encodings
 */
extern void __libc_ucs2Do(UconvObject *uconv, char *s, void *arg, int (*xform)(UniChar *, void *))
{
    /* We'll get maximum that much UniChar's */
    size_t sl = strlen(s) + 1;
    char free_ucs = 0;
    UniChar *ucs;
    void *mbcsbuf;
    UniChar *ucsbuf;
    size_t in_left, out_left, nonid;
    FS_VAR();
    FS_SAVE_LOAD();

    /* Allocate strings up to 2000 characters on the stack */
    if (sl < 4096 / sizeof(UniChar))
        ucs = alloca(sl * sizeof(UniChar));
    else
    {
        free_ucs = 1;
        ucs = malloc(sl * sizeof(UniChar));
    }

    /* Translate the string to Unicode */
    mbcsbuf = s; in_left = sl;
    ucsbuf = ucs; out_left = sl;
    if (UniUconvToUcs(uconv, &mbcsbuf, &in_left, &ucsbuf, &out_left, &nonid))
        goto out;

    /* Apply the transform function */
    if (xform(ucs, arg))
    {
        /* Now convert back to MBCS */
        ucsbuf = ucs; in_left = sl - out_left;
        mbcsbuf = s; out_left = sl;
        UniUconvFromUcs(uconv, &ucsbuf, &in_left, &mbcsbuf, &out_left, &nonid);
    }

out:
    if (free_ucs)
        free(ucs);
    FS_RESTORE();
}
