/* exec.c -- Manage executable files
   Copyright (c) 1991-1996 Eberhard Mattes

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
#include <sys/moddef.h>
#include <sys/user.h>
#include "defs.h"
#include "emxbind.h"


/* The first DOS EXE header read from emxl.exe or emx.exe. */

static struct exe1_header stub_h1;

/* The a.out header to be written to the output executable. */

static struct exec a_out_h;

/* The header of the core dump file. */

static struct user core_h;

/* The offsets of the text section, text relocation table, and data
   relocation table, respectively, of the source a.out (sub)file. */

static long a_in_text;
static long a_in_tr;
static long a_in_dr;

/* The offset of the string table in the source a.out (sub)file. */

static long a_in_str;

/* The offset and size, respectively, of the string table in the
   destination a.out (sub)file. */

static long a_out_str;
static long a_out_str_size;

/* The size of the a.out file, including the symbol table and the
   string table. */

static long a_out_size;

/* The location of the emxl.exe or emx.exe image in the source and
   destination executable, respectively. */

static long src_image;
static long dst_image;

/* The size of the DOS stub image. */

static long stub_size;

/* The size of the LX EXE header of the output executable. */

static long os2_hdr_size;

/* The number of text, data, and heap pages, respectively, in the LX
   executable being created. */

static int text_pages;
static int data_pages;
static int heap_pages;

/* The number of pages between the data pages and the heap pages.
   These are required for moving the heap to the correct address of
   the a.out subfile.  The LX header, however, does not map these
   pages. */

static int gap_pages;

/* The number of zero bytes to insert between the relocation table of
   the DOS header and the emx.exe or emxl.exe image. */

static long fill1;

/* The number of zero bytes to insert between the LX header (or the
   end of the resources, if there are resources) and the a.out
   header. */

static long fill3;

/* header holds the LX header while it is being built. */

static struct grow header = GROW_INIT;


/* Compute the virtual end address of a memory object of an LX
   file.  X is a struct object. */

#define OBJ_END(x) ((x).virt_base + (x).virt_size)

/* Round up to the next multiple of the segment alignment (0x10000). */

#define round_segment(X) ((((X)-1) & ~0xffff) + 0x10000)


/* Derive the main module name from the output file name, if not
   defined by a NAME or LIBRARY statement.  Put the main module name
   into the resident name table.  If there is a description
   (DESCRIPTION statement), put it into the resident name table. */

static void get_module_name (void)
{
  char *p, *q;

  if (module_name == NULL)
    {
      module_name = xmalloc (FNAME_SIZE);
      q = out_fname;
      for (p = out_fname; *p != 0; ++p)
	if (*p == ':' || *p == '/' || *p == '\\')
	  q = p + 1;
      p = module_name;
      while (*q != 0 && *q != '.')
        *p++ = *q++;
      *p = 0;
    }
  entry_name (&resnames, module_name, 0);
  if (description != NULL)
    entry_name (&nonresnames, description, 0);
}


/* Read EXE header and emxbind DOS header of emx.exe */

void read_stub (void)
{
  my_read (&stub_h1, sizeof (stub_h1), &stub_file);
  if (stub_h1.magic != 0x5a4d)
    error ("invalid stub file");
  src_image = (long)stub_h1.hdr_size << 4;
  stub_size = ((long)stub_h1.pages << 9) - src_image;
  if (stub_h1.last_page != 0)
    stub_size += stub_h1.last_page - 512;
}


/* Read the a.out header of the input executable. */

