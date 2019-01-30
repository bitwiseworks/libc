/* fixup.c -- Manage fixups
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
#include <alloca.h>
#include "defs.h"
#include "emxbind.h"

/* This constant is used in the module_data array to indicate that no
   address is assigned to a module. */

#define NO_ADDR             0xffffffff


/* A module referenced by an import definition. */

struct module
{
  char *name;                   /* Name of the module */
  dword addr;                   /* Address of the name in the executable */
};


/* This table holds the modules for method (I1).  module_size is the
   number of entries allocated, module_len is the number of entries
   used. */

static struct module *module_data = NULL;
static int module_size = 0;
static int module_len = 0;

/* This is the hash table for the symbol table. */

#define SYM_HASH_SIZE 1009
static int sym_hash_table[SYM_HASH_SIZE];

/* This table contains the `next' pointer for every symbol table entry
   to handle hash table overflow. */

static int *sym_hash_next = NULL;


/* Compute the hash code of a symbol.  Note that the computation ends
   at an '=' character to be able to use hashing for finding import
   symbols. */

static unsigned sym_hash (const char *name)
{
  unsigned hash;

  for (hash = 0; *name != 0 && *name != '='; ++name)
    hash = (hash << 2) ^ *name;
  return hash % SYM_HASH_SIZE;
}


/* Build the hash table for the symbol table. */

void build_sym_hash_table (void)
{
  int i;
  unsigned hash;
  const char *name;

  sym_hash_next = xmalloc (sym_count * sizeof (*sym_hash_next));
  for (i = 0; i < SYM_HASH_SIZE; ++i)
    sym_hash_table[i] = -1;
  for (i = 0; i < sym_count; ++i)
    sym_hash_next[i] = -1;
  for (i = 0; i < sym_count; ++i)
    {
      name = sym_image[i].n_un.n_strx + (char *)str_image;
      hash = sym_hash (name);
      sym_hash_next[i] = sym_hash_table[hash];
      sym_hash_table[hash] = i;
    }
}


/* Find the symbol NAME in the a.out symbol table of the input executable.
   If the symbol is found, find_symbol() returns a pointer to the symbol
   table entry.  Otherwise, NULL is returned. */

struct nlist *find_symbol (const char *name)
{
  int j, len;
  const char *name1 = name;

  len = strlen (name);

  for (j = sym_hash_table[sym_hash (name1)]; j != -1;
       j = sym_hash_next[j])
    {
      const char *name2 = sym_image[j].n_un.n_strx + (char *)str_image;

      if (memcmp (name1, name2, len) == 0)
      {
        int t = sym_image[j].n_type & ~N_EXT;
        if (t == N_TEXT || t == N_DATA || t == N_BSS)
          return sym_image+j;
      }
    }

  return NULL;
}


/* This function compares two entries of the fixup table for qsort().
   Entries are sorted by object number and address. */

static int fixup_compare (const void *x1, const void *x2)
{
  int o1, o2;
  dword a1, a2;

  o1 = ((struct fixup *)x1)->obj;
  o2 = ((struct fixup *)x2)->obj;
  if (o1 < o2)
    return -1;
  else if (o1 > o2)
    return 1;
  else
    {
      a1 = ((struct fixup *)x1)->addr;
      a2 = ((struct fixup *)x2)->addr;
      if (a1 < a2)
        return -1;
      else if (a1 > a2)
        return 1;
      else
        return 0;
    }
}


/* Sort the fixup table by object and address. */

void sort_fixup (void)
{
  qsort (fixup_data, fixup_len, sizeof (struct fixup), fixup_compare);
}


/* Expand the fixup table. */

static void grow_fixup (void)
{
  if (fixup_len >= fixup_size)
    {
      fixup_size += 256;
      fixup_data = xrealloc (fixup_data, fixup_size * sizeof (struct fixup));
    }
}


/* Add a fixup record to the LX header.  FP points to a structure
   containing information about the fixup.  If NEG is true, a negative
   offset is generated.  This is used for creating the second part of
   a fixup spanning a page boundary. */

