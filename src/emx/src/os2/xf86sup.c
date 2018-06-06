/* xf86sup.c -- Support for Holger Veit's xf86sup device driver
   Copyright (c) 1995-1996 by Eberhard Mattes

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


#define INCL_DOSDEVICES
#define INCL_DOSDEVIOCTL
#define INCL_DOSERRORS
#define INCL_DOSSEMAPHORES
#include <os2emx.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/so_ioctl.h>
#include <sys/termio.h>         /* SysV */
#include <termios.h>            /* POSIX.1 */
#include "emxdll.h"
#include "files.h"
#include "select.h"
#include "xf86sup.h"
#include "clib.h"


ULONG xf86sup_query (ULONG handle, ULONG *pflags)
{
  struct xf86sup_drvid drvid;
  ULONG rc, data_length;
  my_file *d;

  memset (&drvid, 0, sizeof (drvid));
  data_length = sizeof (drvid);
  rc = DosDevIOCtl (handle, IOCTL_XF86SUP, XF86SUP_DRVID,
                    NULL, 0, NULL,
                    &drvid, data_length, &data_length);
  if (rc != 0) return rc;
  if (data_length != sizeof (drvid)
      || drvid.magic != XF86SUP_MAGIC)
    return ERROR_BAD_DEV_TYPE;
  switch (drvid.id)
    {
    case XF86SUP_ID_PTY:
    case XF86SUP_ID_TTY:
    case XF86SUP_ID_CONSOLE:
    case XF86SUP_ID_PMAP:
    case XF86SUP_ID_FASTIO:
    case XF86SUP_ID_PTMS:
      break;
    default:
      return ERROR_BAD_DEV_TYPE;
    }
  *pflags = HF_XF86SUP;
  d = GET_FILE (handle);
  d->x.xf86sup.id = drvid.id;
  d->x.xf86sup.sem_open = FALSE;
  d->x.xf86sup.sem = 0;
  return 0;
}


int xf86sup_avail (ULONG handle, int *errnop)
{
  ULONG rc, data_length, avail;

  data_length = sizeof (avail);
  rc = DosDevIOCtl (handle, IOCTL_XF86SUP, XF86SUP_FIONREAD,
                    NULL, 0, NULL,
                    &avail, data_length, &data_length);
  if (rc != 0)
    {
      *errnop = set_error (rc);
      return -1;
    }
  if (data_length != sizeof (avail))
    {
      *errnop = EINVAL;
      return -1;
    }
  *errnop = 0;
  return (int)avail;
}


/* __fcntl() for a tty or pty handle.  This function is called for
   F_SETFL only, and only if O_NDELAY was changed. */


int xf86sup_fcntl (ULONG handle, ULONG request, ULONG arg, int *errnop)
{
  int on;

  on = arg & O_NDELAY;
  return xf86sup_ioctl (handle, FIONBIO, (ULONG)&on, errnop);
}



/* Common code for xf86sup_termio_get() and xf86sup_termios_get().
   Returns 0 on success, -1 on error.  Always sets *ERRNOP. */

static int xf86sup_tiocgeta (ULONG handle, struct xf86sup_termios *p,
                             int *errnop)
{
  ULONG rc, data_length;

  data_length = sizeof (*p);
  rc = DosDevIOCtl (handle, IOCTL_XF86SUP, XF86SUP_TIOCGETA,
                    NULL, 0, NULL,
                    p, data_length, &data_length);
  if (rc != 0)
    {
      *errnop = set_error (rc);
      return -1;
    }
  *errnop = 0;
  return 0;
}


/* Handle the TCGETA ioctl.  Returns 0 on success, -1 on error.
   Always sets *ERRNOP.*/

static int xf86sup_termio_get (ULONG handle, struct termio *p, int *errnop)
{
  struct xf86sup_termios xtio;

  if (xf86sup_tiocgeta (handle, &xtio, errnop) != 0)
    return -1;
  p->c_iflag = xtio.c_iflag;
  p->c_oflag = xtio.c_oflag;
  p->c_cflag = xtio.c_cflag;
  p->c_lflag = xtio.c_lflag;
  p->c_line = 0;
  /* Note that this drops three characters! */
  memcpy (p->c_cc, xtio.c_cc, NCC);
  return 0;
}


