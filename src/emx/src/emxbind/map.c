/* map.c -- Write .map file
   Copyright (c) 1994-1996 Eberhard Mattes

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

struct map_sym
{
  dword addr;
  char *name;
  char *imp_name;
  const char *imp_mod;
  int imp_ord;
  byte seg;
};

static FILE *map_file;
static int first_dgroup_seg;
static int text_seg;
static int data_seg;
static int map_sym_count = 0;
static int map_sym_alloc = 0;
static struct map_sym *map_sym_table = NULL;


/* Write the header of the .map file.  It includes the module name. */

static void map_header (void)
{
  fprintf (map_file, "\n %s\n\n", module_name);
}


/* Write the segment list to the .map file. */

static void map_segments (void)
{
  int seg;
  char *fmt = " %.4X:%.8X 0%.8XH %-22s %s 32-bit\n";

  fputs (" Start         Length     Name                   Class\n", map_file);
  seg = 0;
  text_seg = ++seg;
  fprintf (map_file, fmt, seg, 0, a_in_h.a_text, "TEXT32", "CODE");
  first_dgroup_seg = data_seg = ++seg;
  fprintf (map_file, fmt, seg, 0, a_in_h.a_data, "DATA32", "DATA");
  fprintf (map_file, fmt, seg, a_in_h.a_data, a_in_h.a_bss, "BSS32", "BSS");

  if (obj_heap.virt_size != 0)
    fprintf (map_file, fmt, ++seg, 0, obj_heap.virt_size, "HEAP", "HEAP");
  if (obj_stk0.virt_size != 0)
    fprintf (map_file, fmt, ++seg, 0, obj_stk0.virt_size, "STACK", "STACK");
}


/* Write the group list to the .map file. */

static void map_groups (void)
{
  char *fmt = " %.4X:0   %s\n";

  fputs ("\n Origin   Group\n", map_file);
  fprintf (map_file, fmt, 0, "FLAT");
  fprintf (map_file, fmt, first_dgroup_seg, "DGROUP");
}


/* Compare two `struct map_sym' by name for qsort(). */

static int cmp_by_name (const void *p1, const void *p2)
{
  return strcmp (((const struct map_sym *)p1)->name,
                 ((const struct map_sym *)p2)->name);
}


/* Compare two `struct map_sym' by value for qsort(). */

static int cmp_by_value (const void *p1, const void *p2)
{
  const struct map_sym *s1, *s2;

  s1 = (const struct map_sym *)p1;
  s2 = (const struct map_sym *)p2;
  if (s1->seg < s2->seg)
    return -1;
  else if (s1->seg > s2->seg)
    return 1;
  else if (s1->addr < s2->addr)
    return -1;
  else if (s1->addr > s2->addr)
    return 1;
  else
    return strcmp (s1->name, s2->name);
}


/* Write a list of public symbols to the .map file. */

static void map_publics (const char *title,
                         int (*compare)(const void *p1, const void *p2))
{
  int i;
  
  fprintf (map_file, "\n  Address         Publics by %s\n\n", title);
  qsort (map_sym_table, map_sym_count, sizeof (*map_sym_table), compare);
  for (i = 0; i < map_sym_count; ++i)
    if (map_sym_table[i].imp_mod == NULL)
      fprintf (map_file, " %.4X:%.8lX       %s\n", map_sym_table[i].seg,
               map_sym_table[i].addr, map_sym_table[i].name);
    else if (map_sym_table[i].imp_name != NULL)
      fprintf (map_file, " %.4X:%.8lX  Imp  %-20s (%s.%s)\n", 0, 0L,
               map_sym_table[i].name, map_sym_table[i].imp_mod,
               map_sym_table[i].imp_name);
    else
      fprintf (map_file, " %.4X:%.8lX  Imp  %-20s (%s.%d)\n", 0, 0L,
               map_sym_table[i].name, map_sym_table[i].imp_mod,
               map_sym_table[i].imp_ord);
}


static void grow_map_sym_table (void)
{
  if (map_sym_count >= map_sym_alloc)
    {
      map_sym_alloc += 256;
      map_sym_table = xrealloc (map_sym_table,
                                map_sym_alloc * sizeof (*map_sym_table));
    }
}


