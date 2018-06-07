/* capture.cc
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
#include "capture.h"


void capture_string::set (const char *s, size_t len)
{
  str = new char[len + 1];
  memcpy (str, s, len);
  str[len] = 0;
}


void delete_capture (capture *p)
{
  while (p != NULL)
    {
      capture *next = p->next;
      delete p;
      p = next;
    }
}
