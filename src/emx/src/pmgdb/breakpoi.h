/* breakpoi.h -*- C++ -*-
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
class capture;
class breakpoint_list;
class breakpoints_window;

class breakpoint
{
public:

  enum bpt
  {
    BPT_BREAKPOINT,
    BPT_HW_BREAKPOINT,
    BPT_UNTIL,
    BPT_FINISH,
    BPT_WATCHPOINT,
    BPT_HW_WATCHPOINT,
    BPT_READ_WATCHPOINT,
    BPT_ACC_WATCHPOINT,
    BPT_LONGJMP,
    BPT_LONGJMP_RESUME,
    BPT_STEP_RESUME,
    BPT_SIGTRAMP,
    BPT_WATCHPOINT_SCOPE,
    BPT_CALL_DUMMY,
    BPT_SHLIB_EVENTS,
    BPT_UNKNOWN
  };

  enum bpd
  {
    BPD_DEL,
    BPD_DIS,
    BPD_KEEP,
    BPD_UNKNOWN
  };

  // Constructors and destructors
  breakpoint ();
  breakpoint (const breakpoint &bp);
  ~breakpoint ();

  // Assignment
  breakpoint &operator = (const breakpoint &src);

  // Setting members
  void set_number (int v) { number = v; }
  void set_enable (bool v) { enable = v; }
  void set_address (ULONG v) { address = v; }
  void set_source (const char *s, int len) { source.set (s, len); }
  void set_lineno (int v) { lineno = v; }
  void set_ignore (int v) { ignore = v; }
  void set_disposition (const char *s);
  void set_condition (const char *s) { condition.set (s); }
  void copy (const breakpoint &src);

  // Querying members
  int get_number ()         const { return number; }
  bpt get_type ()           const { return type; }
  bpd get_disposition ()    const { return disposition; }
  bool get_enable ()        const { return enable; }
  ULONG get_address ()      const { return address; }
  const char *get_source () const { return source; }
  int get_lineno ()         const { return lineno; }
  int get_ignore ()         const { return ignore; }
  const char *get_condition () const { return condition; }
  const char *get_disposition_as_string () const;

  bool operator == (const breakpoint b) const;
  bool operator != (const breakpoint b) const { return !operator== (b); }

  bool set_from_capture (const capture &capt);

  // Dialog box
  MRESULT breakpoint_msg (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);

private:
  int number;
  bpt type;
  bpd disposition;
  bool enable;
  ULONG address;
  // TODO: Use numbers for identifying source files
  fstring source;
  int lineno;
  fstring condition;
  int ignore;
  // *** Update copy() when adding members!
  // *** Update eq_brkpt() when adding members!

  // Friends
  friend breakpoint_list;
  friend breakpoints_window;
};

class breakpoint_list
{
public:

  typedef void (command_window::*update_fun)(int index,
                                             const breakpoint *old_bpt,
                                             const breakpoint *new_bpt);

  // Constructors and destructors
  breakpoint_list ();
  ~breakpoint_list ();

  void steal (breakpoint_list &from);
  void add (const breakpoint &bp);
  void delete_all ();
  void update (const breakpoint_list &old, command_window *cmd,
               update_fun fun);
  const breakpoint *find (ULONG addr) const;
  const breakpoint *find (const char *fname, int lineno) const;
  const breakpoint *get (int n) const;
  bool any_enabled () const;

private:

  class brkpt_node : public breakpoint
  {
  public:
    brkpt_node (const breakpoint &bp) : breakpoint (bp) { next = NULL; }
    ~brkpt_node () {}
    class brkpt_node *next;
  };

  brkpt_node *list, **end;

  // Friends
  friend command_window;
};


// Breakpoint list
class breakpoints_window : public pmtxt
{
public:

  typedef pmtxt parent;

  breakpoints_window (command_window *cmd, gdbio *gdb, unsigned id,
                      const char *fontnamesize);
  ~breakpoints_window ();

  void add_all ();
  void update (int index, const breakpoint *old_bpt,
               const breakpoint *new_bpt);
  void add_line (const char *default_source = NULL, int line_no = -1);

private:
  // Override member functions of pmframe
  MRESULT wm_activate (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
  MRESULT wm_close (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
  MRESULT wm_command (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
  void button_event (int line, int column, int tab, int button, int clicks);

  // Painting
  void put_bpt (int line, const breakpoint &bpt);
  void select_line (int line, bool toggle = false);

  void show_source (int line);
  bool dialog (breakpoint *bpt);
  void send_disposition (int number, breakpoint::bpd disposition, bool enable);

  command_window *cmd;
  gdbio *gdb;
  pmtxt_attr sel_attr;          // Attribute used for hilighting selection
  int sel_line;

  // Expected new breakpoint
  int exp_number;
};
