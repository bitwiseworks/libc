/* display.h -*- C++ -*-
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


class command_window;
class gdbio;

class display_window : public pmtxt
{
public:

  typedef pmtxt parent;

  display_window (command_window *cmd, gdbio *gdb, unsigned id,
                  const SWP *pswp, const char *fontnamesize);
  ~display_window ();

  void delete_all_displays (bool init);
  void update (int number, const char *expr, const char *format,
               const char *value, bool enable = true);
  void enable (int number);
  void disable (int number);
  void remove (int number);
  bool add (HWND hwnd);

private:
  class display;

  // Override member functions of pmframe
  MRESULT wm_activate (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
  MRESULT wm_close (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
  MRESULT wm_command (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
  bool wm_user (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
  void button_event (int line, int column, int tab, int button, int clicks);
  display *find_by_number (int number);
  display *find_by_line (int line);
  void update (display *p);
  void select_line (int line, bool toggle = false);
  void modify (const display *p, const char *new_format, const char *new_expr,
               bool new_enable, bool add);
  void modify (display *p);
  void change_representation (char fmtchr);
  void dereference (display *p, bool add);

  command_window *cmd;
  gdbio *gdb;
  display *head;
  int sel_line, sel_count;
  pmtxt_attr sel_attr;

  // Expected new display
  int exp_number;
  bool exp_enable;
  int exp_line;
};
