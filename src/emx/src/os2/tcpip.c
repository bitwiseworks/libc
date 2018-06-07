/* tcpip.c -- TCP/IP
   Copyright (c) 1994-2000 by Eberhard Mattes

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


/* TODO:

  - inheritance of socket handles
  - add IBM TCP/IP for OS/2 layer (sock_errno() etc.) */

#define INCL_DOSDEVIOCTL
#define INCL_DOSEXCEPTIONS
#define INCL_DOSMODULEMGR
#define INCL_DOSPROCESS
#define INCL_DOSSEMAPHORES
#include <os2emx.h>
#include <emx/syscalls.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/so_ioctl.h>
#include <net/route.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <netdb.h>
#include "emxdll.h"
#include "files.h"
#include "tcpip.h"
#include "select.h"
#include "clib.h"

/* Error codes of IBM TCP/IP for OS/2 start at this number. */

#define SOCK_ERRNO_BASE 10000

/* This variable is zero while tcpip_init() has not been called.  A
   positive value means that initialization of TCP/IP was successful,
   a negative value means that initialization of TCP/IP failed. */

static int tcpip_initialized;

/* This variable holds the value returned by the most recent call to
   sock_errno().  In the future, it will be used for providing a
   sock_errno() system call. */

static int last_sock_errno;

/* Module handles of the two IBM TCP/IP for OS/2 DLLs. */

static HMODULE hmod_so32dll;
static HMODULE hmod_tcp32dll;

/* List of file handles created by sock_init().  This is used for
   fork(). */

static ULONG sock_init_count;
static ULONG sock_init_array[MAXFH_SOCK_INIT];

struct sockaddr;

/* Pointers to entrypoints of the IBM TCP/IP for OS/2 DLLs. */

static int (*tip_accept)(int, struct sockaddr *, int *);
static void (*tip_addsockettolist)(int);
static int (*tip_bind)(int, struct sockaddr *, int);
static int (*tip_connect)(int, struct sockaddr *, int);
static void * (*tip_gethostbyaddr) (const char *, int, int);
static void * (*tip_gethostbyname) (const char *);
static int (*tip_gethostid) (void);
static int (*tip_gethostname) (char *, int len);
static void * (*tip_getnetbyaddr) (long);
static void * (*tip_getnetbyname) (const char *);
static int (*tip_getpeername) (int, void *, int *);
static void * (*tip_getprotobyname) (const char *);
static void * (*tip_getprotobynumber) (int);
static void * (*tip_getservbyname) (const char *, const char *);
static void * (*tip_getservbyport) (int, const char *);
static int (*tip_getsockname) (int, void *, int *);
static int (*tip_getsockopt) (int, int, int, void *, int *);
static int (*tip_ioctl)(int, int, void *, int);
static int (*tip_listen)(int, int);
static int (*tip_removesocketfromlist)(int);
static int (*tip_recv)(int, void *, int, int);
static int (*tip_recvfrom)(int, void *, int, int, struct sockaddr *, int *);
static int (*tip_select)(int *, int, int, int, long);
static int (*tip_send)(int, const void *, int, int);
static int (*tip_sendto)(int, const void *, int, int, const struct sockaddr *,
    int);
static int (*tip_setsockopt) (int, int, int, const void *, int);
static int (*tip_shutdown) (int, int);
static int (*tip_socket) (int domain, int type, int protocol);
static int (*tip_sock_errno)(void);
static int (*tip_sock_init)(void);
static int (*tip_soclose)(int handle);
static int *tip_h_errno;

/* These macros are used for getting the addresses of DLL entrypoints. */

#define LOAD(hmod,name,var) (DosQueryProcAddr (hmod, 0, name, (PPFN)var) != 0)
#define LOAD_SO(name,var)   LOAD (hmod_so32dll, name, var)
#define LOAD_TCP(name,var)  LOAD (hmod_tcp32dll, name, var)


/* Close a socket for this process (the per-process reference count
   has become zero).  Adjust the global reference count and close the
   socket if the number of processes using that socket gets zero. */

static int tcpip_close_process (int s)
{
  int flag;

  LOCK_COMMON;
  flag = (sock_proc_count[s] > 0 && --sock_proc_count[s] == 0);
  UNLOCK_COMMON;

  if (flag)
    {
      int rc = tip_soclose (s);
      if (rc != 0 && (debug_flags & DEBUG_SOCLOSE)
          && tip_sock_errno () == SOCK_ERRNO_BASE + 38) /* ENOTSOCK */
        rc = 0;             /* Don't mind socket closed behind our back */
      return rc;
    }
  else
    {
      /* Call removesocketfromlist() to prevent SO32DLL.DLL's exit
         list procedure from closing the socket -- it's still in use
         by another process which inherited or imported the socket
         handle. */

      tip_removesocketfromlist (s);
      return 0;
    }
}


/* This function is called by the exit list procedure (and by
   term_tcpip(), which is in turn called by the DLL termination
   function).  It closes all open sockets.  Calling exit_tcpip() more
   than once is harmless. */

void exit_tcpip (void)
{
  ULONG i;

  if (tcpip_initialized > 0)
    for (i = 0; i < handle_count; ++i)
      if ((files[i].flags & (HF_OPEN | HF_SOCKET)) == (HF_OPEN | HF_SOCKET)
          && files[i].ref_count != 0)
        {
          files[i].flags = 0;
          files[i].ref_count = 0;
          tcpip_close_process (files[i].x.socket.handle);
        }
}


