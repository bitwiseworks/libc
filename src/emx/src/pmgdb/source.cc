/* source.cc
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>
#include "string.h"
#include "pmapp.h"
#include "pmframe.h"
#include "pmtxt.h"
#include "pmtty.h"
#include "pmgdb.h"
#include "help.h"
#include "breakpoi.h"
#include "source.h"
#include "display.h"
#include "annotati.h"
#include "capture.h"
#include "gdbio.h"
#include "command.h"

#define LINENO_WIDTH    6
#define SOURCE_COLUMN   (LINENO_WIDTH + 4)

source_window::source_window (pmapp *in_app, unsigned in_id,
                              const char *fontnamesize,
                              const char *short_fname, const char *long_fname,
                              command_window *in_cmd, gdbio *in_gdb)
  : pmtxt (in_app, in_id, FCF_SHELLPOSITION, NULL, fontnamesize)
{
  cmd = in_cmd; gdb = in_gdb;
  short_filename = short_fname;
  long_filename = long_fname;

  astring title;
  title = "pmgdb - ";
  title.append (short_filename);
  set_title (title);

  set_keys_help_id (HELP_SRC_KEYS);

  hilite_attr = get_default_attr ();
  set_fg_color (hilite_attr, CLR_WHITE);
  set_bg_color (hilite_attr, CLR_BLACK);

  sel_attr = get_default_attr ();
  set_fg_color (sel_attr, CLR_BLACK);
  set_bg_color (sel_attr, CLR_PALEGRAY);

  bpt_attr = get_default_attr ();
  set_fg_color (bpt_attr, CLR_WHITE);
  set_bg_color (bpt_attr, CLR_RED);

  sel_bpt_attr = get_default_attr ();
  set_fg_color (sel_bpt_attr, CLR_WHITE);
  set_bg_color (sel_bpt_attr, CLR_PINK);

  // Initialize menu
  WinPostMsg (get_hwndClient (), UWM_STATE, MPFROMLONG (~0), 0);

  load ();
  set_font (*cmd);
  show (true);
}


source_window::~source_window ()
{
}


void source_window::load ()
{
  char buf[max_line_len+2];     // Line read from source file
  char pfx[10];                 // Line number
  int c;

  delete_all ();
  cur_lineno = -1; sel_lineno = -1;
  exp_lineno = -1; find_string = "";
  menu_enable (IDM_FINDNEXT, false);
  menu_enable (IDM_FINDPREV, false);

  FILE *f = fopen (long_filename, "r");
  if (f == NULL)
    {
      // TODO: Use GDB's value of `set directory'
      // TODO: Dialog box, asking for file name
      strcpy (buf, "Cannot open file");
      put (0, 0, strlen (buf), buf, true);
    }
  else
    {
      int y = 0;
      while (fgets (buf, sizeof (buf), f) != NULL)
        {
          size_t len = strlen (buf);
          if (len > 0 && buf[len-1] == '\n')
            --len;
          else
            {
              // Line too long, skip rest
              do
                {
                  c = fgetc (f);
                } while (c != EOF && c != '\n');
            }

          // Insert line number
          int pfx_len = sprintf (pfx, "%*d    ", LINENO_WIDTH, y + 1);
          put (y, 0, pfx_len, pfx, false);
          put_tab (y, LINENO_WIDTH + 1, false);
          put_vrule (y, LINENO_WIDTH + 2, false);

          // Insert source line
          char *tab = (char *)memchr (buf, '\t', len);
          if (tab == NULL)
            put (y, SOURCE_COLUMN, len, buf, false);
          else
            {
              // Expand TABs
              int x = 0;
              size_t i = 0;
              while (tab != NULL)
                {
                  size_t copy = tab - (buf + i);
                  if (copy != 0)
                    {
                      put (y, SOURCE_COLUMN + x, copy, buf + i, false);
                      i += copy; x += copy;
                    }
                  int tab_width = (x | 7) + 1 - x;
                  put (y, SOURCE_COLUMN + x, tab_width, "        ", false);
                  ++i; x += tab_width;
                  tab = (char *)memchr (buf + i, '\t', len - i);
                }
              if (i < len)
                put (y, SOURCE_COLUMN + x, len - i, buf + i, false);
            }
          ++y;
        }
      // TODO: ferror()
      fclose (f);
    }
  cmd->get_breakpoints (this, false);
}


void source_window::show_line (int lineno)
{
  if (lineno != cur_lineno)
    {
      if (cur_lineno != -1)
        {
          put (cur_lineno - 1, SOURCE_COLUMN - 1, max_line_len,
               get_default_attr (), true);
          set_eol_attr (cur_lineno - 1, get_default_attr (), true);
        }
      if (lineno != -1)
        {
          put (lineno - 1, SOURCE_COLUMN - 1, max_line_len, hilite_attr, true);
          set_eol_attr (lineno - 1, hilite_attr, true);
        }
      cur_lineno = lineno;
      if (lineno == exp_lineno)
        exp_lineno = -1;
    }
  if (lineno != -1)
    parent::show_line (lineno - 1, 1, get_window_lines () * 2 / 3);
}


void source_window::select_line (int lineno)
{
  if (lineno != sel_lineno)
    {
      if (sel_lineno != -1)
        {
          const breakpoint *bpt
            = cmd->breakpoints.find (short_filename, sel_lineno);
          put (sel_lineno - 1, 0, LINENO_WIDTH + 2,
               bpt != NULL ? bpt_attr : get_default_attr (), true);
        }
      if (lineno != -1)
        {
          const breakpoint *bpt
            = cmd->breakpoints.find (short_filename, lineno);
          put (lineno - 1, 0, LINENO_WIDTH + 2,
               bpt != NULL ? sel_bpt_attr : sel_attr, true);
        }
      sel_lineno = lineno;
    }
}


ULONG source_window::selected_addr ()
{
  if (sel_lineno == -1)
    return 0;
  return gdb->address_of_line (short_filename, sel_lineno);
}


MRESULT source_window::goto_msg (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
  char buf[80];

  switch (msg)
    {
    case WM_INITDLG:
      dlg_sys_menu (hwnd);
      WinSetWindowPtr (hwnd, QWL_USER, this);
      break;

    case WM_COMMAND:
      switch (SHORT1FROMMP (mp1))
        {
        case DID_OK:
          WinQueryDlgItemText (hwnd, IDC_LINENO, sizeof (buf), (PSZ)buf);
          int lineno = atoi (buf);
          if (lineno > 0)
            parent::show_line (lineno - 1, 1, get_window_lines () / 2);
          WinDismissDlg (hwnd, DID_OK);
          return 0;
        }
      break;
    }
  return WinDefDlgProc (hwnd, msg, mp1, mp2);
}


MRESULT EXPENTRY dlg_goto (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
  source_window *p;

  if (msg == WM_INITDLG)
    p = (source_window *)((struct pm_create *)PVOIDFROMMP (mp2))->ptr;
  else
    p = (source_window *)WinQueryWindowPtr (hwnd, QWL_USER);
  return p->goto_msg (hwnd, msg, mp1, mp2);
}


MRESULT source_window::find_msg (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
  char buf[200];

  switch (msg)
    {
    case WM_INITDLG:
      dlg_sys_menu (hwnd);
      WinSendDlgItemMsg (hwnd, IDC_STRING, EM_SETTEXTLIMIT,
                         MPFROMSHORT (sizeof (buf) - 1), 0);
      WinSetDlgItemText (hwnd, IDC_STRING, (PCSZ)find_string);
      WinSetWindowPtr (hwnd, QWL_USER, this);
      break;

    case WM_COMMAND:
      switch (SHORT1FROMMP (mp1))
        {
        case DID_OK:
          WinQueryDlgItemText (hwnd, IDC_STRING, sizeof (buf), (PSZ)buf);
          if (buf[0] != 0)
            {
              find_string = buf;
              WinDismissDlg (hwnd, DID_OK);
            }
          else
            WinDismissDlg (hwnd, DID_CANCEL);
          return 0;
        }
      break;
    }
  return WinDefDlgProc (hwnd, msg, mp1, mp2);
}


MRESULT EXPENTRY dlg_find (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
  source_window *p;

  if (msg == WM_INITDLG)
    p = (source_window *)((struct pm_create *)PVOIDFROMMP (mp2))->ptr;
  else
    p = (source_window *)WinQueryWindowPtr (hwnd, QWL_USER);
  return p->find_msg (hwnd, msg, mp1, mp2);
}


MRESULT source_window::wm_activate (HWND hwnd, ULONG msg,
                                    MPARAM mp1, MPARAM mp2)
{
  if (SHORT1FROMMP (mp1))
    cmd->associate_help (get_hwndFrame ());
  return WinDefWindowProc (hwnd, msg, mp1, mp2);
}


MRESULT source_window::wm_char (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
  if ((SHORT1FROMMP (mp1) & (KC_VIRTUALKEY|KC_KEYUP)) == KC_VIRTUALKEY)
    {
      HWND hwndTarget = NULLHANDLE;
      switch (SHORT2FROMMP (mp2))
        {
        case VK_UP:
        case VK_DOWN:
          hwndTarget = get_hwndVbar ();
          break;

        case VK_PAGEUP:
        case VK_PAGEDOWN:
          if (SHORT1FROMMP (mp1) & KC_CTRL)
            hwndTarget = get_hwndHbar ();
          else
            hwndTarget = get_hwndVbar ();
          break;

        case VK_LEFT:
        case VK_RIGHT:
          hwndTarget = get_hwndHbar ();
          break;
        }
      if (hwndTarget != NULLHANDLE && WinIsWindowEnabled (hwndTarget))
        WinPostMsg (hwndTarget, msg, mp1, mp2);
    }
  return WinDefWindowProc (hwnd, msg, mp1, mp2);
}


MRESULT source_window::wm_command (HWND hwnd, ULONG msg,
                                   MPARAM mp1, MPARAM mp2)
{
  ULONG addr;
  pm_create create;

  switch (SHORT1FROMMP (mp1))
    {
    case IDM_GOTO:
      create.cb = sizeof (create);
      create.ptr = (void *)this;
      WinDlgBox (HWND_DESKTOP, get_hwndClient (), dlg_goto, 0, IDD_GOTO,
                 &create);
      return 0;

    case IDM_FIND:
      create.cb = sizeof (create);
      create.ptr = (void *)this;
      if (WinDlgBox (HWND_DESKTOP, get_hwndClient (), dlg_find, 0, IDD_FIND,
                     &create) == DID_OK)
        find (true, true);
      return 0;

    case IDM_FINDNEXT:
      find (false, true);
      return 0;

    case IDM_FINDPREV:
      find (false, false);
      return 0;

    case IDM_JUMP:
      // TODO: Don't do this in the PM thread (time!)
      addr = selected_addr ();
      if (addr == 0 || !gdb->call_cmd ("server set var $pc=0x%lx", addr))
        WinAlarm (HWND_DESKTOP, WA_ERROR);
      else
        cmd->where ();
      return 0;

    case IDM_UNTIL:
      // TODO: Don't do this in the PM thread (time!)
      addr = selected_addr ();
      if (addr == 0)
        WinAlarm (HWND_DESKTOP, WA_ERROR);
      else
        {
          gdb->prepare_run ();
          gdb->send_cmd ("server until *0x%lx", addr);
        }
      return 0;

    case IDM_DSP_SHOW:
      cmd->dsp->show (true);
      return 0;

    case IDM_DSP_ADD:
      if (cmd->dsp->add (hwnd))
        cmd->dsp->show (true);
      return 0;

    case IDM_BRKPT_LINE:
      cmd->brk->add_line (short_filename, sel_lineno);
      return 0;

    default:
      return cmd->wm_command (hwnd, msg, mp1, mp2);
    }
}


bool source_window::wm_user (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
  unsigned change;

  if (parent::wm_user (hwnd, msg, mp1, mp2))
    return true;
  switch (msg)
    {
    case UWM_STATE:
      change = LONGFROMMP (mp1);
      if (change & command_window::GSC_PROMPT)
        menu_enable (IDM_RUNMENU, gdb->is_ready ());
      if (change & command_window::GSC_RUNNING)
        {
          if (gdb->is_nochild ())
            show_line (-1);
          // TODO: IDM_GO
          bool running = !gdb->is_nochild ();
          menu_enable (IDM_STEPOVER, running);
          menu_enable (IDM_STEPINTO, running);
          menu_enable (IDM_ISTEPOVER, running);
          menu_enable (IDM_ISTEPINTO, running);
          // TODO:
          menu_enable (IDM_UNTIL, running);
          menu_enable (IDM_JUMP, running);
          menu_enable (IDM_FINISH, running);
        }
      if (change & command_window::GSC_EXEC_FILE)
        {
          menu_enable (IDM_RESTART, cmd->get_debuggee () != NULL);
          menu_enable (IDM_GO, cmd->get_debuggee () != NULL);
        }
      return true;

    default:
      return false;
    }
}


MRESULT source_window::wm_close (HWND, ULONG, MPARAM, MPARAM)
{
  WinPostMsg (cmd->get_hwndClient (), UWM_CLOSE_SRC, MPFROMP (this), 0);
  return 0;
}


static bool is_symbol (int c)
{
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}


void source_window::button1_lineno (int line, int clicks)
{
  int lineno = line + 1;
  if (clicks == 0)
    {
      if (lineno == sel_lineno)
        select_line (-1);
      else
        select_line (lineno);
    }
  else if (clicks == 2)
    {
      const breakpoint *brk
        = cmd->breakpoints.find (short_filename, lineno);
      // TODO: enabled/disabled
      if (brk != NULL)
        gdb->send_cmd ("server delete %d", brk->get_number ());
      else
        gdb->send_cmd ("server break %s:%d",
                       (const char *)short_filename, lineno);
    }
}


void source_window::button1_source (int line, int column, int clicks)
{
  if (clicks == 2)
    {
      if (exp_lineno != -1)
        {
          put (exp_lineno - 1, exp_column, exp_len,
               exp_lineno == cur_lineno ? hilite_attr : get_default_attr (),
               true);
          exp_lineno = -1;
        }
      if (is_symbol (get_char (line, column)))
        {
          int start = column;
          while (start > 0)
            {
              int c = get_char (line, start - 1);
              if (is_symbol (c))
                --start;
              // TODO: blanks
              else if (c == '.'
                       && is_symbol (get_char (line, start - 2)))
                start -= 2;
              // TODO: blanks
              else if (c == '>'
                       && get_char (line, start - 2) == '-'
                       && is_symbol (get_char (line, start - 3)))
                start -= 3;
              else
                break;
            }
          int end = column + 1;
          for (;;)
            {
              int c = get_char (line, end);
              if (is_symbol (c) || (c >= '0' && c <= '9'))
                ++end;
              else
                break;
            }
          char *buf = (char *)alloca (end - start + 1);
          if (get_string (line, start, buf, end - start))
            {
              exp_lineno = line + 1;
              exp_column = start; exp_len = end - start;
              put (line, start, exp_len, sel_attr, true);
              // TODO: Context (class, function, ...)
              gdb->send_cmd ("server display %s", buf);
              cmd->dsp->show (true);
            }
        }
    }
}


void source_window::button_event (int line, int column, int, int button,
                                  int clicks)
{
  if (line >= 0 && column >= 0 && button == 1)
    {
      // TODO: Context menu (run editor, set/edit breakpoint, ...)
      if (column < LINENO_WIDTH + 2)
        button1_lineno (line, clicks);
      else if (column >= SOURCE_COLUMN - 1)
        button1_source (line, column, clicks);
    }
}


void source_window::set_breakpoint (int lineno, bool set, bool paint)
{
  put (lineno - 1, 0, LINENO_WIDTH + 2,
       set ? bpt_attr : lineno == sel_lineno ? sel_attr : get_default_attr (),
       paint);
}


// TODO: Don't do this in the PM thread

void source_window::find (bool start, bool forward)
{
  capture *capt = gdb->capture_cmd ("server list %s:%d,1",
                                    (const char *)short_filename,
                                    start ? 1 : find_lineno);
  delete_capture (capt);
  capt = gdb->capture_cmd ("server %s %s",
                           forward ? "search" : "reverse-search",
                           (const char *)find_string);
  long n;
  if (capt != NULL && (n = strtol (gdb->get_output (), NULL, 10)) >= 1)
    {
      find_lineno = (int)n;
      select_line ((int)n);
      parent::show_line (n - 1, 1, get_window_lines () / 2);
      menu_enable (IDM_FINDNEXT, true);
      menu_enable (IDM_FINDPREV, !start);
    }
  else
    {
      WinMessageBox (HWND_DESKTOP, HWND_DESKTOP,
                     (PSZ)"Not found.", (PSZ)"pmgdb", 0,
                     MB_MOVEABLE | MB_OK | MB_ICONEXCLAMATION);
      if (forward || start)
        menu_enable (IDM_FINDNEXT, false);
      if (!forward || start)
        menu_enable (IDM_FINDPREV, false);
    }
  delete_capture (capt);
}
