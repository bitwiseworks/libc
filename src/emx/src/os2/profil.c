/* profil.c -- Sampling profiler
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


#define INCL_DOSMODULEMGR
#define INCL_DOSERRORS
#include <os2emx.h>
#include <emx/syscalls.h>
#include "emxdll.h"
#include "profil.h"
#include "clib.h"

static enum
{
  PS_INIT,
  PS_FAIL,
  PS_STOP,
  PS_RUN
} profil_state;

static void *profil_buf;
static unsigned profil_bufsiz;
static unsigned profil_offset;
static ULONG (*dosprofile)(ULONG, ULONG, PRFCMD *, PRFRET *);


static int xdosprofile (ULONG func, ULONG pid, PRFCMD *cmd, PRFRET *ret)
{
  ULONG rc;

  rc = dosprofile (func, pid, cmd, ret);
  if (rc != 0)
    return set_error (rc);
  return 0;
}


static int profil_init (void)
{
  char obj[9];
  HMODULE hmod;
  ULONG rc;

  profil_state = PS_FAIL;
  rc = DosLoadModule (obj, sizeof (obj), "DOSCALLS", &hmod);
  if (rc != 0)
    return set_error (rc);
  rc = DosQueryProcAddr (hmod, 377, NULL, (PPFN)&dosprofile);
  if (rc != 0)
    return set_error (rc);
  profil_state = PS_STOP;
  return 0;
}


static int profil_result (void)
{
  PRFRET prof_ret;
  PRFRET3 *ret3;
  ULONG ret3_size, rc;
#ifdef DEBUG
  PRFRET0 prof_ret0;
  unsigned shift, i;
#endif
  int r;

  if (profil_state != PS_RUN)
    return set_error (ERROR_PRF_NOT_STARTED);
  memset (profil_buf, 0, profil_bufsiz);
  r = xdosprofile (PRF_CM_STOP, my_pid, NULL, NULL);
  if (r != 0) return r;
  profil_state = PS_STOP;

#ifdef DEBUG
  memset (&prof_ret0, 0, sizeof (prof_ret0));
  prof_ret.us_cmd = PRF_RET_GLOBAL;
  prof_ret.us_thrdno = 1;
  prof_ret.us_vaddr = 0;
  prof_ret.us_bufsz = sizeof (prof_ret0);
  prof_ret.us_buf = &prof_ret0;
  r = xdosprofile (PRF_CM_DUMP, my_pid, NULL, &prof_ret);
  if (r != 0) return r;
  shift = prof_ret0.r0_shift;
  oprintf ("r0_shift=%u\r\n", shift);
#endif

  /* TODO: Overlay data preceding *profil_buff, if posible. */

  ret3_size = profil_bufsiz + sizeof (PRFRET3);
  rc = DosAllocMem ((PPVOID)&ret3, ret3_size,
                    PAG_READ | PAG_WRITE | PAG_COMMIT);
  if (rc != 0)
    return set_error (rc);
  memset (ret3, 0, ret3_size);

  memset (&prof_ret, 0, sizeof (prof_ret));
  prof_ret.us_cmd = PRF_RET_VADETAIL;
  prof_ret.us_thrdno = 1;
  prof_ret.us_vaddr = profil_offset;
  prof_ret.us_bufsz = ret3_size;
  prof_ret.us_buf = ret3;
  r = xdosprofile (PRF_CM_DUMP, my_pid, NULL, &prof_ret);
  if (r != 0)
    {
      DosFreeMem (ret3);
      return r;
    }
  memcpy (profil_buf, ret3->r3_cnts, profil_bufsiz);
#ifdef DEBUG
  for (i = 0; i < ret3->r3_ncnts; ++i)
    oprintf ("%#.8x: %u\r\n",
             profil_offset + (i << shift), ret3->r3_cnts[i]);
#endif
  DosFreeMem (ret3);
  return xdosprofile (PRF_CM_EXIT, my_pid, NULL, NULL);
}


static int profil_start (const struct _profil *p)
{
  int r;

  profil_buf = p->buff; profil_bufsiz = p->bufsiz; profil_offset = p->offset;
  memset (profil_buf, 0, profil_bufsiz);
  if (profil_state == PS_RUN)
    {
      r = xdosprofile (PRF_CM_STOP, my_pid, NULL, NULL);
      if (r != 0) return r;
      profil_state = PS_STOP;
      r = xdosprofile (PRF_CM_CLEAR, my_pid, NULL, NULL);
      if (r != 0) return r;
    }
  if (p->scale == 2)
    {
      /* TODO */
      return set_error (ERROR_INVALID_PARAMETER);
    }
  else if (p->scale == 0x10000 || p->scale == 0x8000 || p->scale == 0x4000)
    {
      PRFCMD prof_cmd;
      PRFSLOT prof_slots[1];

      memset (&prof_slots, 0, sizeof (prof_slots));
      memset (&prof_cmd, 0, sizeof (prof_cmd));

      prof_slots[0].sl_vaddr = profil_offset;
      prof_slots[0].sl_size = p->bufsiz * (0x10000 / p->scale);
      prof_slots[0].sl_mode = PRF_SL_PRFVA;

#ifdef DEBUG
      oprintf ("bufsiz=%#.8x\r\n", profil_bufsiz);
      oprintf ("scale=%#.8x\r\n", p->scale);
      oprintf ("sl_vaddr=%#.8x\r\n", prof_slots[0].sl_vaddr);
      oprintf ("sl_size=%#.8x\r\n", prof_slots[0].sl_size);
#endif

      prof_cmd.cm_slots = prof_slots;
      prof_cmd.cm_nslots = 1;
      prof_cmd.cm_flags = PRF_PROCESS_ST | PRF_VADETAIL;
      prof_cmd.cm_bufsz = profil_bufsiz;
      prof_cmd.cm_timval = 1000;
      prof_cmd.cm_flgbits = NULL;
      prof_cmd.cm_nflgs = 0;

      r = xdosprofile (PRF_CM_INIT, my_pid, &prof_cmd, NULL);
      if (r != 0) return r;
    }
  r = xdosprofile (PRF_CM_START, my_pid, NULL, NULL);
  if (r != 0) return r;
  profil_state = PS_RUN;
  return 0;
}


int do_profil (const struct _profil *p, int *errnop)
{
  int r;

  if (p->cb < sizeof (struct _profil))
    {
      *errnop = set_error (ERROR_INVALID_PARAMETER);
      return -1;
    }

  if (profil_state == PS_INIT)
    {
      r = profil_init ();
      if (r != 0)
        {
          *errnop = r;
          return -1;
        }
    }
  if (profil_state == PS_FAIL)
    {
      *errnop = set_error (ERROR_CALL_NOT_IMPLEMENTED);
      return -1;
    }

  if (p->scale == 0 || p->scale == 1)
    r = profil_result ();
  else
    r = profil_start (p);
  if (r != 0)
    {
      *errnop = r;
      return -1;
    }
  *errnop = 0;
  return 0;
}
