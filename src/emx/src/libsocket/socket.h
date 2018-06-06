/* $Id: socket.h 3676 2011-03-14 17:37:38Z bird $ */
/** @file
 *
 * Interal libsocket stuff.
 *
 * Copyright (c) 2003-2004 knut st. osmundsen <bird-srcspam@anduin.net>
 *
 *
 * This file is part of Innotek LIBC.
 *
 * Innotek LIBC is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Innotek LIBC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Innotek LIBC; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef __socket_h__
#define __socket_h__

#include <InnoTekLIBC/tcpip.h>
#if 0
/* force all objects using socket.h to link in the init routine. */
asm(".stabs  \"___libsocket_init\",1,0,0,0");
#endif

struct sockaddr;
                                                                     
/** 
 * Safe socket address state info.
 */
typedef struct __LIBSOCKET_SAFEADDR
{
    /** What to feed the API. */
    struct sockaddr    *pAddr;
    /** What to feed the API. */
    int                *pcbAddr;

    /** Bounce buffer storage of the address length. */
    int                 cbAddr;
    /** The input address length value */
    int                 cbAlloc;
    /** The address structure if bouncing was needed, NULL if none of the two
     * parameters was in high memory. */
    void               *pvFree;
} __LIBSOCKET_SAFEADDR;         


int __libsocket_safe_copy(void *pvDst, void const *pvSrc, size_t cbCopy);
int __libsocket_safe_addr_pre(const struct sockaddr *pAddr, const int *pcbAddr, __LIBSOCKET_SAFEADDR *pSafeAddr);
int __libsocket_safe_addr_post(struct sockaddr *pAddr, int *pcbAddr, __LIBSOCKET_SAFEADDR *pSafeAddr, int fSuccess);
void __libsocket_safe_addr_free(__LIBSOCKET_SAFEADDR *pSafeAddr);

#endif
