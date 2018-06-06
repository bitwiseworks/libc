/* source.h -*- C++ -*-
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

class source_window : public pmtxt
{
public:

  typedef pmtxt parent;

  // Constructors and destructors
  source_window (pmapp *app, unsigned id, const char *fontnamesize,
                 const char *short_fname, const char *long_fname,
                 command_window *cmd, gdbio *gdb);
  ~source_window ();

  // Painting
  void show_line (int lineno);
  void select_line (int lineno);
  ULONG selected_addr ();
  void set_breakpoint (int lineno, bool set, bool paint);

  // Querying members
  const char *get_short_filename () const { return short_filename; }
  const char *get_long_filename () const { return long_filename; }

  // Dialog box
  MRESULT goto_msg (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
  MRESULT find_msg (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);

private:
  void load ();
  void button1_lineno (int line, int clicks);
  void button1_source (int line, int column, int clicks);
  void find (bool start, bool forward);

  // Override member functions of pmframe
  bool wm_user (HWND, ULONG msg, MPARAM mp1, MPARAM mp2);
  MRESULT wm_activate (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
  MRESULT wm_char (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
  MRESULT wm_command (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
  MRESULT wm_close (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
  void button_event (int line, int column, int tab, int button, int clicks);

  command_window *cmd;
  gdbio *gdb;
  fstring short_filename;       // File name of source file (symtab)
  fstring long_filename;        // Path name of source file
  int cur_lineno;               // Current line# (1-based) or -1
  int sel_lineno;               // Selected line# (1-based) or -1
  pmtxt_attr hilite_attr;       // Attribute used for hilighting current line
  pmtxt_attr sel_attr;          // Attribute used for hilighting selection
  pmtxt_attr bpt_attr;          // Attribute used for breakpoints
  pmtxt_attr sel_bpt_attr;      // Select and breakpoint

  int exp_lineno, exp_column;
  size_t exp_len;

  int find_lineno;
  vstring find_string;
};
