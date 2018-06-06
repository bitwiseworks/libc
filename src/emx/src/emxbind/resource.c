/* resource.c -- Manage resources
   Copyright (c) 1991-1998 Eberhard Mattes

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


/* Flags for resources. */

#define RES_MOVEABLE    0x0010
#define RES_PRELOAD     0x0040
#define RES_DISCARDABLE 0x1000


/* This is the layout of the header of a binary resource (in a .res
   or .exe file). */

#pragma pack(1)

struct bin_res
{
  byte type_flag;               /* Always 0xff (type is an integer)*/
  word type_value;              /* Type of the resource */
  byte id_flag;                 /* Always 0xff (ID is an integer)*/
  word id_value;                /* Resources identifier */
  word options;                 /* Load and memory options */
  dword length;                 /* Size of the resource in bytes */
};

#pragma pack(4)

/* Internal representation of a resource. */

struct resource
{
  word type;
  word id;
  dword file_pos;
  dword length;
  dword offset;
};

/* Internal respresentation of a resource object. */

struct res_obj
{
  struct resource *res;
  size_t res_size;
  size_t res_len;
  word res_options;
  word obj_number;
  dword obj_size;
};


/* This table describes all the resource objects.  There are
   res_obj_count entries. */

static struct res_obj *res_objs = NULL;

/* This is the number of preload resource objects. */

static int res_preload_obj_count;

/* emxbind tries to use the last resource page for the a.out header.
   If there isn't enough space left in the last resource page, this
   flag is set to indicate that an extra page is to be allocated for
   the a.out header.  That page won't be referenced by the LX
   header. */

static int extra_aout_page;


/* This function compares two resource table entries for qsort().  It
   is used for sorting the resource table by resource attributes
   (preload resources come first) and object number. */

static int res_obj_compare (const void *x1, const void *x2)
{
  word opt1, opt2;

  opt1 = ((struct res_obj *)x1)->res_options;
  opt2 = ((struct res_obj *)x2)->res_options;
  if ((opt1 & RES_PRELOAD) != (opt2 & RES_PRELOAD))
    {
      if (opt1 & RES_PRELOAD)
        return -1;
      else
        return 1;
    }
  return ((int)((struct res_obj *)x1)->obj_number -
	  (int)((struct res_obj *)x2)->obj_number);
}


/* This function compares two resource table entries for qsort().  It
   is used for sorting the resource table by type and ID. */

static int res_compare (const void *x1, const void *x2)
{
  int a1, a2;

  a1 = ((struct resource *)x1)->type;
  a2 = ((struct resource *)x2)->type;
  if (a1 != a2)
    return a1 - a2;
  a1 = ((struct resource *)x1)->id;
  a2 = ((struct resource *)x2)->id;
  return a1 - a2;
}


/* Read a binary resource file. */

void read_res (void)
{
  struct bin_res br;
  struct res_obj *rop;
  struct resource *rp;
  size_t i, j;
  int obj_no;
  long file_size;

  res_count = 0; res_obj_count = 0; res_preload_pages = 0; res_pages = 0;
  res_preload_obj_count = 0; extra_aout_page = FALSE;
  if (opt_r == NULL)
    return;
  file_size = my_size (&res_file);
  my_seek (&res_file, 0);
  while (my_tell (&res_file) != file_size)
    {
      my_read (&br, sizeof (br), &res_file);
      if (br.type_flag != 0xff || br.id_flag != 0xff)
        error ("invalid binary resource file");

      /* Resource types 22 and 23 are ignored for unknown reasons. */

      if (br.type_value != 22 && br.type_value != 23)
	{
	  ++res_count;
	  for (i = 0; i < res_obj_count; ++i)
	    if (res_objs[i].res_options == br.options)
	      break;
	  if (i < res_obj_count)
	    rop = &res_objs[i];
	  else
	    {
	      ++res_obj_count;
	      res_objs = xrealloc (res_objs, res_obj_count
				   * sizeof (struct res_obj));
	      rop = &res_objs[res_obj_count-1];
	      rop->res = NULL;
	      rop->res_size = 0;
	      rop->res_len = 0;
	      rop->res_options = br.options;
	      rop->obj_size = 0;
	      rop->obj_number = res_obj_count;    /* for sorting */
	      if (br.options & RES_PRELOAD)
		++res_preload_obj_count;
	    }
	  if (rop->res_len >= rop->res_size)
	    {
	      rop->res_size += 16;
	      rop->res = xrealloc (rop->res, rop->res_size * sizeof (*rop->res));
	    }
	  rp = &rop->res[rop->res_len++];
	  rp->type = br.type_value;
	  rp->id = br.id_value;
	  rp->file_pos = my_tell (&res_file);
	  rp->length = br.length;
	  rp->offset = 0;
	}
      my_skip (&res_file, br.length);
    }
  if (verbosity > 0)
    printf ("%d resource%s in %d object%s\n",
            res_count, plural_s (res_count),
            res_obj_count, plural_s (res_obj_count));
  qsort (res_objs, res_obj_count, sizeof (struct res_obj), res_obj_compare);
  obj_no = OBJECTS + 1;
  for (i = 0, rop = res_objs; i < res_obj_count; ++i, ++rop)
    {
      qsort (rop->res, rop->res_len, sizeof (struct resource), res_compare);
      rop->obj_number = obj_no++;
      for (j = 0; j < rop->res_len; ++j)
	{
	  rop->obj_size = round_4 (rop->obj_size);
	  rop->res[j].offset = rop->obj_size;
	  rop->obj_size += rop->res[j].length;
	}
      j = npages (rop->obj_size);
      if (rop->res_options & RES_PRELOAD)
	res_preload_pages += j;
      res_pages += j;
    }
  extra_aout_page = (res_pages != 0
		     && ((res_objs[res_obj_count-1].obj_size & 0xfff)
                         > 0x1000 - A_OUT_OFFSET));
  if (extra_aout_page)
    ++res_pages;            /* This one is for the a.out header */
}


