/* pmdebug.h -*- C++ -*-
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


// Important: All member functions of pmdebug must be called in a
// thread that has a message queue.  Moreover, thread 1 must have
// a message queue (DosExitList!)

class pmdebug
{
public:
  enum mode { off, sync, async };

  pmdebug ();
  ~pmdebug ();
  void set_mode (mode new_mode, HWND hwnd, int pid = 0);
  mode get_mode () const { return cur_mode; }
  void start ();                // About to start debuggee
  void stop ();                 // Debuggee stopped
  void term ();                 // Debuggee terminated
  void set_pid (int in_pid) { pid = in_pid; }
  void set_window (HWND hwnd) { hwndLock = hwnd; }
  void exit ();

private:
  mode cur_mode;
  bool locked;
  HWND hwndLock;
  int pid;
};

extern pmdebug pmdbg;
