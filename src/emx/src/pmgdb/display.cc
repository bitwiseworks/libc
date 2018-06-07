/* display.cc
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
#include <ctype.h>
#include "string.h"
#include "pmapp.h"
#include "pmframe.h"
#include "pmtxt.h"
#include "pmtty.h"
#include "pmgdb.h"
#include "help.h"
#include "breakpoi.h"
#include "display.h"
#include "annotati.h"
#include "capture.h"
#include "gdbio.h"
#include "command.h"


class dispfmt
{
public:
  dispfmt () { count = 1; format = 0; size = 0; }
  ~dispfmt () {}
  void parse (const char *str);
  void print (char *str) const;

  int count;
  char format;
  char size;
};


class display_window::display
{
public:
  display () { enable = true; type = not_set; }
  display (const display &src);
  ~display () {}
  bool is_line (int y) { return line <= y && y < line + line_count; }
  void set_type (gdbio *gdb);
  display &operator = (const display &src);

  // Dialog box
  MRESULT display_msg (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);

  vstring expr;
  vstring format;
  vstring value;
  display *next;
  int number;
  int line;
  int line_count;
  enum
  {
    not_set,
    pointer,
    other,
  } type;
  bool enable;
};


void dispfmt::parse (const char *s)
{
  count = 1; size = 0; format = 0;
  s = strchr (s, '/');
  s = s != NULL ? s + 1 : "";
  if (*s >= '0' && *s <= '9')
    {
      count = atoi (s);
      while (*s >= '0' && *s <= '9')
        ++s;
    }
  while (*s != 0)
    if (*s == 'b' || *s == 'h' || *s == 'w' || *s == 'g')
      size = *s++;
    else if (*s >= 'a' && *s <= 'z')
      format = *s++;
    else
      break;
}


void dispfmt::print (char *str) const
{
  if (count != 1 || format != 0 || size != 0)
    {
      if (count != 1)
        str += sprintf (str, "/%d", count);
      else
        *str++ = '/';
      if (format != 0)
        *str++ = format;
      if (size != 0)
        *str++ = size;
    }
  *str = 0;
}


display_window::display::display (const display &src)
{
  expr = src.expr;
  format = src.format;
  value = src.value;
  next = NULL;
  number = src.number;
  line = src.line;
  line_count = src.line_count;
  enable = src.enable;
}


void display_window::display::set_type (gdbio *gdb)
{
  capture *capt;
  capt = gdb->capture_cmd ("whatis %s", (const char *)expr);
  if (capt == NULL)
    return;
  if (capt->error.is_set ())
    type = other;
  else
    {
      const char *s = gdb->get_output ();
      int len = gdb->get_output ().length ();
      while (len > 0 && s[len-1] == '\n')
        --len;
      if (len > 0 && s[len-1] == '*')
        type = pointer;
      else
        type = other;
    }
  delete_capture (capt);
}


display_window::display
&display_window::display::operator = (const display &src)
{
  expr = src.expr;
  format = src.format;
  value = src.value;
  next = NULL;
  number = src.number;
  line = src.line;
  line_count = src.line_count;
  enable = src.enable;
  type = src.type;
  return *this;
}


static const struct
{
  const char *desc;
  char chr;
} formats[] =
  {
    {"Default",          0},
    {"Signed decimal",   'd'},
    {"Unsigned decimal", 'u'},
    {"Hexadecimal",      'x'},
    {"Octal",            'o'},
    {"Binary",           't'},
    {"Character",        'c'},
    {"Address",          'a'},
    {"Floating point",   'f'},
    {"String",           's'},
    {"Instruction",      'i'}
  };

#define N_FORMATS       (sizeof (formats) / sizeof (formats[0]))

MRESULT display_window::display::display_msg (HWND hwnd, ULONG msg,
                                              MPARAM mp1, MPARAM mp2)
{
  MRESULT mr;
  HWND hwndTemp;
  int i;
  char fmtstr[40];

  switch (msg)
    {
    case WM_INITDLG:
      {
        dispfmt fmt;
        dlg_sys_menu (hwnd);
        WinSetWindowPtr (hwnd, QWL_USER, this);
        WinSendDlgItemMsg (hwnd, IDC_ENABLE, BM_SETCHECK,
                           MPFROMSHORT (enable), NULL);
        WinSendDlgItemMsg (hwnd, IDC_EXPR, EM_SETTEXTLIMIT,
                           MPFROMSHORT (128), 0);
        if (!expr.is_null ())
          WinSetDlgItemText (hwnd, IDC_EXPR, (PCSZ)expr);
        if (!format.is_null ())
          fmt.parse (format);
        hwndTemp = WinWindowFromID (hwnd, IDC_FORMAT);
        for (i = 0; i < (int)N_FORMATS; ++i)
          {
            WinSendMsg (hwndTemp, LM_INSERTITEM, MPFROMSHORT (i),
                        MPFROMP (formats[i].desc));
            if (formats[i].chr == fmt.format)
              WinSendMsg (hwndTemp, LM_SELECTITEM,
                          MPFROMSHORT (i), MPFROMSHORT (TRUE));
          }
        if (fmt.format == 0)
          WinSendMsg (hwndTemp, LM_SELECTITEM,
                      MPFROMSHORT (0), MPFROMSHORT (TRUE));
        WinSendDlgItemMsg (hwnd, IDC_COUNT, SPBM_SETCURRENTVALUE,
                           MPFROMLONG (fmt.count), 0);
        WinSendDlgItemMsg (hwnd, IDC_COUNT, SPBM_SETLIMITS,
                           MPFROMLONG (99), MPFROMLONG (1));
        break;
      }

    case WM_CONTROL:
      switch (SHORT1FROMMP (mp1))
        {
        case IDC_FORMAT:
          mr = WinSendDlgItemMsg (hwnd, IDC_FORMAT, LM_QUERYSELECTION,
                                  MPFROMSHORT (LIT_FIRST), 0);
          i = SHORT1FROMMR (mr);
          bool enable_count = (i != LIT_NONE
                               && (formats[i].chr == 's'
                                   || formats[i].chr == 'i'));
          WinEnableWindow (WinWindowFromID (hwnd, IDC_COUNT), enable_count);
          break;
        }
      return 0;

    case WM_COMMAND:
      switch (SHORT1FROMMP (mp1))
        {
        case DID_OK:
          {
            dispfmt fmt;
            LONG n;
            mr = WinSendDlgItemMsg (hwnd, IDC_ENABLE, BM_QUERYCHECK, 0, 0);
            enable = (bool)SHORT1FROMMR (mr);
            LONG len = WinQueryDlgItemTextLength (hwnd, IDC_EXPR);
            expr.set (len);
            WinQueryDlgItemText (hwnd, IDC_EXPR, len + 1, (PSZ)expr.modify ());

            mr = WinSendDlgItemMsg (hwnd, IDC_FORMAT, LM_QUERYSELECTION,
                                    MPFROMSHORT (LIT_FIRST), 0);
            i = SHORT1FROMMR (mr);
            if (i != LIT_NONE && i != 0)
              {
                fmt.format = formats[i].chr;
                if (fmt.format == 's' || fmt.format == 'i')
                  {
                    mr = WinSendDlgItemMsg (hwnd, IDC_COUNT, SPBM_QUERYVALUE,
                                            MPFROMP (&n),
                                            MPFROM2SHORT (0,
                                                          SPBQ_ALWAYSUPDATE));
                    if (SHORT1FROMMR (mr))
                      fmt.count = n;
                  }
                fmt.print (fmtstr);
                format.set (fmtstr);
              }
            else
              format.set ("");
            WinDismissDlg (hwnd, DID_OK);
            return 0;
          }
        }
      break;
    }
  return WinDefDlgProc (hwnd, msg, mp1, mp2);
}


MRESULT EXPENTRY dlg_display (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
  display_window::display *p;

  if (msg == WM_INITDLG)
    p = (display_window::display *)((struct pm_create *)PVOIDFROMMP (mp2))->ptr;
  else
    p = (display_window::display *)WinQueryWindowPtr (hwnd, QWL_USER);
  return p->display_msg (hwnd, msg, mp1, mp2);
}


#define DSP_HEADER_LINES        1

display_window::display_window (command_window *in_cmd, gdbio *in_gdb,
                                unsigned in_id, const SWP *pswp,
                                const char *fontnamesize)
  : pmtxt (in_cmd->get_app (), in_id,
           pswp == NULL ? FCF_SHELLPOSITION : 0, pswp, fontnamesize)
{
  cmd = in_cmd; gdb = in_gdb;
  head = NULL;
  sel_line = -1;
  exp_number = -1;

  sel_attr = get_default_attr ();
  set_fg_color (sel_attr, CLR_BLACK);
  set_bg_color (sel_attr, CLR_PALEGRAY);

  delete_all_displays (true);   // Update menu, insert header line
  set_title ("pmgdb - Display");
  set_keys_help_id (HELP_THR_KEYS);
}


display_window::~display_window ()
{
  delete_all_displays (false);
}


void display_window::delete_all_displays (bool init)
{
  display *p, *next;
  for (p = head; p != NULL; p = next)
    {
      next = p->next;
      delete p;
    }
  head = NULL;
  sel_line = -1;

  if (init)
    {
      delete_all ();
      int x = 0;
      put (0, x, 4, " No ", false); x += 4;
      put_tab (0, x++, false);
      put_vrule (0, x++, false);
      put (0, x, 5, " Ena ", false); x += 5;
      put_tab (0, x++, false);
      put_vrule (0, x++, false);
      put (0, x, 5, " Fmt ", false); x += 5;
      put_tab (0, x++, false);
      put_vrule (0, x++, false);
      put (0, x, 6, " Expr ", false); x += 6;
      put_tab (0, x++, false);
      put_vrule (0, x++, false);
      put (0, x, 6, " Value", false); x += 6;
      underline (0, true, false);
      sync ();
      menu_enable (IDM_ENABLE, false);
      menu_enable (IDM_DISABLE, false);
      menu_enable (IDM_MODIFY, false);
      menu_enable (IDM_DELETE, false);
      menu_enable (IDM_ENABLE_ALL, false);
      menu_enable (IDM_DISABLE_ALL, false);
      menu_enable (IDM_DELETE_ALL, false);
      menu_enable (IDM_REPMENU, false);
      menu_enable (IDM_DEREFERENCE, false);
    }
}


bool display_window::add (HWND hwnd)
{
  bool result = false;
  display temp;
  pm_create create;
  create.cb = sizeof (create);
  create.ptr = (void *)&temp;
  if (WinDlgBox (HWND_DESKTOP, hwnd, dlg_display, 0, IDD_DISPLAY,
                 &create) == DID_OK
      && temp.expr[0] != 0)
    {
      select_line (-1);
      capture *capt;
      capt = gdb->capture_cmd ("server display %s %s",
                               (const char *)temp.format,
                               (const char *)temp.expr);
      if (capt != NULL && capt->disp_number.is_set ())
        {
          int number = capt->disp_number.get ();
          if (!temp.enable)
            gdb->send_cmd ("server disable display %d", number);
          exp_number = number;
          exp_line = -1;
          exp_enable = temp.enable;
          result = true;
        }
      delete_capture (capt);
    }
  return result;
}


MRESULT display_window::wm_close (HWND, ULONG, MPARAM, MPARAM)
{
  show (false);
  return 0;
}


MRESULT display_window::wm_command (HWND hwnd, ULONG msg,
                                    MPARAM mp1, MPARAM mp2)
{
  display *p;

  switch (SHORT1FROMMP (mp1))
    {
    case IDM_ENABLE:
      p = find_by_line (sel_line);
      if (p == NULL)
        WinAlarm (HWND_DESKTOP, WA_ERROR);
      else
        gdb->send_cmd ("server enable display %d", p->number);
      return 0;

    case IDM_DISABLE:
      p = find_by_line (sel_line);
      if (p == NULL)
        WinAlarm (HWND_DESKTOP, WA_ERROR);
      else
        gdb->send_cmd ("server disable display %d", p->number);
      return 0;

    case IDM_DELETE:
      p = find_by_line (sel_line);
      if (p == NULL)
        WinAlarm (HWND_DESKTOP, WA_ERROR);
      else
        gdb->send_cmd ("server undisplay %d", p->number);
      return 0;

    case IDM_ADD:
      add (hwnd);
      return 0;

    case IDM_MODIFY:
      p = find_by_line (sel_line);
      if (p == NULL)
        WinAlarm (HWND_DESKTOP, WA_ERROR);
      else
        modify (p);
      return 0;

    case IDM_ENABLE_ALL:
      gdb->send_cmd ("server enable display");
      return 0;

    case IDM_DISABLE_ALL:
      gdb->send_cmd ("server disable display");
      return 0;

    case IDM_DELETE_ALL:
      gdb->send_cmd ("server undisplay");
      delete_all_displays (true); // GDB doesn't send an annotation!
      return 0;

    case IDM_REP_DEC_S:
      change_representation ('d');
      return 0;

    case IDM_REP_DEC_U:
      change_representation ('u');
      return 0;

    case IDM_REP_HEX:
      change_representation ('x');
      return 0;

    case IDM_REP_OCT:
      change_representation ('o');
      return 0;

    case IDM_REP_BIN:
      change_representation ('t');
      return 0;

    case IDM_REP_ADR:
      change_representation ('a');
      return 0;

    case IDM_REP_CHR:
      change_representation ('c');
      return 0;

    case IDM_REP_FLT:
      change_representation ('f');
      return 0;

    case IDM_REP_STR:
      change_representation ('s');
      return 0;

    case IDM_REP_INS:
      change_representation ('i');
      return 0;

    case IDM_DEREFERENCE:
      p = find_by_line (sel_line);
      if (p == NULL)
        WinAlarm (HWND_DESKTOP, WA_ERROR);
      else
        dereference (p, false);
      return 0;

    default:
      return cmd->wm_command (hwnd, msg, mp1, mp2);
    }
}


bool display_window::wm_user (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
  if (parent::wm_user (hwnd, msg, mp1, mp2))
    return true;
  switch (msg)
    {
    case UWM_MENU:
      // TODO: Cache
      if (sel_line == -1)
        {
          menu_enable (IDM_ENABLE, false);
          menu_enable (IDM_DISABLE, false);
          menu_enable (IDM_MODIFY, false);
          menu_enable (IDM_DELETE, false);
          menu_enable (IDM_REPMENU, false);
          menu_enable (IDM_DEREFERENCE, false);
        }
      else
        {
          const display *p = find_by_line (sel_line);
          menu_enable (IDM_ENABLE, !p->enable);
          menu_enable (IDM_DISABLE, p->enable);
          menu_enable (IDM_MODIFY, true);
          menu_enable (IDM_DELETE, true);
          menu_enable (IDM_REPMENU, true);
          // TODO: null pointer
          menu_enable (IDM_DEREFERENCE, p->type == display::pointer);
        }
      menu_enable (IDM_ENABLE_ALL, head != NULL);
      menu_enable (IDM_DISABLE_ALL, head != NULL);
      menu_enable (IDM_DELETE_ALL, head != NULL);
      return true;

    default:
      return false;
    }
}


MRESULT display_window::wm_activate (HWND hwnd, ULONG msg,
                                     MPARAM mp1, MPARAM mp2)
{
  if (SHORT1FROMMP (mp1))
    cmd->associate_help (get_hwndFrame ());
  return WinDefWindowProc (hwnd, msg, mp1, mp2);
}


void display_window::button_event (int line, int column, int tab, int button,
                                   int clicks)
{
  if (line >= 0 && column >= 0)
    {
      // TODO: Context menu
      if (button == 1 && clicks == 2 && tab == 1)
        {
          const display *p = find_by_line (line);
          if (p == NULL)
            WinAlarm (HWND_DESKTOP, WA_ERROR);
          else
            gdb->send_cmd ("server %s display %d",
                           p->enable ? "disable" : "enable", p->number);
        }
      else if (button == 1 && clicks == 2 && tab >= 2 && tab <= 3)
        {
          display *p = find_by_line (line);
          if (p == NULL)
            WinAlarm (HWND_DESKTOP, WA_ERROR);
          else
            {
              select_line (line);
              modify (p);
            }
        }
      else if (button == 1 && clicks == 1)
        select_line (line, true);
      else if (button == 1 && clicks == 2)
        select_line (line);
    }
  else
    select_line (-1);
}


display_window::display *display_window::find_by_number (int number)
{
  for (display *p = head; p != NULL; p = p->next)
    if (p->number == number)
      return p;
  return NULL;
}


display_window::display *display_window::find_by_line (int line)
{
  for (display *p = head; p != NULL; p = p->next)
    if (p->is_line (line))
      return p;
  return NULL;
}


void display_window::enable (int number)
{
  display *p = find_by_number (number);
  if (p != NULL && !p->enable)
    {
      p->enable = true;
      if (p->line == sel_line)
        WinPostMsg (get_hwndClient (), UWM_MENU, 0, 0);
      update (p);
    }
}


void display_window::disable (int number)
{
  display *p = find_by_number (number);
  if (p != NULL && p->enable)
    {
      p->enable = false;
      if (p->line == sel_line)
        WinPostMsg (get_hwndClient (), UWM_MENU, 0, 0);
      update (p);
    }
}


void display_window::remove (int number)
{
  for (display **patch = &head; *patch != NULL; patch = &(*patch)->next)
    if ((*patch)->number == number)
      {
        display *p = *patch;
        for (display *q = (*patch)->next; q != NULL; q = q->next)
          q->line -= p->line_count;
        if (p->line == sel_line)
          {
            sel_line = -1;
            WinPostMsg (get_hwndClient (), UWM_MENU, 0, 0);
          }
        delete_lines (p->line, p->line_count, true);
        *patch = p->next;
        delete p;
        return;
      }
}


void display_window::update (display *p)
{
  char buf[400];
  int x = 0, len, line_count, diff;
  pmtxt_attr attr;

  if (p->line == sel_line)
    attr = sel_attr;
  else
    attr = get_default_attr ();

  line_count = 1;
  if (p->enable)
    for (const char *s = p->value; *s != 0; ++s)
      if (*s == '\n')
        ++line_count;

  diff = line_count - p->line_count;
  if (diff < 0)
    delete_lines (p->line + line_count, -diff, true);
  if (diff > 0)
    insert_lines (p->line + p->line_count, diff, true);
  if (diff != 0)
    {
      for (display *q = p->next; q != NULL; q = q->next)
        q->line += diff;
      if (sel_line == p->line)
        sel_count = line_count;
      else if (sel_line > p->line)
        sel_line += diff;
      p->line_count = line_count;
    }
  clear_lines (p->line, line_count, true);

  len = snprintf (buf, sizeof (buf), " %d ", p->number);
  put (p->line, x, len, buf, attr, true); x += len;
  put_tab (p->line, x++, attr, true);
  put_vrule (p->line, x++, attr, true);

  len = snprintf (buf, sizeof (buf), " %c ", p->enable ? 'y' : 'n');
  put (p->line, x, len, buf, attr, true); x += len;
  put_tab (p->line, x++, attr, true);
  put_vrule (p->line, x++, attr, true);

  len = snprintf (buf, sizeof (buf), " %s ", (const char *)p->format);
  put (p->line, x, len, buf, attr, true); x += len;
  put_tab (p->line, x++, attr, true);
  put_vrule (p->line, x++, attr, true);

  len = snprintf (buf, sizeof (buf), " %s ", (const char *)p->expr);
  put (p->line, x, len, buf, attr, true); x += len;
  put_tab (p->line, x++, attr, true);
  put_vrule (p->line, x++, attr, true);

  if (p->line == sel_line)
    set_eol_attr (p->line, sel_attr, true);

  if (p->enable)
    {
      int y = p->line;
      const char *s = p->value;
      while (*s != 0)
        {
          const char *nl = strchr (s, '\n');
          len = nl != NULL ? nl - s : strlen (s);
          if (y != p->line)
            {
              if (p->line == sel_line)
                set_eol_attr (y, sel_attr, true);
              for (int i = 0; i < 4; ++i)
                {
                  put_tab (y, x++, attr, true);
                  put_vrule (y, x++, attr, true);
                }
            }
          put (y, x++, 1, " ", attr, true);
          const char *tab;
          while ((tab = (const char *)memchr (s, '\t', len)) != NULL)
            {
              if (tab != s)
                {
                  put (y, x, tab - s, s, attr, true);
                  x += tab - s; len -= tab - s; s = tab;
                }
              ++s; --len;       // Skip TAB
              put (y, x++, 1, " ", attr, true);
              put_tab (y, x++, attr, true);
            }
          if (len != 0)
            {
              put (y, x, len, s, attr, true);
              s += len;
            }
          if (*s != 0)
            {
              ++s;              // Skip linefeed
              ++y; x = 0;       // New line
            }
        }
    }
}


void display_window::update (int number, const char *expr, const char *format,
                             const char *value, bool enable)
{
  int undisplay = -1;
  display *p = find_by_number (number);
  if (value == NULL) value = "";

  if (p == NULL)
    {
      if (exp_number == number && exp_line != -1
          && (p = find_by_line (exp_line)) != NULL)
        {
          // Modifying a display
          undisplay = p->number;
          p->line = exp_line;
        }
      else
        {
          // New display
          if (head == NULL)
            {
              menu_enable (IDM_ENABLE_ALL, true);
              menu_enable (IDM_DISABLE_ALL, true);
              menu_enable (IDM_DELETE_ALL, true);
            }

          display **patch = &head;
          int line = DSP_HEADER_LINES;
          while (*patch != NULL)
            {
              line += (*patch)->line_count;
              patch = &(*patch)->next;
            }
          p = new display;
          *patch = p;
          p->next = NULL;
          p->line = line;
          p->line_count = 1;
          p->enable = true;
        }
      p->number = number;
      p->expr.set (expr);
      p->format.set (format);
      p->value.set (value);
      p->set_type (gdb);
    }
  else
    {
      if (number != exp_number && strcmp (value, p->value) == 0)
        return;
      // TODO: Redraw value only (if only the value changed)
      p->value.set (value);
    }

  // Set the state of a new display before GDB knows about and reports
  // the new state
  if (number == exp_number)
    {
      p->enable = exp_enable;
      select_line (p->line);
    }
  if (!enable)
    p->enable = false;
  exp_number = -1;
  if (undisplay != -1)
    gdb->send_cmd ("server undisplay %d", undisplay);
  update (p);
}


void display_window::select_line (int line, bool toggle)
{
  const display *p = line == -1 ? (display *)NULL : find_by_line (line);
  line = p != NULL ? p->line : -1; // Normalize
  if (toggle && line != -1 && line == sel_line)
    line = -1;
  if (line != sel_line)
    {
      if (sel_line != -1)
        for (int i = 0; i < sel_count; ++i)
          {
            put (sel_line + i, 0, max_line_len, get_default_attr (), true);
            set_eol_attr (sel_line + i, get_default_attr (), true);
          }
      if (line != -1)
        {
          sel_count = p->line_count;
          for (int i = 0; i < sel_count; ++i)
            {
              put (line + i, 0, max_line_len, sel_attr, true);
              set_eol_attr (line + i, sel_attr, true);
            }
        }
      sel_line = line;
    }
  // TODO: Cache
  if (line != -1)
    {
      menu_enable (IDM_ENABLE, !p->enable);
      menu_enable (IDM_DISABLE, p->enable);
      menu_enable (IDM_MODIFY, true);
      menu_enable (IDM_DELETE, true);
      menu_enable (IDM_REPMENU, true);
      menu_enable (IDM_DEREFERENCE, p->type == display::pointer);
      show_line (line, 1, 1);
    }
  else
    {
      menu_enable (IDM_ENABLE, false);
      menu_enable (IDM_DISABLE, false);
      menu_enable (IDM_MODIFY, false);
      menu_enable (IDM_DELETE, false);
      menu_enable (IDM_REPMENU, false);
      menu_enable (IDM_DEREFERENCE, false);
    }
}


void display_window::modify (const display *p, const char *new_format,
                             const char *new_expr, bool new_enable, bool add)
{
  select_line (-1);
  if (new_format == NULL)
    new_format = p->format;
  if (new_expr == NULL)
    new_expr = p->expr;
  capture *capt;
  capt = gdb->capture_cmd ("server display %s %s", new_format, new_expr);
  if (capt != NULL && capt->disp_number.is_set ())
    {
      int number = capt->disp_number.get ();
      exp_number = number;
      exp_enable = new_enable;
      exp_line = add ? -1 : p->line;
      if (!new_enable)
        gdb->send_cmd ("server disable display %d", number);
    }
  delete_capture (capt);
}


void display_window::change_representation (char fmtchr)
{
  display *p = find_by_line (sel_line);
  if (p == NULL)
    {
      WinAlarm (HWND_DESKTOP, WA_ERROR);
      return;
    }

  dispfmt fmt;
  fmt.parse (p->format);

  if (fmtchr == fmt.format)
    return;
  fmt.format = fmtchr;
  char new_format_buf[20];
  fmt.print (new_format_buf);
  modify (p, new_format_buf, NULL, p->enable, false);
}


void display_window::modify (display *p)
{
  display temp = *p;
  pm_create create;
  create.cb = sizeof (create);
  create.ptr = (void *)&temp;
  if (WinDlgBox (HWND_DESKTOP, get_hwndClient (), dlg_display, 0, IDD_DISPLAY,
                 &create) == DID_OK
      && temp.expr[0] != 0
      && (temp.enable != p->enable
          || strcmp (temp.expr, p->expr) != 0
          || strcmp (temp.format, p->format) != 0))
    {
      if (strcmp (temp.expr, p->expr) == 0
          && strcmp (temp.format, p->format) == 0)
        gdb->send_cmd ("server %s display %d",
                       temp.enable ? "enable" : "disable", p->number);
      else
        modify (p, temp.format, temp.expr, temp.enable, false);
    }
}


void display_window::dereference (display *p, bool add)
{
  astring expr;
  expr = "*(";
  expr.append (p->expr);
  expr.append (")");
  modify (p, "", expr, true, add);
}
