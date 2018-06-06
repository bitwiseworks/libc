/* pmdebug.cc
   Copyright (c) 1996 Eberhard Mattes

This file is part of pmgdb.

pmgdb is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

pmgdb is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with pmgdb; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */


#define INCL_DOS
#define INCL_WIN
#include <os2.h>
#include <stdlib.h>
#include <string.h>
#include "pmdebug.h"


pmdebug pmdbg;


static VOID APIENTRY pmdebug_exitlist (ULONG)
{
  pmdbg.exit ();
  DosExitList (EXLST_EXIT, NULL);
}


pmdebug::pmdebug ()
{
  cur_mode = off; locked = false; pid = 0; hwndLock = NULLHANDLE;
}


pmdebug::~pmdebug ()
{
  exit ();
}


void pmdebug::exit ()
{
  if (cur_mode == sync && locked)
    {
      // TODO: Destroy all windows of the debuggee (pid)
      // TODO: Kill debuggee (important!)
      WinLockInput (NULLHANDLE, FALSE);
      locked = false;
    }
  set_mode (off, NULLHANDLE);
}


void pmdebug::set_mode (mode new_mode, HWND hwnd, int in_pid)
{
  if (new_mode == cur_mode)
    return;
  if (cur_mode == sync && locked)
    {
      WinLockInput (NULLHANDLE, FALSE);
      locked = false;
    }
  if (new_mode == sync)
    DosExitList (EXLST_ADD | (0 << 8), pmdebug_exitlist);
  else if (cur_mode == sync)
    DosExitList (EXLST_REMOVE, pmdebug_exitlist);
  cur_mode = new_mode;
  pid = in_pid; hwndLock = hwnd;
}


void pmdebug::start ()
{
  switch (cur_mode)
    {
    case sync:
      if (locked)
        WinLockInput (NULLHANDLE, FALSE);
      locked = false;
      break;
    default:
      break;
    }
}


void pmdebug::stop ()
{
  switch (cur_mode)
    {
    case sync:
      if (!locked)
        {
          locked = true;
          WinLockInput (NULLHANDLE, FALSE);
          WinLockInput (NULLHANDLE, WLI_NOBUTTONUP);
          WinSetSysModalWindow (HWND_DESKTOP, hwndLock);
          WinSetFocus (HWND_DESKTOP, hwndLock);
          WinSetSysModalWindow (HWND_DESKTOP, NULLHANDLE);
        }
      break;
    default:
      break;
    }
}


void pmdebug::term ()
{
  pid = 0;
  start ();
}
