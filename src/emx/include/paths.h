/* paths.h,v 1.2 2004/09/14 22:27:35 bird Exp */
/** @file
 * FreeBSD 5.2
 * @changed bird: /@unixroot
 * @changed bird: A few of the /dev/'s.
 */

/*
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
 *	@(#)paths.h	8.1 (Berkeley) 6/2/93
 * $FreeBSD: src/include/paths.h,v 1.25 2004/01/04 17:17:46 iedowse Exp $
 */

#ifndef _PATHS_H_
#define	_PATHS_H_

#include <sys/cdefs.h>

/* Default search path. */
#define	_PATH_DEFPATH	"/@unixroot/usr/bin"
/* All standard utilities path. */
#define	_PATH_STDPATH \
	"/@unixroot/usr/bin:/@unixroot/usr/sbin"
/* Locate system binaries */
#define _PATH_SYSPATH	\
	"/@unixroot/usr/sbin"

#define	_PATH_AUTHCONF	"/@unixroot/etc/auth.conf"
#define	_PATH_BSHELL	"/@unixroot/usr/bin/sh"
#define	_PATH_CAPABILITY	"/@unixroot/etc/capability"
#define	_PATH_CAPABILITY_DB	"/@unixroot/etc/capability.db"
#define	_PATH_CONSOLE	"/dev/con"
#define	_PATH_CP	"/@unixroot/usr/bin/cp"
#define	_PATH_CSHELL	"/@unixroot/usr/bin/csh"
#define	_PATH_DEFTAPE	"/@unixroot/dev/sa0"
#define	_PATH_DEVNULL	"/dev/null"
#define	_PATH_DEVZERO	"/@unixroot/dev/zero"
#define	_PATH_DRUM	"/@unixroot/dev/drum"
#define	_PATH_ETC	"/@unixroot/etc"
#define	_PATH_FTPUSERS	"/@unixroot/etc/ftpusers"
#define	_PATH_HALT	"/@unixroot/usr/sbin/halt"
#define	_PATH_IFCONFIG	"/@unixroot/usr/sbin/ifconfig"
#define	_PATH_KMEM	"/@unixroot/dev/kmem"
#define	_PATH_LIBMAP_CONF	"/@unixroot/etc/libmap.conf"
#define	_PATH_LOCALE	"/@unixroot/usr/share/locale"
#define	_PATH_LOGIN	"/@unixroot/usr/bin/login"
#define	_PATH_MAILDIR	"/@unixroot/var/mail"
#define	_PATH_MAN	"/@unixroot/usr/share/man"
#define	_PATH_MDCONFIG	"/@unixroot/usr/sbin/mdconfig"
#define	_PATH_MEM	"/@unixroot/dev/mem"
#define	_PATH_MKSNAP_FFS	"/@unixroot/usr/sbin/mksnap_ffs"
#define	_PATH_MOUNT	"/@unixroot/usr/sbin/mount"
#define	_PATH_NEWFS	"/@unixroot/usr/sbin/newfs"
#define	_PATH_NOLOGIN	"/@unixroot/var/run/nologin"
#define	_PATH_RCP	"/@unixroot/usr/bin/rcp"
#define	_PATH_REBOOT	"/@unixroot/usr/sbin/reboot"
#define	_PATH_RLOGIN	"/@unixroot/usr/bin/rlogin"
#define	_PATH_RM	"/@unixroot/usr/bin/rm"
#define	_PATH_RSH	"/@unixroot/usr/bin/rsh"
#define	_PATH_SENDMAIL	"/@unixroot/usr/sbin/sendmail"
#define	_PATH_SHELLS	"/@unixroot/etc/shells"
#define	_PATH_TTY	"/dev/tty"
#define	_PATH_UNIX	"don't use _PATH_UNIX"
#define	_PATH_VI	"/@unixroot/usr/bin/vi"
#define	_PATH_WALL	"/@unixroot/usr/bin/wall"

/* Provide trailing slash, since mostly used for building pathnames. */
#define	_PATH_DEV	"/@unixroot/dev/"
#define	_PATH_TMP	"/@unixroot/tmp/"
#define	_PATH_VARDB	"/@unixroot/var/db/"
#define	_PATH_VARRUN	"/@unixroot/var/run/"
#define	_PATH_VARTMP	"/@unixroot/var/tmp/"
#define	_PATH_YP	"/@unixroot/var/yp/"
#define	_PATH_UUCPLOCK	"/@unixroot/var/spool/lock/"

/* How to get the correct name of the kernel. */
__BEGIN_DECLS
const char *getbootfile(void);
__END_DECLS

#ifdef RESCUE
#undef	_PATH_DEFPATH
#define	_PATH_DEFPATH	"/@unixroot/rescue:/@unixroot/usr/bin"
#undef	_PATH_STDPATH
#define	_PATH_STDPATH	"/@unixroot/rescue:/@unixroot/usr/bin:/@unixroot/usr/sbin"
#undef	_PATH_SYSPATH
#define	_PATH_SYSPATH	"/@unixroot/rescue:/@unixroot/usr/sbin"
#undef	_PATH_BSHELL
#define	_PATH_BSHELL	"/@unixroot/rescue/sh"
#undef	_PATH_CP
#define	_PATH_CP	"/@unixroot/rescue/cp"
#undef	_PATH_CSHELL
#define	_PATH_CSHELL	"/@unixroot/rescue/csh"
#undef	_PATH_HALT
#define	_PATH_HALT	"/@unixroot/rescue/halt"
#undef	_PATH_IFCONFIG
#define	_PATH_IFCONFIG	"/@unixroot/rescue/ifconfig"
#undef	_PATH_MDCONFIG
#define	_PATH_MDCONFIG	"/@unixroot/rescue/mdconfig"
#undef	_PATH_MOUNT
#define	_PATH_MOUNT	"/@unixroot/rescue/mount"
#undef	_PATH_NEWFS
#define	_PATH_NEWFS	"/@unixroot/rescue/newfs"
#undef	_PATH_RCP
#define	_PATH_RCP	"/@unixroot/rescue/rcp"
#undef	_PATH_REBOOT
#define	_PATH_REBOOT	"/@unixroot/rescue/reboot"
#undef	_PATH_RM
#define	_PATH_RM	"/@unixroot/rescue/rm"
#undef	_PATH_VI
#define	_PATH_VI	"/@unixroot/rescue/vi"
#undef	_PATH_WALL
#define	_PATH_WALL	"/@unixroot/rescue/wall"
#endif /* RESCUE */

#endif /* !_PATHS_H_ */
