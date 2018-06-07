/* emxomf.h -- Global header file for emxomf
   Copyright (c) 1992-1996 Eberhard Mattes

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


#include "a_out.h"
#include "a_out_stab.h"

/* These functions are defined in emxomf.c. */

extern void error (const char *fmt, ...) NORETURN2 ATTR_PRINTF (1, 2);
extern void warning (const char *fmt, ...) ATTR_PRINTF (1, 2);
extern void *xmalloc (size_t n);
void *xrealloc (void *ptr, size_t n);
char *xstrdup (const char *s);
extern const struct nlist *find_symbol (const char *name);
extern const struct nlist *find_symbol_ex (const char *name, int not_entry, int fext);
extern void set_hll_type (int index, int hll_type);

/* These variables are defined in emxomf.c. */

extern int sym_count;
extern const struct nlist *sym_ptr;
extern const byte *str_ptr;
extern byte *text_ptr;
extern long text_size;
extern struct buffer tt;
extern struct buffer sst;
extern struct buffer sst_reloc;
extern struct grow sst_boundary_grow;
extern struct grow tt_boundary_grow;
extern int *sst_boundary;
extern int *tt_boundary;
extern int hll_version;
extern const char *error_fname;
extern int warning_level;
