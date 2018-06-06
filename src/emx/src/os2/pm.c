/* pm.c -- Presentation Manager stuff
   Copyright (c) 1994-1995 by Eberhard Mattes

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
#define INCL_DOSSESMGR
#define INCL_DOSPROCESS
#define INCL_WINWINDOWMGR
#include <os2emx.h>
#include "emxdll.h"

static HMODULE hmod_pmwin;
static char pm_initialized;


static HAB (*p_WinInitialize) (ULONG fsOptions);
static HMQ (*p_WinCreateMsgQueue) (HAB hab, LONG cmsg);
static ULONG (*p_WinMessageBox) (HWND hwndParent, HWND hwndOwner, PCSZ pszText,
    PCSZ pszCaption, ULONG idWindow, ULONG flStyle);


#define LOAD_PMWIN(ord,var) \
    (DosQueryProcAddr (hmod_pmwin, ord, NULL, (PPFN)var) != 0)

int pm_init (void)
{
  HAB hab;
  HMQ hmq;
  HMODULE hmod_pmwin;
  char obj[9];

  /* Loading PMWIN.DLL starts the Presentation Manager.  We don't want
     to do this when running a non-PM program from the OS/2 recovery
     command line.  Strangely enough, WinInitialize succeeds for
     non-PM programs run from the OS/2 recovery command line. */

  if (init_pib_ptr->pib_ultype != SSF_TYPE_PM)
    return FALSE;

  /* Dynamically load PMWIN.DLL.  If references to PMWIN.DLL were made
     at link time, programs using EMX.DLL could not be used when
     booting from diskette and with RUN= of config.sys. */

  if (DosLoadModule (obj, sizeof (obj), "PMWIN", &hmod_pmwin) != 0)
    return FALSE;

  if (LOAD_PMWIN (763, &p_WinInitialize)
      || LOAD_PMWIN (716, &p_WinCreateMsgQueue)
      || LOAD_PMWIN (789, &p_WinMessageBox))
    {
      DosFreeModule (hmod_pmwin);
      return FALSE;
    }
  pm_initialized = TRUE;

  hab = p_WinInitialize (0);
  if (hab == NULLHANDLE) return FALSE;
  hmq = p_WinCreateMsgQueue (hab, 0);
  return hmq != NULLHANDLE;
}


/* Unload any DLLs loaded by pm_init(). */

void pm_term (void)
{
  if (pm_initialized)
    {
      pm_initialized = FALSE;
      DosFreeModule (hmod_pmwin);
    }
}


void pm_message_box (PCSZ text)
{
  if (p_WinMessageBox != NULL)
    p_WinMessageBox (HWND_DESKTOP, HWND_DESKTOP, text, "emx.dll", 0,
                     MB_MOVEABLE | MB_OK | MB_ICONHAND);
}