void read_a_out_header (void)
{
  byte buf[16];
  int syms;

  a_in_pos = my_tell (&inp_file);
  my_read (&a_in_h, sizeof (a_in_h), &inp_file);
  a_out_h = a_in_h;
  if (N_MAGIC (a_in_h) != ZMAGIC || a_in_h.a_entry != TEXT_BASE)
    error ("invalid a.out file (header)");
  a_out_h.a_drsize = 0;
  a_out_h.a_trsize = 0;
  my_seek (&inp_file, A_OUT_OFFSET);
  my_read (buf, sizeof (buf), &inp_file);
  a_in_text = A_OUT_OFFSET;
  a_in_data = a_in_text + round_page (a_in_h.a_text);
  a_in_tr = a_in_data + round_page (a_in_h.a_data);
  a_in_dr = a_in_tr + a_in_h.a_trsize;
  a_in_sym = a_in_dr + a_in_h.a_drsize;
  data_base = round_segment (TEXT_BASE + a_in_h.a_text);
  text_off = a_in_pos + a_in_text - TEXT_BASE;
  data_off = a_in_pos + a_in_data - data_base;
  syms = (a_in_h.a_syms != 0 &&
	  a_in_sym + a_in_h.a_syms + sizeof (a_in_str_size) <=
	  my_size (&inp_file));
  if (syms)
    {
      a_in_str = a_in_sym + a_in_h.a_syms;
      my_seek (&inp_file, a_in_str);
      my_read (&a_in_str_size, sizeof (a_in_str_size), &inp_file);
    }
  else
    {
      a_in_str = a_in_sym;
      a_in_str_size = 4;
    }
  if (syms && !opt_s)
    {
      a_out_str = a_in_str;
      a_out_str_size = a_in_str_size;
    }
  else
    {
      a_out_str = a_in_sym;
      a_out_str_size = 4;
      a_out_h.a_syms = 0;
    }
  a_out_str -= a_in_h.a_drsize + a_in_h.a_trsize;
  a_out_size = a_out_str + a_out_str_size;
}


/* Read the header of the core dump file. */

void read_core (void)
{
  my_read (&core_h, sizeof (core_h), &core_file);
  if (core_h.u_magic != UMAGIC)
    error ("invalid core file (header)");
  if (core_h.u_data_base != data_base
      || core_h.u_data_end != data_base + a_in_h.a_data + a_in_h.a_bss)
    error ("core file doesn't match a.out file");
  a_out_h.a_data = round_page (core_h.u_data_end - core_h.u_data_base);
  a_out_h.a_bss = 0;
  if (core_h.u_heap_brk > core_h.u_heap_base)
    {
      if (core_h.u_heap_brk - core_h.u_heap_base > heap_size)
        error ("the heap size is too small for the core file's heap");
      a_out_h.a_data = round_page (core_h.u_heap_brk - core_h.u_data_base);
    }
  if (core_h.u_heapobjs_off != 0)
    error ("%s: multiple heap objects are not supported", opt_c);
}


/* Read the LX header. */

void read_os2_header (void)
{
  my_seek (&inp_file, sizeof (inp_h1));
  my_read (&inp_h2, sizeof (inp_h2), &inp_file);
  inp_os2_pos = COMBINE (inp_h2.new_lo, inp_h2.new_hi);
  my_seek (&inp_file, inp_os2_pos);
  my_read (&os2_h, sizeof (os2_h), &inp_file);
}


/* Read the text and data relocation tables of the input a.out
   executable. */

void read_reloc (void)
{
  if (tr_image == NULL)
    {
      my_seek (&inp_file, a_in_tr);
      tr_image = xmalloc (a_in_h.a_trsize);
      my_read (tr_image, a_in_h.a_trsize, &inp_file);
    }
  if (dr_image == NULL)
    {
      my_seek (&inp_file, a_in_dr);
      dr_image = xmalloc (a_in_h.a_drsize);
      my_read (dr_image, a_in_h.a_drsize, &inp_file);
    }
}


/* Read the text and data sections from the input a.out file. */

