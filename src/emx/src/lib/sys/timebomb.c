/* $Id: timebomb.c 3805 2014-02-06 11:37:29Z ydario $ */
/** @file
 *
 * LIBC timebomb
 *
 * Copyright (c) 2004 knut st. osmundsen <bird-srcspam@anduin.net>
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


#include "libc-alias.h"
#include <emx/startup.h>
#define INCL_BASE
#define INCL_FSMACROS
#include <os2emx.h>

#ifdef TIMEBOMB
_CRT_INIT1(__libc_Timebomb)
#endif

CRT_DATA_USED
void __libc_Timebomb(void)
{
    DATETIME    dt = {0,0,0,0,31,12,2004,0};
    FS_VAR();

    FS_SAVE_LOAD();
    DosGetDateTime(&dt);
    if (dt.year == 2004 && (dt.month == 10 || dt.month == 11 || dt.month == 12))
    {
        FS_RESTORE();
        return;
    }

    /* failed! */
    static char szMsg[] = "\r\n"
                          "LIBC alpha has timed out!!\r\n"
                          "\r\n"
                          "It was clearly stated in the release documents that this LIBC\r\n"
                          "release was timebombed because of non-finalized shared structures.\r\n"
                          "The purpose of the release was to get feedback on new features,\r\n"
                          "particularly on the fork() implementation\r\n"
                          "\r\n";

    ULONG cb;
    DosWrite(2, szMsg, sizeof(szMsg), &cb);
    for (;;)
        DosExit(EXIT_PROCESS, 127);
}

