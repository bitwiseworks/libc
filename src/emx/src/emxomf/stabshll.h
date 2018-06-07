/* stabshll.h -- Convert GNU-style a.out debug information into
                 IBM OS/2 HLL 2 debug information
   Copyright (c) 1993-1995 Eberhard Mattes

This file is part of emxomf.

emxomf is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

emxomf is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with emxomf; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */


/* Used by emxomf.  May also be used for emxbind, but isn't
   currently. */

void convert_debug (void);
