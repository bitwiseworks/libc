/* pmtty.h -*- C++ -*-
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


// A frame window showing scrollable text; with tty-like output and cursor

class pmtty : public pmtxt
{
public:
  typedef pmtxt parent;

  // Constructors and destructors, initialization
  pmtty (pmapp *app, unsigned id, ULONG frame_flags, const SWP *pswp,
         const char *fontnamesize = 0);
  ~pmtty ();
  void delete_all ();

  // Painting
  void puts (const char *s, size_t len);
  void puts (const char *s) { puts (s, strlen (s)); }
  void backspace (int n = 1);
  void cursor (bool on);

  // Attributes
  void set_fg_color (COLOR color) { pmtxt::set_fg_color (sel_attr, color); }
  void set_fg_color () { pmtxt::set_fg_color (sel_attr); }

  void set_bg_color (COLOR color) { pmtxt::set_bg_color (sel_attr, color); }
  void set_bg_color () { pmtxt::set_bg_color (sel_attr); }

  const struct pmtxt_attr &get_attr () const {return sel_attr;}
  void set_attr (const struct pmtxt_attr &a) {sel_attr = a;}
  void set_attr () {sel_attr = get_default_attr ();}

  void set_line_width (int width);
  int get_line () const { return y; }
  int get_column () const { return x; }
  bool get_xy (POINTL *ptl);

private:
  int x, y;
  bool cursor_enabled;          // True iff cursor enabled with cursor()
  bool cursor_painted;          // True iff cursor painted
  int cursor_hidden;            // Positive iff cursor temporarily hidden
  pmtxt_attr sel_attr;
  int line_width;
  int cursor_width;
  HPS hpsCursor;
  RECTL rclCursor;

  // Override member functions of pmframe
  MRESULT wm_setfocus (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
  MRESULT wm_timer (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
  MRESULT wm_presparamchanged (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);

  // Override member functions of pmtxt
  void painting (bool start);
  void font_changed ();

  void create_hpsCursor ();
  void cursor_on ();
  void cursor_off ();
  void paint_cursor ();
  void remove_cursor ();
  void puts1 (const char *s, size_t len);
};