void create_fixup (const struct fixup *fp, int neg)
{
  byte flags, type;
  word addr;

  switch (fp->type)
    {
    case FIXUP_REL:
      type = NRSOFF32; break;
    case FIXUP_ABS:
      type = NROFF32; break;
    case FIXUP_FAR16:
      type = NRSPTR | NRALIAS; break;
    default:
      error ("internal error 3");
    }
  addr = fp->addr & 0x0fff;
  if (neg)
    addr |= 0xf000;
  switch (fp->target)
    {
    case TARGET_ORD:
      flags = NRRORD;
      if (fp->dst <= 255)
        flags |= NR8BITORD;
      if (fp->mod > 255)
        flags |= NR16OBJMOD;
      if (fp->add != 0)
        {
          flags |= NRADD;
          if (fp->add > 0xffff)
            flags |= NR32BITADD;
        }
      put_header_byte (type);
      put_header_byte (flags);
      put_header_word (addr);
      if (flags & NR16OBJMOD)
        put_header_word (fp->mod);
      else
        put_header_byte (fp->mod);
      if (flags & NR8BITORD)
        put_header_byte (fp->dst);
      else
        put_header_word (fp->dst);
      if (flags & NRADD)
        {
          if (flags & NR32BITADD)
            put_header_dword (fp->add);
          else
            put_header_word (fp->add);
        }
      break;
    case TARGET_NAME:
      flags = NRRNAM;
      if (fp->mod > 255)
        flags |= NR16OBJMOD;
      if (fp->add != 0)
        {
          flags |= NRADD;
          if (fp->add > 0xffff)
            flags |= NR32BITADD;
        }
      put_header_byte (type);
      put_header_byte (flags);
      put_header_word (addr);
      if (flags & NR16OBJMOD)
        put_header_word (fp->mod);
      else
        put_header_byte (fp->mod);
      put_header_word (fp->dst);
      if (flags & NRADD)
        {
          if (flags & NR32BITADD)
            put_header_dword (fp->add);
          else
            put_header_word (fp->add);
        }
      break;
    case TARGET_ADDR:
      flags = NRRINT;
      if (fp->dst >= 0x10000)
        flags |= NR32BITOFF;
      put_header_byte (type);
      put_header_byte (flags);
      put_header_word (addr);
      put_header_byte (fp->mod + 1);
      if (flags & NR32BITOFF)
        put_header_dword (fp->dst);
      else
        put_header_word (fp->dst);
      break;
    }
}


/* Find an entry in the table of imported procedures (symbols).  NAME
   is the name of the procedure.  If the procedure is not found, it
   will be added to the table.  find_proc() returns the offset of the
   entry. */

static int find_proc (const char *name)
{
  int i, len, name_len;
  byte blen;

  name_len = strlen (name);
  i = 0;
  while (i < procs.len)
    {
      len = procs.data[i];
      if (len == name_len && memcmp (procs.data + i + 1, name, len) == 0)
	return i;
      i += 1 + len;
    }
  i = procs.len;
  blen = (byte)name_len;
  put_grow (&procs, &blen, 1);
  put_grow (&procs, name, name_len);
  return i;
}


/* Add a reference to an imported procedure (symbol SYM_NAME) to the
   fixup table.  The fixup is to be applied to address ADDR of object
   OBJ, an index into the obj_h array.  If NAME is NULL, the procedure
   is imported by ordinal number ORD.  Otherwise, the procedure is
   imported by name NAME (ORD will be ignored).  FIXUP_TYPE is the
   type of the fixup: FIXUP_ABS (absolute 0:32 fixup), FIXUP_REL
   (relative 0:32 fixup) or FIXUP_FAR16 (absolute 16:16 fixup).  ADD
   is a number to be added to the fixup target. */

static void ref_proc (const char *sym_name, int obj, dword addr, int mod,
                      const char *name, int ord, int fixup_type, dword add)
{
  grow_fixup ();
  fixup_data[fixup_len].type = fixup_type;
  if (name != NULL)
    {
      fixup_data[fixup_len].target = TARGET_NAME;
      fixup_data[fixup_len].dst = find_proc (name);
    }
  else
    {
      fixup_data[fixup_len].target = TARGET_ORD;
      fixup_data[fixup_len].dst = ord;
    }
  fixup_data[fixup_len].addr = addr;
  fixup_data[fixup_len].obj = obj;
  fixup_data[fixup_len].mod = mod;
  fixup_data[fixup_len].add = add;
  ++fixup_len;

  /* Remember the import for the .map file. */

  if (opt_m != NULL && sym_name != NULL)
    map_import (sym_name, module_data[mod-1].name, name, ord);
}


/* Build fixups for the relocation table TABLE. */