void term_tcpip (void)
{
  /* Close all sockets in case the exit list procedure has not been
     called.  This happens if DosFreeModule is used.  Calling
     exit_tcpip() more than once is benign. */

  exit_tcpip ();

  /* Unload the TCP/IP DLLs. */

  if (tcpip_initialized > 0)
    {
      tcpip_initialized = 0;
      DosFreeModule (hmod_so32dll);
      DosFreeModule (hmod_tcp32dll);
    }
}


/* See tcpip_init() for details.  This function is called by
   tcpip_init() and tcp_init_fork().  See those functions for
   additional actions to be taken. */

static int tcpip_init2 (void)
{
  /* Load the two IBM TCP/IP for SO/2 DLLs. */

  if (load_module ("SO32DLL", &hmod_so32dll) != 0)
    {
      tcpip_initialized = -1;
      return FALSE;
    }
  if (load_module ("TCP32DLL", &hmod_tcp32dll) != 0)
    {
      DosFreeModule (hmod_so32dll);
      tcpip_initialized = -1;
      return FALSE;
    }

  /* Initialize the pointers to the DLL entrypoints and call
     sock_init(). */

  if (LOAD_SO ("SOCK_INIT", &tip_sock_init)
      || LOAD_SO ("SOCK_ERRNO", &tip_sock_errno)
      || LOAD_SO ("ACCEPT", &tip_accept)
      || LOAD_SO ("ADDSOCKETTOLIST", &tip_addsockettolist)
      || LOAD_SO ("BIND", &tip_bind)
      || LOAD_SO ("CONNECT", &tip_connect)
      || LOAD_SO ("GETHOSTID", &tip_gethostid)
      || LOAD_SO ("GETPEERNAME", &tip_getpeername)
      || LOAD_SO ("GETSOCKNAME", &tip_getsockname)
      || LOAD_SO ("GETSOCKOPT", &tip_getsockopt)
      || LOAD_SO ("IOCTL", &tip_ioctl)
      || LOAD_SO ("LISTEN", &tip_listen)
      || LOAD_SO ("REMOVESOCKETFROMLIST", &tip_removesocketfromlist)
      || LOAD_SO ("RECV", &tip_recv)
      || LOAD_SO ("RECVFROM", &tip_recvfrom)
      || LOAD_SO ("SELECT", &tip_select)
      || LOAD_SO ("SEND", &tip_send)
      || LOAD_SO ("SENDTO", &tip_sendto)
      || LOAD_SO ("SETSOCKOPT", &tip_setsockopt)
      || LOAD_SO ("SHUTDOWN", &tip_shutdown)
      || LOAD_SO ("SOCKET", &tip_socket)
      || LOAD_SO ("SOCLOSE", &tip_soclose)
      || LOAD_TCP ("GETHOSTBYADDR", &tip_gethostbyaddr)
      || LOAD_TCP ("GETHOSTBYNAME", &tip_gethostbyname)
      || LOAD_TCP ("GETHOSTNAME", &tip_gethostname)
      || LOAD_TCP ("GETNETBYADDR", &tip_getnetbyaddr)
      || LOAD_TCP ("GETNETBYNAME", &tip_getnetbyname)
      || LOAD_TCP ("GETPROTOBYNAME", &tip_getprotobyname)
      || LOAD_TCP ("GETPROTOBYNUMBER", &tip_getprotobynumber)
      || LOAD_TCP ("GETSERVBYNAME", &tip_getservbyname)
      || LOAD_TCP ("GETSERVBYPORT", &tip_getservbyport)
      || LOAD_TCP ("H_ERRNO", &tip_h_errno)
      || tip_sock_init () != 0)
    {
      DosFreeModule (hmod_so32dll);
      DosFreeModule (hmod_tcp32dll);
      tcpip_initialized = -1;
      return FALSE;
    }

  /* Success! */

  tcpip_initialized = 1;
  return TRUE;
}


/* Initialize TCP/IP.  Return FALSE on failure, return TRUE on
   success.  This function must be called before using any of the
   above `tip_*' pointers.  Functions which check HF_SOCKET don't have
   to call tcpip_init() as the HF_SOCKET bit can only be set by a
   function which calls tcpip_init(). */

static int tcpip_init (void)
{
  int result;
  ULONG count;

  /* If tcpip_init() has already been called, use the result of the
     first call. */

  if (tcpip_initialized < 0)
    return FALSE;
  if (tcpip_initialized > 0)
    return TRUE;

  /* Ensure that only one thread can enter tcpip_init2().  We can use
     the `files_access' semaphore for this.

     We should suspend all other threads to prevent them from opening
     or closing handles while we look for unused handles.
     Unfortunately, that cannot be done without risking a deadlock.
     As fork() works with single-thread processes only anyway, we
     ignore the problem of other threads opening or closing
     handles. */

  DosEnterMustComplete (&count);
  LOCK_FILES;

  /* Check tcpip_initialized, in case another thread blocked us in the
     previous statement. */

  if (tcpip_initialized < 0)
    result = FALSE;
  else if (tcpip_initialized > 0)
    result = TRUE;
  else
    {
      /* Call tcpip_init2(), recording the file handles used by
         sock_init() for fork(). */

      sock_init_count = find_unused_handles (sock_init_array, MAXFH_SOCK_INIT);
      result = tcpip_init2 ();

      /* Adjust sock_init_count in case sock_init() creates less than
         MAXFH_SOCK_INIT handles. */

      if (result)
        {
          ULONG temp;

          if (find_unused_handles (&temp, 1) == 1)
            while (sock_init_count > 0
                   && sock_init_array[sock_init_count-1] >= temp)
              --sock_init_count;
        }
      else
        sock_init_count = 0;
    }

  UNLOCK_FILES;
  DosExitMustComplete (&count);
  return result;
}


/* Initialize TCP/IP in a forked process that inherits socket handles
   from its parent.  As we have only one thread now, we don't have to
   lock out other threads. */