void read_segs (void)
{
  int d_size, t_size;

  if (text_image == NULL)
    {
      t_size = round_page (a_in_h.a_text);
      text_image = xmalloc (t_size);
      my_seek (&inp_file, a_in_text);
      my_read (text_image, t_size, &inp_file);
    }
  if (data_image == NULL)
    {
      d_size = round_page (a_in_h.a_data);
      data_image = xmalloc (d_size);
      my_seek (&inp_file, a_in_data);
      my_read (data_image, d_size, &inp_file);
    }
}


/* Read the symbol and string tables from the input a.out file. */

void read_sym (void)
{
  if (sym_image == NULL)
    {
      sym_image = xmalloc (a_in_h.a_syms);
      str_image = xmalloc (a_in_str_size);
      my_seek (&inp_file, a_in_sym);
      my_read (sym_image, a_in_h.a_syms, &inp_file);
      my_seek (&inp_file, a_in_str);
      my_read (str_image, a_in_str_size, &inp_file);
      sym_count = a_in_h.a_syms / sizeof (struct nlist);
      build_sym_hash_table ();
    }
}


/* Setup the DOS EXE headers for the destination executable.  Compute
   the location of the LX header. */
 
void set_exe_header (void)
{
  long i;

  i = (stub_h1.reloc_size << 2) + sizeof (out_h1) + sizeof (out_h2);
#if 1
  dst_image = (i + 0x0f) & ~0x0f;
#else
  dst_image = (i + 0x1ff) & ~0x1ff;
#endif
  fill1 = dst_image - i;
  out_h1 = stub_h1;
  out_h1.reloc_ptr = sizeof (out_h1) + sizeof (out_h2);
  out_h1.hdr_size = dst_image >> 4;
  out_h1.chksum = 0;
  memset (out_h2.res1, 0, sizeof (out_h2.res1));
  out_h2.new_lo = out_h2.new_hi = 0;
  i = os2_hdr_pos = dst_image + stub_size;
  if (os2_hdr_pos & 0x1ff)
    os2_hdr_pos = (os2_hdr_pos + 0x1ff) & ~0x1ff;
  fill2 = os2_hdr_pos - i;
}


/* Add the byte X to the LX header. */

void put_header_byte (byte x)
{
  put_grow (&header, &x, sizeof (x));
}


/* Add the 16-bit word X to the LX header. */

void put_header_word (word x)
{
  put_grow (&header, &x, sizeof (x));
}


/* Add the 32-bit word X to the LX header. */

void put_header_dword (dword x)
{
  put_grow (&header, &x, sizeof (x));
}


/* Add SIZE bytes at SRC to the LX header. */

void put_header_bytes (const void *src, size_t size)
{
  put_grow (&header, src, size);
}


/* Setup the OS/2 LX header. */

