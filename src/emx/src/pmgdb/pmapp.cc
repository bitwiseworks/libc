/* pmapp.cc
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
#include "string.h"
#include "pmapp.h"


pmapp::pmapp (const char *in_name)
{
  name = in_name;
  hab = WinInitialize (0);
  hmq = WinCreateMsgQueue (hab, 0);
  WinQueryWindowPos (HWND_DESKTOP, &swpScreen);
}


pmapp::~pmapp ()
{
  WinDestroyMsgQueue (hmq);
  WinTerminate (hab);
}


void pmapp::no_message_loop () const
{
  WinCancelShutdown (hmq, TRUE);
}


void pmapp::message_loop () const
{
  QMSG qmsg;

  while (WinGetMsg (hab, &qmsg, 0, 0, 0))
    WinDispatchMsg (hab, &qmsg);
}


void pmapp::win_error () const
{
  // TODO: Make thread-safe
  static char buf[] = "WinGetLastError: 0x????????";

  ERRORID eid = WinGetLastError (hab);
  unsigned err = ERRORIDERROR (eid);
  _ltoa ((long)err, strchr (buf, 'x') + 1, 16);
  if (WinMessageBox (HWND_DESKTOP, HWND_DESKTOP, (PSZ)buf, (PCSZ)name, 0,
                     MB_MOVEABLE | MB_OK | MB_ICONEXCLAMATION) == MBID_ERROR)
    DosBeep (440, 500);
}


/* Remove all unwanted items from the system menu of a dialog box, as
   required by SAA CUA '89. */

void dlg_sys_menu (HWND hwnd)
{
  HWND hwndSysMenu;
  MENUITEM mi;
  MRESULT mr;
  int i, n, id;

  /* Get the window handle of the system menu. */

  hwndSysMenu = WinWindowFromID (hwnd, FID_SYSMENU);
  if (hwndSysMenu == NULLHANDLE)
    return;

  /* Well, actually the system menu proper is a submenu of the menu we
     just got. */

  mr = WinSendMsg (hwndSysMenu, MM_QUERYITEM,
                   MPFROM2SHORT (SC_SYSMENU, FALSE), (MPARAM)&mi);
  if (!SHORT1FROMMR (mr))
    return;
  hwndSysMenu = mi.hwndSubMenu;
  if (hwndSysMenu == NULLHANDLE)
    return;

  /* Delete all the unwanted menu items. */

  WinSendMsg (hwndSysMenu, MM_DELETEITEM,
              MPFROM2SHORT (SC_MINIMIZE, FALSE), 0);
  WinSendMsg (hwndSysMenu, MM_DELETEITEM,
              MPFROM2SHORT (SC_MAXIMIZE, FALSE), 0);
  WinSendMsg (hwndSysMenu, MM_DELETEITEM,
              MPFROM2SHORT (SC_SIZE, FALSE), 0);
  WinSendMsg (hwndSysMenu, MM_DELETEITEM,
              MPFROM2SHORT (SC_RESTORE, FALSE), 0);
  WinSendMsg (hwndSysMenu, MM_DELETEITEM,
              MPFROM2SHORT (SC_TASKMANAGER, FALSE), 0);
  WinSendMsg (hwndSysMenu, MM_DELETEITEM,
              MPFROM2SHORT (SC_HIDE, FALSE), 0);

  /* Deleting menu items may leave two or more successive separator
     lines.  Now we remove all the separator lines.  */

redo:

  /* Get the number of menu items. */

  mr = WinSendMsg (hwndSysMenu, MM_QUERYITEMCOUNT, 0, 0);
  n = SHORT1FROMMR (mr);

  /* Find the first one which is a separator line. */

  for (i = 0; i < n; ++i)
    {
      mr = WinSendMsg (hwndSysMenu, MM_ITEMIDFROMPOSITION, MPFROMSHORT (i), 0);
      id = SHORT1FROMMR (mr);
      if (id != MID_ERROR)
        {
          mr = WinSendMsg (hwndSysMenu, MM_QUERYITEM,
                           MPFROM2SHORT (id, FALSE), (MPARAM)&mi);
          if (SHORT1FROMMR (mr) && (mi.afStyle & MIS_SEPARATOR))
            {
              /* We found a separator line.  Delete that menu item. */

              WinSendMsg (hwndSysMenu, MM_DELETEITEM,
                          MPFROM2SHORT (id, FALSE), 0);

              /* Restart the loop as the menu items have been
                 renumbered. */

              goto redo;
            }
        }
    }

  /* Now there's no separator line left. */
}
