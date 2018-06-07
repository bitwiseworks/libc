/* select.h -- Header file for select.c
   Copyright (c) 1993-1998 by Eberhard Mattes

This file is part of emx.

emx is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

emx is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with emx; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.

As special exception, emx.dll can be distributed without source code
unless it has been changed.  If you modify emx.dll, this exception
no longer applies and you must remove this paragraph from all source
files for emx.dll.  */


struct select_data
{
  SEMRECORD *list;
  fd_set *rbits;
  fd_set *wbits;
  fd_set *ebits;
  int nbytes;
  int max_sem;
  HMUX sem_mux;
  BYTE sem_npipe_flag;
  BYTE sem_kbd_flag;
  BYTE sem_mux_flag;
  BYTE socket_thread_flag;
  int sem_count;
  int ready_flag;
  int return_value;
  ULONG timeout;
  ULONG start_ms;
  thread_data *td;
  int socket_count;
  int socket_nread;
  int socket_nwrite;
  int socket_nexcept;
  int *sockets;
  int *socketh;
  int *sockett;
  int async_flag;
};
