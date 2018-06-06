/* threads.h -*- C++ -*-
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

class threads_window : public pmtxt
{
public:

  typedef pmtxt parent;

  threads_window (command_window *cmd, gdbio *gdb, unsigned id,
                  const SWP *pswp, const char *fontnamesize);
  ~threads_window ();

  void add (int number, int pid);
  void select (int number);
  void enable (int number);
  void disable (int number);
  void remove (int tid);

private:
  class thread;

  // Override member functions of pmframe
  MRESULT wm_activate (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
  MRESULT wm_close (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
  MRESULT wm_command (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
  bool wm_user (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
  void button_event (int line, int column, int tab, int button, int clicks);

  thread *find_by_number (int number);
  thread *find_by_line (int line);
  void update (const thread *p);
  void current_line (int line);
  void select_line (int line, bool toggle = false);

  command_window *cmd;
  gdbio *gdb;
  thread *head;
  pmtxt_attr sel_attr;          // Selected thread
  pmtxt_attr hilite_attr;       // Current thread
  pmtxt_attr sel_hilite_attr;   // Selected & current thread
  int cur_line;                 // Line of current thread
  int sel_line;                 // Line of selected thread
};
