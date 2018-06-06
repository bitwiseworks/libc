/* pmtty.cc
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


// TODO: jump to end on input

#define INCL_DOS
#define INCL_WIN
#include <os2.h>
#include <stdlib.h>
#include <string.h>
#include "string.h"
#include "pmapp.h"
#include "pmframe.h"
#include "pmtxt.h"
#include "pmtty.h"


#define IDT_CURSOR      1

pmtty::pmtty (pmapp *in_app, unsigned in_id, ULONG frame_flags,
              const SWP *pswp, const char *fontnamesize)
  : pmtxt (in_app, in_id, frame_flags, pswp, fontnamesize)
{
  x = 0; y = 0;
  line_width = max_line_len;
  cursor_enabled = false; cursor_painted = false; cursor_hidden = 0;
  sel_attr = get_default_attr ();
  create_hpsCursor ();
  cursor_width = (int)WinQuerySysValue (HWND_DESKTOP, SV_CXBORDER);
}


pmtty::~pmtty ()
{
}


void pmtty::set_line_width (int width)
{
  if (width > max_line_len)
    width = max_line_len;
  if (x >= width)
    puts ("\n", 1);
  line_width = width;
}


void pmtty::create_hpsCursor ()
{
  hpsCursor = WinGetPS (get_hwndClient ());
  set_font (hpsCursor);
}



// Roll our own cursor because a cursor created with WinCreateCursor
// can be hidden and destroyed only by the thread which created it.

void pmtty::paint_cursor ()
{
  POINTL ptl;

  if (get_bbox_pos (hpsCursor, &ptl, y, x))
    {
      // TODO: Nicer cursor (WinDrawPointer?)
      rclCursor.xLeft = ptl.x;
      rclCursor.xRight = rclCursor.xLeft + cursor_width;
      rclCursor.yBottom = ptl.y;
      rclCursor.yTop = rclCursor.yBottom + get_cyChar ();
      WinInvertRect (hpsCursor, &rclCursor);
      cursor_painted = true;
    }
}


void pmtty::remove_cursor ()
{
  WinInvertRect (hpsCursor, &rclCursor);
  cursor_painted = false;
}


inline void pmtty::cursor_on ()
{
  if (--cursor_hidden == 0 && cursor_enabled && !cursor_painted
      && WinQueryFocus (HWND_DESKTOP) == get_hwndClient ())
    paint_cursor ();
}


inline void pmtty::cursor_off ()
{
  if (++cursor_hidden == 1 && cursor_enabled && cursor_painted)
    remove_cursor ();
}


void pmtty::puts1 (const char *s, size_t len)
{
  while (x + (int)len >= line_width)
    {
      size_t n = line_width - x;
      put (y, x, n, s, sel_attr, true);
      x = 0; ++y;
      s += n; len -= n;
    }
  if (len != 0)
    {
      put (y, x, len, s, sel_attr, true);
      x += len;
    }
}


void pmtty::puts (const char *s, size_t len)
{
  if (len == 0)
    return;
  cursor_off ();
  while (len != 0)
    {
      size_t i;
      for (i = 0; i < len; ++i)
        if (s[i] == '\n' || s[i] == '\t')
          break;
      if (i >= len)
        {
          puts1 (s, len);
          break;
        }
      if (i != 0)
        puts1 (s, i);
      if (s[i] == '\t')
        {
          int tab_width = (x | 7) + 1 - x;
          puts1 ("        ", tab_width);
        }
      else
        {
          x = 0; ++y;
        }
      len -= i + 1; s += i + 1;
    }
  if (x == 0)
    put (y, x, 0, "", sel_attr, true);
  // TODO: Optionally scroll horizontally/vertically to make cursor visible
  cursor_on ();
}


class pmtty_const_str
{
public:
  char *str;
  pmtty_const_str (char c, size_t len)
  {
    str = new char[len + 1];
    memset (str, c, len);
    str[len] = 0;
  }
};


static pmtty_const_str spaces (' ', pmtxt::max_line_len);

void pmtty::backspace (int n)
{
  if (n > x)
    n = x;
  if (n > 0)
    {
      x -= n;
      cursor_off ();
      put (y, x, n, spaces.str, sel_attr, true);
      cursor_on ();
    }
}


void pmtty::cursor (bool on)
{
  if (on && !cursor_enabled)
    {
      cursor_enabled = true;
      if (cursor_hidden == 0
          && WinQueryFocus (HWND_DESKTOP) == get_hwndClient ())
        paint_cursor ();
      // TODO: Restart timer when SV_CURSORRATE is changed
      WinStartTimer (get_hab (), get_hwndClient (), IDT_CURSOR,
                     WinQuerySysValue (HWND_DESKTOP, SV_CURSORRATE));
    }
  else if (!on && cursor_enabled)
    {
      WinStopTimer (get_hab(), get_hwndClient (), IDT_CURSOR);
      cursor_enabled = false;
      if (cursor_painted)
        remove_cursor ();
    }
}


MRESULT pmtty::wm_setfocus (HWND, ULONG, MPARAM, MPARAM mp2)
{
  if (SHORT1FROMMP (mp2))
    {
      if (cursor_enabled && cursor_hidden == 0 && !cursor_painted)
        paint_cursor ();
    }
  else
    {
      if (cursor_painted)
        remove_cursor ();
    }
  return 0;
}


MRESULT pmtty::wm_timer (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
  if (SHORT1FROMMP (mp1) == IDT_CURSOR)
    {
      if (cursor_painted)
        remove_cursor ();
      else if (cursor_enabled && cursor_hidden == 0
               && WinQueryFocus (HWND_DESKTOP) == get_hwndClient ())
        paint_cursor ();
      return FALSE;
    }
  else
    return WinDefWindowProc (hwnd, msg, mp1, mp2);
}


MRESULT pmtty::wm_presparamchanged (HWND hwnd, ULONG msg,
                                    MPARAM mp1, MPARAM mp2)
{
  parent::wm_presparamchanged (hwnd, msg, mp1, mp2);
  if (LONGFROMMP (mp1) == PP_FONTNAMESIZE && !get_initializing_font ())
    {
      WinReleasePS (hpsCursor);
      create_hpsCursor ();
    }
  return 0;
}


void pmtty::font_changed ()
{
  set_font (hpsCursor);
}


void pmtty::painting (bool start)
{
  if (start)
    cursor_off ();
  else
    cursor_on ();
}


void pmtty::delete_all ()
{
  cursor_off ();
  x = 0; y = 0;
  parent::delete_all ();
}


bool pmtty::get_xy (POINTL *ptl)
{
  return get_bbox_pos (hpsCursor, ptl, y, x);
}
