/* pmapp.h -*- C++ -*-
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


class pmapp
{
public:
  pmapp (const char *name);
  ~pmapp ();
  void win_error () const;
  void no_message_loop () const;
  void message_loop () const;
  HAB get_hab () const { return hab; }
  const char *get_name () const { return name; }
  int get_screen_width () const { return swpScreen.cx; }
  int get_screen_height () const { return swpScreen.cy; }

private:
  HAB hab;
  HMQ hmq;
  SWP swpScreen;
  fstring name;
};


struct pm_create
{
  USHORT cb;
  void *ptr;
  void *ptr2;
};

void dlg_sys_menu (HWND hwnd);
