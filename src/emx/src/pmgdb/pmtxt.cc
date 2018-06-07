/* pmtxt.cc
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
#define INCL_WIN
#define INCL_GPI
#include <os2.h>
#include <stdlib.h>
#include <string.h>
#include "string.h"
#include "pmapp.h"
#include "pmframe.h"
#include "pmtxt.h"

#define UWM_PMTHREAD            (WM_USER+200)

#define PMTHREAD_SBAR_X         0x01
#define PMTHREAD_SBAR_Y         0x02


#define LF_UNDERLINE            0x01

#define EOL_COLUMN      (pmtxt::max_line_len + 1)

#define MAX(a,b) ((a) > (b) ? (a) : (b))

struct pmtxt_line_attr
{
  // TODO: x
  struct pmtxt_line_attr *next;
  int column;
  pmtxt::element el;
  pmtxt_attr attr;
};

struct pmtxt_line
{
  char *str;
  pmtxt_line_attr *attr;
  int len, alloc;
  // TODO: Add flag bits (HAS_TABS, OUTPUT_PENDING, ...)
  unsigned char flags;
};


pmtxt::pmtxt (pmapp *in_app, unsigned in_id, ULONG frame_flags,
              const SWP *pswp, const char *fontnamesize)
  : pmframe (in_app)
{
  _fmutex_checked_create (&mutex, 0);

  track = true;
  default_font = true;
  init_fontnamesize = fontnamesize;
  initializing_font = false;

  aptl_line = new POINTL[max_line_len+1];

  pmthread_pending = 0;
  x_off = 0; y_off = 0;
  x_pos = 0; max_x_pos = 0;
  output_stopped = false; output_pending = false;
  cache_line = -1;

  lines_used = 0; lines_alloc = 0;
  lines_vector = NULL;

  tab_x = NULL; tab_count = 0; tab_x_size = 0;
  tab_repaint = false;

  default_attr.fg_color = CLR_BLACK;
  default_attr.bg_color = CLR_WHITE;

  create ("pmtxt.client", in_id, frame_flags, pswp);
}


pmtxt::~pmtxt ()
{
  delete_all ();
  delete[] aptl_line;
  delete[] tab_x;
  // TODO: mutex
}


void pmtxt::delete_all ()
{
  _fmutex_checked_request (&mutex, _FMR_IGNINT);

  output_pending = lines_used != 0;

  for (int i = 0; i < lines_alloc; ++i)
    free_line (i);
  delete[] lines_vector;

  x_off = 0; y_off = 0;
  x_pos = 0; max_x_pos = 0;
  cache_line = -1;

  lines_used = 0; lines_alloc = 0;
  lines_vector = NULL;

  tab_count = 0;

  unlock ();
}


void pmtxt::free_line (int line)
{
  delete[] lines_vector[line].str;
  delete_attr (lines_vector[line].attr);
}


void pmtxt::init_line (int line)
{
  lines_vector[line].str = NULL;
  lines_vector[line].attr = NULL;
  lines_vector[line].len = 0;
  lines_vector[line].alloc = 0;
  lines_vector[line].flags = 0;
}


void pmtxt::delete_attr (pmtxt_line_attr *a)
{
  pmtxt_line_attr *next;

  while (a != NULL)
    {
      next = a->next;
      delete a;
      a = next;
    }
}


void pmtxt::more_lines (int new_count)
{
  int n = new_count < 1024 ? new_count + 128 : new_count + 1024;
  pmtxt_line *new_vec = new pmtxt_line[n];
  for (int i = 0; i < lines_alloc; ++i)
    new_vec[i] = lines_vector[i];
  delete[] lines_vector;
  lines_vector = new_vec;
  for (int i = lines_alloc; i < n; ++i)
    init_line (i);
  lines_alloc = n;
}


static inline void query_hps_attr (HPS hps, pmtxt_attr &cache)
{
  cache.fg_color = GpiQueryColor (hps);
  cache.bg_color = GpiQueryBackColor (hps);
}


static inline void set_hps_attr (HPS hps, pmtxt_attr &cache,
                                 const pmtxt_attr &attr)
{
  if (attr.fg_color != cache.fg_color)
    {
      GpiSetColor (hps, attr.fg_color);
      cache.fg_color = attr.fg_color;
    }
  if (attr.bg_color != cache.bg_color)
    {
      GpiSetBackColor (hps, attr.bg_color);
      cache.bg_color = attr.bg_color;
    }
}


static inline bool eq_attr (const pmtxt_attr &a1, const pmtxt_attr &a2)
{
  return (a1.fg_color == a2.fg_color
          && a1.bg_color == a2.bg_color);
}


MRESULT pmtxt::wm_create (HWND hwnd, ULONG, MPARAM, MPARAM)
{
  hwndHbar = WinWindowFromID (get_hwndFrame (), FID_HORZSCROLL);
  hwndVbar = WinWindowFromID (get_hwndFrame (), FID_VERTSCROLL);

  // Set the initial font

  if (init_fontnamesize)
    {
      initializing_font = true;
      WinSetPresParam (hwnd, PP_FONTNAMESIZE, strlen (init_fontnamesize) + 1,
                       (PVOID)init_fontnamesize);
      initializing_font = false;
    }

  // Create a presentation space handle for asynchronous painting (and
  // for retrieving the font metrics).

  create_hpsClient ();

  // Get the default line (rule) width
  // TODO: Does this really yield the width in pixels?
  LINEBUNDLE lbundle;
  if (GpiQueryAttrs (hpsClient, PRIM_LINE, LBB_WIDTH, &lbundle)
      == GPI_ALTERROR)
    rule_width = 1;
  else
    rule_width = FIXEDINT (lbundle.fxWidth);

  // Retrieve the current attributes of hpsClient.

  query_hps_attr (hpsClient, hpsClient_attr);

  return FALSE;
}


// Retrieve the font metrics of the default font.

void pmtxt::get_fontmetrics ()
{
  FONTMETRICS fm;
  GpiQueryFontMetrics (hpsClient, (LONG)sizeof (fm), &fm);
  cxChar = (int)fm.lMaxCharInc;
  cyChar = (int)fm.lMaxBaselineExt;
  cyDesc = (int)fm.lMaxDescender;

  fixed_pitch = (fm.fsType & FM_TYPE_FIXED);
}


void pmtxt::create_hpsClient ()
{
  hpsClient = WinGetPS (get_hwndClient ());
  setup_ps (hpsClient);
  get_fontmetrics ();
}


void pmtxt::setup_ps (HPS hps)
{
  GpiSetBackMix (hps, BM_OVERPAINT);
  set_font (hps);
}


void pmtxt::set_font (HPS hps)
{
  if (!default_font)
    {
      GpiCreateLogFont (hps, NULL, 1, &fattrs);
      GpiSetCharSet (hps, 1);
    }
}


void pmtxt::set_font (const pmtxt &s)
{
  SIZEF charbox;
  char fontnamesize[FACESIZE+10];

  // TODO: dropped font (and don't forget to update the constructor
  // of source_window which depends on the current behavior)
  if (!s.default_font && GpiQueryCharBox (s.hpsClient, &charbox))
    {
      _fmutex_checked_request (&mutex, _FMR_IGNINT);
      fattrs = s.fattrs;
      default_font = false;
      GpiCreateLogFont (hpsClient, NULL, 1, &fattrs);
      GpiSetCharSet (hpsClient, 1);
      GpiSetCharBox (hpsClient, &charbox);
      get_fontmetrics ();
      cache_line = -1;
      unlock ();
      clear_tabs ();
      change_size ();
      font_changed ();
      repaint ();
    }
  else if (s.default_font
           && WinQueryPresParam (s.get_hwndClient(), PP_FONTNAMESIZE, 0,
                                 NULL, sizeof (fontnamesize), fontnamesize,
                                 0) != 0)
    WinSetPresParam (get_hwndClient (), PP_FONTNAMESIZE,
                     strlen (fontnamesize) + 1, fontnamesize);
}


MRESULT pmtxt::wm_paint (HWND hwnd, ULONG, MPARAM, MPARAM)
{
  pmtxt_attr hps_attr;

  _fmutex_checked_request (&mutex, _FMR_IGNINT);
  HPS hps = WinBeginPaint (hwnd, NULLHANDLE, NULL);
  setup_ps (hps);
  query_hps_attr (hps, hps_attr);
  painting (true);
  GpiErase (hps);

  // TODO: examine update region to find lines to be displayed

  int line = y_off;

  // Compute the number of lines to display
  int count = window_lines;
  if (count > lines_used - line)
    count = lines_used - line;

  tab_repaint = false;
  for (int i = 0; i < count; ++i)
    paint (hps, hps_attr, line++, 0, false);
  painting (false);
  WinEndPaint (hps);
  if (tab_repaint)
    repaint ();
  else
    pmthread_pending |= PMTHREAD_SBAR_X | PMTHREAD_SBAR_Y;
  unlock ();
  return 0;
}


// Side effect: tab_repaint

void pmtxt::paint (HPS hps, pmtxt_attr &cur_attr, int line, int column,
                   bool paint_eol)
{
  int end;
  POINTL ptl;

  if (!get_bbox_pos (hps, &ptl, line, column))
    return;
  if (ptl.y < 0 || ptl.y >= cyClient)
    return;
  ptl.y += cyDesc;

  const pmtxt_line *pline = &lines_vector[line];
  char *p = pline->str + column;

  if (pline->attr == NULL)
    {
      // Optimization
      set_hps_attr (hps, cur_attr, default_attr);
      GpiCharStringAt (hps, &ptl, pline->len - column, (PCH)p);
      if (paint_eol)
        {
          // TODO
          pmtxt_line_attr la;
          la.el = E_FILL;
          la.attr = default_attr;
          paint (hps, cur_attr, &la, (char *)NULL, -1);
        }
    }
  else
    {
      tab_index = 0;
      const pmtxt_line_attr *a = pline->attr;
      while (a->column < column)
        {
          if (a->next == NULL || a->next->column > column)
            break;
          if (a->el == E_TAB && a->next != NULL
              && a->next->column != EOL_COLUMN)
            tab_index += a->next->column - a->column;
          a = a->next;
        }

      if (column < a->column)
        {
          set_hps_attr (hps, cur_attr, default_attr);
          end = a->column == EOL_COLUMN ? pline->len : a->column;
          GpiCharStringAt (hps, &ptl, end - column, (PCH)p);
          p += end - column;
          column = end;
        }
      else
        GpiMove (hps, &ptl);

      while (a != NULL && a->column != EOL_COLUMN)
        {
          if (a->next == NULL || a->next->column == EOL_COLUMN)
            end = pline->len;
          else
            end = a->next->column;
          if (column < end)
            paint (hps, cur_attr, a, p, end - column);
          p += end - column; column = end;
          a = a->next;
        }
      // TODO: Don't always clear to the end of the line (we need
      // to remember the previous max x
      if (a != NULL)            // a->column == EOL_COLUMN
        paint (hps, cur_attr, a, p, -1);
      else if (paint_eol)
        {
          // TODO
          pmtxt_line_attr la;
          la.el = E_FILL;
          la.attr = default_attr;
          paint (hps, cur_attr, &la, (char *)NULL, -1);
        }
    }

  POINTL ptl_end;
  GpiQueryCurrentPosition (hps, &ptl_end);
  int x = (ptl_end.x - (-x_off * cxChar) + cxChar - 1) / cxChar;
  if (x > max_x_pos)
    max_x_pos = x;

  if (pline->flags & LF_UNDERLINE)
    {
      RECTL rcli;
      rcli.xLeft = 0; rcli.xRight = cxClient;
      rcli.yBottom = ptl.y - cyDesc;
      rcli.yTop = rcli.yBottom + 1;
      WinFillRect (hps, &rcli, default_attr.fg_color);
    }
}


void pmtxt::paint (HPS hps, pmtxt_attr &cur_attr, const pmtxt_line_attr *pla,
                   const char *p, int len)
{
  POINTL ptl;
  RECTL rcli;
  int x, add, i;

  switch (pla->el)
    {
    case E_FILL:
      // TODO: Use GpiBox
      GpiQueryCurrentPosition (hps, &ptl);
      rcli.xLeft = ptl.x;
      // TODO: Width specified in string (pixels)
      rcli.xRight = len == -1 ? cxClient : rcli.xLeft + len * cxChar;
      rcli.yBottom = ptl.y - cyDesc;
      rcli.yTop = rcli.yBottom + cyChar;
      WinFillRect (hps, &rcli, pla->attr.bg_color);
      if (len != -1)
        {
          ptl.x = rcli.xRight;
          GpiMove (hps, &ptl);
        }
      break;

    case E_TEXT:
      set_hps_attr (hps, cur_attr, pla->attr);
      GpiCharString (hps, len, (PCH)p);
      break;

    case E_VRULE:
      // TODO: Use GpiBox
      GpiQueryCurrentPosition (hps, &ptl);
      rcli.xLeft = ptl.x;
      rcli.xRight = rcli.xLeft + rule_width;
      rcli.yBottom = ptl.y - cyDesc;
      rcli.yTop = rcli.yBottom + cyChar;
      // Note: CLR_DEFAULT does not work (background vs. foreground)
      WinFillRect (hps, &rcli, pla->attr.fg_color);
      ptl.x += rule_width;
      GpiMove (hps, &ptl);
      break;

    case E_TAB:
      // TODO: Use GpiBox
      GpiQueryCurrentPosition (hps, &ptl);
      x = ptl.x + x_off * cxChar; add = 0;
      while (len != 0)
        {
          if (x > tab_x[tab_index])
            {
              tab_repaint = true;
              add += x - tab_x[tab_index];
              for (i = tab_index; i < tab_count; ++i)
                tab_x[i] += add;
            }
          x = tab_x[tab_index];
          ++tab_index; --len;
        }

      rcli.xLeft = ptl.x;
      rcli.xRight = x - x_off * cxChar;
      rcli.yBottom = ptl.y - cyDesc;
      rcli.yTop = rcli.yBottom + cyChar;
      WinFillRect (hps, &rcli, pla->attr.bg_color);
      ptl.x = rcli.xRight;
      GpiMove (hps, &ptl);
      break;

    default:
      break;
    }
}


void pmtxt::sync ()
{
  if (output_pending)
    repaint ();
}


// TODO: Cache current scrollbar status to avoid WinSendMsg if nothing changed

static void check_size (int new_value, int target, HWND sbar, int *off)
{
  int m = target - new_value;
  if (m > 0)
    {
      WinEnableWindow (sbar, TRUE);
      if (*off > m)
        *off = m;
      WinSendMsg (sbar, SBM_SETSCROLLBAR,
                  MPFROMSHORT (*off), MPFROM2SHORT (0, MAX (0, (SHORT)m)));
      WinSendMsg (sbar, SBM_SETTHUMBSIZE,
                  MPFROM2SHORT ((USHORT)new_value, (USHORT)target), 0);
    }
  else
    {
      WinEnableWindow (sbar, FALSE);
      *off = 0;
    }
}


void pmtxt::check_x_size ()
{
  if (output_stopped)
    pmthread_pending |= PMTHREAD_SBAR_X;
  else if (same_tid ())
    {
      // Note that check_y_size() might be called recursively because
      // check_size() calls WinSendMsg
      unsigned rc = _fmutex_request (&mutex, _FMR_IGNINT | _FMR_NOWAIT);
      if (rc != 0)
        {
          pmthread_pending |= PMTHREAD_SBAR_X;
          // Avoid time window
          rc = _fmutex_request (&mutex, _FMR_IGNINT | _FMR_NOWAIT);
        }
      if (rc == 0)
        {
          check_size (window_columns, max_x_pos, hwndHbar, &x_off);
          unlock ();
        }
    }
  else
    pmthread_request (PMTHREAD_SBAR_X);
}


void pmtxt::check_y_size ()
{
  if (output_stopped)
    pmthread_pending |= PMTHREAD_SBAR_Y;
  else if (same_tid ())
    {
      // Note that check_y_size() might be called recursively because
      // check_size() calls WinSendMsg
      unsigned rc = _fmutex_request (&mutex, _FMR_IGNINT | _FMR_NOWAIT);
      if (rc != 0)
        {
          pmthread_pending |= PMTHREAD_SBAR_Y;
          // Avoid time window
          rc = _fmutex_request (&mutex, _FMR_IGNINT | _FMR_NOWAIT);
        }
      if (rc == 0)
        {
          check_size (window_lines, lines_used, hwndVbar, &y_off);
          unlock ();
        }
    }
  else
    pmthread_request (PMTHREAD_SBAR_Y);
}


void pmtxt::painting (bool)
{
}


void pmtxt::button_event (int, int, int, int, int)
{
}


MRESULT pmtxt::button_event (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2,
                             int button, int clicks)
{
  if (WinQueryFocus (HWND_DESKTOP) == hwnd)
    {
      int line, column, tab;
      _fmutex_checked_request (&mutex, _FMR_IGNINT);
      line_column_from_x_y (hpsClient, &line, &column, &tab,
                            (USHORT)SHORT1FROMMP (mp1),
                            (USHORT)SHORT2FROMMP (mp1));
      unlock ();
      button_event (line, column, tab, button, clicks);
      return (MRESULT)TRUE;
    }

  // Continue processing -- make window active.
  return WinDefWindowProc (hwnd, msg, mp1, mp2);
}


MRESULT pmtxt::wm_size (HWND, ULONG, MPARAM, MPARAM mp2)
{
  cxClient = SHORT1FROMMP (mp2);
  cyClient = SHORT2FROMMP (mp2);
  change_size ();
  return 0;
}


void pmtxt::change_size ()
{
  window_lines = (int)(cyClient / cyChar);
  window_columns = (int)(cxClient / cxChar);
  if (cyClient != window_lines * cyChar && !get_minimized ())
    {
      // I don't like calling WinSetWindowPos from WM_SIZE processing;
      // however, there seems to be no other way which works with
      // FCF_SHELLPOSITION (WM_ADJUSTWINDOWPOS does not work)
      SWP swp;
      WinQueryWindowPos (get_hwndClient (), &swp);
      POINTL ptl;
      ptl.x = swp.x;
      ptl.y = swp.y + cyClient;
      WinMapWindowPoints (get_hwndFrame (), HWND_DESKTOP, &ptl, 1);
      RECTL rcl;
      rcl.xLeft = ptl.x;
      rcl.xRight = rcl.xLeft + cxClient;
      rcl.yTop = ptl.y;
      rcl.yBottom = rcl.yTop - window_lines * cyChar;
      WinCalcFrameRect (get_hwndFrame (), &rcl, FALSE);
      // This causes recursion on WM_SIZE
      WinSetWindowPos (get_hwndFrame (), HWND_TOP, rcl.xLeft, rcl.yBottom,
                       rcl.xRight - rcl.xLeft, rcl.yTop - rcl.yBottom,
                       SWP_SIZE | SWP_MOVE);
    }
  check_x_size ();
  check_y_size ();
}


MRESULT pmtxt::wm_button (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
  switch (msg)
    {
    case WM_BUTTON1DOWN:
      return button_event (hwnd, msg, mp1, mp2, 1, 0);

    case WM_BUTTON2DOWN:
      return button_event (hwnd, msg, mp1, mp2, 2, 0);

    case WM_BUTTON3DOWN:
      return button_event (hwnd, msg, mp1, mp2, 3, 0);

    case WM_BUTTON1CLICK:
      return button_event (hwnd, msg, mp1, mp2, 1, 1);

    case WM_BUTTON2CLICK:
      return button_event (hwnd, msg, mp1, mp2, 2, 1);

    case WM_BUTTON3CLICK:
      return button_event (hwnd, msg, mp1, mp2, 3, 1);

    case WM_BUTTON1DBLCLK:
      return button_event (hwnd, msg, mp1, mp2, 1, 2);

    case WM_BUTTON2DBLCLK:
      return button_event (hwnd, msg, mp1, mp2, 2, 2);

    case WM_BUTTON3DBLCLK:
      return button_event (hwnd, msg, mp1, mp2, 3, 2);
    }
  return 0;
}


MRESULT pmtxt::wm_vscroll (HWND, ULONG, MPARAM, MPARAM mp2)
{
  int n, m, diff;

  m = lines_used - window_lines;
  switch (SHORT2FROMMP (mp2))
    {
    case SB_LINEUP:
      n = y_off - 1; break;
    case SB_LINEDOWN:
      n = y_off + 1; break;
    case SB_PAGEUP:
      n = y_off - window_lines; break;
    case SB_PAGEDOWN:
      n = y_off + window_lines; break;
    case SB_SLIDERTRACK:
      disable_output ();
      if (!track)
        return 0;
      n = SHORT1FROMMP (mp2);
      break;
    case SB_SLIDERPOSITION:
      enable_output ();
      n = SHORT1FROMMP (mp2);
      break;
    default:
      return 0;
    }
  if (n > m) n = m;
  if (n < 0) n = 0;
  diff = n - y_off;
  if (diff > 0)
    {
      y_off = n;
      scroll_up (diff, true);
    }
  else if (diff < 0)
    {
      y_off = n;
      scroll_down (-diff);
    }
  if (SHORT2FROMMP (mp2) != SB_SLIDERTRACK)
    WinSendMsg (hwndVbar, SBM_SETSCROLLBAR, MPFROMSHORT (y_off),
                MPFROM2SHORT (0, (SHORT)m));
  return 0;
}


MRESULT pmtxt::wm_hscroll (HWND, ULONG, MPARAM, MPARAM mp2)
{
  int n, m;

  m = max_x_pos - window_columns;
  switch (SHORT2FROMMP (mp2))
    {
    case SB_LINELEFT:
      n = x_off - 1; break;
    case SB_LINERIGHT:
      n = x_off + 1; break;
    case SB_PAGELEFT:
      n = x_off - window_columns; break;
    case SB_PAGERIGHT:
      n = x_off + window_columns; break;
    case SB_SLIDERTRACK:
      disable_output ();
      if (!track)
        return 0;
      n = SHORT1FROMMP (mp2);
      break;
    case SB_SLIDERPOSITION:
      enable_output ();
      n = SHORT1FROMMP (mp2);
      break;
    default:
      return 0;
    }
  if (n > m) n = m;
  if (n < 0) n = 0;
  if (n != x_off)
    {
      // TODO: use GpiBitBlt
      x_pos += (x_off - n) * cxChar;
      x_off = n;
      repaint ();
    }
  if (SHORT2FROMMP (mp2) != SB_SLIDERTRACK)
    WinSendMsg (hwndHbar, SBM_SETSCROLLBAR, MPFROMSHORT (x_off),
                MPFROM2SHORT (0, (SHORT)m));
  return 0;
}


MRESULT pmtxt::wm_presparamchanged (HWND hwnd, ULONG msg,
                                    MPARAM mp1, MPARAM mp2)
{
  parent::wm_presparamchanged (hwnd, msg, mp1, mp2);
  if (LONGFROMMP (mp1) == PP_FONTNAMESIZE && !initializing_font)
    {
      _fmutex_checked_request (&mutex, _FMR_IGNINT);
      default_font = true;
      WinReleasePS (hpsClient);
      create_hpsClient ();
      query_hps_attr (hpsClient, hpsClient_attr);
      cache_line = -1;
      unlock ();
      clear_tabs ();
      change_size ();
      repaint ();
    }
  return 0;
}


void pmtxt::font_changed ()
{
}


bool pmtxt::font_dialog ()
{
  FONTMETRICS fm;
  FONTDLG fd;
  UCHAR family_name[FACESIZE+1];
  LONG pel_per_inch;
  SIZEF charbox;

  _fmutex_checked_request (&mutex, _FMR_IGNINT);
  HDC hdc = GpiQueryDevice (hpsClient);
  bool ok = (GpiQueryFontMetrics (hpsClient, (LONG)sizeof (fm), &fm)
             && DevQueryCaps (hdc, CAPS_VERTICAL_FONT_RES, 1, &pel_per_inch)
             && GpiQueryCharBox (hpsClient, &charbox)
             && pel_per_inch != 0);
  unlock ();
  if (!ok)
    {
      WinAlarm (HWND_DESKTOP, WA_ERROR);
      return false;
    }

  memcpy (family_name, fm.szFamilyname, FACESIZE);
  family_name[FACESIZE] = 0;

  memset (&fd, 0, sizeof (fd));
  fd.cbSize = sizeof (fd);
  fd.hpsScreen = hpsClient;
  fd.hpsPrinter = NULLHANDLE;
  fd.pszTitle = (PSZ)"Font";
  fd.pszPreview = NULL;
  fd.pszPtSizeList = NULL;
  fd.pfnDlgProc = NULL;
  fd.pszFamilyname = family_name;
  if (fixed_pitch)
    fd.fxPointSize = MAKEFIXED (fm.sNominalPointSize, 0) / 10;
  else
    fd.fxPointSize = (72 * charbox.cy + pel_per_inch / 2) / pel_per_inch;
  fd.fl = (FNTS_HELPBUTTON | FNTS_RESETBUTTON | FNTS_CENTER
           | FNTS_INITFROMFATTRS);
  fd.flFlags = 0;
  fd.flType = fm.fsType;
  fd.flTypeMask = 0;
  fd.flStyle = 0;
  fd.flStyleMask = 0;
  fd.clrFore = CLR_BLACK;
  fd.clrBack = CLR_WHITE;
  fd.ulUser = 0;
  fd.lEmHeight = fm.lEmHeight;
  fd.lXHeight = fm.lXHeight;
  fd.lExternalLeading = fm.lExternalLeading;
  fd.fAttrs.usRecordLength = sizeof (FATTRS);
  fd.fAttrs.fsSelection = fm.fsSelection;
  fd.fAttrs.lMatch = fm.lMatch;
  memcpy (fd.fAttrs.szFacename, fm.szFacename, FACESIZE);
  fd.fAttrs.idRegistry = fm.idRegistry;
  fd.fAttrs.usCodePage = fm.usCodePage;
  fd.fAttrs.lMaxBaselineExt = fm.lMaxBaselineExt;
  fd.fAttrs.lAveCharWidth = fm.lAveCharWidth;
  fd.fAttrs.fsType = fm.fsType;
  fd.fAttrs.fsFontUse = 0;      // TODO?
  fd.sNominalPointSize = fm.sNominalPointSize;
  fd.usWeight = fm.usWeightClass;
  fd.usWidth = fm.usWidthClass;
  fd.usFamilyBufLen = FACESIZE;
  if (WinFontDlg (HWND_DESKTOP, get_hwndClient (), &fd) == NULLHANDLE)
    WinAlarm (HWND_DESKTOP, WA_ERROR);
  else if (fd.lReturn == DID_OK)
    {
      fd.fAttrs.fsFontUse = FATTR_FONTUSE_NOMIX;
      charbox.cx = charbox.cy = (fd.fxPointSize * pel_per_inch + 72/2)  / 72;
      _fmutex_checked_request (&mutex, _FMR_IGNINT);
      if (GpiCreateLogFont (hpsClient, NULL, 1, &fd.fAttrs) != FONT_MATCH
          || !GpiSetCharSet (hpsClient, 1)
          || !GpiSetCharBox (hpsClient, &charbox))
        {
          unlock ();
          WinAlarm (HWND_DESKTOP, WA_ERROR);
        }
      else
        {
          fattrs = fd.fAttrs;
          default_font = false;
          get_fontmetrics ();
          cache_line = -1;
          unlock ();
          clear_tabs ();
          change_size ();
          font_changed ();
          repaint ();
          return true;
        }
    }
  return false;
}


void pmtxt::unlock ()
{
  _fmutex_checked_release (&mutex);
  pmthread_perform ();
}


void pmtxt::pmthread_request (unsigned char bits)
{
  if (bits & ~pmthread_pending)
    {
      pmthread_pending |= bits;
      WinPostMsg (get_hwndClient (), UWM_PMTHREAD, 0, 0);
    }
}


void pmtxt::pmthread_perform ()
{
  if (same_tid ())
    {
      // TODO: Atomic
      unsigned char x = pmthread_pending;
      pmthread_pending = 0;
      if (x & PMTHREAD_SBAR_X)
        check_x_size ();
      if (x & PMTHREAD_SBAR_Y)
        check_y_size ();
    }
  else if (pmthread_pending != 0)
    WinPostMsg (get_hwndClient (), UWM_PMTHREAD, 0, 0);
}


bool pmtxt::wm_user (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
  if (parent::wm_user (hwnd, msg, mp1, mp2))
    return true;
  switch (msg)
    {
    case UWM_PMTHREAD:
      pmthread_perform ();
      return true;

    default:
      return false;
    }
}


// Fill-in TRACKINFO structure for resizing or moving a window.
// This function is called from the frame window procedure.

MRESULT pmtxt::wm_querytrackinfo (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
  // First call the normal frame window procedure to set up the
  // structure.

  MRESULT mr = get_old_frame_msg () (hwnd, msg, mp1, mp2);

  // If we're resizing the window, set the grid.

  USHORT fs = SHORT1FROMMP (mp1);
  if (((fs & TF_MOVE) != TF_MOVE)
      && ((fs & TF_MOVE) || (fs & TF_SETPOINTERPOS))
      && cxChar != 0 && cyChar != 0)
    {
      PTRACKINFO pti = (PTRACKINFO)mp2;
      pti->fs |= TF_GRID;
      if (fixed_pitch)
        pti->cxGrid = pti->cxKeyboard = cxChar;
      else
        pti->cxGrid = pti->cxKeyboard = 1;
      pti->cyGrid = pti->cyKeyboard = cyChar;
    }
  return mr;
}


void pmtxt::delete_lines (int line, int count, bool do_paint)
{
  _fmutex_checked_request (&mutex, _FMR_IGNINT);
  if (line >= 0 && line < lines_used && count >= 1)
    {
      if (line + count > lines_used)
        count = lines_used - line;
      if (cache_line >= line)
        cache_line = -1;
      int i;
      for (i = 0; i < count; ++i)
        free_line (line + i);
      for (i = line; i < lines_used - count; ++i)
        lines_vector[i] = lines_vector[i+count];
      for (i = lines_used - count; i < lines_used; ++i)
        init_line (i);
      lines_used -= count;
      int first = line - y_off;
      if (first < 0) first = 0;
      if (y_off > lines_used - window_lines)
        {
          y_off = lines_used - window_lines;
          if (y_off < 0)
            y_off = 0;
          // TODO: Use GpiBitBlt?
          if (do_paint && !output_stopped)
            repaint ();
          else
            output_pending = true;
        }
      else if (first < window_lines)
        {
          if (!(do_paint && !output_stopped))
            output_pending = true;
          else if (first + count >= window_lines)
            {
              // No scrolling, just overwrite the last line(s)
              // Clear first because paint() does nothing for lines
              // beyond the current end of lines_vector[]
              int n = window_lines - first;
              RECTL rcli;
              rcli.xLeft = 0; rcli.xRight = cxClient;
              rcli.yBottom = 0; rcli.yTop = n * cyChar;
              painting (true);
              WinFillRect (hpsClient, &rcli, CLR_BACKGROUND);

              // Repaint the last line(s)
              for (i = 0; i < n; ++i)
                paint (hpsClient, hpsClient_attr,
                       window_lines - n + i + y_off, 0, false);
              painting (false);
            }
          else
            {
              RECTL rcli, rcl, rcl_update, rcl_dummy;
              rcli.xLeft = 0; rcli.xRight = cxClient;
              rcli.yBottom = 0; rcli.yTop = cyClient - first * cyChar;
              rcl.xLeft = rcli.xLeft; rcl.xRight = rcli.xRight - 1;
              rcl.yBottom = rcli.yBottom; rcl.yTop = rcli.yTop - 1;
              if (!output_pending
                  && GpiRectVisible (hpsClient, &rcl) == RVIS_VISIBLE
                  && (!WinQueryUpdateRect (get_hwndClient (), &rcl_update)
                      || !WinIntersectRect (get_hab (), &rcl_dummy,
                                            &rcl_update, &rcli)))
                {
                  // Scroll
                  POINTL aptl[3];
                  aptl[0].x = 0; aptl[0].y = count * cyChar;
                  aptl[1].x = cxClient; aptl[1].y = cyClient - first * cyChar;
                  aptl[2].x = 0; aptl[2].y = 0;
                  painting (true);
                  GpiBitBlt (hpsClient, hpsClient, 3, aptl, ROP_SRCCOPY,
                             BBO_IGNORE);

                  // Clear the last line(s)
                  rcli.xLeft = 0; rcli.xRight = cxClient;
                  rcli.yBottom = 0; rcli.yTop = count * cyChar;
                  WinFillRect (hpsClient, &rcli, CLR_BACKGROUND);

                  // Repaint the last line(s)
                  for (i = 0; i < count; ++i)
                    paint (hpsClient, hpsClient_attr,
                           window_lines - count + i + y_off, 0, false);

                  painting (false);
                }
              else
                {
                  // TODO: Don't have to repaint all the lines
                  repaint ();
                }
            }
        }
    }
  unlock ();
  check_y_size ();
}


void pmtxt::insert_lines (int line, int count, bool do_paint)
{
  _fmutex_checked_request (&mutex, _FMR_IGNINT);
  if (line >= 0 && line <= lines_used && count >= 1)
    {
      if (lines_used + count > lines_alloc)
        more_lines (lines_used + count);
      if (cache_line >= line)
        cache_line = -1;
      if (line == lines_used)
        lines_used += count;
      else
        {
          int i;
          for (i = 0; i < count; ++i)
            free_line (lines_used + i);
          for (i = lines_used - 1; i >= line; --i)
            lines_vector[i+count] = lines_vector[i];
          for (i = 0; i < count; ++i)
            init_line (line + i);
          lines_used += count;

          int first = line - y_off;
          if (first < 0) first = 0;
          if (first < window_lines)
            {
              if (!(do_paint && !output_stopped))
                output_pending = true;
              else if (first + count >= window_lines)
                {
                  // No scrolling, just clear the inserted line(s)
                  int n = window_lines - first;
                  RECTL rcli;
                  rcli.xLeft = 0; rcli.xRight = cxClient;
                  rcli.yBottom = 0; rcli.yTop = n * cyChar;
                  painting (true);
                  WinFillRect (hpsClient, &rcli, CLR_BACKGROUND);
                  painting (false);
                }
              else
                {
                  RECTL rcli, rcl, rcl_update, rcl_dummy;
                  rcli.xLeft = 0; rcli.xRight = cxClient;
                  rcli.yBottom = 0; rcli.yTop = cyClient - first * cyChar;
                  rcl.xLeft = rcli.xLeft; rcl.xRight = rcli.xRight - 1;
                  rcl.yBottom = rcli.yBottom; rcl.yTop = rcli.yTop - 1;
                  if (!output_pending
                      && GpiRectVisible (hpsClient, &rcl) == RVIS_VISIBLE
                      && (!WinQueryUpdateRect (get_hwndClient (), &rcl_update)
                          || !WinIntersectRect (get_hab (), &rcl_dummy,
                                                &rcl_update, &rcli)))
                    {
                      // Scroll
                      POINTL aptl[3];
                      aptl[0].x = 0; aptl[0].y = 0;
                      aptl[1].x = cxClient;
                      aptl[1].y = (window_lines - first - count) * cyChar;
                      aptl[2].x = 0; aptl[2].y = count * cyChar;
                      painting (true);
                      GpiBitBlt (hpsClient, hpsClient, 3, aptl, ROP_SRCCOPY,
                                 BBO_IGNORE);

                      // Clear the inserted line(s)
                      rcli.xLeft = 0; rcli.xRight = cxClient;
                      rcli.yTop = cyClient - first * cyChar;
                      rcli.yBottom = rcl.yTop - count * cyChar;
                      WinFillRect (hpsClient, &rcli, CLR_BACKGROUND);
                      painting (false);
                    }
                  else
                    {
                      // TODO: Don't have to repaint all the lines
                      repaint ();
                    }
                }
            }
        }
    }
  unlock ();
  check_y_size ();
}


void pmtxt::clear_lines (int line, int count, bool do_paint)
{
  _fmutex_checked_request (&mutex, _FMR_IGNINT);
  if (line >= 0 && line < lines_used && count >= 1)
    {
      if (line + count > lines_used)
        count = lines_used - line;
      if (cache_line >= line && cache_line < line + count)
        cache_line = -1;
      for (int i = 0; i < count; ++i)
        {
          lines_vector[line + i].len = 0;
          delete_attr (lines_vector[line + i].attr);
          lines_vector[line + i].attr = NULL;
        }

      int first = line - y_off;
      if (first < window_lines && first + count > 0)
        {
          if (first < 0)
            {
              count += first;
              first = 0;
            }
          if (first + count > window_lines)
            count = window_lines - first;
          if (do_paint && !output_stopped)
            {
              RECTL rcli;
              rcli.xLeft = 0; rcli.xRight = cxClient;
              rcli.yTop =  cyClient - first * cyChar;
              rcli.yBottom = rcli.yTop - count * cyChar;
              painting (true);
              WinFillRect (hpsClient, &rcli, CLR_BACKGROUND);
              painting (false);
            }
          else
            output_pending = true;
        }
    }
  unlock ();
}


void pmtxt::truncate_line (int line, int count, bool do_paint)
{
  _fmutex_checked_request (&mutex, _FMR_IGNINT);
  if (line >= 0 && line < lines_used)
    {
      pmtxt_line *pline = &lines_vector[line];
      if (pline->len > count)
        {
          pmtxt_line_attr **pa = &pline->attr;
          while (*pa != NULL && (*pa)->column <= count)
            pa = &(*pa)->next;
          pmtxt_line_attr *a = *pa, *next;
          while (a != NULL && a->column != EOL_COLUMN)
            {
              next = a->next;
              delete a;
              a = next;
            }
          *pa = a;
          pline->len = count;
          if (cache_line == line && cache_column > count)
            cache_line = -1;
          if (do_paint)
            paint (hpsClient, hpsClient_attr, line, count, true);
        }
    }
  unlock ();
}


// Return true iff the caller is responsible for painting the area
// becoming free

bool pmtxt::scroll_up (int n, bool do_paint)
{
  RECTL rcl, rcli, rcl_update, rcl_dummy;

  if (n >= window_lines || output_pending)
    {
      if (!do_paint)
        cache_line = -1;
      repaint ();
      output_pending = false;
      return false;
    }
  rcli.xLeft = 0; rcli.xRight = cxClient;
  rcli.yBottom = 0; rcli.yTop = cyClient - n * cyChar;
  rcl.xLeft = 0; rcl.xRight = cxClient - 1;
  rcl.yBottom = 0; rcl.yTop = cyClient - n * cyChar - 1;
  if (GpiRectVisible (hpsClient, &rcl) == RVIS_VISIBLE
      && (!WinQueryUpdateRect (get_hwndClient (), &rcl_update)
          || !WinIntersectRect (get_hab (), &rcl_dummy, &rcl_update, &rcli)))
    {
      // Source rectangle completely visible and not to be updated; scroll!
      POINTL aptl[3];
      aptl[0].x = 0; aptl[0].y = n * cyChar;
      aptl[1].x = cxClient; aptl[1].y = cyClient;
      aptl[2].x = 0; aptl[2].y = 0;
      painting (true);
      GpiBitBlt (hpsClient, hpsClient, 3, aptl, ROP_SRCCOPY, BBO_IGNORE);
      rcli.xLeft = 0; rcli.xRight = cxClient;
      rcli.yBottom = 0; rcli.yTop = n * cyChar;
      if (do_paint)
        {
          painting (false);
          repaint (&rcli);
          return false;
        }
      else
        {
          WinFillRect (hpsClient, &rcli, CLR_BACKGROUND);
          painting (false);
          return true;
        }
    }
  else
    {
      if (!do_paint)
        cache_line = -1;
      repaint ();
      return false;
    }
}


void pmtxt::scroll_down (int n)
{
  RECTL rcl, rcli, rcl_update, rcl_dummy;

  if (n >= window_lines || output_pending)
    {
      repaint ();
      output_pending = false;
      return;
    }
  rcli.xLeft = 0; rcli.xRight = cxClient;
  rcli.yBottom = n * cyChar; rcli.yTop = cyClient;
  rcl.xLeft = 0; rcl.xRight = cxClient - 1;
  rcl.yBottom = n * cyChar; rcl.yTop = cyClient - 1;
  if (GpiRectVisible (hpsClient, &rcl) == RVIS_VISIBLE
      && (!WinQueryUpdateRect (get_hwndClient (), &rcl_update)
          || !WinIntersectRect (get_hab (), &rcl_dummy, &rcl_update, &rcli)))
    {
      // Source rectangle completely visible and not to be updated; scroll!
      POINTL aptl[3];
      aptl[0].x = 0; aptl[0].y = 0;
      aptl[1].x = cxClient; aptl[1].y = cyClient - n * cyChar;
      aptl[2].x = 0; aptl[2].y = n * cyChar;
      painting (true);
      GpiBitBlt (hpsClient, hpsClient, 3, aptl, ROP_SRCCOPY, BBO_IGNORE);
      painting (false);
      rcli.xLeft = 0; rcli.xRight = cxClient;
      rcli.yTop = cyClient; rcli.yBottom = cyClient - n * cyChar;
      repaint (&rcli);
    }
  else
    repaint ();
}


void pmtxt::disable_output ()
{
  output_stopped = true;
}


void pmtxt::enable_output ()
{
  output_stopped = false;
  if (output_pending)
    {
      output_pending = false;
      repaint ();
    }
}


void pmtxt::show_line (int line, int threshold, int overshoot)
{
  int n;

  if (overshoot < threshold)
    overshoot = threshold;
  if (line - threshold < y_off)
    {
      line = line >= overshoot ? line - overshoot : 0;
      n = y_off - line;
      y_off = line;
      scroll_down (n);
      check_y_size ();
    }
  else if (line + threshold >= y_off + window_lines)
    {
      line = line + overshoot < lines_used ? line + overshoot : lines_used - 1;
      n = line - (y_off + window_lines - 1);
      y_off = line - (window_lines - 1);
      scroll_up (n, true);
      check_y_size ();
    }
}


// TODO: E_TEXT only
int pmtxt::get_char (int line, int column)
{
  if (line < 0 || line >= lines_used || column < 0)
    return -1;
  if (column >= lines_vector[line].len)
    return -1;
  return (unsigned char)lines_vector[line].str[column];
}


// TODO: E_TEXT only
bool pmtxt::get_string (int line, int column, char *str, size_t count)
{
  if (line < 0 || line >= lines_used || column < 0)
    return false;
  if (column >= lines_vector[line].len)
    count = 0;
  else if (column + (int)count > lines_vector[line].len)
    count = lines_vector[line].len - column;
  memcpy (str, lines_vector[line].str + column, count);
  str[count] = 0;
  return true;
}


void pmtxt::clear_tabs ()
{
  _fmutex_checked_request (&mutex, _FMR_IGNINT);
  for (int i = 0; i < tab_count;++i)
    tab_x[i] = 0;
  unlock ();
}


bool pmtxt::get_bbox_pos (HPS hps, POINTL *pptl, int line, int column)
{
  if (line >= lines_used)
    return false;

  int x = -1;
  int line_len = lines_vector[line].len;

  if (column == 0)
    x = 0;
  else if (line == cache_line && column == cache_column)
    x = cache_x;

  if (column > line_len)
    return false;

  if (x == -1)
    {
      POINTL ptl; ptl.x = 0; ptl.y = 0;
      char *p = lines_vector[line].str;
      const pmtxt_line_attr *a = lines_vector[line].attr;

      // TODO: Use extra PS when adding fonts
      // TODO: Start at cache_column if possible

      if (a == NULL || column <= a->column)
        {
          if (fixed_pitch)
            x = ptl.x + column * cxChar;
          else
            {
              GpiQueryCharStringPosAt (hps, &ptl, 0, column, (PCH)p,
                                       NULL, aptl_line);
              x = aptl_line[column].x;
            }
        }
      else
        {
          int cur_column = 0, t = 0, end, len;
          x = 0;

          if (a->column != 0)
            {
              if (fixed_pitch)
                x += a->column * cxChar;
              else
                {
                  GpiQueryCharStringPosAt (hps, &ptl, 0, a->column,
                                           (PCH)p, NULL, aptl_line);
                  x += aptl_line[a->column].x;
                }
              cur_column = a->column;
            }

          while (a != NULL && cur_column != column)
            {
              // TODO: Don't skip past font changes
              while (a->next != NULL && a->next->el == a->el)
                {
                  if (a->el == E_TAB && a->next->column != EOL_COLUMN)
                    t += a->next->column - a->column;
                  a = a->next;
                }

              if (a->column == EOL_COLUMN || a->next == NULL)
                end = line_len;
              else
                end = a->next->column;
              if (column < end)
                len = column - cur_column;
              else
                len = end - cur_column;
              switch (a->el)
                {
                case E_TEXT:
                  if (fixed_pitch)
                    x += len * cxChar;
                  else
                    {
                      GpiQueryCharStringPosAt (hps, &ptl, 0, len,
                                               (PCH)p + cur_column, NULL,
                                               aptl_line);
                      x += aptl_line[len].x;
                    }
                  break;
                case E_FILL:
                  // TODO: Width specified in string (pixels)
                  x += len * cxChar;
                  break;
                case E_VRULE:
                  x += rule_width;
                  break;
                case E_TAB:
                  t += len;
                  x = tab_x[t-1];
                  break;
                default:
                  break;
                }
              a = a->next;
              cur_column += len;
            }
        }
      cache_line = line; cache_column = column; cache_x = x;
    }
  pptl->x = x - x_off * cxChar;
  pptl->y = cyClient - cyChar * (line + 1 - y_off);
  return true;
}


// This is the reverse of get_bbox_pos()
// TODO: Use cache

void pmtxt::line_column_from_x_y (HPS hps, int *out_line, int *out_column,
                                  int *out_tab, int x, int y)
{
  int line = (cyClient - 1 - y) / cyChar + y_off;
  if (line >= 0 && line < lines_used)
    {
      *out_line = line;
      pmtxt_line *pline = &lines_vector[line];
      POINTL ptl;
      ptl.x = -x_off * cxChar; ptl.y = 0;
      if (x >= ptl.x)
        {
          const pmtxt_line_attr *a = pline->attr;
          pmtxt_line_attr temp;
          if (a == NULL || a->column > 0)
            {
              temp.el = E_TEXT;
              temp.next = (pmtxt_line_attr *)a; // Remove `const'
              temp.column = 0;
              a = &temp;
            }
          int tab_index = 0;
          while (a != NULL && a->column != EOL_COLUMN)
            {
              int len;
              if (a->next != NULL && a->next->column != EOL_COLUMN)
                len = a->next->column - a->column;
              else
                len = pline->len - a->column;
              if (a->el == E_TEXT)
                {
                  if (fixed_pitch)
                    {
                      if (x < ptl.x + len * cxChar)
                        {
                          *out_column = a->column + (x - ptl.x) / cxChar;
                          *out_tab = tab_index;
                          return;
                        }
                      ptl.x += len * cxChar;
                    }
                  else
                    {
                      GpiQueryCharStringPosAt (hps, &ptl, 0, len,
                                               (PCH)pline->str + a->column,
                                               NULL, aptl_line);
                      for (int i = 0; i < len; ++i)
                        if (x < aptl_line[i+1].x)
                          {
                            *out_column = a->column + i;
                            *out_tab = tab_index;
                            return;
                          }
                      ptl.x = aptl_line[len].x;
                    }
                }
              else if (a->el == E_VRULE)
                {
                  if (x < ptl.x + rule_width)
                    {
                      *out_column = a->column;
                      *out_tab = tab_index;
                      return;
                    }
                  ptl.x += rule_width;
                }
              else if (a->el == E_FILL)
                {
                  if (x < ptl.x + len * cxChar)
                    {
                      *out_column = a->column + (x - ptl.x) / cxChar;
                      *out_tab = tab_index;
                      return;
                    }
                  ptl.x += len * cxChar;
                }
              else if (a->el == E_TAB)
                {
                  if (x < tab_x[tab_index])
                    {
                      *out_column = a->column;
                      *out_tab = tab_index;
                      return;
                    }
                  ptl.x = tab_x[tab_index++] - x_off * cxChar;
                }
              a = a->next;
            }
        }
    }
  else
    *out_line = -1; 
  *out_column = -1;
  *out_tab = -1;
}


// 
// Add text
//
// Note that this function is not a no-op with LEN==0: it may add
// an empty line at the end, for the cursor.

void pmtxt::put (int line, int column, size_t len, const char *s,
                 const pmtxt_attr &attr, element el, bool do_paint)
{
  int lines_added;

  if (line < 0 || column < 0)
    return;

  _fmutex_checked_request (&mutex, _FMR_IGNINT);

  if (line >= lines_alloc)
    more_lines (line + 1);

  pmtxt_line *pline = &lines_vector[line];
  int tlen = pline->len;

  if (column > tlen)
    {
      unlock ();
      return;
    }

  if (line >= lines_used)
    {
      lines_added = line + 1 - lines_used;
      lines_used = line + 1;
    }
  else
    lines_added = 0;

  if (column + (int)len > max_line_len)
    {
      len = max_line_len - column;
      if (len == 0)
        {
          unlock ();
          return;
        }
    }
  if (column + (int)len > pline->alloc)
    {
      int new_alloc = ((column + len + 39) / 40) * 40;
      char *t = new char[new_alloc];
      memcpy (t, pline->str, tlen);
      delete[] pline->str;
      pline->alloc = new_alloc;
      pline->str = t;
    }

  char *target = pline->str;

  memcpy (target + column, s, len);

  if (len != 0)
    {
      if (column + (int)len > tlen)
        pline->len = column + len;
      change_attr (pline, column, column + (int)len, attr, el);
      if (line == cache_line && column < cache_column)
        cache_line = -1;
    }

  if (do_paint && !output_stopped)
    {
      if (lines_added != 0 && lines_used > window_lines)
        {
          if (y_off + lines_added == lines_used - window_lines)
            {
              y_off += lines_added;
              bool paint_after_scroll = scroll_up (lines_added, false);
              check_y_size ();
              if (!paint_after_scroll)
                {
                  unlock ();
                  return;
                }
            }
          else
            check_y_size ();
        }

      if (len != 0)
        {
          tab_repaint = false;
          // TODO: Set do_paint to true only if required
          painting (true);
          paint (hpsClient, hpsClient_attr, line, column, true);
          painting (false);
          if (tab_repaint)
            repaint ();
        }
    }
  else
    {
      if (output_stopped && (lines_added || len != 0))
        output_pending = true;
    }

  unlock ();
}


//
// Change attributes of existing text
//
void pmtxt::put (int line, int column, size_t len, const pmtxt_attr &attr,
                 bool do_paint)
{
  if (line < 0 || line >= lines_used || column < 0)
    return;

  _fmutex_checked_request (&mutex, _FMR_IGNINT);

  pmtxt_line *pline = &lines_vector[line];
  int tlen = pline->len;
  if (column + (int)len > tlen)
    {
      if (column >= tlen)
        len = 0;
      else
        len = tlen - column;
    }
  if (len == 0)
    {
      unlock ();
      return;
    }

  change_attr (pline, column, column + (int)len, attr, E_KEEP);

  // TODO: Invalidate cache if font width changed

  if (do_paint && !output_stopped)
    {
      tab_repaint = false;
      painting (true);
      paint (hpsClient, hpsClient_attr, line, column, true);
      painting (false);
      if (tab_repaint)
        repaint ();
    }
  else
    {
      if (output_stopped)
        output_pending = true;
    }

  unlock ();
}


void pmtxt::set_eol_attr (int line, const pmtxt_attr &attr, bool do_paint)
{
  _fmutex_checked_request (&mutex, _FMR_IGNINT);
  if (line >= 0 && line < lines_used)
    {
      pmtxt_line_attr **ppa;
      for (ppa = &lines_vector[line].attr; *ppa != NULL; ppa = &(*ppa)->next)
        if ((*ppa)->column == EOL_COLUMN)
          break;
      bool change = false;
      if (*ppa == NULL)
        {
          if (!eq_attr (attr, default_attr))
            {
              pmtxt_line_attr *a = new pmtxt_line_attr;
              a->next = NULL;
              a->column = EOL_COLUMN;
              a->el = E_FILL;
              a->attr = attr;
              *ppa = a;
              change = true;
            }
        }
      else
        {
          if (eq_attr (attr, default_attr))
            {
              pmtxt_line_attr *a = *ppa;
              *ppa = NULL;
              delete a;
              change = true;
            }
          else if (!eq_attr (attr, (*ppa)->attr))
            {
              (*ppa)->attr = attr;
              change = true;
            }
        }
      if (change)
        {
          if (do_paint && !output_stopped)
            {
              POINTL ptl;
              RECTL rcli;
              if (!get_bbox_pos (hpsClient, &ptl, line,
                                 lines_vector[line].len))
                abort ();
              rcli.xLeft = ptl.x; rcli.xRight = cxClient;
              rcli.yBottom = ptl.y; rcli.yTop = ptl.y + cyChar;
              painting (true);
              WinFillRect (hpsClient, &rcli, attr.bg_color);
              painting (false);
            }
          else
            output_pending = true;
        }
    }
  unlock ();
}


// TODO: underline off, underline to end of line, underline to width of window
// TODO: attribute
void pmtxt::underline (int line, bool on, bool do_paint)
{
  _fmutex_checked_request (&mutex, _FMR_IGNINT);
  if (line >= 0 && line < lines_used
      && (bool)(lines_vector[line].flags & LF_UNDERLINE) != on)
    {
      if (on)
        lines_vector[line].flags |= LF_UNDERLINE;
      else
        lines_vector[line].flags &= ~LF_UNDERLINE;
      if (do_paint && !output_stopped)
        {
          painting (true);
          paint (hpsClient, hpsClient_attr, line, 0, true);
          painting (false);
        }
    }
  unlock ();
}


struct change_data
{
  pmtxt_line_attr **add;
  pmtxt_line_attr *last;
  pmtxt_attr cur_attr;
  pmtxt::element cur_el;
};


void change_at (change_data &c, pmtxt_line_attr *a, int column,
                const pmtxt_attr &attr, pmtxt::element el)
{
  if (column != EOL_COLUMN && eq_attr (attr, c.cur_attr) && el == c.cur_el)
    {
      if (a != NULL)
        delete a;
    }
  else if (c.last != NULL && c.last->column == column)
    {
      c.last->attr = attr;
      c.last->el = el;
      c.cur_attr = attr;
      c.cur_el = el;
      if (a != NULL)
        delete a;
    }
  else
    {
      if (a == NULL)
        a = new pmtxt_line_attr;
      a->next = NULL;
      a->column = column;
      a->attr = attr;
      a->el = el;
      c.cur_attr = attr;
      c.cur_el = el;
      c.last = a;
      *c.add = a;
      c.add = &a->next;
    }
}


// TODO: Optimize to avoid copying/writing

void pmtxt::change_attr (pmtxt_line *p, int start, int end,
                         const pmtxt_attr &new_attr, element new_el)
{
  pmtxt_line_attr *new_list, *a, *next;
  pmtxt_attr prev_attr;
  element prev_el;
  change_data c;

  new_list = NULL;
  c.add = &new_list;
  c.last = NULL;
  c.cur_attr = default_attr;
  c.cur_el = E_TEXT;

  a = p->attr;
  while (a != NULL && a->column < start)
    {
      next = a->next;
      change_at (c, a, a->column, a->attr, a->el);
      a = next;
    }

  prev_attr = c.cur_attr; prev_el = c.cur_el;
  if (a == NULL)
    change_at (c, NULL, start, new_attr,
               new_el != E_KEEP ? new_el : c.cur_el);
  else if (new_el == E_KEEP)
    {
      change_at (c, NULL, start, new_attr, c.cur_el);
      while (a != NULL && a->column < end)
        {
          next = a->next;
          change_at (c, a, a->column, new_attr, a->el);
          a = next;
        }
    }
  else
    {
      if (a->column == start)
        {
          prev_attr = a->attr; prev_el = a->el;
          next = a->next;
          change_at (c, a, start, new_attr, new_el);
          a = next;
        }
      else
        {
          // TODO: if a->column < end, we can reuse the node
          change_at (c, NULL, start, new_attr, new_el);
        }
      while (a != NULL && a->column < end)
        {
          next = a->next;
          prev_attr = a->attr; prev_el = a->el;
          delete a;
          a = next;
        }
    }

  if (a == NULL || a->column != end)
    change_at (c, NULL, end, prev_attr, prev_el);

  while (a != NULL)
    {
      next = a->next;
      change_at (c, a, a->column, a->attr, a->el);
      a = next;
    }

  p->attr = new_list;

  int t = 0, i;
   for (a = p->attr; a != NULL; a = a->next)
    ++t;
  if (t > tab_x_size)
    {
      tab_x_size = t + 16;
      int *new_x = new int[tab_x_size];
      for (i = 0; i < tab_count; ++i)
        new_x[i] = tab_x[i];
      delete[] tab_x;
      tab_x = new_x;
    }
  if (t > tab_count)
    {
      for (i = tab_count; i < t; ++i)
        tab_x[i] = 0;
      tab_count = t;
    }
}
