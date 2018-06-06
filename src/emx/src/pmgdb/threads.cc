/* threads.cc
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


#define INCL_WIN
#include <os2.h>
#include <stdlib.h>
#include <stdio.h>              // sprintf()
#include <stdarg.h>
#include <string.h>
#include <sys/ptrace.h>
#include "string.h"
#include "pmapp.h"
#include "pmframe.h"
#include "pmtxt.h"
#include "pmtty.h"
#include "pmgdb.h"
#include "help.h"
#include "breakpoi.h"
#include "threads.h"
#include "annotati.h"
#include "gdbio.h"
#include "command.h"


class threads_window::thread
{
public:
  thread () {}
  ~thread () {}

  thread *next;
  int number;
  int pid;
  int line;
  bool enable;
};

#define THR_HEADER_LINES        1

threads_window::threads_window (command_window *in_cmd, gdbio *in_gdb,
                                unsigned in_id, const SWP *pswp,
                                const char *fontnamesize)
  : pmtxt (in_cmd->get_app (), in_id,
           pswp == NULL ? FCF_SHELLPOSITION : 0, pswp, fontnamesize)
{
  cmd = in_cmd; gdb = in_gdb;
  head = NULL;
  cur_line = -1; sel_line = -1;

  sel_attr = get_default_attr ();
  set_fg_color (sel_attr, CLR_BLACK);
  set_bg_color (sel_attr, CLR_PALEGRAY);

  hilite_attr = get_default_attr ();
  set_fg_color (hilite_attr, CLR_WHITE);
  set_bg_color (hilite_attr, CLR_BLACK);

  sel_hilite_attr = get_default_attr ();
  set_fg_color (sel_hilite_attr, CLR_WHITE);
  set_bg_color (sel_hilite_attr, CLR_DARKGRAY);

  menu_enable (IDM_EDITMENU, false);

  int x = 0;
  put (0, x, 6, " TID ", true); x += 6;
  put_tab (0, x++, true);
  put_vrule (0, x++, true);
  put (0, x, 5, " Ena ", true); x += 5;
  underline (0, true, true);

  set_title ("pmgdb - Threads");
  set_keys_help_id (HELP_THR_KEYS);
}


threads_window::~threads_window ()
{
  thread *p, *next;
  for (p = head; p != NULL; p = next)
    {
      next = p->next;
      delete p;
    }
}


MRESULT threads_window::wm_activate (HWND hwnd, ULONG msg,
                                     MPARAM mp1, MPARAM mp2)
{
  if (SHORT1FROMMP (mp1))
    cmd->associate_help (get_hwndFrame ());
  return WinDefWindowProc (hwnd, msg, mp1, mp2);
}


MRESULT threads_window::wm_close (HWND, ULONG, MPARAM, MPARAM)
{
  show (false);
  return 0;
}


MRESULT threads_window::wm_command (HWND hwnd, ULONG msg,
                                    MPARAM mp1, MPARAM mp2)
{
  const thread *p;

  switch (SHORT1FROMMP (mp1))
    {
    case IDM_ENABLE:
      p = find_by_line (sel_line);
      if (p == NULL)
        WinAlarm (HWND_DESKTOP, WA_ERROR);
      else
        gdb->send_cmd ("server thread enable %d", p->number);
      break;

    case IDM_DISABLE:
      p = find_by_line (sel_line);
      if (p == NULL)
        WinAlarm (HWND_DESKTOP, WA_ERROR);
      else
        gdb->send_cmd ("server thread disable %d", p->number);
      break;

    case IDM_SWITCH:
      p = find_by_line (sel_line);
      if (p == NULL)
        WinAlarm (HWND_DESKTOP, WA_ERROR);
      else
        gdb->send_cmd ("server thread %d", p->number);
      break;

    default:
      return cmd->wm_command (hwnd, msg, mp1, mp2);
    }
  return WinDefWindowProc (hwnd, msg, mp1, mp2);
}


bool threads_window::wm_user (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
  if (parent::wm_user (hwnd, msg, mp1, mp2))
    return true;
  switch (msg)
    {
    case UWM_MENU:
      if (sel_line == -1)
        menu_enable (IDM_EDITMENU, false);
      else
        {
          const thread *p = find_by_line (sel_line);
          menu_enable (IDM_ENABLE, !p->enable);
          menu_enable (IDM_DISABLE, p->enable);
        }
      return true;

    default:
      return false;
    }
}


void threads_window::button_event (int line, int column, int tab, int button,
                                   int clicks)
{
  if (line >= 0 && column >= 0)
    {
      // TODO: Context menu
      if (button == 1 && clicks == 1)
        select_line (line, true);
      else if (button == 1 && clicks == 2 && tab == 1)
        {
          const thread *p = find_by_line (line);
          if (p == NULL)
            WinAlarm (HWND_DESKTOP, WA_ERROR);
          else
            gdb->send_cmd ("server thread %s %d",
                           p->enable ? "disable" : "enable", p->number);
        }
      else if (button == 1 && clicks == 2)
        {
          const thread *p = find_by_line (line);
          if (p == NULL)
            WinAlarm (HWND_DESKTOP, WA_ERROR);
          else
            {
              gdb->call_cmd ("server thread %d", p->number);
              cmd->where ();
            }
        }
    }
  else
    select_line (-1);
}


threads_window::thread *threads_window::find_by_number (int number)
{
  for (thread *p = head; p != NULL; p = p->next)
    if (p->number == number)
      return p;
  return NULL;
}


threads_window::thread *threads_window::find_by_line (int line)
{
  for (thread *p = head; p != NULL; p = p->next)
    if (p->line == line)
      return p;
  return NULL;
}


void threads_window::add (int number, int pid)
{
  thread *p = find_by_number (number);
  if (p == NULL)
    {
      int line = THR_HEADER_LINES;
      thread **patch = &head;
      while (*patch != NULL && (*patch)->pid < pid)
        {
          patch = &(*patch)->next;
          ++line;
        }
      p = new thread;
      p->next = *patch;
      *patch = p;
      p->number = number;
      p->pid = pid;
      p->line = line;
      p->enable = true;
      for (thread *q = p->next; q != NULL; q = q->next)
        ++q->line;
      insert_lines (p->line, 1, true);
      if (cur_line != -1 && p->line <= cur_line)
        ++cur_line;
      if (sel_line != -1 && p->line <= sel_line)
        ++sel_line;
      update (p);
    }
}


void threads_window::enable (int number)
{
  thread *p = find_by_number (number);
  if (p != NULL && !p->enable)
    {
      p->enable = true;
      if (p->line == sel_line)
        WinPostMsg (get_hwndClient (), UWM_MENU, 0, 0);
      update (p);
    }
}


void threads_window::disable (int number)
{
  thread *p = find_by_number (number);
  if (p != NULL && p->enable)
    {
      p->enable = false;
      if (p->line == sel_line)
        WinPostMsg (get_hwndClient (), UWM_MENU, 0, 0);
      update (p);
    }
}


void threads_window::remove (int pid)
{
  for (thread **patch = &head; *patch != NULL; patch = &(*patch)->next)
    if ((*patch)->pid == pid)
      {
        for (thread *p = (*patch)->next; p != NULL; p = p->next)
          --p->line;
        thread *p = *patch;
        delete_lines (p->line, 1, true);
        *patch = p->next;
        if (cur_line != -1 && p->line == cur_line)
          cur_line = -1;
        if (sel_line != -1 && p->line == sel_line)
          {
            sel_line = -1;
            WinPostMsg (get_hwndClient (), UWM_MENU, 0, 0);
          }
        delete p;
        return;
      }
}


void threads_window::select (int number)
{
  thread *p = find_by_number (number);
  if (p != NULL)
    current_line (p->line);
}


void threads_window::update (const thread *p)
{
  char buf[400];
  int x = 0, len;
  pmtxt_attr attr;

  if (p->line == cur_line && p->line == sel_line)
    attr = sel_hilite_attr;
  else if (p->line == cur_line)
    attr = hilite_attr;
  else if (p->line == sel_line)
    attr = sel_attr;
  else
    attr = get_default_attr ();

  clear_lines (p->line, 1, true);
  len = snprintf (buf, sizeof (buf), " %d ", PTRACE_GETTID (p->pid));
  put (p->line, x, len, buf, attr, true); x += len;
  put_tab (p->line, x++, attr, true);
  put_vrule (p->line, x++, attr, true);

  len = snprintf (buf, sizeof (buf), " %c ", p->enable ? 'y' : 'n');
  put (p->line, x, len, buf, attr, true); x += len;
}


void threads_window::current_line (int line)
{
  if (line != cur_line)
    {
      if (cur_line != -1)
        put (cur_line, 0, max_line_len,
             cur_line == sel_line ? sel_attr : get_default_attr (), true);
      if (line != -1)
        put (line, 0, max_line_len,
             line == sel_line ? sel_hilite_attr : hilite_attr, true);
      cur_line = line;
    }
}


void threads_window::select_line (int line, bool toggle)
{
  const thread *p;
  if (toggle && line != -1 && line == sel_line)
    line = -1;
  if (line == -1)
    p = NULL;
  else
    {
      p = find_by_line (line);
      if (p == NULL) line = -1;
    }
  if (line != sel_line)
    {
      if (sel_line != -1)
        put (sel_line, 0, max_line_len,
             sel_line == cur_line ? hilite_attr : get_default_attr (), true);
      if (line != -1)
        put (line, 0, max_line_len,
             line == cur_line ? sel_hilite_attr : sel_attr, true);
      sel_line = line;
    }
  if (line != -1)
    {
      menu_enable (IDM_ENABLE, !p->enable);
      menu_enable (IDM_DISABLE, p->enable);
      show_line (line, 0, 0);
    }
  menu_enable (IDM_EDITMENU, line != -1);
}
