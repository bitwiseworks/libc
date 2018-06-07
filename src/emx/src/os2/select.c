/* select.c -- Implement select()
   Copyright (c) 1993-1998 by Eberhard Mattes

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


#define INCL_DOSDEVIOCTL
#define INCL_DOSSEMAPHORES
#define INCL_DOSPROCESS
#define INCL_DOSMISC
#define INCL_DOSERRORS
#include <os2emx.h>
#include <emx/syscalls.h>
#include <sys/types.h>
#include <sys/termio.h>
#include <sys/time.h>
#include <sys/errno.h>
#include "emxdll.h"
#include "files.h"
#include "tcpip.h"
#include "select.h"
#include "clib.h"

/* Note that select() is not thread-safe for various reasons. */

static HEV npipe_sem;
static BYTE npipe_sem_open;
static ULONG mem_size;
static void *mem;


static int select_add_read_npipe (struct select_data *d, int fd)
{
  ULONG rc;

  if (!d->sem_npipe_flag)
    {
      if (d->sem_count >= d->max_sem)
        return EINVAL;
      if (!npipe_sem_open)
        {
          if (create_event_sem (&npipe_sem, DC_SEM_SHARED) != 0)
            return EINVAL;
          npipe_sem_open = TRUE;
        }
      if (reset_event_sem (npipe_sem) != 0)
        return EINVAL;
      d->list[d->sem_count].hsemCur = (HSEM)npipe_sem;
      d->list[d->sem_count].ulUser = 0;
      ++d->sem_count;
      d->sem_npipe_flag = TRUE;
    }
  rc = DosSetNPipeSem (fd, (HSEM)npipe_sem, 0);
  if (rc != 0)
    {
      error (rc, "DosSetNPipeSem");
      return EACCES;
    }
  return 0;
}


static int select_add_read_con (struct select_data *d, int fd)
{
  if (IS_VALID_FILE (fd) && !(GET_FILE (fd)->c_lflag & IDEFAULT)
      && !d->sem_kbd_flag)
    {
      if (d->sem_count >= d->max_sem)
        return EINVAL;
      if (reset_event_sem (kbd_sem_new) != 0)
        return EINVAL;
      d->list[d->sem_count].hsemCur = (HSEM)kbd_sem_new;
      d->list[d->sem_count].ulUser = 0;
      ++d->sem_count;
      d->sem_kbd_flag = TRUE;
    }
  return 0;
}


static int select_add_read_socket (struct select_data *d, int fd)
{
  if (!IS_VALID_FILE (fd))
    return EBADF;
  d->sockets[d->socket_count] = GET_FILE (fd)->x.socket.handle;
  d->socketh[d->socket_count] = fd;
  ++d->socket_count; ++d->socket_nread;
  return 0;
}


static int select_add_read (struct select_data *d, int fd)
{
  ULONG ht;

  if (fd >= handle_count)
    return EBADF;
  ht = handle_flags[fd];
  if (!(ht & HF_OPEN))
    return EBADF;
  if (ht & HF_NPIPE)
    return select_add_read_npipe (d, fd);
  else if (ht & HF_CON)
    return select_add_read_con (d, fd);
  else if (ht & HF_ASYNC)
    d->async_flag = TRUE;
  else if (ht & HF_SOCKET)
    return select_add_read_socket (d, fd);
  else if (ht & HF_XF86SUP)
    return xf86sup_select_add_read (d, fd);
  else
    {
      /* Ignore other types of handles */
    }
  return 0;
}


static int select_add_write (struct select_data *d, int fd)
{
  ULONG ht;

  if (fd >= handle_count)
    return EBADF;
  ht = handle_flags[fd];
  if (!(ht & HF_OPEN))
    return EBADF;
  if (ht & HF_ASYNC)
    d->async_flag = TRUE;
  else if (ht & HF_SOCKET)
    {
      if (!IS_VALID_FILE (fd))
        return EBADF;
      d->sockets[d->socket_count] = GET_FILE (fd)->x.socket.handle;
      d->socketh[d->socket_count] = fd;
      ++d->socket_count; ++d->socket_nwrite;
    }
  else
    {
      /* Ignore other types of handles */
    }
  return 0;
}