void set_os2_header (void)
{
  dword i, first_heap_page;
  int j, last_page, cur_page, obj;
  struct object obj_res;

  exe_flags ();
  out_h2.new_lo = LOWORD (os2_hdr_pos);
  out_h2.new_hi = HIWORD (os2_hdr_pos);
  obj_text.virt_size = a_in_h.a_text;
  obj_data.virt_size = a_in_h.a_data + a_in_h.a_bss;
  obj_stk0.virt_size = stack_size;
  obj_data.virt_base = round_segment (obj_text.virt_base + a_in_h.a_text);
  obj_heap.virt_base = round_segment (OBJ_END (obj_data));
  obj_stk0.virt_base = round_segment (OBJ_END (obj_heap));
  os2_h.stack_esp = obj_stk0.virt_size;
  text_pages = npages (a_in_h.a_text);
  heap_pages = 0;  gap_pages = 0;
  if (opt_c != NULL)
    {
      data_pages = npages (a_in_h.a_data + a_in_h.a_bss);
      heap_pages = npages (core_h.u_heap_brk - core_h.u_heap_base);

      /* Compute the number of pages between the last data page
         and the first heap page, for padding the a.out subfile.
         These pages are not mapped to any object.  Round down! */

      gap_pages = (core_h.u_heap_base - core_h.u_data_end) / 0x1000;
    }
  else
    data_pages = npages (a_in_h.a_data);

  /* The object page table and the fixup page table don't include the
     pages between the last data page and the first heap page
     (gap_pages). */

  os2_h.mod_pages = res_pages + text_pages + data_pages + heap_pages;
  obj_text.map_first = 1;
  obj_text.map_count = text_pages;
  obj_data.map_first = obj_text.map_first + obj_text.map_count;
  obj_data.map_count = data_pages;
  obj_heap.map_first = obj_data.map_first + obj_data.map_count;
  obj_heap.map_count = heap_pages;
  obj_stk0.map_first = obj_heap.map_first + obj_heap.map_count;
  obj_stk0.map_count = 0;

  /* The object numbers for the text object (OBJ_TEXT+1) and for the
     data object (OBJ_DATA+1) are fixed, the remaining object numbers
     are computed here. */

  os2_h.obj_count = 0;
  os2_h.stack_obj = 0;
  for (j = 0; j < OBJECTS; ++j)
    if (obj_h[j].virt_size != 0)
      {
        os2_h.obj_count += 1;
        if (j == OBJ_STK0 && obj_stk0.virt_size != 0)
          os2_h.stack_obj = os2_h.obj_count;
      }
  os2_h.obj_count += res_obj_count;

  get_module_name ();
  exports ();

  header.len = 0;
  put_header_bytes (&os2_h, sizeof (os2_h));

  os2_h.obj_offset = header.len;
  for (j = 0; j < OBJECTS; ++j)
    if (obj_h[j].virt_size != 0)
      put_header_bytes (&obj_h[j], sizeof (obj_h[j]));
  memset (&obj_res, 0, sizeof (obj_res));
  for (j = 0; j < res_obj_count; ++j)
    put_header_bytes (&obj_res, sizeof (obj_res));
  os2_h.pagemap_offset = header.len;
  for (j = 0; j < text_pages + data_pages; ++j)
    {
      put_header_dword (j + res_pages);
      put_header_word (0x1000);
      put_header_word (0);
    }
  first_heap_page = res_pages + text_pages + data_pages + gap_pages;
  for (j = 0; j < heap_pages; ++j)
    {
      put_header_dword (j + first_heap_page);
      put_header_word (0x1000);
      put_header_word (0);
    }
  for (j = 0; j < res_pages; ++j)
    {
      put_header_dword (j);
      put_header_word (0x1000);
      put_header_word (0);
    }

  if (res_count > 0)
    {
      os2_h.rsctab_offset = header.len;
      put_rsctab ();
    }

  os2_h.resname_offset = header.len;
  put_header_bytes (resnames.data, resnames.len);
  put_header_byte (0);

  os2_h.entry_offset = header.len;
  put_header_bytes (entry_tab.data, entry_tab.len);

  /* Record the location of the fixup page table and write a dummy
     fixup page table to be filled-in while creating the fixup record
     table.  Note that there's one additional fixup page table entry
     to hold the end address of the fixup section for the last
     page. */

  os2_h.fixpage_offset = header.len;
  for (j = 0; j <= os2_h.mod_pages; ++j)
    put_header_dword (0);           /* will be patched later */

  /* Record the location of the fixup record table. */

  os2_h.fixrecord_offset = header.len;

  /* Write the fixup record table.  Fill-in the fixup page table
     whenever changing to a new page.  Note that the header may move
     while adding fixup records. */

#define FIXPAGE(PAGE) (((dword *)(header.data + os2_h.fixpage_offset))[PAGE])

  last_page = 0;
  for (j = 0; j < fixup_len; ++j)
    {
      /* Get the object number of the fixup.  Note that fixups are
         sorted by object, then by address. */

      obj = fixup_data[j].obj;

      /* Compute the page number for the fixup. */

      cur_page = div_page (fixup_data[j].addr) + obj_h[obj].map_first - 1;

      /* Abort if the page number is out of range. */

      if (cur_page >= os2_h.mod_pages)
        error ("internal error 1");
      if (cur_page < last_page - 1)
        error ("internal error 4");

      /* Fill-in the fixup page table if we're on a new page. */

      while (last_page <= cur_page)
        {
          FIXPAGE (last_page) = header.len - os2_h.fixrecord_offset;
          ++last_page;
        }

      /* Add the fixup record to the fixup record table.  This may
         move the header in memory. */

      create_fixup (&fixup_data[j], FALSE);

      /* Treat fixups which cross page bounaries specially.  Two fixup
         records are created for such a fixup, one in the first page,
         one in the second page involved.  The offset for the second
         page is negative.  The method used here doesn't work with
         overlapping fixups as we move to the next page and can't go
         back (see internal error 4 above). */

      if ((fixup_data[j].addr & 0x0fff) > 0x0ffc)
        {
	  FIXPAGE (last_page) = header.len - os2_h.fixrecord_offset;
	  ++last_page;
          create_fixup (&fixup_data[j], TRUE);
        }
    }

  /* Fill-in the remaining entries of the fixup page table. */

  while (last_page <= os2_h.mod_pages)
    {
      FIXPAGE (last_page) = header.len - os2_h.fixrecord_offset;
      ++last_page;
    }

#undef FIXPAGE

  os2_h.impmod_offset = header.len;
  put_impmod ();

  os2_h.impprocname_offset = header.len;
  put_header_bytes (procs.data, procs.len);
  os2_h.fixup_size = header.len - os2_h.fixpage_offset;
  put_header_byte (0);
  os2_hdr_size = header.len;

  i = os2_hdr_pos + os2_hdr_size;
  if (res_count > 0)
    {
      a_out_pos = round_page (i);
      fill3 = a_out_pos - i;
      a_out_pos += res_pages * 0x1000 - A_OUT_OFFSET;
    }
  else
    {
      a_out_pos = round_page (i + A_OUT_OFFSET) - A_OUT_OFFSET;
      fill3 = a_out_pos - i;
    }
  os2_h.loader_size = os2_h.fixpage_offset - os2_h.obj_offset;
  os2_h.enum_offset = a_out_pos + A_OUT_OFFSET - res_pages * 0x1000;
  os2_h.instance_demand = data_pages + heap_pages;
  os2_h.preload_count = res_preload_pages;

  if (nonresnames.len != 0)
    {
      os2_h.nonresname_offset =  a_out_pos + a_out_size;
      os2_h.nonresname_size = nonresnames.len;
    }

  header.len = 0;
  put_header_bytes (&os2_h, sizeof (os2_h));
  for (j = 0; j < OBJECTS; ++j)
    if (obj_h[j].virt_size != 0)
      put_header_bytes (&obj_h[j], sizeof (obj_h[j]));

  put_res_obj (obj_stk0.map_first + obj_stk0.map_count);
  header.len = os2_hdr_size;
}


