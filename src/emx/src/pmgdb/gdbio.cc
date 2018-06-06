/* gdbio.cc
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


#define INCL_DOS
#define INCL_DOSERRORS
#include <os2.h>
#include <stdio.h>              // vsnprintf()
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <signal.h>
#include <io.h>
#include <sys/ptrace.h>
#include "string.h"
#include "pmapp.h"
#include "pmframe.h"
#include "pmtxt.h"
#include "pmtty.h"
#include "breakpoi.h"
#include "display.h"
#include "threads.h"
#include "annotati.h"
#include "capture.h"
#include "gdbio.h"
#include "pmdebug.h"
#include "command.h"


gdbio::gdbio (command_window *in_cmd)
{
  cmd = in_cmd;
  ignore_prompt = false; ignore_output = false;
  show_annotations = false;
  breakpoints_invalid = false; breakpoints_invalid_pending = false;
  exec_file_pending = false; stopped_pending = false;
  call_flag = false;
  capture_head = capture_cur = NULL;
  display_head = display_cur = NULL;
  frame_head = frame_cur = NULL;
  gdb_prompt = GSP_NOTREADY;
  gdb_running = GSR_NONE;
  cmd_head = NULL; cmd_add = &cmd_head;
  to_gdb = -1;
  _fmutex_checked_create (&mutex_cmd_queue, 0);
  if (DosCreateEventSem (NULL, &hev_done, 0, FALSE) != 0)
    abort ();
}


gdbio::~gdbio ()
{
}


void gdbio::lock_cmd_queue ()
{
  _fmutex_checked_request (&mutex_cmd_queue, _FMR_IGNINT);
}


void gdbio::unlock_cmd_queue ()
{
  _fmutex_checked_release (&mutex_cmd_queue);
}


void gdbio::reset_hev_done ()
{
  ULONG post_count;
  ULONG rc = DosResetEventSem (hev_done, &post_count);
  if (rc != 0 && rc != ERROR_ALREADY_RESET) abort ();
}


void gdbio::post_hev_done ()
{
  ULONG rc = DosPostEventSem (hev_done);
  if (rc != 0) abort ();
}


void gdbio::wait_hev_done ()
{
  ULONG rc;

  do
    {
      rc = DosWaitEventSem (hev_done, SEM_INDEFINITE_WAIT);
    } while (rc == ERROR_INTERRUPT);
  if (rc != 0) abort ();
}


void gdbio::fetch ()
{
  if (get_next () == READ_ERROR)
    {
      // TODO: Message box?
      DosExit (EXIT_THREAD, 0);
    }
  if (show_annotations && get_code () != TEXT)
    cmd->show_annotation (this);
}


void gdbio::handle_output (const char *text, int len)
{
  if (!ignore_output)
    {
      cmd->handle_output (text, len);
      ignore_prompt = false;
    }
}


void gdbio::copy_text (bool to_screen, bool to_field)
{
  if (to_field)
    field_init ();
  for (;;)
    {
      fetch ();
      if (get_code () != TEXT)
        break;
      if (to_field)
        field.append (get_text (), get_text_len ());
      if (to_screen)
        handle_output (get_text (), get_text_len ());
    }
}


void gdbio::parse_gdb ()
{
  char *end;
  long n;
  int pid;

  fetch ();
  for (;;)
    {
      switch (get_code ())
        {
        case TEXT:
          handle_output (get_text (), get_text_len ());
          // TODO: not always needed, add arg to capture_cmd()
          if (capture_head != NULL)
            output.append (get_text (), get_text_len ());
          fetch ();
          break;

        case breakpoint_new:
          // Note: add to head!
          // TODO: strtol()
          if (capture_head != NULL)
            capture_head->bpt_number.set (atoi (get_args ()));
          fetch ();
          break;

        case breakpoints_table:
          parse_breakpoints_table ();
          break;

        case annotation::breakpoints_invalid:
          breakpoints_invalid = true;
          breakpoints_invalid_pending = true;
          fetch ();
          break;

        case display_begin:
          parse_display_begin ();
          break;

        case display_delete:
          if (cmd->dsp != NULL)
            cmd->dsp->remove (atoi (get_args ()));
          fetch ();
          break;

        case display_disable:
          if (cmd->dsp != NULL)
            cmd->dsp->disable (atoi (get_args ()));
          fetch ();
          break;

        case display_enable:
          if (cmd->dsp != NULL)
            cmd->dsp->enable (atoi (get_args ()));
          fetch ();
          break;

        case error_begin:
          parse_error_begin ();
          break;

        case error:
          cmd->set_fg_color ();
          fetch ();
          break;

        case exec_file:
          exec_file_pending = true;
          cmd->exec_file (get_args (), get_args_len ());
          fetch ();
          break;

        case exec_file_invalid:
          cmd->exec_file (NULL, 0);
          fetch ();
          break;

        case exited:
          gdb_running = GSR_NONE;
          cmd->pmdbg_term ();
          cmd->notify (command_window::GSC_RUNNING);
          fetch ();
          break;

        case frame_begin:
          parse_frame_begin ();
          break;

        case pre_commands:
        case pre_overload_choice:
        case pre_prompt:
        case pre_prompt_for_continue:
        case pre_query:
          parse_pre_prompt ();
          break;

        case show_value:
          copy_text (true, capture_head != NULL);
          if (get_code () == show_value_end)
            {
              if (capture_head != NULL)
                capture_head->show_value.set (field, field.length ());
              fetch ();
            }
          break;

        case starting:
          gdb_running = GSR_RUNNING;
          // TODO: This may be too late
          cmd->pmdbg_start ();
          cmd->notify (command_window::GSC_RUNNING);
          fetch ();
          break;

        case stopped:
          if (gdb_running == GSR_RUNNING)
            {
              gdb_running = GSR_STOPPED;
              cmd->pmdbg_stop ();
            }
          stopped_pending = true;
          cmd->notify (command_window::GSC_RUNNING);
          fetch ();
          break;

        case signalled:
          gdb_running = GSR_NONE;
          cmd->pmdbg_term ();
          cmd->notify (command_window::GSC_RUNNING);
          fetch ();
          break;

        case source:
          parse_source ();
          fetch ();
          break;

        case source_file:
          if (capture_cur != NULL)
            {
              if (capture_cur->srcs_file.is_set ())
                {
                  capture_cur->next = new capture;
                  capture_cur = capture_cur->next;
                }
              capture_cur->srcs_file.set (get_args ());
            }
          fetch ();
          break;

        case source_location:
          parse_source_location ();
          break;

        case thread_add:
          n = strtol (get_args (), &end, 10);
          if (*end == ' ')
            {
              pid = atoi (end);
              if (PTRACE_GETTID (pid) == 1)
                pmdbg.set_pid (PTRACE_GETPID (pid));
              cmd->thr->add ((int)n, pid);
            }
          fetch ();
          break;

        case thread_disable:
          cmd->thr->disable (atoi (get_args ()));
          fetch ();
          break;

        case thread_enable:
          cmd->thr->enable (atoi (get_args ()));
          fetch ();
          break;

        case thread_end:
          cmd->thr->remove (atoi (get_args ()));
          fetch ();
          break;

        case thread_switch:
          cmd->thr->select (atoi (get_args ()));
          fetch ();
          break;

        default:
          fetch ();
          break;
        }
    }
}


// Handle `source FILENAME;LINE;CHARACTER;MIDDLE;ADDR' annotation

#define SOURCE_SEP      ';'

// TODO: `info line' typed by user must not update source window!
void gdbio::parse_source ()
{
  const char *args = get_args ();
  const char *p;
  char *filename, *end;
  long lineno, addr;
  int len;

  // FILENAME
  p = strchr (args, SOURCE_SEP);
  if (p == NULL) return;
  len = p - args;
  filename = (char *)alloca (len + 1);
  memcpy (filename, args, len);
  filename[len] = 0;

  // LINE
  lineno = strtol (p + 1, &end, 10);
  if (lineno <= 0 || *end != SOURCE_SEP) return;

  // CHARACTER
  p = strchr (end + 1, SOURCE_SEP);
  if (p == NULL) return;

  // MIDDLE
  p = strchr (p + 1, SOURCE_SEP);
  if (p == NULL) return;

  // ADDR
  addr = strtol (p + 1, &end, 16);
  if (end == p + 1 || *end != 0) return;

  capture *capt;
  if (capture_head != NULL)
    capt = capture_head;
  else
    {
      if (frame_head == NULL)
        frame_head = frame_cur = new capture;
      capt = frame_head;
    }
  capt->source_file.set (filename, len);
  capt->source_lineno.set (lineno);
  capt->source_addr.set (addr);
}


void gdbio::parse_breakpoints_table ()
{
  long n = -1;                  // Keep the compiler happy
  char *end;

  fetch ();
  if (capture_head == NULL)
    return;
  while (get_code () == record)
    {
      capture_cur->next = new capture;
      capture_cur = capture_cur->next;
      fetch ();
      while (get_code () == annotation::field)
        {
          int number = atoi (get_args ());
          copy_text (true, true);
          switch (number)
            {
            case 0:
              // NUMBER: "1  "
              capture_cur->bpt_number.set (atoi (field));
              break;
            case 1:
              // TYPE: "breakpoint  "
              break;
            case 2:
              // DISPOSITION: "keep "
              field.remove_trailing (' ');
              capture_cur->bpt_disposition.set (field, field.length ());
              break;
            case 3:
              // ENABLE: "y  "
              capture_cur->bpt_enable.set (field[0] == 'y');
              break;
            case 4:
              // ADDRESS: "0x100b2 "
              // TODO: This field is language-dependent
              n = strtol (field, &end, 16);
              if (end != (const char *)field && (*end == ' ' || *end == 0))
                capture_cur->bpt_address.set (n);
              break;
            case 5:
              // WHAT: "in main at source.c:217"
              capture_cur->bpt_what.set (field, field.length ());
              break;
            case 6:
              // FRAME: TODO
              break;
            case 7:
              // CONDITION: "\tstop only if i >= 2\n"
              field.remove_trailing ('\n');
              if (strncmp (field, "\tstop only if ", 14) == 0)
                capture_cur->bpt_condition.set ((const char *)field + 14,
                                                field.length () - 14);
              break;
            case 8:
              // IGNORE-COUNT: "\tignore next X hits\n"
              if (strncmp (field, "\tignore next ", 13) == 0
                  && (n = strtol ((const char *)field + 13, &end, 10)) >= 1)
                capture_cur->bpt_ignore.set (n);
              break;
            case 9:
              // COMMANDS: TODO
              break;
            }
        }
    }
  if (get_code () == breakpoints_table_end)
    fetch ();
}


void gdbio::parse_display_begin ()
{
  bool to_screen = frame_head == NULL;
  capture *capt = new capture;
  if (display_cur == NULL)
    display_head = capt;
  else
    display_cur->next = capt;
  display_cur = capt;

  // display-begin NUMBER display-number-end
  copy_text (to_screen, true);
  if (get_code () != display_number_end)
    return;
  char *end;
  long n = strtol (field, &end, 10);
  if (end == (const char *)field || *end != 0)
    return;
  display_cur->disp_number.set (n);
  // For adding displays, see IDM_ADD of display_window
  if (capture_head != NULL)
    capture_head->disp_number.set (n);

  // NUMBER-SEPARATOR display-format FORMAT display-expression
  copy_text (to_screen, false);
  if (get_code () != display_format)
    return;
  copy_text (to_screen, true);
  if (get_code () != display_expression)
    return;
  field.remove_trailing (' ');
  display_cur->disp_format.set (field, field.length ());

  // EXPRESSION display-expression-end
  copy_text (to_screen, true);
  if (get_code () == error_begin)
    {
      display_cur->disp_expr.set ("*ERROR*");
      display_cur->disp_value.set ("");
      parse_error_begin ();
      display_cur->disp_enable.set (false);
      return;
    }
  if (get_code () != display_expression_end)
    return;
  display_cur->disp_expr.set (field, field.length ());

  // EXPRESSION-SEPARATOR display-value VALUE display-end
  copy_text (to_screen, false);
  if (get_code () != display_value)
    return;
  parse_value (to_screen, true);
  if (get_code () != display_end)
    return;
  field.remove_trailing ('\n');
  display_cur->disp_value.set (field, field.length ());
  display_cur->disp_enable.set (true);
  fetch ();
}


void gdbio::parse_value (bool to_screen, bool to_field)
{
  if (to_field)
    field_init ();
  bool more = true;
  while (more)
    {
      fetch ();
      switch (get_code ())
        {
        case TEXT:
          if (to_field)
            field.append (get_text (), get_text_len ());
          if (to_screen)
            handle_output (get_text (), get_text_len ());
          break;
        case array_section_begin:
        case array_section_end:
        case elt:
        case elt_rep:
        case elt_rep_end:
        case field_begin:
        case field_name_end:
        case field_value:
        case field_end:
          break;
        default:
          more = false;
          break;
        }
    }
}


void gdbio::parse_error_begin ()
{
  cmd->set_fg_color (CLR_RED);
  copy_text (true, capture_head != NULL);
  if (get_code () == quit || get_code () == error)
    {
      if (capture_head != NULL)
        {
          field.remove_trailing ('\n');
          capture_head->error.set (field, field.length ());
        }
      fetch ();
    }
  cmd->set_fg_color ();
}


void gdbio::parse_source_location ()
{
  copy_text (true, capture_head != NULL);
  if (get_code () == source_location_end)
    {
      if (capture_head != NULL)
        capture_head->source_location.set (field, field.length ());
      fetch ();
    }
}


void gdbio::parse_frame_begin ()
{
  capture *capt = new capture;
  if (frame_cur == NULL)
    frame_head = capt;
  else
    frame_cur->next = capt;
  frame_cur = capt;
  // TODO: parse
  fetch ();
}


void gdbio::parse_pre_prompt ()
{
  cmd->set_fg_color (CLR_BLUE);
  copy_text (!ignore_prompt, false);
  switch (get_code ())
    {
    case commands:
    case overload_choice:
    case prompt_for_continue:
    case query:
      if (gdb_prompt != GSP_OTHER)
        {
          gdb_prompt = GSP_OTHER;
          cmd->set_fg_color (CLR_DARKBLUE);
          cmd->notify (command_window::GSC_PROMPT);
        }
      break;

    case prompt:
      if (cmd_head != NULL)
        {
          lock_cmd_queue ();
          cmd_queue *q = cmd_head;
          cmd_head = cmd_head->next;
          if (cmd_head == NULL)
            cmd_add = &cmd_head;
          unlock_cmd_queue ();
          ignore_prompt = true;
          send_str (q->cmd, strlen (q->cmd));
          delete q;
        }
      else
        {
          gdb_prompt = GSP_CMD;
          ignore_prompt = false;
          unsigned change = command_window::GSC_PROMPT;
          cmd->set_fg_color (CLR_DARKBLUE);
          if (call_flag)
            {
              // Note that this is done before posting UWM_STATE, so
              // that, say, a new display number can be picked up
              // before updating the displays
              post_hev_done ();
            }
          if (breakpoints_invalid_pending)
            {
              breakpoints_invalid_pending = false;
              change |= command_window::GSC_BREAKPOINTS;
            }
          if (exec_file_pending)
            {
              exec_file_pending = false;
              change |= command_window::GSC_EXEC_FILE;
            }
          cmd->notify (change);
        }
      break;

    default:
      cmd->set_fg_color ();
      ignore_prompt = false;
      return;
    }

  // input
  copy_text (true, false);
  switch (get_code ())
    {
    case post_commands:
    case post_overload_choice:
    case post_prompt:
    case post_prompt_for_continue:
    case post_query:
      gdb_prompt = GSP_NONE;
      cmd->set_fg_color ();
      cmd->notify (command_window::GSC_PROMPT);
      fetch ();
      break;

    default:
      cmd->set_fg_color ();
      break;
    }
}


void gdbio::prepare_run ()
{
  if (frame_head != NULL)
    {
      delete_capture (frame_head);
      frame_head = NULL; frame_cur = NULL;
    }
}


static void sigpipe_handler (int)
{
}


void gdbio::send_init (int fd)
{
  to_gdb = fd;
}


void gdbio::send_init ()
{
  struct sigaction sa;

  sa.sa_handler = sigpipe_handler;
  sa.sa_flags = 0;
  sigemptyset (&sa.sa_mask);
  sigaction (SIGPIPE, &sa, NULL);
}


void gdbio::send_str (const char *str, int len)
{
  write (to_gdb, str, len);
}


void gdbio::send_vfmt (const char *fmt, va_list args)
{
  char buf[512];

  int len = vsnprintf (buf, sizeof (buf) - 1, fmt, args);
  if (len >= (int)sizeof (buf) - 1)
    abort ();
  buf[len] = '\n';
  buf[len+1] = 0;
  send_str (buf, len + 1);
}


bool gdbio::send_cmd (const char *fmt, ...)
{
  va_list arg_ptr;

  if (!is_ready () || cmd_head != NULL)
    {
      DosBeep (400, 200);
      return false;
    }
  gdb_prompt = GSP_NOTREADY;
  ignore_prompt = true;
  va_start (arg_ptr, fmt);
  send_vfmt (fmt, arg_ptr);
  va_end (arg_ptr);
  return true;
}


void gdbio::queue_cmd (const char *fmt, ...)
{
  va_list arg_ptr;

  char buf[512];
  va_start (arg_ptr, fmt);
  int len = vsnprintf (buf, sizeof (buf) - 1, fmt, arg_ptr);
  va_end (arg_ptr);
  if (len >= (int)sizeof (buf) - 1)
    abort ();
  buf[len] = '\n';
  buf[len+1] = 0;

  lock_cmd_queue ();
  if (is_ready () && cmd_head == NULL)
    {
      gdb_prompt = GSP_NOTREADY;
      unlock_cmd_queue ();
      ignore_prompt = true;
      send_str (buf, len + 1);
    }
  else
    {
      cmd_queue *q = new cmd_queue;
      q->cmd.set (buf, len + 1);
      q->next = NULL;
      *cmd_add = q; cmd_add = &q->next;
      unlock_cmd_queue ();
    }
}


bool gdbio::call_cmd (const char *fmt, ...)
{
  va_list arg_ptr;

  // TODO: mutex
  if (!is_ready () || cmd_head != NULL)
    return false;

  gdb_prompt = GSP_NOTREADY;
  ignore_prompt = true; ignore_output = true; call_flag = true;
  output.set (0);
  reset_hev_done ();
  va_start (arg_ptr, fmt);
  send_vfmt (fmt, arg_ptr);
  va_end (arg_ptr);
  wait_hev_done ();
  ignore_output = false; call_flag = false;
  return true;
}


capture * gdbio::capture_cmd (const char *fmt, ...)
{
  va_list arg_ptr;

  // TODO: mutex
  if (!is_ready () || capture_head != NULL || cmd_head != NULL)
    return NULL;

  gdb_prompt = GSP_NOTREADY;
  ignore_prompt = true; ignore_output = true; call_flag = true;
  capture_head = capture_cur = new capture;
  output.set (0);
  reset_hev_done ();
  va_start (arg_ptr, fmt);
  send_vfmt (fmt, arg_ptr);
  va_end (arg_ptr);
  wait_hev_done ();
  capture *tmp = capture_head;
  capture_head = capture_cur = NULL;
  ignore_output = false; call_flag = false;
  return tmp;
}


ULONG gdbio::address_of_line (const char *fname, int lineno)
{
  capture *capt = capture_cmd ("server info line %s:%d", fname, lineno);
  if (capt == NULL)
    return 0;
  ULONG addr = 0;
  if (capt->source_addr.is_set ())
    addr = (ULONG)capt->source_addr.get ();
  delete_capture (capt);
  return addr;
}


bool gdbio::get_setshow_boolean (const char *name)
{
  capture *capt = capture_cmd ("server show %s", name);
  if (capt == NULL)
    return false;
  bool result = (capt->show_value.is_set ()
                 && strcmp (capt->show_value.get (), "on") == 0);
  delete_capture (capt);
  return result;
}


void gdbio::set_setshow_boolean (const char *name, bool value)
{
  queue_cmd ("server set %s %s", name, value ? "on" : "off");
}


char *gdbio::get_setshow_string (const char *name)
{
  capture *capt = capture_cmd ("server show %s", name);
  if (capt == NULL)
    return NULL;
  char *result = NULL;
  if (capt->show_value.is_set ())
    {
      const char *s = capt->show_value.get ();
      size_t len = strlen (s);
      if (len >= 2 && s[0] == '"' && s[len-1] == '"')
        {
          ++s; len -= 2;
        }
      result = new char[len + 1];
      memcpy (result, s, len);
      result[len] = 0;
    }
  delete_capture (capt);
  return result;
}


void gdbio::set_setshow_string (const char *name, const char *value)
{
  queue_cmd ("server set %s %s", name, value);
}
