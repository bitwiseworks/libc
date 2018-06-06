/* register.h -*- C++ -*-
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

class register_window : public pmtxt
{
public:

  typedef pmtxt parent;

  // Constructors and destructors
  register_window (command_window *cmd, unsigned id, const SWP *pswp,
                   const char *fontnamesize);
  ~register_window ();
  void update (const char *s);

private:
  // Override member functions of pmframe
  MRESULT wm_activate (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
  MRESULT wm_command (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
  MRESULT wm_close (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);

  command_window *cmd;
};
