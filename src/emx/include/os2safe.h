/* os2safe.h,v 1.1 2003/10/06 00:54:20 bird Exp */
/** @file
 * OS/2 API High Memory Wrappers.
 * You include this file before os2.h and link with -los2safe.
 */

/*
 *
 * Copyright (c) 2003-2015 knut st. osmundsen <bird-srcspam@anduin.net>
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
#define DosEnumAttribute        SafeDosEnumAttribute

#define DosMapCase              SafeDosMapCase
#define DosQueryCollate         SafeDosQueryCollate
#define DosQueryCp              SafeDosQueryCp
#define DosQueryCtryInfo        SafeDosQueryCtryInfo
#define DosQueryDBCSEnv         SafeDosQueryDBCSEnv

#define WinUpper                SafeWinUpper

/*
 * Note! The DOS queue API does not work with data, but with addresses, request
 * numbers, priorities and process IDs.  I.e. the data address you give
 * DosWriteQueue is NOT used to memcpy() what it points to into some internal
 * buffer that is then queued.  Instead that address is converted to 16-bit and
 * placed on the queue.
 *
 * This means that the data pointer passed to DosWriteQueue CANNOT be a high
 * address!  (The wrappers below makes sure all the other pointer parameters
 * can be pointing to high memory, though.)
 */
#define DosCreateQueue          SafeDosCreateQueue
#define DosOpenQueue            SafeDosOpenQueue
#define DosPeekQueue            SafeDosPeekQueue
#define DosQueryQueue           SafeDosQueryQueue
#define DosReadQueue            SafeDosReadQueue

#endif

