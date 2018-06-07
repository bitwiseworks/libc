/* register.cc
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
#include <os2.h>
#include <stdlib.h>
#include <string.h>
#include "string.h"
#include "pmapp.h"
#include "pmframe.h"
#include "pmtxt.h"
#include "pmtty.h"
#include "pmgdb.h"
#include "help.h"
#include "breakpoi.h"
#include "register.h"
#include "command.h"


register_window::register_window (command_window *in_cmd, unsigned in_id,
                                  const SWP *pswp, const char *fontnamesize)
  : pmtxt (in_cmd->get_app (), in_id,
           pswp == NULL ? FCF_SHELLPOSITION : 0, pswp, fontnamesize)
{
  cmd = in_cmd;

  int x = 0;
  put (0, x, 6, " Reg ", true); x += 6;
  put_tab (0, x++, true);
  put_vrule (0, x++, true);
  put (0, x, 5, " Hex ", true); x += 5;
  put_tab (0, x++, true);
  put_vrule (0, x++, true);
  put (0, x, 5, " Dec ", true); x += 5;
  underline (0, true, true);

  set_title ("pmgdb - Registers");
  set_keys_help_id (HELP_REG_KEYS);
}


register_window::~register_window ()
{
}


MRESULT register_window::wm_close (HWND, ULONG, MPARAM, MPARAM)
{
  show (false);
  return 0;
}


MRESULT register_window::wm_command (HWND hwnd, ULONG msg,
                                     MPARAM mp1, MPARAM mp2)
{
  switch (SHORT1FROMMP (mp1))
    {
    case IDM_REFRESH:
      cmd->update_registers ();
      return 0;
    }
  return cmd->wm_command (hwnd, msg, mp1, mp2);
}


MRESULT register_window::wm_activate (HWND hwnd, ULONG msg,
                                          MPARAM mp1, MPARAM mp2)
{
  if (SHORT1FROMMP (mp1))
    cmd->associate_help (get_hwndFrame ());
  return WinDefWindowProc (hwnd, msg, mp1, mp2);
}


#define WHITE(c) ((c) == ' ' || (c) == '\t')

void register_window::update (const char *s)
{
  char buf[20];
  buf[0] = ' ';
  int line = 1;
  while (*s != 0)
    {
      int len = 0;
      while (s[len] != 0 && !WHITE (s[len]))
        ++len;
      if (!WHITE (s[len]))
        break;
      int x = 0;
      memcpy (buf + 1, s, len);
      buf[len+1] = ' ';
      put (line, x, len + 2, buf, true); x += len + 2;
      put_tab (line, x++, true);
      put_vrule (line, x++, true);
      s += len;
      while (WHITE (*s))
        ++s;
      if (*s++ != '0' || *s++ != 'x')
        break;
      len = 0;
      while (s[len] != 0 && !WHITE (s[len]))
        ++len;
      if (!WHITE (s[len]) || len > 8)
        break;
      memset (buf + 1, '0', 7);
      memcpy (buf + 1 + 8 - len, s, len);
      buf[9] = ' ';
      put (line, x, 10, buf, true); x += 10;
      put_tab (line, x++, true);
      put_vrule (line, x++, true);
      s += len;
      while (WHITE (*s))
        ++s;
      len = 0;
      while (s[len] != 0 && s[len] != '\n')
        ++len;
      if (s[len] != '\n')
        break;
      if (s[0] == '0' && s[1] == 'x')
        {
          long n = strtol (s + 2, NULL, 16);
          int len2 = strlen (_ltoa (n, buf + 1, 10));
          put (line, x, len2 + 1, buf, true); x += len2 + 1;
        }
      else
        {
          memcpy (buf + 1, s, len);
          put (line, x, len + 1, buf, true); x += len + 1;
        }
      truncate_line (line, x, true);
      ++line;
      s += len + 1;
    }
}