/* Handle the _TCGA ioctl for tcgetattr().  Returns 0 on success, -1
   on error.  Always sets *ERRNOP. */

static int xf86sup_termios_get (ULONG handle, struct termios *p, int *errnop)
{
  struct xf86sup_termios xtio;

  if (xf86sup_tiocgeta (handle, &xtio, errnop) != 0)
    return -1;
  p->c_iflag = xtio.c_iflag;
  p->c_oflag = xtio.c_oflag;
  p->c_cflag = xtio.c_cflag;
  p->c_lflag = xtio.c_lflag;
  memcpy (p->c_cc, xtio.c_cc, NCCS);
  return 0;
}


/* Common code for xf86sup_termio_set() and xf86sup_termios_set().
   Returns 0 on success, -1 on error.  Always sets *ERRNOP. */

static int xf86sup_tiocseta (ULONG handle, struct xf86sup_termios *p,
                             ULONG request, int *errnop)
{
  ULONG rc, data_length;

  data_length = sizeof (*p);
  rc = DosDevIOCtl (handle, IOCTL_XF86SUP, request,
                    p, data_length, &data_length,
                    NULL, 0, NULL);
  if (rc != 0)
    {
      *errnop = set_error (rc);
      return -1;
    }
  *errnop = 0;
  return 0;
}


/* Handle the TCSETA, TCSETAF, and TCSETAW ioctls.  Returns 0 on
   success, -1 on error.  Always sets *ERRNOP. */

static int xf86sup_termio_set (ULONG handle, const struct termio *p,
                               ULONG request, int *errnop)
{
  struct xf86sup_termios xtio;

  /* Get the current settings.  This is required for VSUSP, VSTOP, and
     VSTART, which are not provided in `struct termio'. */

  if (xf86sup_tiocgeta (handle, &xtio, errnop) != 0)
    return -1;
  xtio.c_iflag = p->c_iflag;
  xtio.c_oflag = p->c_oflag;
  xtio.c_cflag = p->c_cflag;
  xtio.c_lflag = p->c_lflag;
  /* This leaves alone the last three characters! */
  memcpy (xtio.c_cc, p->c_cc, NCC);
  return xf86sup_tiocseta (handle, &xtio, request, errnop);
}


/* Handle the _TCSANOW, _TCSADRAIN, and _TCSAFLUSH ioctls for
   tcsetattr().  Returns 0 on success, -1 on error.  Always sets
   *ERRNOP. */

static int xf86sup_termios_set (ULONG handle, const struct termios *p,
                                ULONG request, int *errnop)
{
  struct xf86sup_termios xtio;

  memset (&xtio, 0, sizeof (xtio)); /* Clear reserved fields */
  xtio.c_iflag = p->c_iflag;
  xtio.c_oflag = p->c_oflag;
  xtio.c_cflag = p->c_cflag;
  xtio.c_lflag = p->c_lflag;
  memcpy (xtio.c_cc, p->c_cc, NCCS);
  return xf86sup_tiocseta (handle, &xtio, request, errnop);
}


/* __ioct2l() for a tty or pty handle. */