/* Write resources to the destination executable file. */

void write_res (void)
{
  struct res_obj *rop;
  int i, j;
  long pos;
  enum {ro_unknown, ro_preload, ro_demand} ro_last, ro_this;

  if (res_count == 0)
    return;
  ro_last = ro_unknown;
  if (verbosity >= 2)
    puts ("Writing resources");
  for (i = 0, rop = res_objs; i < res_obj_count; ++i, ++rop)
    {
      if (verbosity >= 2)
	{
	  if (rop->res_options & RES_PRELOAD)
	    ro_this = ro_preload;
	  else
	    ro_this = ro_demand;
	  if (ro_this != ro_last)
	    {
	      ro_last = ro_this;
	      if (ro_this == ro_preload)
		printf ("Writing %d PRELOAD resource object%s\n",
                        res_preload_obj_count,
                        plural_s (res_preload_obj_count));
	      else
		printf ("Writing %d DEMAND resource object%s\n",
                        res_obj_count - res_preload_obj_count,
                        plural_s (res_obj_count - res_preload_obj_count));
	    }
	  printf ("  Writing:  %ld bytes in %lu page%s\n",
                  round_4 (rop->obj_size),
                  npages (rop->obj_size), plural_s (npages (rop->obj_size)));
	}
      pos = 0;
      for (j = 0; j < rop->res_len; ++j)
	{
	  if (verbosity >= 2)
	    printf ("    %d.%d (%lu byte%s)\n",
                    rop->res[j].id, rop->res[j].type,
                    rop->res[j].length, plural_s (rop->res[j].length));
	  fill (rop->res[j].offset - pos);
	  my_seek (&res_file, rop->res[j].file_pos);
	  copy (&res_file, rop->res[j].length);
	  pos = rop->res[j].offset + rop->res[j].length;
	}
      if (pos != rop->obj_size)
        error ("internal error 2");
      if (i < res_obj_count - 1)
	fill (round_page (pos) - pos);
      else if (extra_aout_page)
	fill (round_page (pos) - pos + 0x1000 - A_OUT_OFFSET);
      else
	fill (round_page (pos) - pos - A_OUT_OFFSET);
    }
}


void put_rsctab (void)
{
  int j, k;
  const struct res_obj *rop;

  os2_h.rsctab_count = res_count;
  for (j = 0, rop = res_objs; j < res_obj_count; ++j, ++rop)
    for (k = 0; k < rop->res_len; ++k)
      {
        put_header_word (rop->res[k].type);
        put_header_word (rop->res[k].id);
        put_header_dword (rop->res[k].length);
        put_header_word (rop->obj_number);
        put_header_dword (rop->res[k].offset);
      }
}


void put_res_obj (int map)
{
  int j;
  const struct res_obj *rop;
  struct object obj_res;

  for (j = 0, rop = res_objs; j < res_obj_count; ++j, ++rop)
    {
      obj_res.virt_size = rop->obj_size;
      obj_res.virt_base = 0;
      obj_res.attr_flags = 0x2029;
      if (rop->res_options & RES_PRELOAD)
	obj_res.attr_flags |= 0x0040;
      if (rop->res_options & RES_DISCARDABLE)
	obj_res.attr_flags |= 0x0010;
      obj_res.map_first = map;
      obj_res.map_count = npages (rop->obj_size);
      obj_res.reserved = 0;
      put_header_bytes (&obj_res, sizeof (obj_res));
      map += obj_res.map_count;
    }
}
