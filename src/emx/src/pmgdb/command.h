/* command.h -*- C++ -*-
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


class capture;
class annotation;
class gdbio;
class display_window;
class threads_window;
class breakpoints_window;
class breakpoint;
class source_window;
class source_files_window;
class register_window;


class command_window : public pmtty
{
public:
  typedef pmtty parent;

  // Contructors and destructors
  command_window (pmapp *app, unsigned id, const char *fontnamesize,
                  char **argv);
  ~command_window ();

  // Querying members
  const breakpoint_list::brkpt_node *get_breakpoint_list () const
    { return breakpoints.list; }
  const breakpoint *get_breakpoint (int n) const;
  const char *get_debuggee () const { return debuggee; }
  const source_files_window *get_srcs () const { return srcs; }

  // Override member functions of pmframe
  MRESULT wm_activate (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
  MRESULT wm_command (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);

  // etc.
  void show_source (const char *short_fname, const char *long_fname,
                    bool set_focus, int lineno = -1);
  void capture_error (const capture *p);
  void run_command ();
  void prepare_run ();
  void show_annotation (const annotation *ann);
  void exec_file (const char *s, int len);
  void where ();
  void handle_output (const char *text, int len);
  void associate_help (HWND hwnd);
  void update_registers ();
  void get_breakpoints ();

  // Constants

  enum
  {
    GSC_PROMPT      = 0x0001,
    GSC_RUNNING     = 0x0002,
    GSC_BREAKPOINTS = 0x0004,
    GSC_EXEC_FILE   = 0x0008
  };

private:

  struct new_source;

  void lock ();
  void unlock ();
  void key (char c);
  void kill_input ();
  void complete ();
  void complete (const char *p);
  HWND create_hwnd_menu ();
  void completion_menu (HWND hwnd, int start, int count, int skip);
  void from_history (bool next);
  void replace_command (const char *s);
  void exec_command ();
  void font ();
  void arg_add (const char *str);
  void arg_delete_all ();
  void thread ();
  void notify (unsigned change);
  void show_new_source (const new_source *ns);
  void delete_source (source_window *src);
  bool open_dialog (char *buf, const char *title, const char *fname);
  void open_exec ();
  void open_core ();
  void open_source ();
  void brkpt_update (int index, const breakpoint *old_bpt,
                     const breakpoint *new_bpt);
  void get_breakpoints (source_window *src, bool paint);
  source_window *find_source_short (const char *fname);
  source_window *find_source_long (const char *fname);
  void pmdbg_start ();
  void pmdbg_stop ();
  void pmdbg_term ();
  MRESULT pmdebugmode_msg (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
  MRESULT startup_msg (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
  MRESULT history_msg (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
  void new_debuggee ();
  void help_disable (const pmframe *frame);

  // Override member functions of pmtxt
  MRESULT wm_char (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
  bool wm_user (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
  MRESULT wm_close (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);

  struct src_node               // Perhaps we should use a container class...
  {
    struct src_node *next;
    source_window *src;
    HWND hwndWinMenu;
    unsigned menu_id;
  };

  struct cmd_hist;

  // Data members

  bool exec_file_pending;
  bool help_ok;
  char **argv;
  int argv_used;
  int argv_size;
  fstring debuggee;

  _fmutex mutex;                // Hide pmtxt::mutex

  astring input_line;
  lstring completions;
  cmd_hist *history;
  cmd_hist *cur_cmd;

  src_node *src_list;
  int src_menu_id_count;
  char *src_menu_id_used;

  breakpoint_list breakpoints;

  breakpoints_window *brk;
  display_window *dsp;
  threads_window *thr;
  source_files_window *srcs;
  register_window *reg;

  gdbio *gdb;

  // Friends
  // TODO: Use more friends instead of public members?

  friend source_window;
  friend gdbio;
  friend void command_window_thread (void *arg);
  friend MRESULT EXPENTRY
    dlg_pmdebugmode (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
  friend MRESULT EXPENTRY
    dlg_startup (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
  friend MRESULT EXPENTRY
    dlg_history (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
};
