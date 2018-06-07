/* pmframe.cc
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
#include "pmframe.h"

// TODO
static pmframe *pmframe_init_kludge;

pmframe::pmframe (pmapp *in_app)
{
  app = in_app;
  minimized = false;
  hwndHelp = NULLHANDLE;
  keys_help_id = 0;
  tid = _gettid ();
}


void pmframe::create (const char *classname, unsigned id, ULONG frame_flags,
                      const SWP *pswp)
{
  WinRegisterClass (app->get_hab (), (PSZ)classname, pmframe_client_msg,
                    CS_SIZEREDRAW, sizeof (pmframe *));

  frame_flags |= (FCF_TITLEBAR      | FCF_SYSMENU |
                  FCF_SIZEBORDER    | FCF_MINMAX  |
                  FCF_MENU          | FCF_ACCELTABLE |
                  FCF_VERTSCROLL    | FCF_HORZSCROLL);

  pmframe_init_kludge = this;
  if (WinCreateStdWindow (HWND_DESKTOP, 0,
                          &frame_flags, (PSZ)classname,
                          NULL, 0L, 0, id, NULL) == NULLHANDLE)
    {
      win_error ();
      abort ();
    }
  old_frame_msg = WinSubclassWindow (hwndFrame, pmframe_frame_msg);
  if (pswp != NULL)
    WinSetWindowPos (hwndFrame, HWND_DESKTOP, pswp->x, pswp->y,
                     pswp->cx, pswp->cy, SWP_MOVE | SWP_SIZE);
}


pmframe::~pmframe ()
{
  WinDestroyWindow (hwndFrame);
}


void pmframe::set_title (const char *title)
{
  WinSetWindowText (hwndFrame, (PCSZ)title);
}


void pmframe::repaint (RECTL *prcl)
{
  WinInvalidateRect (hwndClient, prcl, FALSE);
}


repaint (RECTL *prcl = NULL);


MRESULT pmframe::wm_activate (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
  return WinDefWindowProc (hwnd, msg, mp1, mp2);
}


MRESULT pmframe::wm_button (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
  return WinDefWindowProc (hwnd, msg, mp1, mp2);
}


MRESULT pmframe::wm_char (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
  return WinDefWindowProc (hwnd, msg, mp1, mp2);
}


MRESULT pmframe::wm_close (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
  return WinDefWindowProc (hwnd, msg, mp1, mp2);
}


MRESULT pmframe::wm_command (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
  return WinDefWindowProc (hwnd, msg, mp1, mp2);
}


MRESULT pmframe::wm_create (HWND, ULONG, MPARAM, MPARAM)
{
  return FALSE;
}


MRESULT pmframe::wm_hscroll (HWND, ULONG, MPARAM, MPARAM)
{
  return 0;
}


MRESULT pmframe::wm_presparamchanged (HWND, ULONG, MPARAM, MPARAM)
{
  return 0;
}


MRESULT pmframe::wm_setfocus (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
  return WinDefWindowProc (hwnd, msg, mp1, mp2);
}


MRESULT pmframe::wm_size (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
  return WinDefWindowProc (hwnd, msg, mp1, mp2);
}


MRESULT pmframe::wm_timer (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
  return WinDefWindowProc (hwnd, msg, mp1, mp2);
}


bool pmframe::wm_user (HWND, ULONG, MPARAM, MPARAM)
{
  return false;
}


MRESULT pmframe::wm_vscroll (HWND, ULONG, MPARAM, MPARAM)
{
  return 0;
}


MRESULT pmframe::client_msg (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
  PSWP pswp;

  switch (msg)
    {
    case WM_ACTIVATE:
      return wm_activate (hwnd, msg, mp1, mp2);

    case WM_BUTTON1DOWN:
    case WM_BUTTON2DOWN:
    case WM_BUTTON3DOWN:
    case WM_BUTTON1CLICK:
    case WM_BUTTON2CLICK:
    case WM_BUTTON3CLICK:
    case WM_BUTTON1DBLCLK:
    case WM_BUTTON2DBLCLK:
    case WM_BUTTON3DBLCLK:
      return wm_button (hwnd, msg, mp1, mp2);

    case WM_CHAR:
      return wm_char (hwnd, msg, mp1, mp2);

    case WM_CLOSE:
      return wm_close (hwnd, msg, mp1, mp2);

    case WM_COMMAND:
      return wm_command (hwnd, msg, mp1, mp2);

    case WM_CREATE:
      hwndClient = hwnd;
      hwndFrame = WinQueryWindow (hwnd, QW_PARENT);
      hwndMenu = WinWindowFromID (hwndFrame, FID_MENU);

      WinSetWindowPtr (hwndClient, QWL_USER, this);
      WinSetWindowPtr (hwndFrame, QWL_USER, this);
      return wm_create (hwnd, msg, mp1, mp2);

    case WM_DESTROY:
      if (hwndHelp != NULLHANDLE)
        {
          WinAssociateHelpInstance (NULLHANDLE, hwndFrame);
          WinDestroyHelpInstance (hwndHelp);
        }
      return 0;

    case WM_HSCROLL:
      return wm_hscroll (hwnd, msg, mp1, mp2);

    case WM_MINMAXFRAME:
      pswp = (PSWP)mp1;
      if (pswp->fl & SWP_MINIMIZE)
        minimized = true;
      else if (pswp->fl & (SWP_RESTORE | SWP_MAXIMIZE))
        minimized = false;
      return FALSE;

    case WM_PAINT:
      return wm_paint (hwnd, msg, mp1, mp2);

    case WM_PRESPARAMCHANGED:
      return wm_presparamchanged (hwnd, msg, mp1, mp2);

    case WM_SETFOCUS:
      return wm_setfocus (hwnd, msg, mp1, mp2);

    case WM_SIZE:
      return wm_size (hwnd, msg, mp1, mp2);

    case WM_TIMER:
      return wm_timer (hwnd, msg, mp1, mp2);

    case WM_VSCROLL:
      return wm_vscroll (hwnd, msg, mp1, mp2);

    case HM_QUERY_KEYS_HELP:
      return (MRESULT)keys_help_id;

    default:
      if (msg >= WM_USER)
        {
          wm_user (hwnd, msg, mp1, mp2);
          return FALSE;
        }
      else
        return WinDefWindowProc (hwnd, msg, mp1, mp2);
    }
}


MRESULT pmframe::wm_querytrackinfo (HWND hwnd, ULONG msg, MPARAM mp1,
                                    MPARAM mp2)
{
  return old_frame_msg (hwnd, msg, mp1, mp2);
}




MRESULT pmframe::frame_msg (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
  switch (msg)
    {
    case WM_QUERYTRACKINFO:
      return wm_querytrackinfo (hwnd, msg, mp1, mp2);
    }
  return old_frame_msg (hwnd, msg, mp1, mp2);
}


MRESULT EXPENTRY
pmframe_client_msg (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
  if (msg == WM_CREATE)
    return pmframe_init_kludge->client_msg (hwnd, msg, mp1, mp2);
  else
    {
      pmframe *p = (pmframe *)WinQueryWindowPtr (hwnd, QWL_USER);
      return p->client_msg (hwnd, msg, mp1, mp2);
    }
}


MRESULT EXPENTRY
pmframe_frame_msg (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
  pmframe *p = (pmframe *)WinQueryWindowPtr (hwnd, QWL_USER);
  return p->frame_msg (hwnd, msg, mp1, mp2);
}


void pmframe::focus () const
{
  WinSetActiveWindow (HWND_DESKTOP, hwndFrame);
}


void pmframe::top () const
{
  WinSetWindowPos (hwndFrame, HWND_TOP, 0, 0, 0, 0, SWP_ZORDER);
}


void pmframe::show (bool visible) const
{
  if (visible)
    WinSetWindowPos (hwndFrame, HWND_TOP, 0, 0, 0, 0, SWP_SHOW | SWP_RESTORE);
  else
    WinShowWindow (hwndFrame, FALSE);
}


void pmframe::menu_check (unsigned idm, bool on) const
{
  if (!WinSendMsg (hwndMenu, MM_SETITEMATTR,
                   MPFROM2SHORT (idm, TRUE),
                   MPFROM2SHORT (MIA_CHECKED, (on ? MIA_CHECKED : 0))))
    WinAlarm (HWND_DESKTOP, WA_ERROR);
}


void pmframe::menu_enable (unsigned idm, bool on, bool silent) const
{
  if (!WinSendMsg (hwndMenu, MM_SETITEMATTR,
                   MPFROM2SHORT (idm, TRUE),
                   MPFROM2SHORT (MIA_DISABLED, (on ? 0 : MIA_DISABLED)))
      && !silent)
    WinAlarm (HWND_DESKTOP, WA_ERROR);
}


HWND pmframe::get_hwndMenu_from_id (unsigned idm) const
{
  MENUITEM mi;
  MRESULT mr = WinSendMsg (hwndMenu, MM_QUERYITEM,
                           MPFROM2SHORT (idm, FALSE), (MPARAM)&mi);
  return SHORT1FROMMR (mr) ? mi.hwndSubMenu : NULLHANDLE;
}


bool pmframe::is_visible () const
{
  return !minimized && WinIsWindowVisible (hwndClient);
}


bool pmframe::help_init (unsigned help_table_id, const char *title,
                         const char *file, unsigned in_keys_help_id)
{
  HELPINIT helpinit;
  memset (&helpinit, 0, sizeof (helpinit));
  helpinit.cb = sizeof (helpinit);
  helpinit.pszTutorialName = NULL;
  helpinit.phtHelpTable = (PHELPTABLE)MAKELONG (help_table_id, 0xFFFF);
  helpinit.hmodHelpTableModule = 0;
  helpinit.hmodAccelActionBarModule = 0;
  helpinit.idActionBar = 0;
  helpinit.pszHelpWindowTitle = (PSZ)title;
  helpinit.fShowPanelId = CMIC_HIDE_PANEL_ID;
  helpinit.pszHelpLibraryName = (PSZ)file;
  hwndHelp = WinCreateHelpInstance (get_hab (), &helpinit);
  if (hwndHelp != NULLHANDLE)
    if (!WinAssociateHelpInstance (hwndHelp, hwndFrame))
      hwndHelp = NULLHANDLE;
  keys_help_id = in_keys_help_id;
  return hwndHelp != NULLHANDLE;
}
