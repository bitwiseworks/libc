/* command.cc
   Copyright (c) 1996, 1998 Eberhard Mattes

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
#define INCL_WIN
#include <os2.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <process.h>
#include <signal.h>
#include <io.h>
#include <fcntl.h>
#include "string.h"
#include "pmapp.h"
#include "pmframe.h"
#include "pmtxt.h"
#include "pmtty.h"
#include "pmgdb.h"
#include "help.h"
#include "breakpoi.h"
#include "display.h"
#include "threads.h"
#include "register.h"
#include "source.h"
#include "srcfiles.h"
#include "annotati.h"
#include "capture.h"
#include "gdbio.h"
#include "pmdebug.h"
#include "command.h"

static HWND hwndFatal;

struct command_window::new_source
{
  char *short_filename;
  char *long_filename;
  int lineno;
  bool set_focus;
};

struct command_window::cmd_hist
{
  struct cmd_hist *next;
  struct cmd_hist *prev;
  fstring cmd;
};

void command_window_thread (void *arg)
{
  ((struct command_window *)arg)->thread ();
}


// Note: This signal handler is called in thread 1, which has a
// PM message queue.

static void sigchld_handler (int)
{
  // TODO: Offer to save settings
  // TODO: WinMessageBox has a message loop and can therefore cause
  //       a reentrancy problem
  WinMessageBox (HWND_DESKTOP, HWND_DESKTOP, (PSZ)"GDB terminated.",
                 (PSZ)"pmgdb", 0,
                 MB_MOVEABLE | MB_OK | MB_ICONEXCLAMATION);
  // TODO: Get and return GDB's termination code
  exit (0);
}


command_window::command_window (pmapp *in_app, unsigned in_id,
                                const char *fontnamesize, char **in_argv)
  : pmtty (in_app, in_id, FCF_TASKLIST, NULL, fontnamesize)
{
  SWP swp_cmd, swp_dsp, swp_thr, swp_srcs;

  hwndFatal = get_hwndClient ();
  _fmutex_checked_create (&mutex, 0);

  swp_srcs.cx = in_app->get_screen_width () / 4;
  swp_srcs.cy = in_app->get_screen_height () / 4;
  swp_srcs.x = in_app->get_screen_width () - swp_srcs.cx;
  swp_srcs.y = in_app->get_screen_height () - swp_srcs.cy;

  swp_cmd.cx = swp_srcs.x;
  swp_cmd.cy = swp_srcs.cy;
  swp_cmd.x = 0;
  swp_cmd.y = swp_srcs.y;
  WinSetWindowPos (get_hwndFrame (), HWND_DESKTOP, swp_cmd.x, swp_cmd.y,
                   swp_cmd.cx, swp_cmd.cy, SWP_SIZE | SWP_MOVE);

  WinQueryWindowPos (get_hwndFrame (), &swp_cmd);
  swp_srcs.x = swp_cmd.x + swp_cmd.cx;
  swp_srcs.cx = in_app->get_screen_width () - swp_srcs.x;

  swp_thr.x = swp_srcs.x;
  swp_thr.y = swp_srcs.y - swp_srcs.cy;
  swp_thr.cx = swp_srcs.cx;
  swp_thr.cy = swp_srcs.cy;

  swp_dsp.x = 0;
  swp_dsp.y = swp_thr.y;
  swp_dsp.cx = swp_cmd.cx;
  swp_dsp.cy = swp_thr.cy;

  help_ok = help_init (HELP_TABLE, "pmgdb Help", "pmgdb.hlp", HELP_CMD_KEYS);
  if (!help_ok)
    {
      WinMessageBox (HWND_DESKTOP, HWND_DESKTOP,
                     (PSZ)"pmgdb.hlp not found; please set HELP correctly "
                     "in config.sys.", (PSZ)"pmgdb", 0,
                     MB_MOVEABLE | MB_OK | MB_ICONEXCLAMATION);
      help_disable (this);
    }

  show (true);
  src_list = NULL; src_menu_id_used = NULL;
  src_menu_id_count = 0;
  history = NULL; cur_cmd = NULL;
  gdb = new gdbio (this);
  cursor (true);

  argv = NULL; argv_used = 0; argv_size = 0;
  arg_add ("gdb.exe");
  arg_add ("--annotate=2");
  arg_add ("--args");
  for (int i = 0; in_argv[i] != NULL; ++i)
    arg_add (in_argv[i]);
  arg_add (NULL);

  srcs = new source_files_window (this, ID_SOURCE_FILES_WINDOW, &swp_srcs,
                                  fontnamesize);
  if (!help_ok) help_disable (srcs);
  thr = new threads_window (this, gdb, ID_THREADS_WINDOW, &swp_thr,
                            fontnamesize);
  if (!help_ok) help_disable (thr);
  brk = new breakpoints_window (this, gdb, ID_BREAKPOINTS_WINDOW,
                                fontnamesize);
  if (!help_ok) help_disable (brk);
  dsp = new display_window (this, gdb, ID_DISPLAY_WINDOW, &swp_dsp,
                            fontnamesize);
  if (!help_ok) help_disable (dsp);
  reg = new register_window (this, ID_REGISTER_WINDOW, NULL, fontnamesize);
  if (!help_ok) help_disable (reg);

  notify (GSC_RUNNING | GSC_EXEC_FILE);

  gdb->send_init ();
  gdb->queue_cmd ("server set height 0");

  struct sigaction sa;
  sa.sa_handler = sigchld_handler;
  sa.sa_flags = 0;
  sigemptyset (&sa.sa_mask);
  sigaction (SIGCHLD, &sa, NULL);

  if (_beginthread (command_window_thread, NULL, 0x8000, (void *)this) == -1)
    abort ();
}


command_window::~command_window ()
{
  // TODO: delete cmd_head...
  pmdbg.exit ();
  delete brk;
  delete dsp;
  delete thr;
  delete srcs;
  delete reg;
  src_node *s, *snext;
  for (s = src_list; s != NULL; s = snext)
    {
      snext = s->next;
      delete s->src;
      delete s;
    }
  delete[] src_menu_id_used;
  arg_delete_all ();
  cmd_hist *h, *hprev;
  for (h = history; h != NULL; h = hprev)
    {
      hprev = h->prev;
      delete h;
    }

  // TODO: mutex
}


void command_window::arg_delete_all ()
{
  for (int i = 0; argv[i] != NULL; ++i)
    delete[] argv[i];
  delete[] argv;
  argv = NULL; argv_used = 0; argv_size = 0;
}


void command_window::arg_add (const char *str)
{
  if (argv_used >= argv_size)
    {
      int new_size = argv_size + 16;
      char **new_argv = new char *[new_size];
      for (int i = 0; i < argv_size; ++i)
        new_argv[i] = argv[i];
      for (int i = argv_size; i < new_size; ++i)
        new_argv[i] = NULL;
      delete[] argv;
      argv = new_argv; argv_size = new_size;
    }

  char *p;
  if (str == NULL)
    p = NULL;
  else
    {
      p = new char[strlen (str) + 1];
      strcpy (p, str);
    }
  argv[argv_used++] = p;
}


void command_window::lock ()
{
  _fmutex_checked_request (&mutex, _FMR_IGNINT);
}


void command_window::unlock ()
{
  _fmutex_checked_release (&mutex);
}


void command_window::capture_error (const capture *p)
{
  WinMessageBox (HWND_DESKTOP, HWND_DESKTOP,
                 (PSZ)p->error.get (), (PSZ)"pmgdb", 0,
                 MB_MOVEABLE | MB_OK | MB_ICONEXCLAMATION);
}


void command_window::get_breakpoints ()
{
  breakpoint_list old;
  old.steal (breakpoints);
  capture *capt = gdb->capture_cmd ("server info breakpoints");
  if (capt != NULL)
    {
      for (capture *p = capt->next; p != NULL; p = p->next)
        {
          breakpoint bpt;
          if (bpt.set_from_capture (*p))
            breakpoints.add (bpt);
        }
      gdb->breakpoints_invalid = false;
      delete_capture (capt);
    }

  breakpoints.update (old, this, &brkpt_update);

  // OLD goes out of scope here; that will call delete_all()
}


// Callback function for breakpoint_list::update()

void command_window::brkpt_update (int index, const breakpoint *old_bpt,
                                   const breakpoint *new_bpt)
{
  if (new_bpt == NULL)
    {
      // Deletion
      if (old_bpt->get_source () != NULL)
        {
          source_window *src = find_source_short (old_bpt->get_source ());
          if (src != NULL)
            src->set_breakpoint (old_bpt->get_lineno (), false, true);
        }
    }
  else if (old_bpt == NULL)
    {
      // Insertion
      if (new_bpt->get_source () != NULL)
        {
          source_window *src = find_source_short (new_bpt->get_source ());
          if (src != NULL)
            src->set_breakpoint (new_bpt->get_lineno (), true, true);
        }
    }
  else
    {
      // Possible update
      if (old_bpt->get_address () != new_bpt->get_address ())
        {
          if (old_bpt->get_source () != NULL)
            {
              source_window *src = find_source_short (old_bpt->get_source ());
              if (src != NULL)
                src->set_breakpoint (old_bpt->get_lineno (), false, true);
            }
          if (new_bpt->get_source () != NULL)
            {
              source_window *src = find_source_short (new_bpt->get_source ());
              if (src != NULL)
                src->set_breakpoint (new_bpt->get_lineno (), true, true);
            }
        }
    }
  if (brk != NULL)
    brk->update (index, old_bpt, new_bpt);
}


void command_window::get_breakpoints (source_window *src, bool paint)
{
  const char *name = src->get_short_filename ();
  // TODO: iterator
  for (breakpoint_list::brkpt_node *p = breakpoints.list; p != NULL;
       p = p->next)
    if (p->get_source () != NULL
        && strcasecmp ((const unsigned char *)p->get_source (),
                       (const unsigned char *)name) == 0)
      src->set_breakpoint (p->get_lineno (), true, paint);
}


const breakpoint *command_window::get_breakpoint (int n) const
{
  return breakpoints.get (n);
}


source_window *command_window::find_source_short (const char *fname)
{
  for (src_node *node = src_list; node != NULL; node = node->next)
    if (strcasecmp ((const unsigned char *)node->src->get_short_filename (),
                    (const unsigned char *)fname) == 0)
      return node->src;
  return NULL;
}


source_window *command_window::find_source_long (const char *fname)
{
  for (src_node *node = src_list; node != NULL; node = node->next)
    if (strcasecmp ((const unsigned char *)node->src->get_long_filename (),
                    (const unsigned char *)fname) == 0)
      return node->src;
  return NULL;
}


// lineno == -1: don't change line if file already loaded;
//               don't select line for new file
// Deselect current line in any source window if both filenames are NULL

void command_window::show_source (const char *short_fname,
                                  const char *long_fname, bool set_focus,
                                  int lineno)
{
  for (const src_node *node = src_list; node != NULL; node = node->next)
    node->src->show_line (-1);
  if (short_fname == NULL && long_fname == NULL)
    return;

  source_window *src = NULL;
  if (long_fname != NULL)
    src = find_source_long (long_fname);
  if (src == NULL && short_fname != NULL)
    src = find_source_short (short_fname);

  if (src != NULL)
    {
      if (lineno != -1)
        src->show_line (lineno);
      if (set_focus)
        src->focus ();
    }
  else
    {
      new_source ns;
      ns.short_filename = NULL; ns.long_filename = NULL;
      if (short_fname != NULL)
        {
          // TODO: Use fstring?
          ns.short_filename = new char [strlen (short_fname) + 1];
          strcpy (ns.short_filename, short_fname);
        }
      if (long_fname != NULL)
        {
          ns.long_filename = new char [strlen (long_fname) + 1];
          strcpy (ns.long_filename, long_fname);
        }
      ns.lineno = lineno;
      ns.set_focus = set_focus;
      if (same_tid ())
        show_new_source (&ns);
      else
        WinSendMsg (get_hwndClient (), UWM_SOURCE, MPFROMP (&ns), 0);
    }
}


// FILENAME has been allocated with new char[]!

void command_window::show_new_source (const new_source *ns)
{
  unsigned menu_id;
  MENUITEM mi;
  char *long_filename = ns->long_filename;
  char *short_filename = ns->short_filename;

  if (long_filename == NULL || access (long_filename, R_OK) != 0)
    {
      char *new_filename = NULL;

      capture *capt = gdb->capture_cmd ("server list %s:1,0",
                                        (short_filename != NULL
                                         ? short_filename : long_filename));
      if (capt != NULL && !capt->error.is_set ())
        {
          delete_capture (capt);
          capt = gdb->capture_cmd ("server info source");
          if (capt != NULL && capt->source_location.is_set ())
            {
              const char *s = capt->source_location.get ();
              new_filename = new char [strlen (s) + 1];
              strcpy (new_filename, s);
            }
        }
      delete_capture (capt);
      delete[] long_filename;
      if (new_filename == NULL)
        {
          delete[] short_filename;
          return;               // TODO: Message box
        }
      long_filename = new_filename;
    }

  if (short_filename == NULL)
    {
      const char *s = _getname (long_filename);
      short_filename = new char[strlen (s)];
      strcpy (short_filename, s);
    }
  char *p = (char *)memchr (src_menu_id_used, 0, src_menu_id_count);
  if (p != NULL)
    menu_id = p - src_menu_id_used;
  else
    {
      int new_count = src_menu_id_count + 32;
      p = new char[new_count];
      memcpy (p, src_menu_id_used, src_menu_id_count);
      memset (p + src_menu_id_count, 0, new_count - src_menu_id_count);
      delete[] src_menu_id_used;
      src_menu_id_used = p;
      menu_id = src_menu_id_count;
      src_menu_id_count = new_count;
    }
  src_menu_id_used[menu_id] = 1;
  menu_id += IDM_WIN_SRC;

  src_node *new_node = new src_node;
  new_node->next = NULL;
  new_node->src = new source_window (get_app (), ID_SOURCE_WINDOW, NULL,
                                     short_filename, long_filename, this, gdb);
  if (!help_ok) help_disable (new_node->src);
  new_node->menu_id = menu_id;
  new_node->hwndWinMenu = new_node->src->get_hwndMenu_from_id (IDM_WINMENU);

  mi.iPosition = MIT_END;
  mi.afStyle = MIS_TEXT;
  mi.afAttribute = 0;
  mi.hwndSubMenu = NULLHANDLE;
  mi.hItem = 0;
  mi.id = new_node->menu_id;
  const char *new_name = short_filename;

  WinSendMsg (get_hwndMenu_from_id (IDM_WINMENU),
              MM_INSERTITEM, MPFROMP (&mi), MPFROMP (new_name));
  WinSendMsg (thr->get_hwndMenu_from_id (IDM_WINMENU),
              MM_INSERTITEM, MPFROMP (&mi), MPFROMP (new_name));
  WinSendMsg (brk->get_hwndMenu_from_id (IDM_WINMENU),
              MM_INSERTITEM, MPFROMP (&mi), MPFROMP (new_name));
  WinSendMsg (dsp->get_hwndMenu_from_id (IDM_WINMENU),
              MM_INSERTITEM, MPFROMP (&mi), MPFROMP (new_name));
  WinSendMsg (srcs->get_hwndMenu_from_id (IDM_WINMENU),
              MM_INSERTITEM, MPFROMP (&mi), MPFROMP (new_name));

  for (src_node *pn = src_list; pn != NULL; pn = pn->next)
    {
      mi.id = new_node->menu_id;
      WinSendMsg (pn->hwndWinMenu, MM_INSERTITEM, MPFROMP (&mi),
                  MPFROMP (new_name));
      mi.id = pn->menu_id;
      WinSendMsg (new_node->hwndWinMenu, MM_INSERTITEM, MPFROMP (&mi),
                  MPFROMP (pn->src->get_short_filename ()));
    }
  src_node **patch = &src_list;
  while (*patch != NULL)
    patch = &(*patch)->next;
  *patch = new_node;

  if (ns->lineno != -1)
    new_node->src->show_line (ns->lineno);
  if (ns->set_focus)
    new_node->src->focus ();

  delete[] short_filename;
  delete[] long_filename;
}


void command_window::delete_source (source_window *src)
{
  // TODO: Use Mutex to prevent another thread from accessing src_list and SRC
  src_node **patch = &src_list;
  while (*patch != NULL && (*patch)->src != src)
    patch = &(*patch)->next;
  if (*patch != NULL)
    {
      src_node *p = *patch;
      *patch = p->next;
      src_menu_id_used[p->menu_id] = 0;
      for (src_node *pn = src_list; pn != NULL; pn = pn->next)
        WinSendMsg (pn->src->get_hwndMenu (), MM_DELETEITEM,
                    MPFROM2SHORT (p->menu_id, TRUE), 0);
      WinSendMsg (get_hwndMenu (), MM_DELETEITEM,
                  MPFROM2SHORT (p->menu_id, TRUE), 0);
      WinSendMsg (thr->get_hwndMenu (), MM_DELETEITEM,
                  MPFROM2SHORT (p->menu_id, TRUE), 0);
      WinSendMsg (brk->get_hwndMenu (), MM_DELETEITEM,
                  MPFROM2SHORT (p->menu_id, TRUE), 0);
      WinSendMsg (dsp->get_hwndMenu (), MM_DELETEITEM,
                  MPFROM2SHORT (p->menu_id, TRUE), 0);
      WinSendMsg (srcs->get_hwndMenu (), MM_DELETEITEM,
                  MPFROM2SHORT (p->menu_id, TRUE), 0);
      delete p;
    }
  delete src;
}


bool command_window::open_dialog (char *buf, const char *title,
                                  const char *fname)
{
  FILEDLG fdlg;

  memset (&fdlg, 0, sizeof (fdlg));
  fdlg.cbSize = sizeof (fdlg);
  fdlg.fl = FDS_OPEN_DIALOG | FDS_CENTER;
  fdlg.ulUser = 0;
  fdlg.lReturn = 0;
  fdlg.pszTitle = (PSZ)title;
  fdlg.pszOKButton = (PSZ)"Open";
  fdlg.pfnDlgProc = NULL;
  fdlg.pszIType = NULL;
  fdlg.papszITypeList = NULL;
  fdlg.pszIDrive = NULL;
  fdlg.papszIDriveList = NULL;
  fdlg.hMod = NULLHANDLE;
  strcpy (fdlg.szFullFile, fname);
  fdlg.papszFQFilename = NULL;
  fdlg.ulFQFCount = 0;
  fdlg.usDlgId = 0;
  fdlg.x = 0; fdlg.y = 0;
  fdlg.sEAType = 0;
  return (WinFileDlg (HWND_DESKTOP, get_hwndClient (), &fdlg)
          && fdlg.lReturn == DID_OK
          && _abspath (buf, fdlg.szFullFile, CCHMAXPATH) == 0);
}


void command_window::open_exec ()
{
  char aname[CCHMAXPATH];
  if (open_dialog (aname, "Open program file", "*.exe"))
    gdb->send_cmd ("server file %s", aname);
}


void command_window::open_core ()
{
  char aname[CCHMAXPATH];
  if (open_dialog (aname, "Open core file", "*"))
    gdb->send_cmd ("server core-file %s", aname);
}


void command_window::open_source ()
{
  char aname[CCHMAXPATH];
  if (open_dialog (aname, "Open source file", "*"))
    show_source (NULL, aname, true, -1);
}


void command_window::kill_input ()
{
  backspace (input_line.length ());
  input_line.set (0);
}


void command_window::replace_command (const char *s)
{
  kill_input ();
  input_line.set (s);
  puts (s);
}


void command_window::exec_command ()
{
  if (input_line.length () != 0)
    {
      cmd_hist *p = new cmd_hist;
      p->cmd = input_line;
      p->prev = history;
      p->next = NULL;
      if (history != NULL)
        history->next = p;
      history = p;
    }
  cur_cmd = NULL;

  input_line.append ("\n", 1);
  puts ("\n", 1);
  gdb->send_str ((const char *)input_line, input_line.length ());
  input_line.set (0);
}


void command_window::from_history (bool next)
{
  if (cur_cmd == NULL)
    {
      if (history == NULL || next)
        {
          WinAlarm (HWND_DESKTOP, WA_ERROR);
          return;
        }
      cur_cmd = history;
    }
  else
    {
      cmd_hist *p = next ? cur_cmd->next : cur_cmd->prev;
      if (p == NULL)
        {
          WinAlarm (HWND_DESKTOP, WA_ERROR);
          return;
        }
      cur_cmd = p;
    }
  replace_command (cur_cmd->cmd);
}


void command_window::key (char c)
{
  if (gdb->gdb_prompt != gdbio::GSP_CMD && gdb->gdb_prompt != gdbio::GSP_OTHER)
    {
      WinAlarm (HWND_DESKTOP, WA_ERROR);
      return;
    }
  if (c == 8 && input_line.length () > 0)
    {
      backspace ();
      input_line.set (input_line.length () - 1);
    }
  else if (c == '\n' || c == '\r')
    {
      exec_command ();
    }
  else if ((unsigned char)c >= 0x20)
    {
      input_line.append (&c, 1);
      puts ((char *)&c, 1);
    }
}


HWND command_window::create_hwnd_menu ()
{
  return WinCreateWindow (get_hwndClient (), WC_MENU, (PSZ)"",
                          0, 0, 0, 0, 0,
                          get_hwndClient (), HWND_TOP, 0,
                          NULL, NULL);
}


#define MAX_MENU        26

void command_window::completion_menu (HWND hwnd, int start, int count,
                                      int skip)
{
  MENUITEM mi;
  if (count < MAX_MENU)
    {
      mi.iPosition = MIT_END;
      mi.afStyle = MIS_TEXT;
      mi.afAttribute = 0;
      mi.hwndSubMenu = NULLHANDLE;
      mi.hItem = 0;
      for (int i = 0; i < count; ++i)
        {
          mi.id = IDM_COMPLETIONS + start + i;
          WinSendMsg (hwnd, MM_INSERTITEM, MPFROMP (&mi),
                      MPFROMP (completions[start + i] + skip));
        }
    }
  else
    {
      int i, j, nsub = 1;
      for (i = 1; i < count; ++i)
        if (completions[start+i][skip] != completions[start+i-1][skip])
          ++nsub;
      bool initial_char = nsub >= 2 && nsub < MAX_MENU;
      int number = 1;
      mi.iPosition = MIT_END;
      mi.afStyle = MIS_SUBMENU | MIS_TEXT;
      mi.afAttribute = 0;
      mi.hItem = 0;
      mi.id = 0;
      int subcount, max_subcount = (count + MAX_MENU - 1) / MAX_MENU;
      char text[20];
      for (i = 0; i < count; i += subcount)
        {
          if (initial_char)
            {
              text[0] = completions[start+i][skip];
              text[1] = 0;
              for (j = i + 1; j < count; ++j)
                if (completions[start+j][skip] != completions[start+i][skip])
                  break;
              subcount = j - i;
            }
          else
            {
              _itoa (number++, text, 10);
              subcount = count - i;
              if (subcount > max_subcount) subcount = max_subcount;
            }
          mi.hwndSubMenu = create_hwnd_menu ();
          completion_menu (mi.hwndSubMenu, start + i, subcount, skip);
          WinSendMsg (hwnd, MM_INSERTITEM, MPFROMP (&mi), MPFROMP (text));
        }
    }
}


void command_window::complete ()
{
  if (gdb->gdb_prompt != gdbio::GSP_CMD && gdb->gdb_prompt != gdbio::GSP_OTHER)
    {
      WinAlarm (HWND_DESKTOP, WA_ERROR);
      return;
    }
  // TODO: Don't do this in the PM thread (time!)
  // TODO: hourglass
  capture *capt = gdb->capture_cmd ("server complete %s",
                                    (const char *)input_line);
  if (capt == NULL)
    {
      WinAlarm (HWND_DESKTOP, WA_ERROR);
      return;
    }
  delete_capture (capt);
  completions = gdb->get_output ();
  POINTL ptl;
  if (completions.get_lines () == 1)
    complete (completions[0]);
  else if (completions.get_lines () != 0 && get_xy (&ptl))
    {
      completions.sort ();
      int skip = 0;
      char *blank = strrchr (input_line, ' ');
      if (blank != NULL)
        skip = blank + 1 - (const char *)input_line;

      HWND hwndPopup = create_hwnd_menu ();
      completion_menu (hwndPopup, 0, completions.get_lines (), skip);
      WinPopupMenu (get_hwndFrame (), get_hwndClient (),
                    hwndPopup, ptl.x, ptl.y, 0,
                    (PU_HCONSTRAIN | PU_VCONSTRAIN | PU_NONE
                     | PU_KEYBOARD | PU_MOUSEBUTTON1));
    }
}


void command_window::complete (const char *p)
{
  if (p != NULL && strncmp (p, input_line, input_line.length ()) == 0)
    {
      p += input_line.length ();
      size_t len = strlen (p);
      input_line.append (p, len);
      puts (p, len);
    }
}


void command_window::font ()
{
  if (font_dialog ())
    {
      srcs->set_font (*this);
      thr->set_font (*this);
      brk->set_font (*this);
      dsp->set_font (*this);
      reg->set_font (*this);
      for (src_node *pn = src_list; pn != NULL; pn = pn->next)
        pn->src->set_font (*this);
    }
}


MRESULT command_window::wm_char (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
  unsigned fsflags = SHORT1FROMMP (mp1);
  unsigned usch = SHORT1FROMMP (mp2);
  unsigned usvk = SHORT2FROMMP (mp2);
  if (fsflags & KC_KEYUP)
    return 0;
  if (fsflags & KC_VIRTUALKEY)
    {
      HWND hwndTarget = NULLHANDLE;
      switch (usvk)
        {
        case VK_UP:
          from_history (false);
          return 0;
        case VK_DOWN:
          from_history (true);
          return 0;
        case VK_PAGEUP:
        case VK_PAGEDOWN:
          if (SHORT1FROMMP (mp1) & KC_CTRL)
            hwndTarget = get_hwndHbar ();
          else
            hwndTarget = get_hwndVbar ();
          break;
        }
      if (hwndTarget != NULLHANDLE)
        {
          if (WinIsWindowEnabled (hwndTarget))
            WinPostMsg (hwndTarget, msg, mp1, mp2);
          return 0;
        }
    }
  if (fsflags & KC_CHAR)
    {
      key (usch);
      return 0;
    }
  if ((fsflags & (KC_CTRL|KC_VIRTUALKEY)) == KC_CTRL
      && usch >= 0x40 && usch <= 0x7f)
    {
      key (usch & 0x1f);
      return 0;
    }
  return WinDefWindowProc (hwnd, msg, mp1, mp2);
}


MRESULT command_window::pmdebugmode_msg (HWND hwnd, ULONG msg,
                                         MPARAM mp1, MPARAM mp2)
{
  MRESULT mr;

  switch (msg)
    {
    case WM_INITDLG:
      dlg_sys_menu (hwnd);
      WinSetWindowPtr (hwnd, QWL_USER, this);
      WinSendDlgItemMsg (hwnd, IDC_SYNC, BM_SETCHECK,
                         MPFROMSHORT (pmdbg.get_mode () == pmdebug::sync), 0);
      break;

    case WM_COMMAND:
      switch (SHORT1FROMMP (mp1))
        {
        case DID_OK:
          mr = WinSendDlgItemMsg (hwnd, IDC_SYNC, BM_QUERYCHECK, 0, 0);
          pmdbg.set_mode (SHORT1FROMMR (mr) ? pmdebug::sync : pmdebug::off,
                          get_hwndClient ());
          WinDismissDlg (hwnd, 0);
          return 0;
        }
      break;
    }
  return WinDefDlgProc (hwnd, msg, mp1, mp2);
}


MRESULT EXPENTRY dlg_pmdebugmode (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
  command_window *p;

  if (msg == WM_INITDLG)
    p = (command_window *)((struct pm_create *)PVOIDFROMMP (mp2))->ptr;
  else
    p = (command_window *)WinQueryWindowPtr (hwnd, QWL_USER);
  return p->pmdebugmode_msg (hwnd, msg, mp1, mp2);
}


MRESULT command_window::startup_msg (HWND hwnd, ULONG msg,
                                     MPARAM mp1, MPARAM mp2)
{
  MRESULT mr;
  char *s, buf[512];

  switch (msg)
    {
    case WM_INITDLG:
      dlg_sys_menu (hwnd);
      WinSetWindowPtr (hwnd, QWL_USER, this);
      WinSendDlgItemMsg (hwnd, IDC_CLOSE_WIN, BM_SETCHECK,
                         MPFROMSHORT (gdb->get_setshow_boolean ("close")), 0);

      WinSendDlgItemMsg (hwnd, IDC_ARGS, EM_SETTEXTLIMIT,
                         MPFROMSHORT (128), 0);
      s = gdb->get_setshow_string ("args");
      WinSetDlgItemText (hwnd, IDC_ARGS, (PSZ)s);
      delete[] s;
      break;

    case WM_COMMAND:
      switch (SHORT1FROMMP (mp1))
        {
        case DID_OK:
        case IDC_RUN:
          mr = WinSendDlgItemMsg (hwnd, IDC_CLOSE_WIN, BM_QUERYCHECK, 0, 0);
          // TODO: Only if changed (handle in caller, using a structure)
          gdb->set_setshow_boolean ("close", (bool)SHORT1FROMMR (mr));

          WinQueryDlgItemText (hwnd, IDC_ARGS, sizeof (buf), (PSZ)buf);
          gdb->set_setshow_string ("args", buf);
          WinDismissDlg (hwnd, SHORT1FROMMP (mp1));
          return 0;
        }
      break;
    }
  return WinDefDlgProc (hwnd, msg, mp1, mp2);
}


MRESULT EXPENTRY dlg_startup (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
  command_window *p;

  if (msg == WM_INITDLG)
    p = (command_window *)((struct pm_create *)PVOIDFROMMP (mp2))->ptr;
  else
    p = (command_window *)WinQueryWindowPtr (hwnd, QWL_USER);
  return p->startup_msg (hwnd, msg, mp1, mp2);
}


MRESULT EXPENTRY dlg_about (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
  switch (msg)
    {
    case WM_INITDLG:
      dlg_sys_menu (hwnd);
      break;
    case WM_COMMAND:
      switch (SHORT1FROMMP (mp1))
        {
        case DID_OK:
          WinDismissDlg (hwnd, TRUE);
          return 0;
        }
      break;
    }
  return WinDefDlgProc (hwnd, msg, mp1, mp2);
}


MRESULT command_window::history_msg (HWND hwnd, ULONG msg,
                                     MPARAM mp1, MPARAM mp2)
{
  switch (msg)
    {
    case WM_INITDLG:
      dlg_sys_menu (hwnd);
      WinSetWindowPtr (hwnd, QWL_USER, this);
      WinSetDlgItemText (hwnd, IDC_LIST, input_line);
      for (cmd_hist *p = history; p != NULL; p = p->prev)
        WinSendDlgItemMsg (hwnd, IDC_LIST, LM_INSERTITEM,
                           MPFROMSHORT (LIT_END),
                           MPFROMP ((const char *)p->cmd));
      break;

    case WM_COMMAND:
      switch (SHORT1FROMMP (mp1))
        {
        case DID_OK:
          int len = WinQueryDlgItemTextLength (hwnd, IDC_LIST);
          char *text = (char *)alloca (len + 1);
          *text = 0;
          if (WinQueryDlgItemText (hwnd, IDC_LIST, len + 1, (PSZ)text) != 0)
            {
              replace_command (text);
              exec_command ();
            }
          WinDismissDlg (hwnd, 0);
          return 0;
        }
      break;
    }
  return WinDefDlgProc (hwnd, msg, mp1, mp2);
}


MRESULT EXPENTRY dlg_history (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
  command_window *p;

  if (msg == WM_INITDLG)
    p = (command_window *)((struct pm_create *)PVOIDFROMMP (mp2))->ptr;
  else
    p = (command_window *)WinQueryWindowPtr (hwnd, QWL_USER);
  return p->history_msg (hwnd, msg, mp1, mp2);
}


MRESULT command_window::wm_command (HWND hwnd, ULONG msg,
                                    MPARAM mp1, MPARAM mp2)
{
  pm_create create;
  unsigned id = SHORT1FROMMP (mp1);
  switch (id)
    {
    case IDM_EXIT:
      WinPostMsg (get_hwndFrame (), WM_CLOSE, 0, 0);
      return 0;

    case IDM_OPEN_EXEC:
      open_exec ();
      return 0;

    case IDM_OPEN_CORE:
      open_core ();
      return 0;

    case IDM_OPEN_SOURCE:
      open_source ();
      return 0;

    case IDM_WHERE:
      where ();
      return 0;

    case IDM_COMPLETE:
      complete ();
      return 0;

    case IDM_HISTORY:
      create.cb = sizeof (create);
      create.ptr = (void *)this;
      WinDlgBox (HWND_DESKTOP, hwnd, dlg_history, 0, IDD_HISTORY,
                 &create);
      return 0;

    case IDM_FONT:
      font ();
      return 0;

    case IDM_ANNOTATE:
      gdb->show_annotations = !gdb->show_annotations;
      menu_check (IDM_ANNOTATE, gdb->show_annotations);
      return 0;

    case IDM_PMDEBUGMODE:
      create.cb = sizeof (create);
      create.ptr = (void *)this;
      WinDlgBox (HWND_DESKTOP, hwnd, dlg_pmdebugmode, 0, IDD_PMDEBUGMODE,
                 &create);
      return 0;

    case IDM_STARTUP:
      create.cb = sizeof (create);
      create.ptr = (void *)this;
      if (WinDlgBox (HWND_DESKTOP, hwnd, dlg_startup, 0, IDD_STARTUP,
                     &create) == IDC_RUN)
        {
          prepare_run ();
          run_command ();
        }
      return 0;

    case IDM_BRKPT_LIST:
      brk->show (true);
      brk->focus ();
      return 0;

    case IDM_BRKPT_LINE:
      brk->add_line (NULL);
      return 0;

    case IDM_GO:
      prepare_run ();
      if (gdb->gdb_running == gdbio::GSR_NONE)
        run_command ();
      else
        gdb->send_cmd ("server continue");
      return 0;

    case IDM_RESTART:
      prepare_run ();
      run_command ();
      return 0;

    case IDM_STEPINTO:
      prepare_run ();
      gdb->send_cmd ("server step");
      return 0;

    case IDM_STEPOVER:
      prepare_run ();
      gdb->send_cmd ("server next");
      return 0;

    case IDM_ISTEPINTO:
      prepare_run ();
      gdb->send_cmd ("server stepi");
      return 0;

    case IDM_ISTEPOVER:
      prepare_run ();
      gdb->send_cmd ("server nexti");
      return 0;

    case IDM_FINISH:
      prepare_run ();
      gdb->send_cmd ("server finish");
      return 0;

    case IDM_WIN_CMD:
      show (true);
      focus ();
      return 0;

    case IDM_WIN_BRK:
      brk->show (true);
      brk->focus ();
      return 0;

    case IDM_WIN_THR:
      thr->show (true);
      thr->focus ();
      return 0;

    case IDM_WIN_DSP:
      dsp->show (true);
      dsp->focus ();
      return 0;

    case IDM_WIN_SRCS:
      srcs->show (true);
      srcs->focus ();
      return 0;

    case IDM_WIN_REG:
      reg->show (true);
      update_registers ();
      reg->focus ();
      return 0;

    case IDM_ABOUT:
      WinDlgBox (HWND_DESKTOP, hwnd, dlg_about, 0, IDD_ABOUT, NULL);
      return 0;

    case IDM_TUTORIAL:
      WinSendMsg (get_hwndHelp (), HM_DISPLAY_HELP,
                  MPFROMSHORT (HELP_TUTORIAL), MPFROMSHORT (HM_RESOURCEID));
      return 0;

    case IDM_HELP_HELP:
      WinSendMsg (get_hwndHelp (), HM_DISPLAY_HELP,
                  MPFROMSHORT (0), MPFROMSHORT (HM_RESOURCEID));
      return 0;

    case IDM_HELP_KEYS:
      WinSendMsg (get_hwndHelp (), HM_KEYS_HELP, 0, 0);
      return 0;

    case IDM_HELP_INDEX:
      WinSendMsg (get_hwndHelp (), HM_HELP_INDEX, 0, 0);
      return 0;

    case IDM_HELP_CONTENTS:
      WinSendMsg (get_hwndHelp (), HM_HELP_CONTENTS, 0, 0);
      return 0;

    default:
      if (id >= IDM_WIN_SRC && id < IDM_WIN_SRC + (unsigned)src_menu_id_count)
        {
          src_node *pn = src_list;
          while (pn != NULL && pn->menu_id != id)
            pn = pn->next;
          if (pn != NULL)
            {
              pn->src->show (true);
              pn->src->focus ();
              return 0;
            }
        }
      else if (id >= IDM_COMPLETIONS
               && id < IDM_COMPLETIONS + (unsigned)completions.get_lines ())
        complete (completions[id - IDM_COMPLETIONS]);
      break;
    }
  return WinDefWindowProc (hwnd, msg, mp1, mp2);
}


MRESULT command_window::wm_activate (HWND hwnd, ULONG msg,
                                     MPARAM mp1, MPARAM mp2)
{
  if (SHORT1FROMMP (mp1))
    associate_help (get_hwndFrame ());
  return WinDefWindowProc (hwnd, msg, mp1, mp2);
}


bool command_window::wm_user (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
  unsigned change;

  if (parent::wm_user (hwnd, msg, mp1, mp2))
    return true;
  switch (msg)
    {
    case UWM_STATE:
      change = LONGFROMMP (mp1);
      if (change & GSC_PROMPT)
        {
          // TODO: Other menus!
          menu_enable (IDM_RUNMENU, gdb->is_ready ());
          if (gdb->is_ready ())
            {
              if (gdb->stopped_pending)
                {
                  gdb->stopped_pending = false;
                  capture *frames = gdb->frame_head;
                  if (frames != NULL)
                    {
                      gdb->frame_head = NULL; gdb->frame_cur = NULL;
                      if (frames->source_file.is_set ())
                        show_source (NULL, frames->source_file.get (),
                                     true, frames->source_lineno.get ());
                      else
                        show_source (NULL, NULL, false, -1);
                      delete_capture (frames);
                    }
                  else
                    show_source (NULL, NULL, false, -1);
                  if (reg->is_visible ())
                    update_registers ();
                }
              capture *displays = gdb->display_head;
              if (displays != NULL)
                {
                  gdb->display_head = NULL; gdb->display_cur = NULL;
                  for (capture *p = displays; p != NULL; p = p->next)
                    dsp->update (p->disp_number.get (), p->disp_expr.get (),
                                 p->disp_format.get (), p->disp_value.get (),
                                 p->disp_enable.get ());
                  delete_capture (displays);
                }
            }
        }
      if (change & GSC_RUNNING)
        {
          // TODO: IDM_GO
          bool running = !gdb->is_nochild ();
          menu_enable (IDM_STEPOVER, running);
          menu_enable (IDM_STEPINTO, running);
          menu_enable (IDM_ISTEPOVER, running);
          menu_enable (IDM_ISTEPINTO, running);
          menu_enable (IDM_FINISH, running);
        }
      if (change & GSC_BREAKPOINTS)
        {
          // TODO: Don't do this in the PM thread (time!)
          // TODO: This may break if this message is processed while
          //       we're operating on breakpoints (Edit->Modify!)
          get_breakpoints ();
        }
      if (change & GSC_EXEC_FILE)
        new_debuggee ();
      return true;

    case UWM_SOURCE:
      show_new_source ((const new_source *)PVOIDFROMMP (mp1));
      return true;

    case UWM_CLOSE_SRC:
      delete_source ((source_window *)PVOIDFROMMP (mp1));
      return true;

    case UWM_PMDBG_START:
      pmdbg.start ();
      return true;

    case UWM_PMDBG_STOP:
      pmdbg.stop ();
      return true;

    case UWM_PMDBG_TERM:
      pmdbg.term ();
      return true;

    case UWM_FATAL:
      WinMessageBox (HWND_DESKTOP, HWND_DESKTOP, (PSZ)PVOIDFROMMP (mp1),
                     (PSZ)"pmgdb", 0,
                     MB_MOVEABLE | MB_OK | MB_ICONEXCLAMATION);
      exit (1);

    default:
      return false;
    }
}


MRESULT command_window::wm_close (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
  if (gdb->gdb_running != gdbio::GSR_NONE)
    {
      LONG response
        = WinMessageBox (HWND_DESKTOP, HWND_DESKTOP,
                         (PSZ)"The program is running -- quit anyway?",
                         (PSZ)get_name (), 0,
                         (MB_YESNO | MB_QUERY | MB_DEFBUTTON1 | MB_APPLMODAL
                          | MB_MOVEABLE));
      if (response != MBID_YES)
        return 0;
    }
  return parent::wm_close (hwnd, msg, mp1, mp2);
}


void command_window::thread ()
{
  int ipipe[2], opipe[2], sin, sout, serr;
  int pid, from_gdb;
  pmapp app ("pmgdb");
  app.no_message_loop ();

  if ((sin = dup (0)) == -1) abort ();
  if ((sout = dup (1)) == -1) abort ();
  if ((serr = dup (2)) == -1) abort ();

  if (pipe (ipipe) != 0) abort ();
  if (pipe (opipe) != 0) abort ();
  if (fcntl (ipipe[0], F_SETFD, 1) != 0) abort ();
  if (fcntl (opipe[1], F_SETFD, 1) != 0) abort ();

  from_gdb = ipipe[0];
  setmode (opipe[1], O_BINARY);
  gdb->send_init (opipe[1]);

  if (dup2 (opipe[0], 0) != 0) abort ();
  if (dup2 (ipipe[1], 1) != 1) abort ();
  if (dup2 (ipipe[1], 2) != 2) abort ();

  close (ipipe[1]);
  close (opipe[0]);

  // Get the current priority of this thread.

  PPIB ppib;
  PTIB ptib;
  DosGetInfoBlocks (&ptib, &ppib);
  ULONG old_priority = ptib->tib_ptib2->tib2_ulpri;

  // Temporarily change the priority of this thread to let GDB inherit
  // foreground server priority.  Setting the priority of a process in
  // another session does not work after starting that session.

  if (DosSetPriority (PRTYS_THREAD, PRTYC_FOREGROUNDSERVER, 0, 0) != 0)
    DosBeep (400, 200);

  pid = spawnvp (P_SESSION | P_MINIMIZE | P_BACKGROUND, argv[0], argv);

  // Restore the priority of this thread.  (This won't restore the
  // priority of any threads started by spawnvp() -- but that doesn't
  // matter because the thread created by emx.dll for
  // spawnv(P_SESSION) just waits for termination of our child
  // sessions.  Having too high a priority for such a thread doesn't
  // hurt.)

  if (DosSetPriority (PRTYS_THREAD, (old_priority >> 8) & 0xff,
                      old_priority & 0xff, 0) != 0
      || DosSetPriority (PRTYS_THREAD, (old_priority >> 8) & 0xff,
                         old_priority & 0xff, 0) != 0)
    DosBeep (400, 200);

  dup2 (sin, 0); close (sin);
  dup2 (sout, 1); close (sout);
  dup2 (serr, 2); close (serr);

  if (pid == -1)
    {
      WinPostMsg (get_hwndClient (), UWM_FATAL,
                  MPFROMP ("Cannot run gdb.exe."), 0);
      return;
    }

  gdb->send_init ();
  gdb->start (from_gdb);
  gdb->parse_gdb ();
}


void command_window::handle_output (const char *text, int len)
{
  puts (text, len);
}


void command_window::show_annotation (const annotation *ann)
{
  pmtxt_attr a = get_attr ();
  set_attr ();
  if (ann->get_code () == annotation::UNKNOWN)
    set_bg_color (CLR_GREEN);
  else
    set_fg_color (CLR_DARKGREEN);
  if (get_column () + ann->get_text_len () + 2 > 80)
    puts ("{}\n", 3);
  puts ("{", 1);
  puts (ann->get_text (), ann->get_text_len ());
  puts ("}", 1);
  set_attr (a);
}


void command_window::exec_file (const char *s, int len)
{
  lock ();
  if (s == NULL)
    debuggee.set_null ();
  else
    debuggee.set (s, len);
  unlock ();
}


void command_window::notify (unsigned change)
{
  // Use WinPostMsg instead of WinSendMsg because there may not yet be
  // a message loop
  WinPostMsg (get_hwndClient (), UWM_STATE, MPFROMLONG (change), 0);
  for (src_node *p = src_list; p != NULL; p = p->next)
    WinPostMsg (p->src->get_hwndClient (), UWM_STATE, MPFROMLONG (change), 0);
}


void command_window::new_debuggee ()
{
  srcs->delete_all ();
  srcs->sync ();
  lock ();
  menu_enable (IDM_RESTART, !debuggee.is_null ());
  menu_enable (IDM_GO, !debuggee.is_null ());
  if (!debuggee.is_null ())
    {
      ULONG flags;
      if (DosQueryAppType (debuggee, &flags) == 0
          && (flags & FAPPTYP_EXETYPE) == FAPPTYP_WINDOWAPI)
        pmdbg.set_mode (pmdebug::sync, get_hwndClient ());
      const char *temp = _getname (debuggee);
      unlock ();
      astring title; title = "pmgdb - ";
      title.append (temp);
      set_title (title);
      bool ok = false;
      capture *capt = gdb->capture_cmd ("server info sources");
      if (capt != NULL)
        {
          for (capture *c = capt; c != NULL; c = c->next)
            if (c->srcs_file.is_set ())
              {
                srcs->add (c->srcs_file.get ());
                ok = true;
              }
          srcs->done ();
          delete_capture (capt);
          capt = gdb->capture_cmd ("server info line main");
          if (capt != NULL && capt->source_file.is_set ())
            srcs->select (_getname (capt->source_file.get ()));
          delete_capture (capt);
        }
      if (ok)
        srcs->show (true);
    }
  else
    {
      unlock ();
      set_title ("pmgdb");
    }
}


void command_window::where ()
{
  capture *capt = gdb->capture_cmd ("server info line *$pc");
  if (capt != NULL && capt->source_file.is_set ())
    show_source (NULL, capt->source_file.get (), true,
                 capt->source_lineno.get ());
  else
    show_source (NULL, NULL, false, -1);
  delete_capture (capt);
}


void command_window::update_registers ()
{
  capture *capt = gdb->capture_cmd ("server info registers");
  if (capt != NULL)
    reg->update (gdb->get_output ());
  delete_capture (capt);
}


void command_window::help_disable (const pmframe *frame)
{
  frame->menu_enable (IDM_HELP_EXT, false, true);
  frame->menu_enable (IDM_HELP_HELP, false, true);
  frame->menu_enable (IDM_HELP_KEYS, false, true);
  frame->menu_enable (IDM_HELP_INDEX, false, true);
  frame->menu_enable (IDM_HELP_CONTENTS, false, true);
  frame->menu_enable (IDM_TUTORIAL, false, true);
}


void command_window::associate_help (HWND hwnd)
{
  if (get_hwndHelp () != NULLHANDLE)
    WinAssociateHelpInstance (get_hwndHelp (), hwnd);
}


void command_window::run_command ()
{
  if (!breakpoints.any_enabled ())
    gdb->queue_cmd ("server break main");
  gdb->queue_cmd ("server run");
}


void command_window::prepare_run ()
{
  gdb->prepare_run ();
  HWND hwndLock = WinQueryFocus (HWND_DESKTOP);
  if (hwndLock == NULLHANDLE)
    hwndLock = get_hwndClient ();
  pmdbg.set_window (hwndLock);
  pmdbg_start ();
}


void command_window::pmdbg_start ()
{
  if (same_tid ())
    pmdbg.start ();
  else
    WinSendMsg (get_hwndClient (), UWM_PMDBG_START, 0, 0);
}


void command_window::pmdbg_stop ()
{
  if (same_tid ())
    pmdbg.stop ();
  else
    WinSendMsg (get_hwndClient (), UWM_PMDBG_STOP, 0, 0);
}


void command_window::pmdbg_term ()
{
  if (same_tid ())
    pmdbg.term ();
  else
    WinSendMsg (get_hwndClient (), UWM_PMDBG_TERM, 0, 0);
}
