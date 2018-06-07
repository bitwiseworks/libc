/* $Id: b_dir.h 2424 2005-10-26 21:32:29Z bird $ */
/** @file
 *
 * LIBC SYS Backend - Directory stuff.
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


#ifndef __b_dir_h__
#define __b_dir_h__


/**
 * A directory handle.
 */
typedef struct __LIBC_FHDIR
{
    /** The common fh core. */
    __LIBC_FH       Core;
    /** The directory handle. */
    HDIR            hDir;
    /** Set if the path is in the unix tree, else clear.
     * This is for fchdir() handling. */
    unsigned        fInUnixTree;
    /** The find operation type. */
    unsigned        fType;
    union
    {
        /** Pointer to the old kind of buffer. */
        PFILEFINDBUF4   p4;
        /** Pointer to the large file kind of buffer. */
        PFILEFINDBUF4L  p4L;
        /** Byte pointer used to advance the buffer entries. */
        uint8_t        *pu8;
        /** Void pointer. */
        void           *pv;
    }
    /** Pointer to the find buffer.
     * This is located in low memory. */
                    uBuf,
    /** Pointer to the current buffer position.
     * This is invalid if cFiles is 0. */
                    uCur;
    /** The number of files left in the buffer. */
    ULONG           cFiles;
    /** The size of the find buffer. */
    unsigned        cbBuf;
    /** Current entry number (total, for seeking). */
    unsigned        uCurEntry;
} __LIBC_FHDIR;
/** Pointer to the a directory handle. */
typedef __LIBC_FHDIR *__LIBC_PFHDIR;


int __libc_Back_dirOpenNative(char *pszNativePath, unsigned fInUnixTree, unsigned fLibc, struct stat *pStat);
int __libc_back_dirInherit(int fh, const char *pszNativePath, unsigned fInUnixTree, unsigned fFlags,
                           ino_t Inode, dev_t Dev, unsigned uCurEntry);
ssize_t __libc_back_dirGetEntries(__LIBC_PFHDIR pFHDir, void *pvBuf, size_t cbBuf);

#endif
