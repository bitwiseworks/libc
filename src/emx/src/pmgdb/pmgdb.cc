/* pmgdb.cc
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


#define INCL_WIN
#include <os2.h>
#include <stdlib.h>
#include <stdarg.h>
#include "string.h"
#include "pmapp.h"
#include "pmframe.h"
#include "pmtxt.h"
#include "pmtty.h"
#include "pmgdb.h"
#include "breakpoi.h"
#include "source.h"
#include "command.h"

int main (int, char *argv[])
{
  pmapp *app = new pmapp ("pmgdb");
  command_window *cmd = new command_window (app, ID_COMMAND_WINDOW,
                                            "8.Courier", argv + 1);
  app->message_loop ();
  delete cmd;
  delete app;
  return 0;
}