int tcpip_init_fork (const struct fork_data_done *p)
{
#define MAX_FILL 128
  ULONG unused[MAX_FILL];
  int i, j, n, fill_count, result;

  /* Copy sock_init_array from the parent process in case this process
     forks. */

  sock_init_count = p->sock_init_count;
  memcpy (sock_init_array, p->sock_init_array, sizeof (sock_init_array));

  /* Fill unused handle slots to make sock_init() use the same handles
     as in the parent process.  Temporarily set `handle_count' as it
     has not yet been initialized and is used by
     find_unused_handles(). */

  n = MAX_FILL;
  if (sock_init_count > 0 && sock_init_array[sock_init_count-1] + 1 < n)
    n = sock_init_array[sock_init_count-1] + 1;
  handle_count = n;
  n = find_unused_handles (unused, n);
  handle_count = 0;

  i = j = 0; fill_count = 0;
  while (i < n && j < sock_init_count)
    if (unused[i] == sock_init_array[j])
      {
        /* The slot is unused in this process and in its parent
           process.  It will be used by sock_init(). */

        ++i; ++j;
      }
    else if (unused[i] < sock_init_array[j])
      {
        ULONG handle, action;

        /* The file handle is unused in this process, but is used in
           the parent process.  Fill it.  We could use DosDupHandle if
           we knew an open file handle.  However, if we inherited
           socket handles only, there is no such handle. */

        if (DosOpen ("nul", &handle, &action, 0, 0,
                     OPEN_ACTION_OPEN_IF_EXISTS | OPEN_ACTION_FAIL_IF_NEW,
                     (OPEN_ACCESS_READONLY | OPEN_SHARE_DENYNONE
                      | OPEN_FLAGS_NOINHERIT), NULL) != 0
            || handle != unused[i])
          return FALSE;

        /* Remember the file handle, to be able to close it later.  We
           reuse elements of unused[] which have already been
           processed. */

        unused[fill_count++] = unused[i];
        ++i;
      }
    else
      {
        /* A file handle used by this process is unused in the parent
           process.  This should not happen and cannot be handled. */

        return FALSE;
      }

  /* Now initialize TCP/IP.  This calls sock_init(). */

  result = tcpip_init2 ();

  /* Close stuffed file handles. */

  for (i = 0; i < fill_count; ++i)
    DosClose (unused[i]);

  return result;
}


/* Initialize file tables for inherited socket handles. */

void copy_fork_sock (const struct fork_data_done *p)
{
  ULONG i, fd, s;

  for (i = 0; i < p->sock_count; ++i)
    {
      fd = p->sock_array[i].f;
      s = p->sock_array[i].s;
      close_handle (fd);        /* Clear reference count */
      alloc_file_description (fd);
      if (IS_VALID_FILE (fd))
        {
          int j = 0;
          set_handle_flags (fd, HF_OPEN | HF_SOCKET);
          GET_FILE (fd)->x.socket.handle = s;
          GET_FILE (fd)->x.socket.fd_flags = 0;
          while (j < i && p->sock_array[j].s != s)
            ++j;
          if (j == i)
            tip_addsockettolist (s); /* Inform SO32DLL.DLL */
        }
      else
        tcpip_close_process (s);
    }
}


/* Tell the child process about sockets: Build a list of sockets to be
   inherited.

   TODO: Closing a socket in another thread while fork() is in
   progress might result in a socket handle leak.

   FORK_OBJ_SIZE is big enough for MAX_SOCKETS sockets being passed
   down to the child process. */

void tcpip_fork_sock (struct fork_data_done *p)
{
  ULONG i;
  my_file *f;

  p->sock_init_count = sock_init_count;
  memcpy (p->sock_init_array, sock_init_array, sizeof (p->sock_init_array));
  p->sock_count = 0;
  p->sock_array = (struct fork_sock *)((char *)p + p->size);
  for (i = 0; i < handle_count; ++i)
    if (IS_SOCKET (i) && IS_VALID_FILE (i)
        && p->size + sizeof (struct fork_sock) <= FORK_OBJ_SIZE)
      {
        f = GET_FILE (i);
        p->sock_array[p->sock_count].f = i;
        p->sock_array[p->sock_count].s = f->x.socket.handle;
        ++p->sock_count; p->size += sizeof (struct fork_sock);
        LOCK_COMMON;
        ++sock_proc_count[f->x.socket.handle];
        UNLOCK_COMMON;
      }
}


/* Forking a process failed.  Adjust socket reference counts. */

void tcpip_undo_fork_sock (const struct fork_data_done *p)
{
  ULONG i;

  LOCK_COMMON;
  for (i = 0; i < p->sock_count; ++i)
    --sock_proc_count[p->sock_array[i].s];
  UNLOCK_COMMON;
}


/* Use UNDEF for error codes which are supposed to not being
   returned by IBM TCP/IP for OS/2. */

#define UNDEF   EINVAL

/* This table maps error codes of IBM TCP/IP for OS/2 (offset by
   SOCK_ERRNO_BASE) to emx errno numbers.  */