int xf86sup_ioctl (ULONG handle, ULONG request, ULONG arg, int *errnop)
{
  int n;
  ULONG rc, parm_length;

  switch (request)
    {
    case FGETHTYPE:
      *(int *)arg = HT_DEV_OTHER; /* TODO */
      break;

    case FIONREAD:
      n = xf86sup_avail (handle, errnop);
      if (n == -1)
        return -1;
      *(int *)arg = n;
      break;

    case FIONBIO:
      parm_length = sizeof (int);
      rc = DosDevIOCtl (handle, IOCTL_XF86SUP, XF86SUP_FIONBIO,
                        (PVOID)arg, parm_length, &parm_length,
                        NULL, 0, NULL);
      if (rc != 0)
        {
          *errnop = set_error (rc);
          return -1;
        }
      if (*(int *)arg)
        handle_flags[handle] |= HF_NDELAY;
      else
        handle_flags[handle] &= ~HF_NDELAY;
      break;

    case TCGETA:
      return xf86sup_termio_get (handle, (struct termio *)arg, errnop);

    case TCSETA:
      return xf86sup_termio_set (handle, (const struct termio *)arg,
                                 XF86SUP_TIOCSETA, errnop);

    case TCSETAF:
      return xf86sup_termio_set (handle, (const struct termio *)arg,
                                 XF86SUP_TIOCSETAF, errnop);

    case TCSETAW:
      return xf86sup_termio_set (handle, (const struct termio *)arg,
                                 XF86SUP_TIOCSETAW, errnop);

    case TCFLSH:
      parm_length = sizeof (int);
      rc = DosDevIOCtl (handle, IOCTL_XF86SUP, XF86SUP_TIOCFLUSH,
                        &arg, parm_length, &parm_length,
                        NULL, 0, NULL);
      if (rc != 0)
        {
          *errnop = set_error (rc);
          return -1;
        }
      break;

    case TCSBRK:
      rc = DosDevIOCtl (handle, IOCTL_XF86SUP, XF86SUP_TIOCDRAIN,
                        NULL, 0, NULL,
                        NULL, 0, NULL);
      if (rc != 0)
        {
          *errnop = set_error (rc);
          return -1;
        }
      break;

    case TCXONC:
      /* Flow control. */
      break;

    case _TCGA:
      return xf86sup_termios_get (handle, (struct termios *)arg, errnop);

    case _TCSANOW:
      return xf86sup_termios_set (handle, (const struct termios *)arg,
                                  XF86SUP_TIOCSETA, errnop);

    case _TCSADRAIN:
      return xf86sup_termios_set (handle, (const struct termios *)arg,
                                  XF86SUP_TIOCSETAW, errnop);

    case _TCSAFLUSH:
      return xf86sup_termios_set (handle, (const struct termios *)arg,
                                  XF86SUP_TIOCSETAF, errnop);

    default:
      *errnop = EINVAL;
      return -1;
    }

  /* Function successfully completed. */

  *errnop = 0;
  return 0;
}


/* __select(): Add a semaphore for a pty, tty, or /dev/console$ handle,
   to be posted when data becomes ready for reading. */

