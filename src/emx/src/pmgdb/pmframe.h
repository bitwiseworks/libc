/* pmframe.h -*- C++ -*-
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


// A frame window

class pmframe
{
public:

  // Constructors and destructors, initialization
  pmframe (pmapp *app);
  virtual ~pmframe ();
  void create (const char *classname, unsigned id, ULONG frame_flags,
               const SWP *pswp);

  // Querying private members
  HWND get_hwndMenu ()       const { return hwndMenu; }
  HWND get_hwndClient ()     const { return hwndClient; }
  HWND get_hwndFrame ()      const { return hwndFrame; }
  HWND get_hwndHelp ()       const { return hwndHelp; }
  pmapp *get_app ()          const { return app; }
  HAB get_hab ()             const { return app->get_hab (); }
  const char *get_name ()    const { return app->get_name (); }
  PFNWP get_old_frame_msg () const { return old_frame_msg; }
  bool get_minimized ()      const { return minimized; }

  // Painting
  void repaint (RECTL *prcl = NULL);

  // Miscellanea
  void set_title (const char *title);
  void win_error () const { app->win_error (); }
  void focus () const;
  void top () const;
  void show (bool visible) const;
  void menu_check (unsigned idm, bool on) const;
  void menu_enable (unsigned idm, bool on, bool silent = false) const;
  bool same_tid () const { return tid == _gettid (); }
  HWND get_hwndMenu_from_id (unsigned idm) const;
  bool is_visible () const;

protected:

  // Messages
  virtual MRESULT wm_paint (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2) = 0;
  virtual MRESULT wm_activate (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
  virtual MRESULT wm_button (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
  virtual MRESULT wm_char (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
  virtual MRESULT wm_close (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
  virtual MRESULT wm_command (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
  virtual MRESULT wm_create (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
  virtual MRESULT wm_hscroll (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
  virtual MRESULT wm_presparamchanged (HWND hwnd, ULONG msg, MPARAM mp1,
                                       MPARAM mp2);
  virtual MRESULT wm_querytrackinfo (HWND hwnd, ULONG msg, MPARAM mp1,
                                     MPARAM mp2);
  virtual MRESULT wm_setfocus (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
  virtual MRESULT wm_size (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
  virtual MRESULT wm_timer (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
  virtual bool wm_user (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
  virtual MRESULT wm_vscroll (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);

  bool help_init (unsigned help_table_id, const char *title, const char *file,
                  unsigned keys_help_id);
  void set_keys_help_id (unsigned id) { keys_help_id = id; }

private:

  MRESULT client_msg (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
  MRESULT frame_msg (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);

  friend MRESULT EXPENTRY
    pmframe_client_msg (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
  friend MRESULT EXPENTRY
    pmframe_frame_msg (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);

  // Data members

  pmapp *app;
  HWND hwndFrame;
  HWND hwndClient;
  HWND hwndMenu;
  HWND hwndHelp;
  int tid;
  unsigned keys_help_id;
  PFNWP old_frame_msg;
  bool minimized;
};