static int select_add_except (struct select_data *d, int fd)
{
  ULONG ht;

  if (fd >= handle_count)
    return EBADF;
  ht = handle_flags[fd];
  if (!(ht & HF_OPEN))
    return EBADF;
  if (ht & HF_SOCKET)
    {
      if (!IS_VALID_FILE (fd))
        return EBADF;
      d->sockets[d->socket_count] = GET_FILE (fd)->x.socket.handle;
      d->socketh[d->socket_count] = fd;
      ++d->socket_count; ++d->socket_nexcept;
    }
  else
    {
      /* Ignore other types of handles */
    }
  return 0;
}


static int select_init (struct select_data *d, struct _select *args)
{
  int fd, e;
  ULONG rc;
  struct timeval *tv;

  d->timeout = SEM_INDEFINITE_WAIT;
  tv = args->timeout;
  if (tv != NULL)
    {
      if (tv->tv_sec < 0 || tv->tv_sec >= 4294967 || tv->tv_usec < 0)
        return EINVAL;
      d->timeout = tv->tv_sec * 1000;
      d->timeout += (tv->tv_usec + 999) / 1000;
      if (d->timeout != 0)
        d->start_ms = querysysinfo (QSV_MS_COUNT);
    }
  if (args->readfds != NULL)
    for (fd = 0; fd < args->nfds; ++fd)
      if (FD_ISSET (fd, args->readfds))
        {
          e = select_add_read (d, fd);
          if (e != 0)
            return e;
        }
  if (args->writefds != NULL)
    for (fd = 0; fd < args->nfds; ++fd)
      if (FD_ISSET (fd, args->writefds))
        {
          e = select_add_write (d, fd);
          if (e != 0)
            return e;
        }
  if (args->exceptfds != NULL)
    for (fd = 0; fd < args->nfds; ++fd)
      if (FD_ISSET (fd, args->exceptfds))
        {
          e = select_add_except (d, fd);
          if (e != 0)
            return e;
        }

  if (d->sem_count >= d->max_sem)
    return EINVAL;
  d->list[d->sem_count].hsemCur = (HSEM)signal_sem;
  d->list[d->sem_count].ulUser = 1;
  ++d->sem_count;
  rc = create_muxwait_sem (&d->sem_mux, d->sem_count, d->list, DCMW_WAIT_ANY);
  if (rc != 0)
    return EINVAL;
  d->sem_mux_flag = TRUE;
  return 0;
}


static void select_check_read (struct select_data *d, int fd)
{
  ULONG rc, ht;
  int dummy_errno;

  ht = handle_flags[fd];
  if (!(ht & HF_OPEN))
    return;
  if (ht & HF_NPIPE)
    {
      ULONG buffer, nread, state;
      AVAILDATA avail;

      rc = DosPeekNPipe (fd, &buffer, 0, &nread, &avail, &state);
      if (rc != 0)
        {
          /* Ignore; TODO? */
        }
      else if (avail.cbpipe != 0 || state == NP_STATE_CLOSING)
        {
          FD_SET (fd, d->rbits);
          d->ready_flag = TRUE;
        }
    }
  else if (ht & HF_CON)
    {
      /* Note that file descriptors of types unsupported by select()
         are considered to be always ready. */

      if (!IS_VALID_FILE (fd) || (GET_FILE (fd)->c_lflag & IDEFAULT)
          || termio_avail (fd) != 0)
        {
          FD_SET (fd, d->rbits);
          d->ready_flag = TRUE;
        }
    }
  else if (ht & HF_ASYNC)
    {
      if (async_avail (fd) != 0)
        {
          FD_SET (fd, d->rbits);
          d->ready_flag = TRUE;
        }
    }
  else if (ht & HF_XF86SUP)
    {
      if (xf86sup_avail (fd, &dummy_errno) > 0)
        {
          FD_SET (fd, d->rbits);
          d->ready_flag = TRUE;
        }
    }
  else if (ht & HF_SOCKET)
    {
      /* Socket handles are handled by tcpip_select_poll(). */
    }
  else
    {
      /* File descriptors of types unsupported by select() are
         considered to be always ready. */

      FD_SET (fd, d->rbits);
      d->ready_flag = TRUE;
    }
}