static void reloc_table (const struct relocation_info *table, long tab_size, int seg_obj,
                         dword seg_base, const byte *image, dword image_size)
{
  int n, obj;
  const struct relocation_info *r;
  dword x, dst_base;

  n = tab_size / sizeof (struct relocation_info);
  for (r = table; n > 0; --n, ++r)
    {
      if (r->r_length != 2)
        error ("relocation of size %d not implemented", 1 << r->r_length);
      switch (r->r_symbolnum & ~N_EXT)
	{
	case N_TEXT:
	  obj = OBJ_TEXT;
	  dst_base = TEXT_BASE;
	  break;
	case N_DATA:
	case N_BSS:
	  obj = OBJ_DATA;
	  dst_base = data_base;
	  break;
	default:
	  obj = -1;
	  dst_base = 0;
	  break;
	}
      if (obj >= 0 && (!r->r_pcrel || obj != seg_obj))
	{
	  grow_fixup ();
	  if (r->r_address+3 >= image_size)
            error ("fixup outside image");
	  x = *(dword *)(image + r->r_address);
	  if (r->r_pcrel)
	    x += dst_base  + r->r_address + 4;
	  fixup_data[fixup_len].type = (r->r_pcrel ? FIXUP_REL : FIXUP_ABS);
	  fixup_data[fixup_len].target = TARGET_ADDR;
	  fixup_data[fixup_len].obj = seg_obj;
	  fixup_data[fixup_len].mod = obj;
	  fixup_data[fixup_len].addr = r->r_address;
	  fixup_data[fixup_len].dst = x - dst_base;
	  fixup_data[fixup_len].add = 0;
	  ++fixup_len;
	}
    }
}


/* Find a module in the table of imported modules by name.  NAME is
   the name of the module to find.  If the module is not found, it
   will be added; ADDR is the address of the module name in the a.out
   image for method (I1).  find_module() returns the module index.
   The index of the first module is 1. */

static int find_module (const char *name, dword addr)
{
  int i;
  char *p;

  for (i = 0; i < module_len; ++i)
    if (strcmp (module_data[i].name, name) == 0)
      return i+1;
  if (module_len >= module_size)
    {
      module_size += 16;
      module_data = xrealloc (module_data, module_size *
			      sizeof (struct module));
    }
  i = module_len + 1;
  p = xstrdup (name);
  module_data[module_len].name = p;
  module_data[module_len].addr = addr;
  ++module_len;
  return i;
}


/* Find a module in the table of imported modules by address.  ADDR is
   the address of the module name in the a.out image for method (I1).
   find_module_by_addr() returns the module index.  The index of the
   first module is 1.  If the module is not found, -1 will be
   returned. */

static int find_module_by_addr (dword addr)
{
  int i;

  if (addr != NO_ADDR)
    for (i = 0; i < module_len; ++i)
      if (module_data[i].addr == addr)
	return i+1;
  return -1;
}


/* Build fixups for relocations if the destination executable is to be
   relocatable (DLL). */

void relocations (void)
{
  if (relocatable && (a_in_h.a_trsize != 0 || a_in_h.a_drsize != 0))
    {
      read_segs ();
      read_reloc ();
      reloc_table (tr_image, a_in_h.a_trsize, OBJ_TEXT, TEXT_BASE,
                   text_image, a_in_h.a_text);
      reloc_table (dr_image, a_in_h.a_drsize, OBJ_DATA, data_base,
                   data_image, a_in_h.a_data);
    }
}


/* Process an import symbol for method (I2). */

static void import_symbol (int seg_obj, const struct relocation_info *r,
                           const char *name1, int len, dword x,
                           const char *name2)
{
  int k, mod_idx, fixup_type;
  long ord;
  const char *imp, *proc;
  char mod[256], *q, *end;

  imp = name2 + len + 1;
  q = strchr (imp, '.');
  if (q == NULL)
    error ("invalid import symbol %s", name2);
  k = q - imp;
  memcpy (mod, imp, k);
  mod[k] = 0;
  proc = NULL;
  errno = 0;
  ord = strtol (q+1, &end, 10);
  if (end != q+1 && *end == 0 && errno == 0)
    {
      if (ord < 1 || ord > 65535)
        error ("invalid import symbol %s", name2);
      if (verbosity >= 2)
        printf ("Importing %s.%d\n", mod, (int)ord);
    }
  else
    {
      proc = q + 1;
      if (*proc == 0)
        error ("invalid import symbol %s", name2);
      if (verbosity >= 2)
        printf ("Importing %s.%s\n", mod, proc);
    }
  mod_idx = find_module (mod, NO_ADDR);
  if (memcmp ("_16_", name1, 4) == 0)
    {
      if (r->r_pcrel)
        error ("pc-relative 16:16 fixup is invalid");
      fixup_type = FIXUP_FAR16;
    }
  else
    fixup_type = (r->r_pcrel ? FIXUP_REL : FIXUP_ABS);
  ref_proc (name1, seg_obj, r->r_address, mod_idx, proc, (int)ord,
            fixup_type, x);
}


/* Scan the relocation table TABLE and create appropriate import fixup
   records. */