static unsigned char const sock_errno_tab[] =
{
  UNDEF,                        /* +0 */
  EPERM,                        /* +1 */
  UNDEF,                        /* +2 */
  ESRCH,                        /* +3 */
  EINTR,                        /* +4 */
  UNDEF,                        /* +5 */
  ENXIO,                        /* +6 */
  UNDEF,                        /* +7 */
  UNDEF,                        /* +8 */
  EBADF,                        /* +9 */
  UNDEF,                        /* +10 */
  UNDEF,                        /* +11 */
  UNDEF,                        /* +12 */
  EACCES,                       /* +13 */
  EFAULT,                       /* +14 */
  UNDEF,                        /* +15 */
  UNDEF,                        /* +16 */
  UNDEF,                        /* +17 */
  UNDEF,                        /* +18 */
  UNDEF,                        /* +19 */
  UNDEF,                        /* +20 */
  UNDEF,                        /* +21 */
  EINVAL,                       /* +22 */
  UNDEF,                        /* +23 */
  EMFILE,                       /* +24 */
  UNDEF,                        /* +25 */
  UNDEF,                        /* +26 */
  UNDEF,                        /* +27 */
  UNDEF,                        /* +28 */
  UNDEF,                        /* +29 */
  UNDEF,                        /* +30 */
  UNDEF,                        /* +31 */
  EPIPE,                        /* +32 */
  UNDEF,                        /* +33 */
  UNDEF,                        /* +34 */
  EWOULDBLOCK,                  /* +35 */
  EINPROGRESS,                  /* +36 */
  EALREADY,                     /* +37 */
  ENOTSOCK,                     /* +38 */
  EDESTADDRREQ,                 /* +39 */
  EMSGSIZE,                     /* +40 */
  EPROTOTYPE,                   /* +41 */
  ENOPROTOOPT,                  /* +42 */
  EPROTONOSUPPORT,              /* +43 */
  ESOCKTNOSUPPORT,              /* +44 */
  EOPNOTSUPP,                   /* +45 */
  EPFNOSUPPORT,                 /* +46 */
  EAFNOSUPPORT,                 /* +47 */
  EADDRINUSE,                   /* +48 */
  EADDRNOTAVAIL,                /* +49 */
  ENETDOWN,                     /* +50 */
  ENETUNREACH,                  /* +51 */
  ENETRESET,                    /* +52 */
  ECONNABORTED,                 /* +53 */
  ECONNRESET,                   /* +54 */
  ENOBUFS,                      /* +55 */
  EISCONN,                      /* +56 */
  ENOTCONN,                     /* +57 */
  ESHUTDOWN,                    /* +58 */
  ETOOMANYREFS,                 /* +59 */
  ETIMEDOUT,                    /* +60 */
  ECONNREFUSED,                 /* +61 */
  ELOOP,                        /* +62 */
  ENAMETOOLONG,                 /* +63 */
  EHOSTDOWN,                    /* +64 */
  EHOSTUNREACH,                 /* +65 */
  ENOTEMPTY,                    /* +66 */
  UNDEF,                        /* +67 */
  UNDEF,                        /* +68 */
  UNDEF,                        /* +69 */
  UNDEF,                        /* +70 */
  UNDEF,                        /* +71 */
  UNDEF,                        /* +72 */
  UNDEF,                        /* +73 */
  UNDEF,                        /* +74 */
  UNDEF,                        /* +75 */
  UNDEF,                        /* +76 */
  UNDEF,                        /* +77 */
  UNDEF,                        /* +78 */
  UNDEF,                        /* +79 */
  UNDEF,                        /* +80 */
  UNDEF,                        /* +81 */
  UNDEF,                        /* +82 */
  UNDEF,                        /* +83 */
  UNDEF,                        /* +84 */
  UNDEF,                        /* +85 */
  UNDEF,                        /* +86 */
  UNDEF,                        /* +87 */
  UNDEF,                        /* +88 */
  UNDEF,                        /* +89 */
  UNDEF,                        /* +90 */
  UNDEF,                        /* +91 */
  UNDEF,                        /* +92 */
  UNDEF,                        /* +93 */
  UNDEF,                        /* +94 */
  UNDEF,                        /* +95 */
  UNDEF,                        /* +96 */
  UNDEF,                        /* +97 */
  UNDEF,                        /* +98 */
  UNDEF,                        /* +99 */
  EIO                           /* +100: SOCEOS2ERR */
};


/* Obtain, translate, and return the error code of the most recent
   TCP/IP call. */

static int tcpip_errno (void)
{
  int e;

  e = tip_sock_errno ();
  last_sock_errno = e;
  if (e < SOCK_ERRNO_BASE || e >= SOCK_ERRNO_BASE + sizeof (sock_errno_tab))
    return EINVAL;
  return sock_errno_tab[e - SOCK_ERRNO_BASE];
}


/* Fake a low-level file I/O handle for a socket.  Instead of (or in
   addition to) relocating OS/2 file handles, we might open the "NUL"
   device to fill the slot used for the faked handle. */

static int tcpip_new_handle (int s, int *errnop)
{
  int i;

  /* Check index for sock_proc_count[]. */

  if (s < 0 || s >= MAX_SOCKETS)
    {
      tip_soclose (s);
      *errnop = EMFILE;
      return -1;
    }

  /* We might want to prefer handles with high numbers to minimize
     relocation of file handles; however, we don't know the value of
     the application's `_nfiles' variable (and nowadays there's no
     longer a limit). */

  LOCK_FILES;
  for (i = 0; i < handle_count; ++i)
    if (!(handle_flags[i] & HF_OPEN))
      {
        alloc_file_description (i);
        if (!IS_VALID_FILE (i))
          break;
        set_handle_flags (i, HF_OPEN | HF_SOCKET);
        GET_FILE (i)->x.socket.handle = s;
        GET_FILE (i)->x.socket.fd_flags = 0;

        /* Note: By incrementing sock_proc_count[s] after adding the
           socket to our table of file descriptors, the socket might
           be closed twice if the process terminates between those two
           actions.  That's preferable to not closing the socket.  (It
           can hurt only if another process uses the same socket
           handle at the same time, which is quite unlikely as IBM
           TCP/IP reuses socket handles not before about `2048 new
           sockets later'.)

           As the socket handle is not yet visible to the application
           program, LOCK_COMMON and UNLOCK_COMMON are not required. */

        sock_proc_count[s] += 1;
        UNLOCK_FILES;
        *errnop = 0;
        return i;
      }
  UNLOCK_FILES;
  tip_soclose (s);
  *errnop = EMFILE;
  return -1;
}