static void select_check_write (struct select_data *d, int fd)
{
  ULONG ht;

  ht = handle_flags[fd];
  if (!(ht & HF_OPEN))
    return;

  if (ht & HF_SOCKET)
    {
      /* Socket handles are handled by tcpip_select_poll(). */
    }
  else if (ht & HF_ASYNC)
    {
      if (async_writable (fd) != 0)
        {
          FD_SET (fd, d->wbits);
          d->ready_flag = TRUE;
        }
    }
  else
    {
      /* File descriptors of types unsupported by select() are
         considered to be always ready. */

      FD_SET (fd, d->wbits);
      d->ready_flag = TRUE;
    }
}


/* Poll all handles.  Return 0 or errno. */

static int select_poll (struct select_data *d, struct _select *args)
{
  int fd;

  memset (d->rbits, 0, d->nbytes);
  memset (d->wbits, 0, d->nbytes);
  memset (d->ebits, 0, d->nbytes);

  /* Reset those event semaphores which can be set without data being
     ready (false alarms).  This is required because we might want to
     block after calling select_poll().  Note that the semaphores must
     be reset before polling to avoid a time window. */

  if (d->sem_npipe_flag && reset_event_sem (npipe_sem) != 0)
    return EINVAL;

  if (args->readfds != NULL)
    {
      for (fd = 0; fd < args->nfds; ++fd)
        if (FD_ISSET (fd, args->readfds))
          select_check_read (d, fd);
    }

  if (args->writefds != NULL)
    {
      for (fd = 0; fd < args->nfds; ++fd)
        if (FD_ISSET (fd, args->writefds))
          select_check_write (d, fd);
    }

  if (d->socket_count != 0)
    {
      int n, e;

      n = tcpip_select_poll (d, &e);
      if (n < 0)
        return e;
    }

  if (d->ready_flag)
    {
      if (args->readfds != NULL)
        memcpy (args->readfds, d->rbits, d->nbytes);
      if (args->writefds != NULL)
        memcpy (args->writefds, d->wbits, d->nbytes);
      if (args->exceptfds != NULL)
        memcpy (args->exceptfds, d->ebits, d->nbytes);
      d->return_value = 0;
      for (fd = 0; fd < args->nfds; ++fd)
        if ((args->readfds != NULL && FD_ISSET (fd, args->readfds))
            || (args->writefds != NULL && FD_ISSET (fd, args->writefds))
            || (args->exceptfds != NULL && FD_ISSET (fd, args->exceptfds)))
          d->return_value += 1;
    }
  return 0;
}


#define POLL_INTERVAL   10

/* Wait until a semaphore is posted (pipe ready, keyboard data
   available, signal) or until the timeout elapses, whichever comes
   first.  Call select_poll() unless a timeout or an error occurs.
   Return 0 if successful, return errno otherwise. */

