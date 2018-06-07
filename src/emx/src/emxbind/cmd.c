/* cmd.c -- Handle commands
   Copyright (c) 1991-1995 Eberhard Mattes

This file is part of emxbind.

emxbind is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

emxbind is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with emxbind; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "defs.h"
#include "emxbind.h"


/* Perform initializations for the bind operation. */

static void init_bind (void)
{
  if (dll_flag)
    {
      relocatable = TRUE;
      stack_size = 0;
      heap_size = 0;
    }
}


/* Change the application type. */

static void exe_type (void)
{
  read_os2_header ();
  exe_flags ();
  my_change (&out_file, &inp_file);
  my_seek (&out_file, inp_os2_pos);
  my_write (&os2_h, sizeof (os2_h), &out_file);
}


/* Strip the symbols from a bound executable. */

static void strip_symbols (void)
{
  long size, pos, str_len;

  pos = a_in_pos + a_in_sym;
  size = my_size (&inp_file);
  if (pos + sizeof (str_len) <= size)
    {
      my_seek (&inp_file, pos);
      str_len = 4;
      my_write (&str_len, sizeof (str_len), &inp_file);
      my_trunc (&inp_file);
      a_in_h.a_syms = 0;
      my_seek (&inp_file, a_in_pos);
      my_write (&a_in_h, sizeof (a_in_h), &inp_file);
    }
}


void cmd (int mode)
{
  switch (mode)
    {
    case 'b':

      /* Bind an a.out file into an .exe file. */

      init_bind ();
      init_os2_header ();
      read_stub ();
      read_a_out_header ();
      if (opt_c != NULL)
	read_core ();
      read_res ();
      os2_fixup ();
      relocations ();
      sort_fixup ();
      set_exe_header ();
      set_os2_header ();
      write_header ();
      write_res ();
      copy_a_out ();
      write_nonres ();
      if (opt_m != NULL)
        write_map (opt_m);
      break;

    case 'e':

      /* Set the OS/2 application type. */

      exe_type ();
      break;

    case 's':

      /* Strip symbols. */

      strip_symbols ();
      break;

    default:
      abort ();
    }
}