/* Initialize the fixed part of the OS/2 LX header.  Fields which are
   overwritten later are initialized to X. */

void init_os2_header (void)
{
  byte b;

  b = 0;
  put_grow (&procs, &b, 1);
#define X 0
  memset (&os2_h, 0, sizeof (os2_h));
  os2_h.magic = 0x584c;         /* LX */
  os2_h.byte_order = 0;         /* Little Endian byte order */
  os2_h.word_order = 0;         /* Little Endian word order */
  os2_h.level = 0;              /* Format level */
  os2_h.cpu = 2;                /* 386 */
  os2_h.os = 1;                 /* Operating system type: OS/2 */
  os2_h.ver = 0;                /* Module version */
  os2_h.mod_flags = 0x200;      /* WINDOWCOMPAT */
  if (!relocatable)
    os2_h.mod_flags |= 0x10;  /* no internal fixups */
  os2_h.mod_pages = X;          /* Number of pages in .exe file*/
  os2_h.entry_obj = OBJ_TEXT+1; /* Object number for EIP */
  os2_h.entry_eip = 0;          /* EIP */
  os2_h.stack_obj = X;          /* Stack object */
  os2_h.stack_esp = X;          /* ESP */
  os2_h.pagesize = 0x1000;      /* System page size */
  os2_h.pageshift = 12;         /* Page offset shift */
  os2_h.fixup_size = X;         /* Fixup section size */
  os2_h.fixup_checksum = 0;     /* Fixup section checksum */
  os2_h.loader_size = X;        /* Loader section size */
  os2_h.loader_checksum = 0;    /* Loader section checksum */
  os2_h.obj_offset = X;         /* Object table offset */
  os2_h.obj_count = X;          /* Number of objects */
  os2_h.pagemap_offset = X;
  os2_h.itermap_offset = 0;
  os2_h.rsctab_offset = 0;
  os2_h.rsctab_count = 0;
  os2_h.resname_offset = X;
  os2_h.entry_offset = X;
  os2_h.moddir_offset = 0;
  os2_h.moddir_count = 0;
  os2_h.fixpage_offset = X;
  os2_h.fixrecord_offset = X;
  os2_h.impmod_offset = X;
  os2_h.impmod_count = X;
  os2_h.impprocname_offset = X;
  os2_h.page_checksum_offset = 0;
  os2_h.enum_offset = X;
  os2_h.preload_count = X;
  os2_h.nonresname_offset = 0;
  os2_h.nonresname_size = 0;
  os2_h.nonresname_checksum = 0;
  os2_h.auto_obj = OBJ_DATA+1;
  os2_h.debug_offset = 0;
  os2_h.debug_size = 0;
  os2_h.instance_preload = 0;
  os2_h.instance_demand = X;
  os2_h.heap_size = 0;
  os2_h.stack_size = 0;
  obj_text.virt_size  = 0;
  obj_text.virt_base  = TEXT_BASE;
  obj_text.attr_flags = 0x2005;       /* readable, executable, big */
  obj_text.map_first  = X;
  obj_text.map_count  = X;
  obj_text.reserved   = 0;
  obj_data.virt_size  = 0;
  obj_data.virt_base  = 0;
  obj_data.attr_flags = 0x2003;       /* readable, writable, big */
  obj_data.map_first  = X;
  obj_data.map_count  = X;
  obj_data.reserved   = 0;
  obj_heap.virt_size  = heap_size;
  obj_heap.virt_base  = 0;
  obj_heap.attr_flags = 0x2003;       /* readable, writable, big */
  obj_heap.map_first  = X;
  obj_heap.map_count  = X;
  obj_heap.reserved   = 0;
  obj_stk0.virt_size  = 0;
  obj_stk0.virt_base  = 0;
  obj_stk0.attr_flags = 0x2003;       /* readable, writable, big */
  obj_stk0.map_first  = X;
  obj_stk0.map_count  = X;
  obj_stk0.reserved   = 0;
#undef X
}


