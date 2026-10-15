/* $Id: direct.h 3809 2014-02-16 20:20:59Z bird $ */
/** @file
 *
 * direct.h - VAC/MSC legacy.
 *
 * Copyright (c) 2004 knut st. osmundsen <bird-srcspam@anduin.net>
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

#ifndef _DIRECT_H_
#define _DIRECT_H_

#include <sys/cdefs.h>
#include <sys/_types.h>

#if !defined(_MODE_T_DECLARED) && !defined(_MODE_T)
typedef	__mode_t	mode_t;
#define	_MODE_T_DECLARED
#define _MODE_T
#endif

#if !defined(_SIZE_T_DECLARED) && !defined(_SIZE_T)
typedef	__size_t	size_t;
#define	_SIZE_T_DECLARED
#define	_SIZE_T
#endif

__BEGIN_DECLS

#ifndef __CHDIR_DECLARED
#define	__CHDIR_DECLARED
int _chdir (const char *);
#endif
#ifndef __GETCWD_DECLARED
#define	__GETCWD_DECLARED
char *_getcwd (char *, size_t);
#endif
#ifndef __MKDIR_DECLARED
#define	__MKDIR_DECLARED
int _mkdir (const char *, long);
#endif
#ifndef __RMDIR_DECLARED
#define	__RMDIR_DECLARED
int _rmdir (const char *);
#endif

#ifndef _CHDIR_DECLARED
#define	_CHDIR_DECLARED
int chdir (const char *);
#endif
#ifndef _GETCWD_DECLARED
#define	_GETCWD_DECLARED
char *getcwd (char *, size_t);
#endif
#ifndef _MKDIR_DECLARED
#define	_MKDIR_DECLARED
int mkdir(const char *, mode_t);
#endif
#ifndef _RMDIR_DECLARED
#define	_RMDIR_DECLARED
int rmdir (const char *);
#endif

#ifndef __CHDRIVE_DECLARED
#define	__CHDRIVE_DECLARED
int _chdrive(int);
#endif
char * _getdcwd(int, char *, int);
#ifndef __GETDRIVE_DECLARED
#define	__GETDRIVE_DECLARED
int _getdrive(void);
#endif

/* Special LIBC addition. */
#ifdef __BSD_VISIBLE
#ifndef __GETCWDUX_DECLARED
#define	__GETCWDUX_DECLARED
char *_getcwdux(char *, size_t);
#endif
#endif

__END_DECLS

#endif
