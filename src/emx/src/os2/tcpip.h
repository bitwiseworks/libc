/* tcpip.h -- Interface to tcpip.c
   Copyright (c) 1994-1996 by Eberhard Mattes

This file is part of emx.

emx is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

emx is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with emx; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.

As special exception, emx.dll can be distributed without source code
unless it has been changed.  If you modify emx.dll, this exception
no longer applies and you must remove this paragraph from all source
files for emx.dll.  */


struct select_data;
struct _recvfrom;
struct _sendto;

int tcpip_accept (ULONG handle, void *addr, int *paddrlen, int *errnop);
int tcpip_bind (ULONG handle, void *addr, int addrlen);
int tcpip_close (ULONG handle);
int tcpip_connect (ULONG handle, void *addr, int addrlen);
int tcpip_dup (ULONG handle, ULONG target, int *errnop);
int tcpip_fcntl (ULONG handle, ULONG request, ULONG arg, int *errnop);
int tcpip_fstat (ULONG handle, struct stat *dst, int *errnop);
int tcpip_gethostbyaddr (const char *addr, int len, int type, void **dst);
int tcpip_gethostbyname (const char *name, void **dst);
int tcpip_gethostid (int *dst);
int tcpip_gethostname (char *name, int len);
int tcpip_getnetbyaddr (long net, void **dst);
int tcpip_getnetbyname (const char *name, void **dst);
int tcpip_getpeername (ULONG handle, void *addr, int *paddrlen);
int tcpip_getprotobyname (const char *name, void **dst);
int tcpip_getprotobynumber (int proto, void **dst);
int tcpip_getservbyname (const char *name, const char *proto, void **dst);
int tcpip_getservbyport (int port, const char *proto, void **dst);
int tcpip_getsockhandle (ULONG handle, int *errnop);
int tcpip_getsockname (ULONG handle, void *addr, int *paddrlen);
int tcpip_getsockopt (ULONG handle, int level, int optname, void *optval,
    int *poptlen);
int tcpip_ioctl (ULONG handle, ULONG request, ULONG arg, int *errnop);
int tcpip_listen (ULONG handle, int backlog);
int tcpip_read (ULONG handle, void *buf, ULONG len, int *errnop);
int tcpip_recv (ULONG handle, void *buf, int len, unsigned flags, int *errnop);
int tcpip_recvfrom (const struct _recvfrom *args, int *errnop);
int tcpip_select_poll (struct select_data *d, int *errnop);
int tcpip_send (ULONG handle, const void *buf, int len, unsigned flags,
    int *errnop);
int tcpip_sendto (const struct _sendto *args, int *errnop);
int tcpip_setsockopt (ULONG handle, int level, int optname, const void *optval,
    int optlen);
int tcpip_socket (int domain, int type, int protocol, int *errnop);
int tcpip_shutdown (ULONG handle, int how);
int tcpip_write (ULONG handle, const void *buf, ULONG len, int *errnop);
int tcpip_impsockhandle (ULONG handle, ULONG flags, int *errnop);