static int select_block (struct select_data *d, struct _select *args)
{
  ULONG rc, user;
  ULONG timeout, now, elapsed;
  int n, e;
  char poll;

  poll = (char)(d->async_flag || (d->socket_count != 0 && d->timeout != 0));
  for (;;)
    {
      timeout = d->timeout;
      if (timeout != 0 && timeout != SEM_INDEFINITE_WAIT)
        {
          /* This works even if QSV_MS_COUNT wrapped around once since
             we got d->start_ms, thanks to unsigned arithmetic.  A
             struct timeval which could make QSV_MS_COUNT wrap around
             more than once is rejected by select_init(). */

          now = querysysinfo (QSV_MS_COUNT);
          elapsed = now - d->start_ms;
          if (timeout < elapsed)
            timeout = 0;
          else
            timeout -= elapsed;
          if (poll && timeout == 0)
            return 0;           /* The time is over */
        }
      if (!poll)
        {
          /* No sockets or async devices involved, we can really
             block. */

          rc = DosWaitMuxWaitSem (d->sem_mux, timeout, &user);
          if (rc == ERROR_INTERRUPT || sig_flag)
            return EINTR;
          else if (rc == ERROR_TIMEOUT)
            return 0;
          else if (rc != 0)
            {
              error (rc, "DosWaitMuxWaitSem");
              return EINVAL;
            }
          else
            {
              e = select_poll (d, args);
              if (e != 0 || timeout == 0 || d->ready_flag)
                return e;
            }
        }
      else
        {
          /* Poll sockets. */

          if (d->socket_count != 0)
            {
              n = tcpip_select_poll (d, &e);
              if (n < 0)
                return e;
              if (n > 0)
                return select_poll (d, args);
            }

          /* Sleep for a few ms, waking up as soon as d->sem_mux is
             posted.  This way we don't have to poll all the other
             file descriptors. */

          if (timeout > POLL_INTERVAL || timeout == SEM_INDEFINITE_WAIT)
            timeout = POLL_INTERVAL;
          rc = DosWaitMuxWaitSem (d->sem_mux, timeout, &user);
          if (rc == ERROR_INTERRUPT || sig_flag)
            return EINTR;
          else if (rc == 0 || (rc == ERROR_TIMEOUT && d->async_flag))
            {
              e = select_poll (d, args);
              if (e != 0 || timeout == 0 || d->ready_flag)
                return e;
            }
          else if (rc != ERROR_TIMEOUT)
            {
              error (rc, "DosWaitMuxWaitSem");
              return EINVAL;
            }
        }

    }
}


static void select_cleanup (struct select_data *d)
{
  if (d->sem_mux_flag)
    close_muxwait_sem (d->sem_mux);
}


int do_select (struct _select *args, int *errnop)
{
  struct select_data d;
  int e;
  char *a;
  ULONG size, rc;

  sig_block_start ();
  d.td = get_thread ();
  d.sem_npipe_flag = FALSE; d.sem_kbd_flag = FALSE; d.sem_mux_flag = FALSE;
  d.sem_count = 0; d.ready_flag = FALSE; d.return_value = 0;
  d.socket_count = 0; d.socket_nread = d.socket_nwrite = d.socket_nexcept = 0;
  d.async_flag = FALSE;
  d.nbytes = 4 * ((args->nfds + 31) / 32);
  d.max_sem = args->nfds + 1;
  if (d.nbytes < 256 / 8)
    d.nbytes = 256 / 8;
  size = (3 * d.nbytes + 3 * args->nfds * sizeof (int)
          + d.max_sem * sizeof (SEMRECORD));
  if (size > mem_size)
    {
      if (mem != NULL)
        private_free (mem);
      mem_size = size;
      rc = private_alloc (&mem, mem_size);
      if (rc != 0)
        {
          *errnop = set_error (rc);
          return -1;
        }
    }
  a = mem;
  d.rbits = (fd_set *)a; a += d.nbytes;
  d.wbits = (fd_set *)a; a += d.nbytes;
  d.ebits = (fd_set *)a; a += d.nbytes;
  d.sockets = (int *)a; a += args->nfds * sizeof (int);
  d.socketh = (int *)a; a += args->nfds * sizeof (int);
  d.sockett = (int *)a; a += args->nfds * sizeof (int);
  d.list = (SEMRECORD *)a; /* a += d.max_sem * sizeof (SEMRECORD); */
  e = select_init (&d, args);
  if (e == 0)
    {
      e = select_poll (&d, args);
      /* Block if no handle is ready and the requested timeout is > 0
         or indefinite. */
      if (e == 0 && !d.ready_flag && d.timeout != 0)
        e = select_block (&d, args);
    }

  /* If no handles are ready (time-out), we have to clear all sets. */

  if (e == 0 && !d.ready_flag && !(debug_flags & DEBUG_NOFDZERO))
    {
      if (args->readfds != NULL)
        memset (args->readfds, 0, d.nbytes);
      if (args->writefds != NULL)
        memset (args->writefds, 0, d.nbytes);
      if (args->exceptfds != NULL)
        memset (args->exceptfds, 0, d.nbytes);
    }

  select_cleanup (&d);
  sig_block_end ();
  *errnop = e;
  return (e == 0 ? d.return_value : -1);
}