/* __socket() */

int tcpip_socket (int domain, int type, int protocol, int *errnop)
{
  int s;

  if (!tcpip_init ())
    {
      *errnop = ENETDOWN;       /* TODO: last_sock_errno */
      return -1;
    }

  s = tip_socket (domain, type, protocol);
  if (s == -1)
    {
      *errnop = tcpip_errno ();
      return -1;
    }

  return tcpip_new_handle (s, errnop);
}


/* __getsockhandle() */

int tcpip_getsockhandle (ULONG handle, int *errnop)
{
  if (!IS_SOCKET (handle) || !IS_VALID_FILE (handle))
    {
      *errnop = EBADF;
      return -1;
    }
  *errnop = 0;
  return GET_FILE (handle)->x.socket.handle;
}


/* __accept() */

int tcpip_accept (ULONG handle, void *addr, int *paddrlen, int *errnop)
{
  int s;

  if (!IS_SOCKET (handle) || !IS_VALID_FILE (handle))
    {
      *errnop = EBADF;          /* TODO: last_sock_errno */
      return -1;
    }

  s = tip_accept (GET_FILE (handle)->x.socket.handle, addr, paddrlen);
  if (s == -1)
    {
      *errnop = tcpip_errno ();
      return -1;
    }

  return tcpip_new_handle (s, errnop);
}


/* __bind() */

int tcpip_bind (ULONG handle, void *addr, int addrlen)
{
  int rc;

  if (!IS_SOCKET (handle) || !IS_VALID_FILE (handle))
    return EBADF;               /* TODO: last_sock_errno */

  rc = tip_bind (GET_FILE (handle)->x.socket.handle, addr, addrlen);
  if (rc != 0)
    return tcpip_errno ();
  last_sock_errno = 0;
  return 0;
}


/* __connect() */

int tcpip_connect (ULONG handle, void *addr, int addrlen)
{
  int rc;

  if (!IS_SOCKET (handle) || !IS_VALID_FILE (handle))
    return EBADF;               /* TODO: last_sock_errno */

  rc = tip_connect (GET_FILE (handle)->x.socket.handle, addr, addrlen);
  if (rc != 0)
    return tcpip_errno ();
  last_sock_errno = 0;
  return 0;
}


/* __listen() */

int tcpip_listen (ULONG handle, int backlog)
{
  int rc;

  if (!IS_SOCKET (handle) || !IS_VALID_FILE (handle))
    return EBADF;               /* TODO: last_sock_errno */

  rc = tip_listen (GET_FILE (handle)->x.socket.handle, backlog);
  if (rc != 0)
    return tcpip_errno ();
  last_sock_errno = 0;
  return 0;
}


/* __recv() */

int tcpip_recv (ULONG handle, void *buf, int len, unsigned flags, int *errnop)
{
  int r;

  if (!IS_SOCKET (handle))
    {
      ULONG rc, nread;
      rc = DosRead (handle, buf, (ULONG)len, &nread);
      if (rc != 0)
        {
          *errnop = set_error (rc);
          return -1;
        }
      *errnop = 0;
      return (int)nread;
    }

  if (!IS_VALID_FILE (handle))
    {
      *errnop = EBADF;          /* TODO: last_sock_errno */
      return -1;
    }

  if (len > 0x7fff) len = 0x7ffc;
  r = tip_recv (GET_FILE (handle)->x.socket.handle, buf, len, flags);
  if (r == -1)
    {
      *errnop = tcpip_errno ();
      return -1;
    }
  *errnop = 0;
  last_sock_errno = 0;
  return r;
}


/* __recvfrom() */

int tcpip_recvfrom (const struct _recvfrom *args, int *errnop)
{
  ULONG handle;
  int r, len;

  handle = args->handle;
  if (!IS_SOCKET (handle))
    {
      ULONG rc, nread;
      rc = DosRead (handle, args->buf, (ULONG)args->len, &nread);
      if (rc != 0)
        {
          *errnop = set_error (rc);
          return -1;
        }
      *errnop = 0;
      return (int)nread;
    }

  if (!IS_VALID_FILE (handle))
    {
      *errnop = EBADF;          /* TODO: last_sock_errno */
      return -1;
    }

  len = args->len;
  if (len > 0x7fff) len = 0x7ffc;
  r = tip_recvfrom (GET_FILE (handle)->x.socket.handle, args->buf, len,
                    args->flags, args->from, args->pfromlen);
  if (r == -1)
    {
      *errnop = tcpip_errno ();
      return -1;
    }
  *errnop = 0;
  last_sock_errno = 0;
  return r;
}


/* __send() */

int tcpip_send (ULONG handle, const void *buf, int len, unsigned flags,
                int *errnop)
{
  int r, n, result, e;
  const char *p;

  if (!IS_SOCKET (handle))
    {
      ULONG rc, nwritten;
      rc = DosWrite (handle, buf, (ULONG)len, &nwritten);
      if (rc != 0)
        {
          *errnop = set_error (rc);
          return -1;
        }
      *errnop = 0;
      return (int)nwritten;
    }

  if (!IS_VALID_FILE (handle))
    {
      *errnop = EBADF;          /* TODO: last_sock_errno */
      return -1;
    }

  /* TODO: Return EMSGSIZE if appropriate. */

  p = buf; result = 0;
  while (len > 0)
    {
      n = (len > 0x7fff ? 0x7ffc : len);
      r = tip_send (GET_FILE (handle)->x.socket.handle, p, n, flags);
      if (r == -1)
        {
          e = tcpip_errno ();
          if (e == EWOULDBLOCK && result != 0)
            break;
          *errnop = e;
          return -1;
        }
      result += r;
      if (r != n) break;
      p += r; len -= r;
    }

  *errnop = 0;
  last_sock_errno = 0;
  return result;
}


