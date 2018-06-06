/* annotati.h -*- C++ -*-
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


class annotation
{
public:

  enum code
  {
    UNKNOWN,
    TEXT,
    READ_ERROR,
    arg_begin,
    arg_end,
    arg_name_end,
    arg_value,
    array_section_begin,
    array_section_end,
    breakpoint,
    breakpoint_new,
    breakpoints_headers,
    breakpoints_table,
    breakpoints_table_end,
    breakpoints_invalid,
    commands,
    display_begin,
    display_delete,
    display_disable,
    display_enable,
    display_end,
    display_expression,
    display_expression_end,
    display_format,
    display_number_end,
    display_value,
    error_begin,
    error,
    elt,
    elt_rep,
    elt_rep_end,
    exec_file,
    exec_file_invalid,
    exited,
    field,
    field_begin,
    field_end,
    field_name_end,
    field_value,
    frame,
    frame_args,
    frame_begin,
    frame_end,
    frame_function_name,
    frame_source_begin,
    frame_source_end,
    frame_source_file,
    frame_source_file_end,
    frame_source_line,
    frames_invalid,
    overload_choice,
    prompt,
    prompt_for_continue,
    pre_commands,
    pre_overload_choice,
    pre_prompt,
    pre_prompt_for_continue,
    pre_query,
    post_commands,
    post_overload_choice,
    post_prompt,
    post_prompt_for_continue,
    post_query,
    query,
    quit,
    record,
    show_value,
    show_value_end,
    starting,
    stopped,
    signalled,
    source,
    source_file,
    source_location,
    source_location_end,
    signal,
    signal_handler_caller,
    signal_name,
    signal_name_end,
    signal_string,
    signal_string_end,
    thread_add,
    thread_disable,
    thread_enable,
    thread_end,
    thread_switch,
    value_begin,
    value_end,
    value_history_begin,
    value_history_end,
    value_history_value,
    watchpoint
  };

  annotation ();
  ~annotation ();
  void start (int fd);
  code get_next ();
  code get_code ()        const { return last_code; }
  const char *get_text () const { return last_text; }
  int get_text_len ()     const { return last_text_len; }
  const char *get_args () const { return last_args; }
  int get_args_len ()     const { return last_args_len; }

private:

  enum
  {
    hash_size = 97
  } constants;

  struct node;

  unsigned hash (const char *s, int len);
  void init (code c, const char *s);
  void parse (const char *s, int len);
  bool fill ();

  int fd;
  node *table[hash_size];
  code last_code;
  const char *last_text, *last_args;
  int last_args_len, last_text_len;
  int buf_size;
  int buf_in, buf_out;
  char *buf;
};