/* Set the application type in the OS/2 LX header. */

void exe_flags (void)
{
  if (opt_f)
    {
      os2_h.mod_flags &= ~0x700;
      os2_h.mod_flags |= 0x100;
    }
  else if (opt_p)
    {
      os2_h.mod_flags &= ~0x700;
      os2_h.mod_flags |= 0x300;
    }
  else if (opt_w)
    {
      os2_h.mod_flags &= ~0x700;
      os2_h.mod_flags |= 0x200;
    }
  else
    switch (app_type)
      {
      case _MDT_NOTWINDOWCOMPAT:
        os2_h.mod_flags &= ~0x700;
        os2_h.mod_flags |= 0x100;
        break;
      case _MDT_WINDOWAPI:
        os2_h.mod_flags &= ~0x700;
        os2_h.mod_flags |= 0x300;
        break;
      case _MDT_WINDOWCOMPAT:
        os2_h.mod_flags &= ~0x700;
        os2_h.mod_flags |= 0x200;
        break;
      }
  if (dll_flag)
    {
      os2_h.mod_flags |= 0x8000;
      if (!init_global)
        os2_h.mod_flags |= 0x0004;
      if (!term_global)
        os2_h.mod_flags |= 0x40000000;
    }
}


/* Write the DOS EXE headers, the stub image, and the OS/2 LX header. */