/* __sendto() */

int tcpip_sendto (const struct _sendto *args, int *errnop)
{
  ULONG handle;
  int r, len, n, result, e;
  const char *p;

  handle = args->handle;
  if (!IS_SOCKET (handle))
    {
      ULONG rc, nwritten;
      rc = DosWrite (handle, args->buf, (ULONG)args->len, &nwritten);
      if (rc != 0)
        {
          *errnop = set_error (rc);
          return -1;
        }
      *errnop = 0;
      return (int)nwritten;
    }

  if (!IS_VALID_FILE (handle))
    {
      *errnop = EBADF;          /* TODO: last_sock_errno */
      return -1;
    }

  /* TODO: Return EMSGSIZE if appropriate. */

  p = args->buf; result = 0; len = args->len;
  while (len > 0)
    {
      n = (len > 0x7fff ? 0x7ffc : len);
      r = tip_sendto (GET_FILE (handle)->x.socket.handle, p, n, args->flags,
                      args->to, args->tolen);
      if (r == -1)
        {
          e = tcpip_errno ();
          if (e == EWOULDBLOCK && result != 0)
            break;
          *errnop = e;
          return -1;
        }
      result += r;
      if (r != n) break;
      p += r; len -= r;
    }

  *errnop = 0;
  last_sock_errno = 0;
  return result;
}


/* __close() for a socket handle.  First adjust the local reference
   count.  If it becomes zero, call tcpip_close_process() to adjust
   the global reference count and to close the socket if it becomes
   zero. */

int tcpip_close (ULONG handle)
{
  if (!IS_VALID_FILE (handle))
    return EBADF;
  if (GET_FILE (handle)->ref_count == 1)
    {
      if (tcpip_close_process (GET_FILE (handle)->x.socket.handle) != 0)
        return tcpip_errno ();
    }
  close_handle (handle);
  last_sock_errno = 0;
  return 0;
}


/* __dup() for a socket handle.  The target handle has already been
   closed unless both HANDLE and TARGET are equal. */

int tcpip_dup (ULONG handle, ULONG target, int *errnop)
{
  int i;

  if (!IS_VALID_FILE (handle))
    {
      *errnop = EBADF;
      return -1;
    }
  if (target == (ULONG)(-1))
    {
      LOCK_FILES;
      for (i = 0; i < handle_count; ++i)
        if (!(handle_flags[i] & HF_OPEN))
          {
            handle_flags[i] = handle_flags[handle];
            handle_file[i] = handle_file[handle];
            GET_FILE(handle)->ref_count += 1;
            UNLOCK_FILES;
            *errnop = 0;
            return i;
          }
      UNLOCK_FILES;
      *errnop = EMFILE;
      return -1;
    }
  else if (handle == target)
    {
      *errnop = 0;
      return target;
    }
  else if (target >= handle_count)
    {
      *errnop = EBADF;
      return -1;
    }
  else
    {
      do_close (target);
      handle_flags[target] = handle_flags[handle];
      handle_file[target] = handle_file[handle];
      GET_FILE(handle)->ref_count += 1;
      *errnop = 0;
      return target;
    }
}


/* __read() for a socket handle. */

int tcpip_read (ULONG handle, void *buf, ULONG len, int *errnop)
{
  int rc;

  if (len > 0x7fff) len = 0x7ffc;
  rc = tip_recv (GET_FILE (handle)->x.socket.handle, buf, len, 0);
  if (rc == -1)
    {
      *errnop = tcpip_errno ();
      return -1;
    }
  *errnop = 0;
  last_sock_errno = 0;
  return rc;
}


/* __write() for a socket handle. */

int tcpip_write (ULONG handle, const void *buf, ULONG len, int *errnop)
{
  int rc, n, result, e;
  const char *p;

  p = buf; result = 0;
  while (len > 0)
    {
      n = (len > 0x7fff ? 0x7ffc : len);
      rc = tip_send (GET_FILE (handle)->x.socket.handle, p, n, 0);
      if (rc == -1)
        {
          e = tcpip_errno ();
          if (e == EWOULDBLOCK && result != 0)
            break;
          *errnop = e;
          return -1;
        }
      result += rc;
      if (rc != n) break;
      p += rc; len -= rc;
    }
  *errnop = 0;
  last_sock_errno = 0;
  return result;
}


/* __fstat() for a socket handle. */

int tcpip_fstat (ULONG handle, struct stat *dst, int *errnop)
{
  memset (dst, 0, sizeof (struct stat));
  dst->st_mode = MAKEPERM (S_IREAD|S_IWRITE);
  dst->st_mode |= S_IFSOCK;
  dst->st_size = 0;
  dst->st_dev = 0;
  dst->st_uid = 0;              /* root */
  dst->st_gid = 0;              /* root */
  dst->st_ino = ino_number;
  if (++ino_number == 0)
   ino_number = 1;
  dst->st_rdev = dst->st_dev;
  dst->st_nlink = 1;
  *errnop = 0;
  return 0;
}


/* Call ioctl() of IBM TCP/IP for OS/2. */

static int map_ioctl (ULONG handle, ULONG request, ULONG arg, ULONG datalen,
                      int *errnop)
{
  int rc;

  rc = tip_ioctl (GET_FILE (handle)->x.socket.handle, request,
                  (void *)arg, datalen);
  if (rc == -1)
    {
      *errnop = tcpip_errno ();
      return -1;
    }
  *errnop = 0;
  return rc;
}


