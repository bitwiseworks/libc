/* srcfiles.h -*- C++ -*-
   Copyright (c) 1996-1998 Eberhard Mattes

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

class source_files_window : public pmtxt
{
public:

  typedef pmtxt parent;
  struct srcs_node;

  // Constructors and destructors
  source_files_window (command_window *cmd, unsigned id, const SWP *pswp,
                       const char *fontnamesize);
  ~source_files_window ();
  void add (const char *fname);
  void done ();
  void delete_all ();
  void select (const char *fname);

  class iterator
  {
  public:
    iterator (const source_files_window &s) { cur = s.head; }
    ~iterator () {}
    bool ok () { return cur != NULL; }
    void next () { cur = cur->next; }
    operator const char * () { return cur->fname; }

  private:
    const srcs_node *cur;
  };

private:

  // Override member functions of pmframe
  MRESULT wm_activate (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
  MRESULT wm_command (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
  MRESULT wm_close (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
  void button_event (int line, int column, int tab, int button, int clicks);

  srcs_node *find_by_line (int line);
  void select_line (int line, bool toggle = false);

  command_window *cmd;
  srcs_node *head;
  int lines;
  int sel_line;
  pmtxt_attr sel_attr;

  struct srcs_node
  {
    struct srcs_node *next;
    fstring fname;
  };

  friend class iterator;
};
