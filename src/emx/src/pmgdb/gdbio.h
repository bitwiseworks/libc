/* gdbio.h -*- C++ -*-
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
class capture;

class gdbio : annotation
{
public:

  // Types

  enum gsp
  {
    GSP_CMD,
    GSP_OTHER,
    GSP_NONE,
    GSP_NOTREADY
  };

  enum gsr
  {
    GSR_RUNNING,
    GSR_STOPPED,
    GSR_NONE
  };

  gdbio (command_window *cmd);
  ~gdbio ();

  void parse_gdb ();
  const astring &get_output () const { return output; }
  bool is_nochild () const { return gdb_running == GSR_NONE; }
  bool is_ready () const { return gdb_prompt == GSP_CMD; }
  void prepare_run ();
  void send_init (int fd);
  void send_init ();
  bool send_cmd (const char *fmt, ...);
  void queue_cmd (const char *fmt, ...);
  bool call_cmd (const char *fmt, ...);
  capture *capture_cmd (const char *fmt, ...);
  ULONG address_of_line (const char *fname, int lineno);
  bool get_setshow_boolean (const char *name);
  void set_setshow_boolean (const char *name, bool value);
  char *get_setshow_string (const char *name);
  void set_setshow_string (const char *name, const char *value);

private:
  void parse_breakpoints_table ();
  void parse_display_begin ();
  void parse_error_begin ();
  void parse_source ();
  void parse_source_location ();
  void parse_frame_begin ();
  void parse_pre_prompt ();
  void parse_value (bool to_screen, bool to_field);
  void fetch ();
  void handle_output (const char *text, int len);
  void copy_text (bool to_screen, bool to_field);
  void field_init () { field.set (0); }
  void send_str (const char *str, int len);
  void send_vfmt (const char *fmt, va_list args);
  void lock_cmd_queue ();
  void unlock_cmd_queue ();
  void reset_hev_done ();
  void post_hev_done ();
  void wait_hev_done ();

  struct cmd_queue
  {
    struct cmd_queue *next;
    fstring cmd;
  };

  command_window *cmd;
  capture *capture_head, *capture_cur;
  capture *frame_head, *frame_cur;
  capture *display_head, *display_cur;
  astring field;
  astring output;
  bool ignore_prompt;
  bool ignore_output;
  bool show_annotations;
  bool breakpoints_invalid;
  bool breakpoints_invalid_pending;
  bool exec_file_pending;
  bool stopped_pending;
  bool call_flag;
  gsp gdb_prompt;
  gsr gdb_running;

  _fmutex mutex_cmd_queue;
  HEV hev_done;
  int to_gdb;
  cmd_queue *cmd_head, **cmd_add;

  friend command_window;
};
