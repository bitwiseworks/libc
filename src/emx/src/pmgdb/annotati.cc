/* annotati.cc
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


#include <os2.h>
#include <string.h>
#include <io.h>
#include "annotati.h"


struct annotation::node
{
  node *next;
  char *str;
  int len;
  annotation::code c;
};


annotation::annotation ()
{
  buf = NULL; buf_size = 0; buf_in = buf_out = 0;
  for (int i = 0; i < hash_size; ++i)
    table[i] = NULL;
  init (arg_begin, "arg-begin");
  init (arg_end, "arg-end");
  init (arg_name_end, "arg-name-end");
  init (arg_value, "arg-value");
  init (array_section_begin, "array-section-begin");
  init (array_section_end, "array-section-end");
  init (breakpoint, "breakpoint");
  init (breakpoint_new, "breakpoint-new");
  init (breakpoints_headers, "breakpoints-headers");
  init (breakpoints_table, "breakpoints-table");
  init (breakpoints_table_end, "breakpoints-table-end");
  init (breakpoints_invalid, "breakpoints-invalid");
  init (commands, "commands");
  init (display_begin, "display-begin");
  init (display_delete, "display-delete");
  init (display_disable, "display-disable");
  init (display_enable, "display-enable");
  init (display_end, "display-end");
  init (display_expression, "display-expression");
  init (display_expression_end, "display-expression-end");
  init (display_format, "display-format");
  init (display_number_end, "display-number-end");
  init (display_value, "display-value");
  init (error_begin, "error-begin");
  init (error, "error");
  init (elt, "elt");
  init (elt_rep, "elt-rep");
  init (elt_rep_end, "elt-rep-end");
  init (exec_file, "exec-file");
  init (exec_file_invalid, "exec-file-invalid");
  init (exited, "exited");
  init (field, "field");
  init (field_begin, "field-begin");
  init (field_end, "field-end");
  init (field_name_end, "field-name-end");
  init (field_value, "field-value");
  init (frame, "frame");
  init (frame_args, "frame-args");
  init (frame_begin, "frame-begin");
  init (frame_end, "frame-end");
  init (frame_function_name, "frame-function-name");
  init (frame_source_begin, "frame-source-begin");
  init (frame_source_end, "frame-source-end");
  init (frame_source_file, "frame-source-file");
  init (frame_source_file_end, "frame-source-file-end");
  init (frame_source_line, "frame-source-line");
  init (frames_invalid, "frames-invalid");
  init (overload_choice, "overload-choice");
  init (prompt, "prompt");
  init (prompt_for_continue, "prompt-for-continue");
  init (pre_commands, "pre-commands");
  init (pre_overload_choice, "pre-overload-choice");
  init (pre_prompt, "pre-prompt");
  init (pre_prompt_for_continue, "pre-prompt-for-continue");
  init (pre_query, "pre-query");
  init (post_commands, "post-commands");
  init (post_overload_choice, "post-overload-choice");
  init (post_prompt, "post-prompt");
  init (post_prompt_for_continue, "post-prompt-for-continue");
  init (post_query, "post-query");
  init (query, "query");
  init (quit, "quit");
  init (record, "record");
  init (show_value, "show-value");
  init (show_value_end, "show-value-end");
  init (starting, "starting");
  init (stopped, "stopped");
  init (signalled, "signalled");
  init (source, "source");
  init (source_file, "source-file");
  init (source_location, "source-location");
  init (source_location_end, "source-location-end");
  init (signal, "signal");
  init (signal_handler_caller, "signal-handler-caller");
  init (signal_name, "signal-name");
  init (signal_name_end, "signal-name-end");
  init (signal_string, "signal-string");
  init (signal_string_end, "signal-string-end");
  init (thread_add, "thread-add");
  init (thread_disable, "thread-disable");
  init (thread_enable, "thread-enable");
  init (thread_end, "thread-end");
  init (thread_switch, "thread-switch");
  init (value_begin, "value-begin");
  init (value_end, "value-end");
  init (value_history_begin, "value-history-begin");
  init (value_history_end, "value-history-end");
  init (value_history_value, "value-history-value");
  init (watchpoint, "watchpoint");
}


annotation::~annotation ()
{
  node *next;
  for (int h = 0; h < hash_size; ++h)
    for (node *p = table[h]; p != NULL; p = next)
      {
        next = p->next;
        delete[] p->str;
        delete p;
      }
  delete[] buf;
}


unsigned annotation::hash (const char *s, int len)
{
  unsigned h = 0;
  for (int i = 0; i < len; ++i)
    h = (h << 1) ^ (unsigned char)s[i];
  return h % hash_size;
}


void annotation::init (code c, const char *s)
{
  int len = strlen (s);
  unsigned h = hash (s, len);
  node *n = new node;
  n->c = c;
  n->len = len;
  n->str = new char[len + 1];
  memcpy (n->str, s, len + 1);
  n->next = table[h];
  table[h] = n;
}


void annotation::start (int in_fd)
{
  fd = in_fd;
}


void annotation::parse (const char *s, int len)
{
  const char *args = (const char *)memchr (s, ' ', len);
  int keyword_len = args == NULL ? len : args - s;
  unsigned h = hash (s, keyword_len);
  last_code = UNKNOWN; last_args = s; last_args_len = len;
  const node *n;
  for (n = table[h]; n != NULL; n = n->next)
    if (n->len == keyword_len && memcmp (n->str, s, keyword_len) == 0)
      break;
  if (n != NULL)
    {
      last_code = n->c;
      if (args != NULL)
        {
          last_args = args + 1;
          last_args_len = len - (keyword_len + 1);
        }
      else
        {
          last_args = NULL;
          last_args_len = 0;
        }
    }
}


bool annotation::fill ()
{
  if (buf_in == buf_out)
    buf_in = buf_out = 0;
  if (buf_in == buf_size)
    {
      buf_size += 512;
      char *new_buf = new char [buf_size];
      memcpy (new_buf, buf, buf_in);
      delete[] buf;
      buf = new_buf;
    }
  int n = read (fd, buf + buf_in, buf_size - buf_in);
  if (n <= 0)
    return false;
  buf_in += n;
  return true;
}


annotation::code annotation::get_next ()
{
  if (buf_in == buf_out && !fill ())
    return READ_ERROR;
  const char *nl;
  int text_len = 0;
  for (;;)
    {
      const char *start = buf + buf_out + text_len;
      nl = (const char *)memchr (start, '\n', buf_in - (buf_out + text_len));
      if (nl == NULL)
        {
          text_len = buf_in - buf_out;
          break;
        }
      text_len += (nl - start);
      int pos = nl - buf;

      if (pos + 1 >= buf_in && !fill ())
        return READ_ERROR;
      if (buf[pos+1] != 0x1a)
        ++text_len;             // Beware of \n\n^Z^Z!
      else
        {
          if (pos + 2 >= buf_in && !fill ())
            return READ_ERROR;
          if (buf[pos+2] != 0x1a)
            ++text_len;         // Beware of \n^Z\n^Z^Z
          else
            break;              // Annotation found
        }
    }

  if (text_len != 0)
    {
      last_code = TEXT;
      last_text = buf + buf_out; last_text_len = text_len;
      last_args = NULL; last_args_len = 0;
      buf_out += text_len;
      return last_code;
    }

  while ((nl = (const char *)memchr (buf + buf_out + 3, '\n',
                                     buf_in - (buf_out + 3))) == NULL)
    if (!fill ())
      return READ_ERROR;
  last_text = buf + buf_out + 3;
  last_text_len = nl - last_text;
  parse (last_text, last_text_len);
  buf[buf_out + 3 + last_text_len] = 0;
  buf_out = (nl + 1) - buf;
  return last_code;
}