/* __ioctl() for a socket handle. */

int tcpip_ioctl (ULONG handle, ULONG request, ULONG arg, int *errnop)
{
  int rc;

  if (!IS_VALID_FILE (handle))
    {
      *errnop = EBADF;
      return -1;
    }

  switch (request)
    {
    case TCGETA:
    case TCSETA:
    case TCSETAF:
    case TCSETAW:
    case TCFLSH:
    case TCSBRK:
    case TCXONC:
    case _TCGA:
    case _TCSANOW:
    case _TCSADRAIN:
    case _TCSAFLUSH:

      /* These requests are not yet implemented. */

      *errnop = EINVAL;
      return -1;

    case FGETHTYPE:

      *(int *)arg = HT_SOCKET;
      *errnop = 0;
      return 0;

    case FIONREAD:

      return map_ioctl (handle, _TCPIP_FIONREAD, arg, sizeof (int), errnop);

    case FIOASYNC:
    case SIOCATMARK:

      return map_ioctl (handle, request, arg, sizeof (int), errnop);

    case SIOCADDRT:
    case SIOCDELRT:

      return map_ioctl (handle, request, arg, sizeof (struct rtentry), errnop);

    case SIOCDARP:
    case SIOCGARP:
    case SIOCSARP:

      return map_ioctl (handle, request, arg, sizeof (struct arpreq), errnop);

    case SIOCGIFADDR:
    case SIOCGIFBRDADDR:
    case SIOCGIFDSTADDR:
    case SIOCGIFFLAGS:
    case SIOCGIFMETRIC:
    case SIOCGIFNETMASK:
    case SIOCSIFADDR:
    case SIOCSIFBRDADDR:
    case SIOCSIFDSTADDR:
    case SIOCSIFFLAGS:
    case SIOCSIFMETRIC:
    case SIOCSIFNETMASK:

      return map_ioctl (handle, request, arg, sizeof (struct ifreq), errnop);

    case SIOCGIFCONF:

      return map_ioctl (handle, request, arg, sizeof (struct ifconf), errnop);

    case FIONBIO:
      rc = map_ioctl (handle, request, arg, sizeof (int), errnop);
      if (rc == 0)
        {
          if (*(int *)arg)
            handle_flags[handle] |= HF_NDELAY;
          else
            handle_flags[handle] &= ~HF_NDELAY;
        }
      return rc;

    default:
      *errnop = EINVAL;
      return -1;
    }
}


/* __fcntl() for a socket handle.  This function is called for F_SETFL
   only, and only if O_NDELAY was changed. */

int tcpip_fcntl (ULONG handle, ULONG request, ULONG arg, int *errnop)
{
  int on;

  on = arg & O_NDELAY;
  return tcpip_ioctl (handle, FIONBIO, (ULONG)&on, errnop);
}


/* __getsockopt() */

int tcpip_getsockopt (ULONG handle, int level, int optname, void *optval,
                      int *poptlen)
{
  int rc;

  if (!IS_SOCKET (handle) || !IS_VALID_FILE (handle))
    return ENOTSOCK;            /* TODO: last_sock_errno */

  rc = tip_getsockopt (GET_FILE (handle)->x.socket.handle, level, optname,
                       optval, poptlen);
  if (rc == -1)
    return tcpip_errno ();
  last_sock_errno = 0;
  return 0;
}


/* __setsockopt() */

int tcpip_setsockopt (ULONG handle, int level, int optname, const void *optval,
                      int optlen)
{
  int rc;

  if (!IS_SOCKET (handle) || !IS_VALID_FILE (handle))
    return ENOTSOCK;            /* TODO: last_sock_errno */

  rc = tip_setsockopt (GET_FILE (handle)->x.socket.handle, level, optname,
                       optval, optlen);
  if (rc == -1)
    return tcpip_errno ();
  last_sock_errno = 0;
  return 0;
}


/* __getsockname() */

int tcpip_getsockname (ULONG handle, void *addr, int *paddrlen)
{
  int rc;

  if (!IS_SOCKET (handle) || !IS_VALID_FILE (handle))
    return ENOTSOCK;            /* TODO: last_sock_errno */

  rc = tip_getsockname (GET_FILE (handle)->x.socket.handle, addr, paddrlen);
  if (rc == -1)
    return tcpip_errno ();
  last_sock_errno = 0;
  return 0;
}


/* __shutdown() */

int tcpip_shutdown (ULONG handle, int how)
{
  int rc;

  if (!IS_SOCKET (handle) || !IS_VALID_FILE (handle))
    return ENOTSOCK;            /* TODO: last_sock_errno */

  rc = tip_shutdown (GET_FILE (handle)->x.socket.handle, how);
  if (rc != 0)
    return tcpip_errno ();
  last_sock_errno = 0;
  return 0;
}


/* __getpeername() */

int tcpip_getpeername (ULONG handle, void *addr, int *paddrlen)
{
  int rc;

  if (!IS_SOCKET (handle) || !IS_VALID_FILE (handle))
    return ENOTSOCK;            /* TODO: last_sock_errno */

  rc = tip_getpeername (GET_FILE (handle)->x.socket.handle, addr, paddrlen);
  if (rc == -1)
    return tcpip_errno ();
  last_sock_errno = 0;
  return 0;
}


/* __gethostbyname() */

int tcpip_gethostbyname (const char *name, void **dst)
{
  void *p;

  if (!tcpip_init ())
    return NO_RECOVERY;

  p = tip_gethostbyname (name);
  *dst = p;
  if (p == NULL)
    {
      /* h_errno seems to be always zero.  Bug? */

      if (*tip_h_errno == 0)
        return NO_RECOVERY;
      else
        return *tip_h_errno;
    }
  else
    return 0;
}