void write_header (void)
{
  my_write (&out_h1, sizeof (out_h1), &out_file);
  my_write (&out_h2, sizeof (out_h2), &out_file);
  my_seek (&stub_file, stub_h1.reloc_ptr);
  copy (&stub_file, stub_h1.reloc_size * 4);
  fill (fill1);
  my_seek (&stub_file, src_image);
  copy (&stub_file, stub_size);
  fill (fill2);
  if (mode != 'u')
    {
      my_write (header.data, os2_hdr_size, &out_file);
      fill (fill3);
    }
}


/* Write the non-resident name table. */

void write_nonres (void)
{
  if (nonresnames.len != 0)
    {
      my_seek (&out_file, os2_h.nonresname_offset);
      my_write (nonresnames.data, nonresnames.len, &out_file);
    }
}


/* Copy the a.out subfile (a.out header, text and data sections,
   symbol table, and string table) to the destination executable. */

void copy_a_out (void)
{
  long n, str_len;

  my_write (&a_out_h, sizeof (a_out_h), &out_file);
  fill (A_OUT_OFFSET - sizeof (a_out_h));
  my_seek (&inp_file, a_in_text);
  if (text_image != NULL)
    my_write (text_image, round_page (a_in_h.a_text), &out_file);
  else
    copy (&inp_file, round_page (a_in_h.a_text));
  if (opt_c != NULL)
    {
      my_seek (&core_file, core_h.u_data_off);
      n = core_h.u_data_end - core_h.u_data_base;
      copy (&core_file, n);
      fill (round_page (n) - n);
      if (core_h.u_heap_brk > core_h.u_heap_base)
	{
	  fill (core_h.u_heap_base - round_page (core_h.u_data_end));
	  my_seek (&core_file, core_h.u_heap_off);
	  n = core_h.u_heap_brk - core_h.u_heap_base;
	  copy (&core_file, n);
	  fill (round_page (n) - n);
	}
    }
  else if (data_image != NULL)
    my_write (data_image, round_page (a_in_h.a_data), &out_file);
  else
    {
      my_seek (&inp_file, a_in_data);
      copy (&inp_file, round_page (a_in_h.a_data));
    }
  if (a_out_h.a_syms == 0)
    {
      str_len = 4;
      my_write (&str_len, sizeof (str_len), &out_file);
    }
  else
    {
      if (sym_image != NULL)
	my_write (sym_image, a_out_h.a_syms, &out_file);
      else
	{
	  my_seek (&inp_file, a_in_sym);
	  copy (&inp_file, a_out_h.a_syms);
	}
      if (str_image != NULL)
	my_write (str_image, a_out_str_size, &out_file);
      else
	{
	  my_seek (&inp_file, a_in_str);
	  copy (&inp_file, a_out_str_size);
	}
    }
}


/* Copy SIZE bytes from the file SRC to the destination executable
   file (out_file). */

void copy (struct file *src, long size)
{
  char buf[BUFSIZ];
  size_t n;

  while (size > 0)
    {
      n = MIN (size, BUFSIZ);
      my_read (buf, n, src);
      my_write (buf, n, &out_file);
      size -= n;
    }
}


/* Write COUNT zero bytes to the destination executable file
   (out_file). */

void fill (long count)
{
  char buf[BUFSIZ];
  size_t n;

  memset (buf, 0, BUFSIZ);
  while (count > 0)
    {
      n = MIN (count, BUFSIZ);
      my_write (buf, n, &out_file);
      count -= n;
    }
}