/* Remember an import symbol for the .map file.  Note: MAP points to a
   module_data[].name string. */

void map_import (const char *sym_name, const char *mod, const char *name,
                 int ord)
{
  int i;

  /* TODO: Use hashing. */

  for (i = 0; i < map_sym_count; ++i)
    if (strcmp (map_sym_table[i].name, sym_name) == 0)
      return;

  grow_map_sym_table ();
  map_sym_table[map_sym_count].name = xstrdup (sym_name);
  if (name == NULL)
    {
      map_sym_table[map_sym_count].imp_name = NULL;
      map_sym_table[map_sym_count].imp_ord = ord;
    }
  else
    {
      map_sym_table[map_sym_count].imp_name = xstrdup (name);
      map_sym_table[map_sym_count].imp_ord = -1;
    }

  map_sym_table[map_sym_count].imp_mod = mod;
  map_sym_table[map_sym_count].seg = 0;
  map_sym_table[map_sym_count].addr = 0;
  ++map_sym_count;
}


/* Read and prepare the symbol table, write lists of public symbols. */

static void map_symbols (void)
{
  int i, seg;
  dword addr;
  char *name;

  read_sym ();
  if (sym_count != 0)
    {
      for (i = 0; i < sym_count; ++i)
        {
          switch (sym_image[i].n_type)
            {
            case N_TEXT|N_EXT:
              seg  = text_seg; addr = sym_image[i].n_value - obj_text.virt_base;
              break;
            case N_DATA|N_EXT:
            case N_BSS|N_EXT:
              seg = data_seg; addr = sym_image[i].n_value - obj_data.virt_base;
              break;
            default:
              seg = 0; addr = 0; break;
            }
          if (seg != 0)
            {
              grow_map_sym_table ();
              name = sym_image[i].n_un.n_strx + str_image;
              map_sym_table[map_sym_count].seg = seg;
              map_sym_table[map_sym_count].addr = addr;
              map_sym_table[map_sym_count].name = name;
              map_sym_table[map_sym_count].imp_name = NULL;
              map_sym_table[map_sym_count].imp_mod = NULL;
              map_sym_table[map_sym_count].imp_ord = -1;
              ++map_sym_count;
            }
        }
      if (map_sym_count != 0)
        {
          map_publics ("Name", cmp_by_name);
          map_publics ("Value", cmp_by_value);
        }
    }
}


/* Write the list of exported symbols to the .map file. */

static void map_exports (void)
{
  int i, seg;
  const struct export *p;

  if (!dll_flag || get_export (0) == NULL)
    return;

  fputs ("\n Address       Export                  Alias\n\n", map_file);
  for (i = 0; (p = get_export (i)) != NULL; ++i)
    {
      switch (p->object)
        {
        case OBJ_TEXT:
          seg = text_seg; break;
        case OBJ_DATA:
          seg = data_seg; break;
        default:
          seg = 0; break;
        }
      if (seg != 0)
        fprintf (map_file, " %.4X:%.8lX %-23s %s\n",
                 seg, p->offset, p->entryname, p->internalname);
    }
}


/* Write the entry point to the .map file. */

static void map_entrypoint (void)
{
  if (!dll_flag)
    fprintf (map_file, "\nProgram entry point at 0001:00000000\n");
}


/* Write the .map file. */

void write_map (const char *fname)
{
  char *tmp;

  /* Add an `.map' suffix if there's no file name extension. */

  tmp = alloca (strlen (fname) + 5);
  strcpy (tmp, fname);
  _defext (tmp, "map");
  fname = tmp;

  /* Don't use my_open() etc., those functions are for binary files. */

  map_file = fopen (fname, "w");
  if (map_file == NULL)
    error ("cannot open `%s'", fname);

  /* Write the sections of the .map file. */

  map_header ();
  map_segments ();
  map_groups ();
  map_exports ();
  map_symbols ();
  map_entrypoint ();

  /* Close the .map file. */

  if (fflush (map_file) != 0)
    error ("Cannot write `%s'", fname);
  if (fclose (map_file) != 0)
    error ("Cannot close `%s'", fname);
}
