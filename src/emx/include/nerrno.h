/* nerrno.h,v 1.1 2003/12/10 19:13:03 bird Exp */
/** @file
 * IBM Toolkit Compatability.
 *
 * Copyright (c) 2003 InnoTek Systemberatung GmbH
 * Author: knut st. osmundsen <bird-srcspam@anduin.net>
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with This program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef _NERROR_H_
#define _NERROR_H_


/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include <errno.h>


/*******************************************************************************
*   Defined Constants And Macros                                               *
*******************************************************************************/
#define SOCEPERM           EPERM
#define SOCENOENT          ENOENT
#define SOCESRCH           ESRCH
#define SOCEINTR           EINTR
#define SOCEIO             EIO
#define SOCENXIO           ENXIO
#define SOCE2BIG           E2BIG
#define SOCENOEXEC         ENOEXEC
#define SOCEBADF           EBADF
#define SOCECHILD          ECHILD
#define SOCEDEADLK         EDEADLK
#define SOCENOMEM          ENOMEM
#define SOCEACCES          EACCES
#define SOCEFAULT          EFAULT
#define SOCENOTBLK         ENOTBLK
#define SOCEBUSY           EBUSY
#define SOCEEXIST          EEXIST
#define SOCEXDEV           EXDEV
#define SOCENODEV          ENODEV
#define SOCENOTDIR         ENOTDIR
#define SOCEISDIR          EISDIR
#define SOCEINVAL          EINVAL
#define SOCENFILE          ENFILE
#define SOCEMFILE          EMFILE
#define SOCENOTTY          ENOTTY
#define SOCETXTBSY         ETXTBSY
#define SOCEFBIG           EFBIG
#define SOCENOSPC          ENOSPC
#define SOCESPIPE          ESPIPE
#define SOCEROFS           EROFS
#define SOCEMLINK          EMLINK
#define SOCEPIPE           EPIPE


#define SOCEDOM            EDOM
#define SOCERANGE          ERANGE


#define SOCEAGAIN          EAGAIN
#define SOCEWOULDBLOCK     EWOULDBLOCK
#define SOCEINPROGRESS     EINPROGRESS
#define SOCEALREADY        EALREADY


#define SOCENOTSOCK        ENOTSOCK
#define SOCEDESTADDRREQ    EDESTADDRREQ
#define SOCEMSGSIZE        EMSGSIZE
#define SOCEPROTOTYPE      EPROTOTYPE
#define SOCENOPROTOOPT     ENOPROTOOPT
#define SOCEPROTONOSUPPORT EPROTONOSUPPORT
#define SOCESOCKTNOSUPPORT ESOCKTNOSUPPORT
#define SOCEOPNOTSUPP      EOPNOTSUPP
#define SOCEPFNOSUPPORT    EPFNOSUPPORT
#define SOCEAFNOSUPPORT    EAFNOSUPPORT
#define SOCEADDRINUSE      EADDRINUSE
#define SOCEADDRNOTAVAIL   EADDRNOTAVAIL


#define SOCENETDOWN        ENETDOWN
#define SOCENETUNREACH     ENETUNREACH
#define SOCENETRESET       ENETRESET
#define SOCECONNABORTED    ECONNABORTED
#define SOCECONNRESET      ECONNRESET
#define SOCENOBUFS         ENOBUFS
#define SOCEISCONN         EISCONN
#define SOCENOTCONN        ENOTCONN
#define SOCESHUTDOWN       ESHUTDOWN
#define SOCETOOMANYREFS    ETOOMANYREFS
#define SOCETIMEDOUT       ETIMEDOUT
#define SOCECONNREFUSED    ECONNREFUSED

#define SOCELOOP           ELOOP
#define SOCENAMETOOLONG    ENAMETOOLONG


#define SOCEHOSTDOWN       EHOSTDOWN
#define SOCEHOSTUNREACH    EHOSTUNREACH
#define SOCENOTEMPTY       ENOTEMPTY


#define SOCEPROCLIM        EPROCLIM
#define SOCEUSERS          EUSERS
#define SOCEDQUOT          EDQUOT


#define SOCESTALE          ESTALE
#define SOCEREMOTE         EREMOTE
#define SOCEBADRPC         EBADRPC
#define SOCERPCMISMATCH    ERPCMISMATCH
#define SOCEPROGUNAVAIL    EPROGUNAVAIL
#define SOCEPROGMISMATCH   EPROGMISMATCH
#define SOCEPROCUNAVAIL    EPROCUNAVAIL


#define SOCENOLCK          ENOLCK
#define SOCENOSYS          ENOSYS

#define SOCEFTYPE          EFTYPE
#define SOCEAUTH           EAUTH
#define SOCENEEDAUTH       ENEEDAUTH

#define SOCEOS2ERR         (100)
#define SOCELAST           ELAST

#endif