int xf86sup_select_add_read (struct select_data *d, int fd)
{
  HEV sem;
  my_file *f;
  int i;
  ULONG rc, parm_length, data_length, arm_on;
  struct xf86sup_selreg selreg;

  if (!IS_VALID_FILE (fd))
    return EBADF;

  f = GET_FILE (fd);
  if (f->x.xf86sup.id != XF86SUP_ID_PTY && f->x.xf86sup.id != XF86SUP_ID_TTY
      && f->x.xf86sup.id != XF86SUP_ID_CONSOLE)
    return 0;                   /* Silently ignore */
  if (f->x.xf86sup.sem_open)
    sem = f->x.xf86sup.sem;
  else
    {
      /* Check whether a semaphore is already registered.  As the
         semaphore handles to be registered are null (invalid) and
         xf86sup.sys tries to open the new semaphore handles, we'll
         get ERROR_INVALID_PARAMETER if there are no semaphores
         registered. */

      selreg.rsel = selreg.xsel = 0;
      selreg.code = 0;
      parm_length = sizeof (selreg);
      data_length = sizeof (selreg);
      rc = DosDevIOCtl (fd, IOCTL_XF86SUP, XF86SUP_SELREG,
                        &selreg, parm_length, &parm_length,
                        &selreg, data_length, &data_length);
      if (rc == 0 && selreg.rsel != 0)
        {
          /* There is a semaphore registered.  Open it. */

          sem = (HEV)selreg.rsel;
          rc = DosOpenEventSem (NULL, &sem);
          if (rc != 0) return set_error (rc);
          f->x.xf86sup.sem = sem;
          f->x.xf86sup.sem_open = TRUE;
        }
      else
        {
          /* Create a semaphore which will be registered with
             xf86sup.sys.  Don't use create_event_sem() as the
             semaphore will stick forever (and to avoid overflowing
             the semaphore table). */

          if (DosCreateEventSem (NULL, &sem, DC_SEM_SHARED, FALSE) != 0)
            return EINVAL;

          /* Register the semaphore with xf86sup.sys.  Unfortunately,
             xf86sup.sys does not accept null handles, therefore we
             use the semaphore also for xsel.  TODO: This will cause
             problems with other processes trying to register two
             distinct semaphores with this pty/tty pair in the
             future. */

          selreg.rsel = (ULONG)sem; selreg.xsel = (ULONG)sem;
          selreg.code = 0;
          parm_length = sizeof (selreg);
          data_length = sizeof (selreg);
          rc = DosDevIOCtl (fd, IOCTL_XF86SUP, XF86SUP_SELREG,
                            &selreg, parm_length, &parm_length,
                            &selreg, data_length, &data_length);
          if (rc != 0)
            {
              DosCloseEventSem (sem);
              return set_error (rc);
            }

          /* If someone else has registered a semaphore in the
             meantime, use that semaphore. */

          if (selreg.code & XF86SUP_SEL_READ)
            {
              DosCloseEventSem (sem);
              sem = (HEV)selreg.rsel;
            }
          f->x.xf86sup.sem = sem;
          f->x.xf86sup.sem_open = TRUE;
        }
    }

  /* The semaphore handle is now in SEM.  Add the semaphore to the
     MuxWait list, unless it's already in the list. */

  for (i = 0; i < d->sem_count; ++i)
    if (d->list[i].hsemCur == (HSEM)sem)
      break;
  if (i >= d->sem_count)
    {
      /* The semaphore is not yet in the list.  Add it. */

      if (d->sem_count >= d->max_sem)
        return EINVAL;
      else
        {
          d->list[d->sem_count].hsemCur = (HSEM)sem;
          d->list[d->sem_count].ulUser = 0;
          ++d->sem_count;
        }
    }

  /* Arm the semaphore.  Arming a semaphore multiple times doesn't
     hurt as we first arm all semaphores, then poll all handles. */

  parm_length = sizeof (arm_on);
  arm_on = XF86SUP_SEL_READ;
  rc = DosDevIOCtl (fd, IOCTL_XF86SUP, XF86SUP_SELARM,
                    &arm_on, parm_length, &parm_length,
                    NULL, 0, NULL);
  if (rc != 0)
    return set_error (rc);
  return 0;
}


int xf86sup_ttyname (char *dst, ULONG dst_size, ULONG handle, int *errnop)
{
  ULONG rc, parm_length, data_length;
  char buf[14], *p;

  parm_length = 0;
  data_length = sizeof (buf);
  rc = DosDevIOCtl (handle, IOCTL_XF86SUP, XF86SUP_NAME,
                    NULL, parm_length, &parm_length,
                    buf, data_length, &data_length);
  if (rc != 0)
    {
      *errnop = set_error (rc);
      return -1;
    }

  /* There's no point in handling DBCS characters here. */

  for (p = buf; *p != 0; ++p)
    if (*p == '\\')
      *p = '/';
  return set_ttyname (dst, dst_size, buf, errnop);
}


ULONG xf86sup_uncond_enadup (ULONG handle, ULONG on)
{
  ULONG data_length;

  data_length = sizeof (on);
  return DosDevIOCtl (handle, IOCTL_XF86SUP, XF86SUP_ENADUP,
                      &on, data_length, &data_length,
                      NULL, 0, NULL);
}


ULONG xf86sup_maybe_enadup (ULONG handle, ULONG on)
{
  if (handle < handle_count
      && (handle_flags[handle] & (HF_OPEN|HF_XF86SUP))
      && IS_VALID_FILE (handle)
      && GET_FILE (handle)->x.xf86sup.id == XF86SUP_ID_PTY)
    return xf86sup_uncond_enadup (handle, on);
  else
    return 0;
}


void xf86sup_all_enadup (ULONG on)
{
  ULONG i;

  for (i = 0; i < handle_count; ++i)
    if ((handle_flags[i] & (HF_OPEN|HF_XF86SUP))
        && IS_VALID_FILE (i)
        && GET_FILE (i)->x.xf86sup.id == XF86SUP_ID_PTY)
      xf86sup_uncond_enadup (i, on);
}
