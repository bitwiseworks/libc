/* os2safe.h,v 1.1 2003/10/06 00:54:20 bird Exp */
/** @file
 *
 * OS/2 API High Memory Wrappers.
 * You include this file before os2.h and link with -los2safe.
 *
 * Copyright (c) 2003-2014 knut st. osmundsen <bird-srcspam@anduin.net>
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with This program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#ifndef __os2safe_h__
#define __os2safe_h__

#if defined(_OS2_H) || defined(_OS2EMX_H) || defined(__OS2_H__) || defined(OS2_INCLUDED)
# warning "os2safe.h must be included _before_ os2.h!"
#endif

#define DosForceDelete          SafeDosForceDelete
#define DosQueryHType           SafeDosQueryHType
#define DosCreatePipe           SafeDosCreatePipe
#define DosQueryNPHState        SafeDosQueryNPHState
#define DosWaitNPipe            SafeDosWaitNPipe
#define DosSetFilePtr           SafeDosSetFilePtr
#define DosSetFilePtrL          SafeDosSetFilePtrL
#define DosDupHandle            SafeDosDupHandle
#define DosOpen                 SafeDosOpen
#define DosOpenL                SafeDosOpenL
#define DosQueryFHState         SafeDosQueryFHState
#define DosSetDateTime          SafeDosSetDateTime
#define DosStartSession         SafeDosStartSession
#define DosQueryAppType         SafeDosQueryAppType
#define DosDevIOCtl             SafeDosDevIOCtl
#define WinUpper                SafeWinUpper

#endif
