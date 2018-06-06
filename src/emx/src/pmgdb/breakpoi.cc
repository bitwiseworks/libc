/* breakpoi.cc
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
#include "string.h"
#include "pmapp.h"
#include "pmframe.h"
#include "pmtxt.h"
#include "pmtty.h"
#include "pmgdb.h"
#include "help.h"
#include "breakpoi.h"
#include "srcfiles.h"
#include "annotati.h"
#include "capture.h"
#include "gdbio.h"
#include "command.h"

breakpoint::breakpoint ()
{
  number = 0;
  type = BPT_UNKNOWN;
  disposition = BPD_UNKNOWN;
  enable = false;
  address = 0;
  lineno = 0;
  ignore = 0;
}


breakpoint::breakpoint (const breakpoint &src)
{
  copy (src);
}


breakpoint &breakpoint::operator = (const breakpoint &src)
{
  copy (src);
  return *this;
}


// Note: Does not copy next:

void breakpoint::copy (const breakpoint &src)
{
  number = src.number;
  type = src.type;
  disposition = src.disposition;
  enable = src.enable;
  address = src.address;
  source = src.source;
  lineno = src.lineno;
  condition = src.condition;
  ignore = src.ignore;
}


breakpoint::~breakpoint ()
{
}


bool breakpoint::operator == (const breakpoint b) const
{
  return (number == b.number
          && type == b.type
          && disposition == b.disposition
          && enable == b.enable
          && address == b.address
          && lineno == b.lineno
          && source == b.source
          && condition == b.condition
          && ignore == b.ignore);
}


void breakpoint::set_disposition (const char *s)
{
  if (s[0] == 'd' && s[1] == 'e') // "del"
    disposition = BPD_DEL;
  else if (s[0] == 'd' && s[1] == 'i') // "dis"
    disposition = BPD_DIS;
  else if (s[0] == 'k')
    disposition = BPD_KEEP;     // "keep"
  else
    disposition = BPD_UNKNOWN;
}


const char *breakpoint::get_disposition_as_string () const
{
  switch (disposition)
    {
    case BPD_DEL:
      return "del";
    case BPD_DIS:
      return "dis";
    case BPD_KEEP:
      return "keep";
    default:
      return "?";
    }
}


bool breakpoint::set_from_capture (const capture &capt)
{
  if (!(capt.bpt_number.is_set ()
        && capt.bpt_enable.is_set ()
        && capt.bpt_address.is_set ()
        && capt.bpt_disposition.is_set ()
        && capt.bpt_what.is_set ()))
    return false;

  set_number (capt.bpt_number.get ());
  set_address (capt.bpt_address.get ());
  set_enable (capt.bpt_enable.get ());
  set_disposition (capt.bpt_disposition.get ());
  if (capt.bpt_condition.is_set ())
    set_condition (capt.bpt_condition.get ());
  if (capt.bpt_ignore.is_set ())
    set_ignore (capt.bpt_ignore.get ());
  else
    set_ignore (0);

  const char *p = capt.bpt_what.get ();
  // "in main at source.c:217 "
  if (strncmp (p, "in ", 3) == 0)
    {
      p += 3;
      char *end = strstr (p, " at ");
      if (end == NULL)
        p = strchr (p, ' ');
      if (end == NULL)
        p = end;
      else
        p = end + 1;
    }
  if (strncmp (p, "at ", 3) == 0)
    {
      p += 3;
      const char *colon = p;
      while (*colon != 0 && *colon != ':' && *colon != ' ' && *colon != '\n')
        ++colon;
      if (*colon == ':')
        {
          char *end;
          long lineno = strtol (colon + 1, &end, 10);
          if (end != colon + 1
              && (*end == ' ' || *end == '\n' || *end == 0))
            {
              set_lineno ((int)lineno);
              set_source (p, colon - p);
            }
        }
    }
  return true;
}


static const struct
{
  const char *text;
  breakpoint::bpd disp;
} disposition_table[3] =
  {
    {"keep", breakpoint::BPD_KEEP},
    {"dis", breakpoint::BPD_DIS},
    {"del", breakpoint::BPD_DEL}
  };

MRESULT breakpoint::breakpoint_msg (HWND hwnd, ULONG msg,
                                    MPARAM mp1, MPARAM mp2)
{
  MRESULT mr;
  HWND hwndTemp;
  static char buf[270];         // TODO: query length, see display_window
  const pm_create *pcreate;
  const source_files_window *srcs;
  int i, sel_idx;

  switch (msg)
    {
    case WM_INITDLG:
      pcreate = (const pm_create *)PVOIDFROMMP (mp2);
      srcs = (const source_files_window *)pcreate->ptr2;
      dlg_sys_menu (hwnd);
      WinSetWindowPtr (hwnd, QWL_USER, this);
      WinSendDlgItemMsg (hwnd, IDC_ENABLE, BM_SETCHECK,
                         MPFROMSHORT (enable), NULL);
      hwndTemp = WinWindowFromID (hwnd, IDC_SOURCE);
      i = 0;
      for (source_files_window::iterator s(*srcs); s.ok (); s.next ())
        {
            WinSendMsg (hwndTemp, LM_INSERTITEM, MPFROMSHORT (i),
                        MPFROMP ((const char *)s));
            if (!source.is_null () && strcmp ((const char *)s,
                                              (const char *)source) == 0)
              WinSendMsg (hwndTemp, LM_SELECTITEM,
                          MPFROMSHORT (i), MPFROMSHORT (TRUE));
            ++i;
        }

      if (lineno >= 0)
        {
          _itoa (lineno, buf, 10);
          WinSetDlgItemText (hwnd, IDC_LINENO, (PSZ)buf);
        }
      if (ignore != 0)
        {
          _itoa (ignore, buf, 10);
          WinSetDlgItemText (hwnd, IDC_IGNORE, (PSZ)buf);
        }
      if (!condition.is_null ())
        WinSetDlgItemText (hwnd, IDC_CONDITION, condition);

      hwndTemp = WinWindowFromID (hwnd, IDC_DISPOSITION);
      sel_idx = -1;             // Keep the compiler happy
      for (i = 0; i < 3; ++i)
        {
          WinSendMsg (hwndTemp, LM_INSERTITEM, MPFROMSHORT (i),
                      MPFROMP (disposition_table[i].text));
          if (disposition_table[i].disp == disposition)
            sel_idx = i;
        }
      WinSendMsg (hwndTemp, LM_SELECTITEM,
                  MPFROMSHORT (sel_idx), MPFROMSHORT (TRUE));
      break;

    case WM_COMMAND:
      switch (SHORT1FROMMP (mp1))
        {
        case DID_OK:
          mr = WinSendDlgItemMsg (hwnd, IDC_ENABLE, BM_QUERYCHECK, NULL, NULL);
          enable = (bool)SHORT1FROMMR (mr);
          buf[0] = 0;
          WinQueryDlgItemText (hwnd, IDC_SOURCE, sizeof (buf), (PSZ)buf);
          set_source (buf, strlen (buf));
          buf[0] = 0;
          WinQueryDlgItemText (hwnd, IDC_LINENO, sizeof (buf), (PSZ)buf);
          lineno = atoi (buf);
          buf[0] = 0;
          WinQueryDlgItemText (hwnd, IDC_IGNORE, sizeof (buf), (PSZ)buf);
          ignore = atoi (buf);
          buf[0] = 0;
          WinQueryDlgItemText (hwnd, IDC_CONDITION, sizeof (buf), (PSZ)buf);
          if (buf[0] == 0)      // TODO: Ignore white space
            condition.set_null ();
          else
            set_condition (buf);
          buf[0] = 0;
          WinQueryDlgItemText (hwnd, IDC_DISPOSITION, sizeof (buf), (PSZ)buf);
          for (i = 0; i < 3; ++i)
            if (strcmp (disposition_table[i].text, buf) == 0)
              disposition = disposition_table[i].disp;
          WinDismissDlg (hwnd, DID_OK);
          return 0;
        }
      break;
    }
  return WinDefDlgProc (hwnd, msg, mp1, mp2);
}


MRESULT EXPENTRY dlg_breakpoint (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
  breakpoint *p;

  if (msg == WM_INITDLG)
    p = (breakpoint *)((struct pm_create *)PVOIDFROMMP (mp2))->ptr;
  else
    p = (breakpoint *)WinQueryWindowPtr (hwnd, QWL_USER);
  return p->breakpoint_msg (hwnd, msg, mp1, mp2);
}


breakpoint_list::breakpoint_list ()
{
  list = NULL; end = &list;
}


breakpoint_list::~breakpoint_list ()
{
  delete_all ();
}


void breakpoint_list::delete_all ()
{
  brkpt_node *next;
  for (brkpt_node *p = list; p != NULL; p = next)
    {
      next = p->next;
      delete p;
    }
  list = NULL; end = &list;
}


void breakpoint_list::add (const breakpoint &bp)
{
  brkpt_node *node = new brkpt_node (bp);
  *end = node; end = &node->next;
}


const breakpoint *breakpoint_list::find (ULONG addr) const
{
  for (const brkpt_node *p = list; p != NULL; p = p->next)
    if (p->address == addr)
      return p;
  return NULL;
}


const breakpoint *breakpoint_list::find (const char *fname, int lineno) const
{
  for (const brkpt_node *p = list; p != NULL; p = p->next)
    if (!p->source.is_null () && p->lineno == lineno
        && strcasecmp ((const unsigned char *)p->source,
                       (const unsigned char *)fname) == 0)
      return p;
  return NULL;
}


const breakpoint *breakpoint_list::get (int n) const
{
  for (const brkpt_node *p = list; p != NULL; p = p->next)
    if (n == 0)
      return p;
    else
      --n;
  return NULL;
}


void breakpoint_list::steal (breakpoint_list &from)
{
  list = from.list; end = from.end;
  from.list = NULL; from.end = &from.list;
}


void breakpoint_list::update (const breakpoint_list &old, command_window *cmd,
                              update_fun fun)
{
  const brkpt_node *o = old.list;
  const brkpt_node *n = list;
  int cmp, index = 0;
  while (o != NULL || n != NULL)
    {
      if (o == NULL)
        cmp = 1;
      else if (n == NULL)
        cmp = -1;
      else if (o->get_number () > n->get_number ())
        cmp = 1;
      else if (o->get_number () < n->get_number ())
        cmp = -1;
      else
        cmp = 0;

      // TODO: enabled/disabled

      if (cmp < 0)
        (cmd->*fun) (index, o, NULL);
      else if (cmp > 0)
        (cmd->*fun) (index++, NULL, n);
      else
        (cmd->*fun) (index++, o, n);

      if (cmp >= 0)
        n = n->next;
      if (cmp <= 0)
        o = o->next;
    }
}


bool breakpoint_list::any_enabled () const
{
  for (const brkpt_node *p = list; p != NULL; p = p->next)
    if (p->enable)
      return true;
  return false;
}


#define BRK_HEADER_LINES        1

breakpoints_window::breakpoints_window (command_window *in_cmd, gdbio *in_gdb,
                                        unsigned in_id,
                                        const char *fontnamesize)
  : pmtxt (in_cmd->get_app (), in_id, FCF_SHELLPOSITION, NULL, fontnamesize)
{
  cmd = in_cmd; gdb = in_gdb;
  sel_line = -1; exp_number = -1;

  sel_attr = get_default_attr ();
  set_fg_color (sel_attr, CLR_BLACK);
  set_bg_color (sel_attr, CLR_PALEGRAY);

  set_title ("pmgdb - Breakpoints");
  set_keys_help_id (HELP_BRK_KEYS);

  add_all ();
}


breakpoints_window::~breakpoints_window ()
{
}


void breakpoints_window::add_all ()
{
  const breakpoint_list::brkpt_node *p;
  int line = BRK_HEADER_LINES;

  select_line (-1);
  delete_all ();

  int x = 0;
  put (0, x, 4, " No ", true); x += 4;
  put_tab (0, x++, true);
  put_vrule (0, x++, true);
  put (0, x, 5, " Ena ", true); x += 5;
  put_tab (0, x++, true);
  put_vrule (0, x++, true);
  put (0, x, 9, " Address ", true); x += 9;
  put_tab (0, x++, true);
  put_vrule (0, x++, true);
  put (0, x, 8, " Source ", true); x += 8;
  put_tab (0, x++, true);
  put_vrule (0, x++, true);
  put (0, x, 6, " Line ", true); x += 6;
  put_tab (0, x++, true);
  put_vrule (0, x++, true);
  put (0, x, 6, " Disp ", true); x += 6;
  put_tab (0, x++, true);
  put_vrule (0, x++, true);
  put (0, x, 6, " Ign ", true); x += 5;
  put_tab (0, x++, true);
  put_vrule (0, x++, true);
  put (0, x, 5, " Cond", true); x += 5;
  underline (0, true, true);

  // TODO: iterator
  for (p = cmd->get_breakpoint_list (); p != NULL; p = p->next)
    put_bpt (line++, *p);
  // TODO: Add do_paint argument to pmtxt::delete_all()
  sync ();
}


void breakpoints_window::update (int index, const breakpoint *old_bpt,
                                 const breakpoint *new_bpt)
{
  int line = index + BRK_HEADER_LINES;
  if (new_bpt == NULL)
    {
      delete_lines (line, 1, true);
      if (sel_line != -1)
        {
          if (line == sel_line)
            {
              sel_line = -1;
              menu_enable (IDM_EDITMENU, false);
            }
          else if (line < sel_line)
            --sel_line;
        }
    }
  else if (old_bpt == NULL)
    {
      insert_lines (line, 1, true);
      put_bpt (line, *new_bpt);
      if (sel_line != -1)
        {
          if (line <= sel_line)
            ++sel_line;
        }
      if (exp_number == new_bpt->get_number ())
        {
          exp_number = -1;
          select_line (line);
        }
    }
  else if (*old_bpt != *new_bpt)
    {
      clear_lines (line, 1, true);
      put_bpt (line, *new_bpt);
      if (sel_line != -1)
        {
          if (line == sel_line)
            {
              sel_line = -1;
              select_line (line);
              menu_enable (IDM_ENABLE, !new_bpt->get_enable ());
              menu_enable (IDM_DISABLE, new_bpt->get_enable ());
            }
        }
    }
}


MRESULT breakpoints_window::wm_activate (HWND hwnd, ULONG msg,
                                         MPARAM mp1, MPARAM mp2)
{
  if (SHORT1FROMMP (mp1))
    cmd->associate_help (get_hwndFrame ());
  return WinDefWindowProc (hwnd, msg, mp1, mp2);
}


MRESULT breakpoints_window::wm_close (HWND, ULONG, MPARAM, MPARAM)
{
  show (false);
  return 0;
}


MRESULT breakpoints_window::wm_command (HWND hwnd, ULONG msg,
                                        MPARAM mp1, MPARAM mp2)
{
  const breakpoint *bpt;

  switch (SHORT1FROMMP (mp1))
    {
    case IDM_DELETE:
      bpt = cmd->get_breakpoint (sel_line - BRK_HEADER_LINES);
      if (bpt == NULL)
        WinAlarm (HWND_DESKTOP, WA_ERROR);
      else
        gdb->send_cmd ("server delete %d", bpt->get_number ());
      return 0;

    case IDM_ENABLE:
      bpt = cmd->get_breakpoint (sel_line - BRK_HEADER_LINES);
      if (bpt == NULL)
        WinAlarm (HWND_DESKTOP, WA_ERROR);
      else
        send_disposition (bpt->number, bpt->disposition, true);
      return 0;

    case IDM_DISABLE:
      bpt = cmd->get_breakpoint (sel_line - BRK_HEADER_LINES);
      if (bpt == NULL)
        WinAlarm (HWND_DESKTOP, WA_ERROR);
      else
        send_disposition (bpt->number, bpt->disposition, false);
      return 0;

    case IDM_BRKPT_LINE:
      add_line ();
      return 0;

    case IDM_REFRESH:
      cmd->get_breakpoints ();
      return 0;

    case IDM_MODIFY:
      bpt = cmd->get_breakpoint (sel_line - BRK_HEADER_LINES);
      if (bpt == NULL)
        WinAlarm (HWND_DESKTOP, WA_ERROR);
      else
        {
          breakpoint b = *bpt;
          if (dialog (&b))
            {
              // TODO: bpt might have gone away in the meantime, UWM_STATE!
              // TODO: check values
              // TODO: Check type
              if (b.source != bpt->source || b.lineno != bpt->lineno)
                {
                  capture *capt;
                  capt = gdb->capture_cmd ("server break %s:%d",
                                           b.get_source (), b.lineno);
                  if (capt != NULL)
                    {
                      if (capt->bpt_number.is_set ())
                        {
                          int number = capt->bpt_number.get ();
                          if (gdb->send_cmd ("server delete %d", bpt->number))
                            {
                              send_disposition (number, b.disposition,
                                                b.enable);
                              if (!b.condition.is_null ())
                                gdb->queue_cmd ("server condition %d %s",
                                                number, b.get_condition ());
                              if (b.ignore != 0)
                                gdb->queue_cmd ("server ignore %d %d",
                                                number, b.ignore);
                              exp_number = number;
                            }
                        }
                      else if (capt->error.is_set ())
                        cmd->capture_error (capt);
                      delete_capture (capt);
                    }
                }
              else
                {
                  if (b.disposition != bpt->disposition
                      || b.enable != bpt->enable)
                    send_disposition (b.number, b.disposition, b.enable);
                  if (b.condition != bpt->condition)
                    gdb->queue_cmd ("server condition %d %s", b.number,
                                    (b.condition.is_null ()
                                     ? "" : b.get_condition ()));
                  if (b.ignore != bpt->ignore)
                    gdb->queue_cmd ("server ignore %d %d", b.number, b.ignore);
                }
            }
        }
      return 0;

    default:
      return cmd->wm_command (hwnd, msg, mp1, mp2);
    }
}


void breakpoints_window::put_bpt (int line, const breakpoint &bpt)
{
  char buf[400];
  int x = 0, len;

  len = snprintf (buf, sizeof (buf), " %d ", bpt.get_number ());
  put (line, x, len, buf, true); x += len;
  put_tab (line, x++, true);
  put_vrule (line, x++, true);

  len = snprintf (buf, sizeof (buf), " %c ", bpt.get_enable () ? 'y' : 'n');
  put (line, x, len, buf, true); x += len;
  put_tab (line, x++, true);
  put_vrule (line, x++, true);

  len = snprintf (buf, sizeof (buf), " 0x%.8lx ", bpt.get_address ());
  put (line, x, len, buf, true); x += len;
  put_tab (line, x++, true);
  put_vrule (line, x++, true);

  len = snprintf (buf, sizeof (buf), " %s ",
                  bpt.get_source () != NULL ? bpt.get_source () : "-");
  put (line, x, len, buf, true); x += len;
  put_tab (line, x++, true);
  put_vrule (line, x++, true);

  len = snprintf (buf, sizeof (buf), " %d ", bpt.get_lineno ());
  put (line, x, len, buf, true); x += len;
  put_tab (line, x++, true);
  put_vrule (line, x++, true);

  len = snprintf (buf, sizeof (buf), " %s ",
                  bpt.get_disposition_as_string ());
  put (line, x, len, buf, true); x += len;
  put_tab (line, x++, true);
  put_vrule (line, x++, true);

  if (bpt.get_ignore () == 0)
    len = snprintf (buf, sizeof (buf), " - ");
  else
    len = snprintf (buf, sizeof (buf), " %d ", bpt.get_ignore ());
  put (line, x, len, buf, true); x += len;
  put_tab (line, x++, true);
  put_vrule (line, x++, true);

  len = snprintf (buf, sizeof (buf), " %s",
                  bpt.get_condition () != NULL ? bpt.get_condition () : "-");
  put (line, x, len, buf, true); x += len;
}


void breakpoints_window::select_line (int line, bool toggle)
{
  const breakpoint *bpt;
  if (toggle && line != -1 && line == sel_line)
    line = -1;
  if (line == -1)
    bpt = NULL;
  else
    {
      bpt = cmd->get_breakpoint (line - BRK_HEADER_LINES);
      if (bpt == NULL) line = -1;
    }
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
    {
      menu_enable (IDM_ENABLE, !bpt->get_enable ());
      menu_enable (IDM_DISABLE, bpt->get_enable ());
      show_line (line, 1, 1);
    }
  menu_enable (IDM_EDITMENU, line != -1);
}


void breakpoints_window::button_event (int line, int column, int tab,
                                       int button, int clicks)
{
  if (line >= 0 && column >= 0)
    {
      if (clicks == 1 && button == 1)
        {
          // TODO: Context menu
          select_line (line, true);
        }
      else if (clicks == 2 && button == 1 && tab == 1)
        {
          const breakpoint *p = cmd->get_breakpoint (line - BRK_HEADER_LINES);
          if (p == NULL)
            WinAlarm (HWND_DESKTOP, WA_ERROR);
          else
            {
              gdb->send_cmd ("server %s %d",
                             p->get_enable () ? "disable" : "enable",
                             p->get_number ());
              select_line (line);
            }
        }
      else if (clicks == 2 && button == 1 && tab >= 3 && tab <= 4)
        {
          show_source (line);
          select_line (line);
        }
      else if (clicks == 2)
        {
          // TODO: Double clicking on any other field should change that field
          select_line (line);
        }
    }
  else
    select_line (-1);
}


void breakpoints_window::show_source (int line)
{
  const breakpoint *bpt = cmd->get_breakpoint (line - BRK_HEADER_LINES);
  if (bpt != NULL && bpt->get_source () != NULL)
    cmd->show_source (bpt->get_source (), NULL, true, bpt->get_lineno());
}


bool breakpoints_window::dialog (breakpoint *bpt)
{
  pm_create create;

  create.cb = sizeof (create);
  create.ptr = (void *)bpt;
  create.ptr2 = (void *)cmd->get_srcs ();
  // TODO: Breakpoint type
  return WinDlgBox (HWND_DESKTOP, get_hwndClient (), dlg_breakpoint, 0,
                    IDD_BRKPT_LINE, &create) == DID_OK;
}


void breakpoints_window::add_line (const char *default_source, int line_no)
{
  breakpoint temp;
  // TODO: Breakpoint type
  temp.set_enable (true);
  if (default_source != NULL)
    temp.set_source (default_source, strlen (default_source));
  temp.set_lineno (line_no);
  if (dialog (&temp))
    {
      select_line (-1);
      capture *capt;
      // TODO: Breakpoint type
      // TODO: Check values
      capt = gdb->capture_cmd ("server break %s:%d",
                               temp.get_source (), temp.get_lineno ());
      if (capt != NULL)
        {
          if (capt->bpt_number.is_set ())
            {
              int number = capt->bpt_number.get ();
              send_disposition (number, temp.disposition, temp.enable);
              if (!temp.condition.is_null ())
                gdb->queue_cmd ("server condition %d %s", number,
                                temp.get_condition ());
              if (temp.ignore != 0)
                gdb->queue_cmd ("server ignore %d %d", number, temp.ignore);
              exp_number = number;
            }
          else if (capt->error.is_set ())
            cmd->capture_error (capt);
        }
      delete_capture (capt);
    }
}


void
breakpoints_window::send_disposition (int number, breakpoint::bpd disposition,
                                      bool enable)
{
  switch (disposition)
    {
    case breakpoint::BPD_KEEP:
      gdb->queue_cmd ("server enable %d", number);
      break;
    case breakpoint::BPD_DIS:
      gdb->queue_cmd ("server enable once %d", number);
      break;
    case breakpoint::BPD_DEL:
      gdb->queue_cmd ("server enable delete %d", number);
      break;
    default:
      break;
    }
  if (!enable)
    gdb->queue_cmd ("server disable %d", number);
}