static void import_reloc (const struct relocation_info *table, long tab_size,
                          int seg_obj, dword seg_base, const byte *image,
                          dword image_size)
{
  int reloc_count, i, j, len, ok;
  const struct relocation_info *r;
  const char *name1, *name2;
  dword x;

  reloc_count = tab_size / sizeof (struct relocation_info);
  for (i = 0, r = table; i < reloc_count; ++i, ++r)
    if (r->r_extern && r->r_length == 2)
      {
        if (sym_image == NULL)
          read_sym ();
        if (r->r_symbolnum >= sym_count)
          error ("invalid symbol number");
        if (sym_image[r->r_symbolnum].n_type == (N_IMP1|N_EXT))
          {
            if (r->r_address+3 >= image_size)
              error ("fixup outside image");
            x = *(dword *)(image + r->r_address);
            if (r->r_pcrel)
              x += seg_base + r->r_address + 4;
            name1 = sym_image[r->r_symbolnum].n_un.n_strx + (char *)str_image;
            len = strlen (name1);
            ok = FALSE;
            for (j = sym_hash_table[sym_hash (name1)]; j != -1;
                 j = sym_hash_next[j])
              if (sym_image[j].n_type == (N_IMP2|N_EXT))
                {
                  name2 = sym_image[j].n_un.n_strx + (char *)str_image;
                  if (memcmp (name1, name2, len) == 0 && name2[len] == '=')
                    {
                      import_symbol (seg_obj, r, name1, len, x, name2);
                      ok = TRUE;
                      break;
                    }
                }
            if (!ok)
              error ("import symbol %s undefined", name1);
          }
      }
}


/* Build the fixup table from the (I1) import definitions and the
   a.out symbol table and relocation tables. */

void os2_fixup (void)
{
  int mod_idx;
  dword set_len, i, zero;
  dword *set_vec;
  size_t set_size;
  struct
    {
      dword flag;
      dword addr;
      dword mod;
      dword proc;
    } fixup;
  byte mod_name[256];
  byte proc_name[256];

  /* The very first DWORD in data segment is the __os2_dll set.
     It contains a list of fixups to be replaced by OS/2 DLL references. */
  my_seek (&inp_file, data_base + data_off);
  my_read (&set_len, sizeof (set_len), &inp_file);
  if (set_len != DATASEG_MAGIC)
    error ("invalid data segment (does not start with 0x%x)", DATASEG_MAGIC);
  /* Now read the offset to the __os2_dll set */
  my_read (&set_len, sizeof (set_len), &inp_file);

  /* Start scanning the set */
  my_seek (&inp_file, set_len + data_off);
  my_read (&set_len, sizeof (set_len), &inp_file);
  if (set_len == 0xffffffff)
    {
      /* I think this is a bug in GNU ld */
      my_seek (&inp_file, data_off - 4);
      my_read (&set_len, sizeof (set_len), &inp_file);
    }
  if (set_len > 1)
    {
      set_size = sizeof (dword) * set_len;
      set_vec = xmalloc (set_size);
      my_read (set_vec, set_size, &inp_file);
      my_read (&zero, sizeof (zero), &inp_file);
      if (set_vec[0] != 0xffffffff || zero != 0)
	set_len = 0;                            /* Ignore invalid table */
      for (i = 1; i < set_len; ++i)
	{
	  my_seek (&inp_file, text_off + set_vec[i]);
	  my_read (&fixup, sizeof (fixup), &inp_file);
	  if ((fixup.flag & ~1) != 0)
            error ("invalid fixup");
	  mod_idx = find_module_by_addr (text_off + fixup.mod);
	  if (mod_idx < 0)
	    {
	      my_seek (&inp_file, text_off + fixup.mod);
	      my_read_str (mod_name, sizeof (mod_name), &inp_file);
	      mod_idx = find_module ((char *)mod_name, text_off + fixup.mod);
	    }
	  if (fixup.flag & 1)             /* ordinal */
	    ref_proc (NULL, OBJ_TEXT, fixup.addr - TEXT_BASE, mod_idx,
                      NULL, fixup.proc, FIXUP_REL, 0);
	  else
	    {
	      my_seek (&inp_file, text_off + fixup.proc);
	      my_read_str (proc_name, sizeof (proc_name), &inp_file);
	      ref_proc (NULL, OBJ_TEXT, fixup.addr - TEXT_BASE, mod_idx,
                        (char *)proc_name, 0, FIXUP_REL, 0);
	    }
	}
      free (set_vec);
    }
  if (a_in_h.a_trsize != 0 || a_in_h.a_drsize != 0)
    {
      read_segs ();
      read_reloc ();
      import_reloc (tr_image, a_in_h.a_trsize, OBJ_TEXT, TEXT_BASE, text_image,
                    a_in_h.a_text);
      import_reloc (dr_image, a_in_h.a_drsize, OBJ_DATA, data_base, data_image,
                    a_in_h.a_data);
    }
}


/* Put the imported modules table into the LX header. */

void put_impmod (void)
{
  int j;

  os2_h.impmod_count = module_len;
  for (j = 0; j < module_len; ++j)
    {
      put_header_byte ((byte)strlen (module_data[j].name));
      put_header_bytes (module_data[j].name, strlen (module_data[j].name));
    }
}
