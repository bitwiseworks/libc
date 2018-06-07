/* srcfiles.cc
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
#include "pmtxt.h"
#include "pmtty.h"
#include "pmgdb.h"
#include "help.h"
#include "breakpoi.h"
#include "srcfiles.h"
#include "command.h"


source_files_window::source_files_window (command_window *in_cmd,
                                          unsigned in_id, const SWP *pswp,
                                          const char *fontnamesize)
  : pmtxt (in_cmd->get_app (), in_id,
           pswp == NULL ? FCF_SHELLPOSITION : 0, pswp, fontnamesize)
{
  cmd = in_cmd;
  head = NULL;
  lines = 0; sel_line = -1;
  set_title ("pmgdb - Source Files");
  set_keys_help_id (HELP_SRCS_KEYS);

  sel_attr = get_default_attr ();
  set_fg_color (sel_attr, CLR_BLACK);
  set_bg_color (sel_attr, CLR_PALEGRAY);
}


source_files_window::~source_files_window ()
{
  delete_all ();
}


MRESULT source_files_window::wm_close (HWND, ULONG, MPARAM, MPARAM)
{
  show (false);
  return 0;
}


MRESULT source_files_window::wm_command (HWND hwnd, ULONG msg,
                                         MPARAM mp1, MPARAM mp2)
{
  return cmd->wm_command (hwnd, msg, mp1, mp2);
}


MRESULT source_files_window::wm_activate (HWND hwnd, ULONG msg,
                                          MPARAM mp1, MPARAM mp2)
{
  if (SHORT1FROMMP (mp1))
    cmd->associate_help (get_hwndFrame ());
  return WinDefWindowProc (hwnd, msg, mp1, mp2);
}


void source_files_window::button_event (int line, int column, int, int button,
                                        int clicks)
{
  if (line >= 0 && column >= 0)
    {
      // TODO: Context menu
      if (button == 1 && clicks == 1)
        select_line (line, true);
      else if (button == 1 && clicks == 2)
        {
          const srcs_node *p = find_by_line (line);
          if (p == NULL)
            WinAlarm (HWND_DESKTOP, WA_ERROR);
          else
            cmd->show_source (p->fname, NULL, true);
          select_line (line);
        }
      else
        select_line (-1);
    }
  else
    select_line (-1);
}


source_files_window::srcs_node *source_files_window::find_by_line (int line)
{
  for (srcs_node *p = head; p != NULL; p = p->next)
    if (line == 0)
      return p;
    else
      --line;
  return NULL;
}


void source_files_window::add (const char *fname)
{
  srcs_node *p = new srcs_node;
  p->next = head;
  p->fname = fname;
  head = p;
  ++lines;
}


static int compare (const void *p1, const void *p2)
{
  const source_files_window::srcs_node *n1
    = *((source_files_window::srcs_node **)p1);
  const source_files_window::srcs_node *n2
    = *((source_files_window::srcs_node **)p2);
  return strcasecmp ((const unsigned char *)_getname (n1->fname),
                     (const unsigned char *)_getname (n2->fname));
}


void source_files_window::done ()
{
  int i;
  srcs_node *p;

  if (lines > 1)
    {
      srcs_node **vector = new srcs_node *[lines];
      i = 0;
      for (p = head; p != NULL; p = p->next)
        vector[i++] = p;
      qsort (vector, lines, sizeof (*vector), compare);
      srcs_node **patch = &head;
      for (i = 0; i < lines; ++i)
        {
          *patch = vector[i];
          patch = &(*patch)->next;
        }
      *patch = NULL;
      delete[] vector;
    }
  for (i = 0, p = head; p != NULL; p = p->next, ++i)
    {
      put (i, 0, 1, " ", true);
      put (i, 1, strlen (p->fname), p->fname, true);
    }
}


void source_files_window::delete_all ()
{
  parent::delete_all ();
  srcs_node *p, *next;
  for (p = head; p != NULL; p = next)
    {
      next = p->next;
      delete p;
    }
  head = NULL;
  lines = 0; sel_line = -1;
}


void source_files_window::select (const char *fname)
{
  int line = 0;
  for (srcs_node *p = head; p != NULL; p = p->next, ++line)
    if (strcmp (p->fname, fname) == 0)
      {
        select_line (line);
        return;
      }
  select_line (-1);
}


void source_files_window::select_line (int line, bool toggle)
{
  if (toggle && line != -1 && line == sel_line)
    line = -1;
  if (line != sel_line)
    {
      if (sel_line != -1)
        {
          put (sel_line, 0, max_line_len, get_default_attr (), true);
          set_eol_attr (sel_line, get_default_attr (), true);
        }
      if (line != -1)
        {
          put (line, 0, max_line_len, sel_attr, true);
          set_eol_attr (line, sel_attr, true);
        }
      sel_line = line;
    }
  if (line != -1)
    show_line (line, 0, 0);
}