/* __gethostbyaddr() */

int tcpip_gethostbyaddr (const char *addr, int len, int type, void **dst)
{
  void *p;

  if (!tcpip_init ())
    return NO_RECOVERY;

  p = tip_gethostbyaddr (addr, len, type);
  *dst = p;
  if (p == NULL)
    {
      /* h_errno seems to be always zero.  Bug? */

      if (*tip_h_errno == 0)
        return NO_RECOVERY;
      else
        return *tip_h_errno;
    }
  else
    return 0;
}


/* __getservbyname() */

int tcpip_getservbyname (const char *name, const char *proto, void **dst)
{
  void *p;

  if (!tcpip_init ())
    return -1;

  p = tip_getservbyname (name, proto);
  *dst = p;
  return (p != NULL ? 0 : -1);
}


/* __getservbyport() */

int tcpip_getservbyport (int port, const char *proto, void **dst)
{
  void *p;

  if (!tcpip_init ())
    return -1;

  p = tip_getservbyport (port, proto);
  *dst = p;
  return (p != NULL ? 0 : -1);
}


/* __gethostid() */

int tcpip_gethostid (int *dst)
{
  if (!tcpip_init ())
    return ENETDOWN;

  *dst = tip_gethostid ();
  last_sock_errno = 0;
  return 0;
}


/* __gethostname() */

int tcpip_gethostname (char *name, int len)
{
  int rc;

  /* TODO: Get hostname from environment variable if the net is
     down. */

  if (!tcpip_init ())
    return ENETDOWN;

  /* If HOSTNAME is not set, IBM's gethostname() simply leaves the
     buffer alone! */

  *name = 0;

  rc = tip_gethostname (name, len);
  if (rc != 0)
    return tcpip_errno ();
  last_sock_errno = 0;
  return 0;
}


/* __getprotobyname() */

int tcpip_getprotobyname (const char *name, void **dst)
{
  void *p;

  if (!tcpip_init ())
    return -1;

  p = tip_getprotobyname (name);
  *dst = p;
  return (p != NULL ? 0 : -1);
}


/* __getprotobynumber() */

int tcpip_getprotobynumber (int proto, void **dst)
{
  void *p;

  if (!tcpip_init ())
    return -1;

  p = tip_getprotobynumber (proto);
  *dst = p;
  return (p != NULL ? 0 : -1);
}


/* __getnetbyname() */

int tcpip_getnetbyname (const char *name, void **dst)
{
  void *p;

  if (!tcpip_init ())
    return -1;

  p = tip_getnetbyname (name);
  *dst = p;
  return (p != NULL ? 0 : -1);
}


/* __getnetbyaddr() */

int tcpip_getnetbyaddr (long net, void **dst)
{
  void *p;

  if (!tcpip_init ())
    return -1;

  p = tip_getnetbyaddr (net);
  *dst = p;
  return (p != NULL ? 0 : -1);
}


/* __impsockhandle() */

int tcpip_impsockhandle (ULONG handle, ULONG flags, int *errnop)
{
  int result, optlen, optval;
  ULONG i;

  if (!tcpip_init ())
    {
      *errnop = ENETDOWN;
      return -1;
    }

  if (flags != 0)
    {
      *errnop = EINVAL;
      return -1;
    }

  /* Check if HANDLE is a known socket handle of this process. */

  LOCK_FILES;
  for (i = 0; i < handle_count; ++i)
    if (IS_SOCKET (i) && IS_VALID_FILE (i)
        && GET_FILE (i)->x.socket.handle == handle)
      {
        GET_FILE (i)->ref_count += 1;
        UNLOCK_FILES;
        *errnop = 0;
        return i;
      }

  /* HANDLE is not a known socket handle of this process.  Check if
     it's actually a socket handle. */

  optlen = sizeof (optval);
  if (tip_getsockopt (handle, SOL_SOCKET, SO_TYPE, &optval, &optlen) != 0)
    {
      UNLOCK_FILES;
      *errnop = ENOTSOCK;
      return -1;
    }

  /* It's an entirely new socket handle.  Install it in SO32DLL.DLL's
     tables and in our tables. */

  tip_addsockettolist (handle);
  result = tcpip_new_handle (handle, errnop);
  if (result == -1)
    tip_removesocketfromlist (handle);
  UNLOCK_FILES;
  return result;
}


/* Poll socket handles for __select().  Update the bitstrings if a
   handle is ready. */

int tcpip_select_poll (struct select_data *d, int *errnop)
{
  int i, j, n;

  memcpy (d->sockett, d->sockets, d->socket_count * sizeof (int));
  n = tip_select (d->sockett, d->socket_nread, d->socket_nwrite,
                  d->socket_nexcept, 0);
  if (n < 0)
    {
      *errnop = tcpip_errno ();
      return -1;
    }
  if (n == 0)
    return 0;
  j = 0; n = 0;
  for (i = 0; i < d->socket_nread; ++i, ++j)
    if (d->sockett[j] != -1)
      {
        FD_SET (d->socketh[j], d->rbits);
        n = 1;
      }
  for (i = 0; i < d->socket_nwrite; ++i, ++j)
    if (d->sockett[j] != -1)
      {
        FD_SET (d->socketh[j], d->wbits);
        n = 1;
      }
  for (i = 0; i < d->socket_nexcept; ++i, ++j)
    if (d->sockett[j] != -1)
      {
        FD_SET (d->socketh[j], d->ebits);
        n = 1;
      }
  *errnop = 0;
  d->ready_flag = TRUE;
  return n;
}
