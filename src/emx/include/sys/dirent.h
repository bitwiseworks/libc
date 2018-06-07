/*-
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)dirent.h	8.3 (Berkeley) 8/10/94
 * $FreeBSD: src/sys/sys/dirent.h,v 1.13 2002/09/10 18:12:16 mike Exp $
 */

/** @file
 * FreeBSD 5.1
 * @changed bird: Merged in all the EMX stuff.
 * @changed bird: glibc hack.
 * @changed bird: MAXNAMLEN vs NAME_MAX.
 * @changed bird: Added _D_EXACT_NAMLEN and _D_ALLOC_NAMLEN macros to sys/dirent.h to
 *                make GLIBC code happy.
 */

#ifndef	_SYS_DIRENT_H_
#define	_SYS_DIRENT_H_

#include <sys/cdefs.h>
#include <sys/_types.h>
#if defined(__USE_GNU) && !defined(MAXNAMLEN)
/* Fixing a common problem with tests using NAME_MAX without including limits.h. */
# include <sys/param.h>
#endif


/*
 * The dirent structure defines the format of directory entries returned by
 * the getdirentries(2) system call.
 *
 * A directory entry has a struct dirent at the front of it, containing its
 * inode number, the length of the entry, and the length of the name
 * contained in the entry.  These are followed by the name padded to a 4
 * byte boundary with null bytes.  All names are guaranteed null terminated.
 * The maximum length of a name in a directory is MAXNAMLEN.
 */

struct dirent {
	__uint32_t d_fileno;		/* file number of entry */
	__uint16_t d_reclen;		/* length of this record */
	__uint8_t  d_type; 		/* file type, see below */
	__uint8_t  d_namlen;		/* length of string in d_name */
#if __BSD_VISIBLE
#ifndef MAXNAMLEN                   /* bird */
#ifdef NAME_MAX                     /* bird */
#define	MAXNAMLEN	NAME_MAX    /* bird */
#else                               /* bird */
#define	MAXNAMLEN	256
#endif                              /* bird */
#endif                              /* bird */
	char	d_name[MAXNAMLEN + 1];	/* name must be no longer than this */
#else
	char	d_name[255 + 1];	/* name must be no longer than this */
#endif
/* bird: Extra EMX fields - start */ /** @todo move these up before the name! LIBC07 */
        __uint8_t  d_attr;              /* OS file attributes        */
        __uint16_t d_time;              /* OS file modification time */
        __uint16_t d_date;              /* OS file modification date */
        __off_t    d_size;              /* File size (bytes)         */
/* bird: Extra EMX fields - end */
};

#ifdef __USE_GNU
/* Macros GNU GLIBC defines for accessing the d_namelen variable correctly. */
#define _D_EXACT_NAMLEN(pdir)   ( (pdir)->d_namlen )
#define _D_ALLOC_NAMLEN(pdir)   ( (pdir)->d_namlen + 1 )
#endif

#if __BSD_VISIBLE
/*
 * File types
 */
#define	DT_UNKNOWN	 0
#define	DT_FIFO		 1
#define	DT_CHR		 2
#define	DT_DIR		 4
#define	DT_BLK		 6
#define	DT_REG		 8
#define	DT_LNK		10
#define	DT_SOCK		12
#define	DT_WHT		14

/*
 * Convert between stat structure types and directory types.
 */
#define	IFTODT(mode)	(((mode) & 0170000) >> 12)
#define	DTTOIF(dirtype)	((dirtype) << 12)

/*
 * The _GENERIC_DIRSIZ macro gives the minimum record length which will hold
 * the directory entry.  This requires the amount of space in struct direct
 * without the d_name field, plus enough space for the name with a terminating
 * null byte (dp->d_namlen+1), rounded up to a 4 byte boundary.
 *
 * XXX although this macro is in the implementation namespace, it requires
 * a manifest constant that is not.
 */
#define	_GENERIC_DIRSIZ(dp) \
    ((sizeof (struct dirent) - (MAXNAMLEN+1)) + (((dp)->d_namlen+1 + 3) &~ 3))
#endif /* __BSD_VISIBLE */

#ifdef _KERNEL
#define	GENERIC_DIRSIZ(dp)	_GENERIC_DIRSIZ(dp)
#endif


/* bird: EMX stuff */
#define _DIRENT_D_MODE_RENAMED_D_ATTR

#if !defined (MAXPATHLEN)
#define MAXPATHLEN 260
#endif

#if !defined (A_RONLY)
#define A_RONLY   0x01
#define A_HIDDEN  0x02
#define A_SYSTEM  0x04
#define A_LABEL   0x08
#define A_DIR     0x10
#define A_ARCHIVE 0x20
#endif

#endif /* not SYS_DIRENT_H */
