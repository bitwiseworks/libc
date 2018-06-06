/* pmtxt.h -*- C++ -*-
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


#include <sys/builtin.h>
#include <sys/fmutex.h>

struct pmtxt_line;
struct pmtxt_line_attr;

struct pmtxt_attr
{
  COLOR fg_color;
  COLOR bg_color;
};


// A frame window showing scrollable text

class pmtxt : public pmframe
{
public:

  typedef pmframe parent;

  // Constructors and destructors, initialization
  pmtxt (pmapp *app, unsigned id, ULONG frame_flags, const SWP *pswp,
         const char *fontnamesize = 0);
  virtual ~pmtxt ();
  virtual void delete_all ();

  // Painting
  void put (int line, int column, size_t len, const char *s,
            const pmtxt_attr &attr, bool do_paint)
    { put (line, column, len, s, attr, E_TEXT, do_paint); }
  void put (int line, int column, size_t len, const char *s, bool do_paint)
    { put (line, column, len, s, default_attr, E_TEXT, do_paint); }
  void put_vrule (int line, int column, const pmtxt_attr &attr, bool do_paint)
    { put (line, column, 1, "", attr, E_VRULE, do_paint); }
  void put_vrule (int line, int column, bool do_paint)
    { put_vrule (line, column, default_attr, do_paint); }
  void put_tab (int line, int column, const pmtxt_attr &attr, bool do_paint)
    { put (line, column, 1, " ", attr, E_TAB, do_paint); }
  void put_tab (int line, int column, bool do_paint)
    { put_tab (line, column, default_attr, do_paint); }
  void underline (int line, bool on, bool do_paint);
  void sync ();
  void show_line (int line, int threshold = 0, int overshoot = 0);
  void delete_lines (int line, int count, bool do_paint);
  void insert_lines (int line, int count, bool do_paint);
  void clear_lines (int line, int count, bool do_paint);
  void truncate_line (int line, int count, bool do_paint);

  // Attributes
  void set_fg_color (pmtxt_attr &attr, COLOR color)
    { attr.fg_color = color; }
  void set_fg_color (pmtxt_attr &attr)
    { attr.fg_color = default_attr.fg_color; }
  COLOR get_fg_color (pmtxt_attr const &attr) const
    { return attr.fg_color; }

  void set_bg_color (pmtxt_attr &attr, COLOR color)
    { attr.bg_color = color; }
  void set_bg_color (pmtxt_attr &attr)
    { attr.bg_color = default_attr.bg_color; }
  COLOR get_bg_color (pmtxt_attr const &attr) const
    { return attr.bg_color; }

  const pmtxt_attr &get_default_attr () const { return default_attr; }

  void put (int line, int column, size_t len, const pmtxt_attr &attr,
            bool do_paint);
  void set_eol_attr (int line, const pmtxt_attr &attr, bool do_paint);
  void clear_tabs ();
  void set_font (const pmtxt &s);

  // Querying window contents
  int get_char (int line, int column);
  bool get_string (int line, int column, char *str, size_t count);
  int get_window_lines () const { return window_lines; }

  // Constants

  enum
  {
    max_line_len = 512          // See GpiCharString
  };

protected:

  // Types

  enum element
  {
    E_KEEP,
    E_TEXT,
    E_VRULE,
    E_FILL,
    E_TAB
  };

  // Messages
  MRESULT wm_button (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
  MRESULT wm_create (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
  MRESULT wm_hscroll (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
  MRESULT wm_presparamchanged (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
  MRESULT wm_querytrackinfo (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
  MRESULT wm_size (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
  MRESULT wm_vscroll (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
  virtual bool wm_user (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
  virtual void button_event (int line, int column, int tab, int button,
                             int clicks);

  // Painting
  bool get_bbox_pos (HPS hps, POINTL *pptl, int line, int column);
  void put (int line, int column, size_t len, const char *s,
            const pmtxt_attr &attr, element el, bool do_paint);

  // Notifications for derived classes
  virtual void painting (bool start);
  virtual void font_changed ();

  // Querying members
  int get_cyChar () const { return cyChar; }
  int get_cyDesc () const { return cyDesc; }
  int get_cxChar () const { return cxChar; }
  HWND get_hwndHbar () const { return hwndHbar; }
  HWND get_hwndVbar () const { return hwndVbar; }
  bool get_initializing_font () const { return initializing_font; }

  bool font_dialog ();
  void set_font (HPS hps);

  // Data members

  bool output_stopped;
  bool output_pending;

private:

  // Messages
  MRESULT wm_paint (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
  MRESULT button_event (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2,
                        int button, int clicks);

  // Painting
  bool scroll_up (int n, bool do_paint);
  void scroll_down (int n);
  void check_x_size ();
  void check_y_size ();
  void disable_output ();
  void enable_output ();
  void change_attr (pmtxt_line *p, int start, int end,
                    const pmtxt_attr &new_attr, element new_el);
  void setup_ps (HPS hps);
  void create_hpsClient ();
  void get_fontmetrics ();
  void change_size ();
  void paint (HPS hps, pmtxt_attr &cur_attr, int line, int column,
              bool paint_eol);
  void paint (HPS hps, pmtxt_attr &cur_attr, const pmtxt_line_attr *pla,
              const char *p, int len);

  void unlock ();
  void more_lines (int new_count);
  void free_line (int line);
  void init_line (int line);
  void delete_attr (pmtxt_line_attr *a);
  void pmthread_perform ();
  void pmthread_request (unsigned char bits);
  void line_column_from_x_y (HPS hps, int *line, int *column, int *tab,
                             int x, int y);

  // Data members

  HWND hwndVbar;
  HWND hwndHbar;
  HPS hpsClient;
  LONG cxClient;
  LONG cyClient;
  int cxChar;
  int cyChar;
  int cyDesc;
  int rule_width;
  int window_lines, window_columns;
  bool track;
  int x_off, y_off;
  int x_pos, max_x_pos;
  int lines_used;
  int lines_alloc;
  pmtxt_line *lines_vector;
  _fmutex mutex;
  unsigned char pmthread_pending;

  // TABs
  int tab_count;                // Number of active TABs
  int tab_x_size;               // Size of following array
  int *tab_x;                   // X coordinates of TABs
  bool tab_repaint;             // X coordinate of a TAB changed, repaint!
  int tab_index;                // Current TAB index (temporary)

  pmtxt_attr default_attr;      // In effect at start of each line
  pmtxt_attr hpsClient_attr;    // Current attributes of hpsClient

  int cache_line;               // Input
  int cache_column;             // Input
  int cache_x;                  // Output

  bool fixed_pitch;

  POINTL *aptl_line;

  FATTRS fattrs;
  bool default_font;
  const char *init_fontnamesize;
  bool initializing_font;
};
