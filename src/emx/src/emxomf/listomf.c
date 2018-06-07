/* listomf.c -- List OMF files (.obj and .lib files)
   Copyright (c) 1993-1998 Eberhard Mattes

This file is part of listomf.

listomf is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

listomf is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with listomf; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#ifdef __EMX__
#include <io.h>
#endif /* __EMX__ */
#include <getopt.h>
#include <sys/omflib.h>
#include "defs.h"

#pragma pack(1)

struct timestamp
{
  byte hours;
  byte minutes;
  byte seconds;
  byte hundredths;
  byte day;
  byte month;
  word year;
};

#pragma pack()

struct segment
{
  int name;
  int class;
  dword length;
};

struct group
{
  int name;
};

struct ext
{
  char *name;
};

enum debug
{
  debug_default,
  debug_cv,
  debug_hll
};


static int lname_count;
static int lname_alloc;
static char **lname_list;
static int segment_count;
static int segment_alloc;
static struct segment *segment_list;
static int group_count;
static int group_alloc;
static struct group *group_list;
static int ext_count;
static int ext_alloc;
static struct ext *ext_list;
static int pub_count;
static int debug_type_index;
static int done;
static int rec_type;
static int rec_len;
static int rec_idx;
static long rec_pos;
static byte rec_buf[MAX_REC_SIZE+8];
static enum debug debug_info;
static int hll_style;
static word line_count;
static dword names_length;
static int linnum_started;
static byte linnum_entry_type;
static int linnum_file_idx;
static int linnum_files;
static word page_size;
static word dict_blocks;
static dword dict_offset;

static char list_debug = TRUE;
static char show_addr = FALSE;
/** If set we will do a hexdump after every record we process. */
static char hex_dump = FALSE;


static void out_of_mem (void)
{
  fputs ("Out of memory\n", stderr);
  exit (2);
}


static void *xmalloc (size_t n)
{
  void *p;

  p = malloc (n);
  if (p == NULL)
    out_of_mem ();
  return p;
}


static void *xrealloc (void *src, size_t n)
{
  void *p;

  p = realloc (src, n);
  if (p == NULL)
    out_of_mem ();
  return p;
}


static char *xstrdup (const char *src)
{
  char *p;

  p = strdup (src);
  if (p == NULL)
    out_of_mem ();
  return p;
}


static void show_record (const char *name)
{
  if (show_addr)
    printf ("%ld: %s", rec_pos, name);
  else if (hex_dump)
    printf ("%08lx: %s", rec_pos, name);
  else
    printf ("%s", name);
  if (rec_type & REC32)
    printf ("32");
}


static void get_mem (void *dst, int len)
{
  if (rec_idx + len > rec_len)
    {
      fprintf (stderr, "\nString at %d beyond end of record\n", rec_idx);
      exit (2);
    }
  memcpy (dst, rec_buf + rec_idx, len);
  rec_idx += len;
}


static void get_string (byte *dst)
{
  int len;

  if (rec_idx >= rec_len)
    {
      fprintf (stderr, "\nString at %d beyond end of record\n", rec_idx);
      exit (2);
    }
  len = rec_buf[rec_idx++];
  get_mem (dst, len);
  dst[len] = 0;
}


static byte *get_cpp_string (void)
{
  int len;
  byte *p;

  if (rec_idx >= rec_len)
    {
      fprintf (stderr, "\nString at %d beyond end of record\n", rec_idx);
      exit (2);
    }
  len = rec_buf[rec_idx++];
  if (len & 0x80)
    len = ((len & 0x7f) << 8) + rec_buf[rec_idx++];
  p = xmalloc (len + 1);
  get_mem (p, len);
  p[len] = 0;
  return p;
}


static int get_index (void)
{
  int result;

  if (rec_idx >= rec_len)
    {
      fprintf (stderr, "\nIndex at %d beyond end of record\n", rec_idx);
      exit (2);
    }
  result = rec_buf[rec_idx++];
  if (result & 0x80)
    {
      if (rec_idx >= rec_len)
        {
          fprintf (stderr, "\nIndex at %d beyond end of record\n", rec_idx);
          exit (2);
        }
      result = ((result & 0x7f) << 8) | rec_buf[rec_idx++];
    }
  return result;
}


static dword get_dword (void)
{
  dword result;

  if (rec_idx + 4 > rec_len)
    {
      fprintf (stderr, "\nDword at %d beyond end of record\n", rec_idx);
      exit (2);
    }
  result = rec_buf[rec_idx++];
  result |= rec_buf[rec_idx++] << 8;
  result |= rec_buf[rec_idx++] << 16;
  result |= rec_buf[rec_idx++] << 24;
  return result;
}


static word get_word (void)
{
  word result;

  if (rec_idx + 2 > rec_len)
    {
      fprintf (stderr, "\nWord at %d beyond end of record\n", rec_idx);
      exit (2);
    }
  result = rec_buf[rec_idx++];
  result |= rec_buf[rec_idx++] << 8;
  return result;
}


static word get_byte (void)
{
  if (rec_idx + 1 > rec_len)
    {
      fprintf (stderr, "\nByte at %d beyond end of record\n", rec_idx);
      exit (2);
    }
  return rec_buf[rec_idx++];
}


static dword get_word_or_dword (void)
{
  return (rec_type & REC32 ? get_dword () : get_word ());
}


static dword get_commlen (void)
{
  dword result;

  result = get_byte ();
  if (result <= 0x80)
    return result;
  switch (result)
    {
    case 0x81:
      return get_word ();
    case 0x84:
      result = get_byte ();
      result |= get_byte () << 8;
      result |= get_byte () << 16;
      return result;
    case 0x88:
      return get_dword ();
    default:
      fprintf (stderr, "\nUnknown COMDAT length prefix\n");
      exit (2);
    }
}


static void show_char (byte c)
{
  if (c == '\\')
    printf ("\\\\");
  else if (c < 0x20 || c > 0x7f)
    printf ("\\%.3o", c);
  else
    putchar (c);
}


static void show_string (const byte *p)
{
  printf ("\"");
  while (*p != 0)
    show_char (*p++);
  printf ("\"");
}


static void show_name (int index)
{
  if (index < 1 || index > lname_count)
    printf ("#%d", index);
  else
    show_string (lname_list[index-1]);
}


static void show_seg (int index)
{
  if (index < 1 || index > segment_count)
    printf ("#%d", index);
  else
    { /* bird: both please */
      show_name (segment_list[index-1].name);
      printf ("(#%d)", index);
    }
}


static void show_group (int index)
{
  if (index < 1 || index > group_count)
    printf ("#%d", index);
  else
  { /* bird: both please */
    show_name (group_list[index-1].name);
    printf ("(#%d)", index);
  }
}


static void show_ext (int index)
{
  if (index < 1 || index > ext_count)
    printf ("#%d", index);
  else
  { /* bird: both please */
    show_string (ext_list[index-1].name);
    printf ("(#%d)", index);
  }
}


static void show_enc (void)
{
  byte *p;

  p = get_cpp_string ();
  show_string (p);
  free (p);
}


static void dump_block (int count, int indent, int foff)
{
  int x;
  char ascii[17];

  x = 0;
  while (rec_idx < rec_len && count > 0)
    {
      unsigned int ui = rec_buf[rec_idx++];
      --count;

      if (x >= 16)
        {
          for (x = 0; x < 16; x++)
            if (!isprint (ascii[x]))
              ascii[x] = '.';
          printf ("  %.16s\n", ascii);
          x = 0;
        }
      ascii[x] = ui;
      if (x == 0)
        {
          if (foff)
            printf ("%*s%08lx %03x:", indent, "",
                    rec_pos + rec_idx - 1 + sizeof (struct omf_rec),
                    rec_idx - 1 + sizeof (struct omf_rec));
          else
            printf ("%*s", indent, "");
        }
      if (x != 8)
        printf (" %.2x", ui);
      else
        printf ("-%.2x", ui);
      ++x;
    }
  if (x != 0)
    {
      int cch = x;
      printf("%*s", (16 - cch)*3, "");
      for (x = 0; x < cch; x++)
        if (!isprint (ascii[x]))
          ascii[x] = '.';
      ascii[cch] = '\0';
       printf ("  %.*s\n", cch, ascii);
    }
}


static void dump_rest (void)
{
  dump_block (rec_len - rec_idx, 2, 0);
}


static void add_ext (const byte *name)
{
  if (ext_count >= ext_alloc)
    {
      ext_alloc += 16;
      ext_list = xrealloc (ext_list, ext_alloc * sizeof (*ext_list));
    }
  ext_list[ext_count++].name = xstrdup (name);
}


static void list_libhdr (void)
{
  byte flags;

  printf ("LIBHDR:\n");
  page_size = rec_len + 1 + 3;
  printf ("  Page size: %u\n", page_size);
  dict_offset = get_dword ();
  printf ("  Dictionary offset: 0x%lx\n", dict_offset);
  dict_blocks = get_word ();
  printf ("  Dictionary blocks: %u\n", dict_blocks);
  flags = get_byte ();
  printf ("  Flags: 0x%.2x", flags);
  if (flags & 0x01)
    printf (" (case-sensitive)");
  printf ("\n");
}


static void list_theadr (void)
{
  byte string[256];

  if (page_size != 0)
    printf ("------------------------------------------------------------\n");
  show_record ("THEADR");
  printf (" ");
  get_string (string);
  show_string (string);
  printf ("\n");
  lname_count = 0; segment_count = 0; group_count = 0;
  ext_count = 0; pub_count = 0;
  debug_type_index = 512;
  debug_info = debug_default; hll_style = 0; linnum_started = FALSE;
}


static void list_coment (void)
{
  byte string[256], string2[256], string3[256];
  byte flag, rec, subrec, c1, c2;
  word ord = 0;
  int len, ext1, ext2;

  show_record ("COMENT");
  printf (": ");
  get_byte ();                  /* Comment type */
  rec = get_byte ();
  switch (rec)
    {
    case 0x9e:
      printf ("DOSSEG\n");
      break;

    case 0x9f:
      len = rec_len - rec_idx;
      get_mem (string, len);
      string[len] = 0;
      printf ("Default library: ");
      show_string (string);
      break;

    case 0xa0:
      subrec = get_byte ();
      switch (subrec)
        {
        case 0x01:
          printf ("IMPDEF ");
          flag = get_byte ();
          get_string (string);
          get_string (string2);
          if (flag != 0)
            {
              ord = get_word ();
              printf ("%s %s.%u", string, string2, ord);
            }
          else
            {
              get_string (string3);
              if (string3[0] == 0)
                strcpy (string3, string);
              printf ("%s %s.%s", string, string2, string3);
            }
          break;

        case 0x02:
          printf ("EXPDEF ");
          flag = get_byte ();
          get_string (string);
          get_string (string2);
          if (flag & 0x80)
            ord = get_word ();
          show_string (string);
          printf (" (");
          show_string (string2);
          printf (")");
          if (flag & 0x80)
            printf (" @%u", ord);
          if (flag & 0x40)
            printf (", resident");
          if (flag & 0x20)
            printf (", no data");
          if (flag & 0x1f)
            printf (", %u parameters", flag & 0x1f);
          break;

        case 0x04:
          printf ("Protected DLL\n");
          break;

        default:
          printf ("unknown comment class: %.2x\n", (unsigned)subrec);
          dump_rest ();
          return;               /* Don't print a newline */
        }
      break;

    case 0xa1:
      if (rec_idx + 3 > rec_len) /* Borland vs. IBM clash */
        goto generic;
      flag = get_byte ();
      c1 = get_byte ();
      c2 = get_byte ();
      if (c1 == 'C' && c2 == 'V')
        {
          debug_info = debug_cv;
          printf ("CodeView style (%u) debug tables", flag);
        }
      else if (c1 == 'H' && c2 == 'L')
        {
          debug_info = debug_hll;
          hll_style = flag;
          printf ("HLL style (%u) debug tables", flag);
        }
      else
        printf ("New OMF extension");
      break;

    case 0xa2:
      flag = get_byte ();
      if (flag == 1)
        printf ("Link pass separator");
      else
        printf ("Unknown link pass: %.2x", flag);
      break;

    case 0xa3:
      get_string (string);
      printf ("LIBMOD ");
      show_string (string);
      break;

    case 0xa8:
      ext1 = get_index ();
      ext2 = get_index ();
      printf ("WKEXT ");
      show_ext (ext1);
      printf (" ");
      show_ext (ext2);
      break;

    case 0xad:
      get_string (string);
      printf ("debug pack DLL: ");
      show_string (string);
      break;

    case 0xaf:
      get_string (string);
      printf ("Identifier manipulator DLL: ");
      show_string (string);
      get_string (string);
      printf (", initialization parameter: ");
      show_string (string);
      break;

    default:
generic:
      printf ("unknown comment class: %.2x\n", rec);
      dump_rest ();
      return;                   /* Don't print a newline */
    }
  printf ("\n");
}


static void list_lnames (void)
{
  byte string[256];

  show_record ("LNAMES");
  printf (":\n");
  while (rec_idx < rec_len)
    {
      get_string (string);
      if (lname_count >= lname_alloc)
        {
          lname_alloc += 16;
          lname_list = xrealloc (lname_list,
                                 lname_alloc * sizeof (*lname_list));
        }
      lname_list[lname_count] = xstrdup (string);
      ++lname_count;
      printf ("  #%d: ", lname_count);
      show_string (string);
      printf ("\n");
    }
}


static void list_segdef (void)
{
  int attributes, alignment, combination, frame, offset, name, class, overlay;
  dword length;
  struct segment *p;

  frame = 0;                    /* Keep the compiler happy */

  show_record ("SEGDEF");
  printf (" #%d ", segment_count + 1);

  attributes = get_byte ();
  alignment = attributes >> 5;
  combination = (attributes >> 2) & 7;
  if (alignment == 0)
    {
      frame = get_word ();
      offset = get_byte ();
    }
  length = get_word_or_dword ();
  name = get_index ();
  class = get_index ();
  overlay = get_index ();
  if (!(rec_type & REC32) && (attributes & 2))
    length = 1L << 16;

  if (segment_count >= segment_alloc)
    {
      segment_alloc += 8;
      segment_list = xrealloc (segment_list,
                               segment_alloc * sizeof (*segment_list));
    }
  p = &segment_list[segment_count++];
  p->name = name;
  p->class = class;
  p->length = length;

  show_name (name);
  printf ("  ");
  switch (alignment)
    {
    case 0:
      printf ("AT %#.4x", frame);
      break;
    case 1:
      printf ("BYTE");
      break;
    case 2:
      printf ("WORD");
      break;
    case 3:
      printf ("PARA");
      break;
    case 4:
      printf ("PAGE");
      break;
    case 5:
      printf ("DWORD");
      break;
    case 6:
      printf ("unknown alignment: %d", alignment);
      break;
    }
  printf (" ");
  switch (combination)
    {
    case 0:
      printf ("PRIVATE");
      break;
    case 2:
    case 4:
    case 7:
      printf ("PUBLIC");
      break;
    case 5:
      printf ("STACK");
      break;
    case 6:
      printf ("COMMON");
      break;
    default:
      printf ("unknown combination: %d", combination);
      break;
    }
  if (attributes & 1)
    printf (" USE32");
  printf (" Length: %#lx", length);
  printf (" CLASS ");
  show_name (class);
  printf ("\n");
}


static void list_grpdef (void)
{
  int name, type, seg;

  show_record ("GRPDEF");
  printf (" #%d ", group_count + 1);

  name = get_index ();
  if (group_count >= group_alloc)
    {
      group_alloc += 8;
      group_list = xrealloc (group_list,
                             group_alloc * sizeof (*group_list));
    }
  group_list[group_count++].name = name;
  show_name (name);
  printf (":");
  while (rec_idx < rec_len)
    {
      type = get_byte ();
      if (type != 0xff)
        printf (" [unknown type: %#x] ", type);
      seg = get_index ();
      printf (" ");
      show_seg (seg);
    }
  printf ("\n");
}


static void list_extdef (void)
{
  int type;
  byte name[256];

  show_record ("EXTDEF");
  printf (":\n");

  while (rec_idx < rec_len)
    {
      printf ("  #%d: ", ext_count + 1);
      get_string (name);
      show_string (name);
      type = get_index ();
      printf (", type: %d\n", type);
      add_ext (name);
    }
}


static void list_pubdef (void)
{
  int type, group, seg;
  dword offset;
  byte name[256];

  show_record ("PUBDEF");
  group = get_index ();
  seg = get_index ();
  printf (" base group: ");
  show_group (group);
  printf (", base seg: ");
  show_seg (seg);
  if (seg == 0)
    printf (", frame=0x%.4x", get_word ());
  printf ("\n");

  while (rec_idx < rec_len)
    {
      printf ("  #%d: ", pub_count + 1);
      get_string (name);
      show_string (name);
      offset = get_word_or_dword ();
      type = get_index ();
      printf (", offset: %#lx, type: %d\n", offset, type);
      ++pub_count;
    }
}


static void list_comdef (void)
{
  int type_index, data_type;
  byte name[256];
  dword comm_len, comm_count;

  show_record ("COMDEF");
  printf ("\n");
  while (rec_idx < rec_len)
    {
      get_string (name);
      type_index = get_index ();
      data_type = get_byte ();
      printf ("  #%d: ", ext_count + 1);
      show_string (name);
      printf (", type index: %d, ", type_index);
      switch (data_type)
        {
        case 0x61:
          comm_count = get_commlen ();
          comm_len = get_commlen ();
          printf ("FAR, %lu times %lu bytes\n", comm_count, comm_len);
          break;
        case 0x62:
          comm_len = get_commlen ();
          printf ("NEAR, %lu bytes\n", comm_len);
          break;
        default:
          printf ("unknown data type: %#x\n", data_type);
          dump_rest ();
          return;
        }
      add_ext (name);
    }
  dump_rest ();
}


static void list_alias (void)
{
  byte name[256];

  show_record ("ALIAS");
  printf (": ");
  get_string (name);
  show_string (name);
  printf (" ");
  get_string (name);
  show_string (name);
  printf ("\n");
  dump_rest ();
}


static void list_symbols (void)
{
  int len, next, total_rec_len, sst, location, loc_base, start;
  word seg, file;
  dword offset, length, reg, line;
  dword type, prologue, body, reserved, near_far, compiler_id, class_type;
  byte name[256];
  struct timestamp ts;

  total_rec_len = rec_len;
  loc_base = rec_idx;
  while (rec_idx < rec_len)
    {
      start = rec_idx;
      len = get_byte ();
      if (len & 0x80)
        { /* bird: wondered if this is HL04 and later... */
          len = ((0x7f & len) << 8) | get_byte ();
        }
      if (len == 0)
        {
          printf ("Length of SST entry is 0\n");
          break;
        }
      --len;
      sst = get_byte ();
      next = rec_idx + len;
      rec_len = next;
      printf ("  ");
      switch (sst)
        {
        case 0x00:
          printf ("Begin ");
          location = rec_idx - loc_base;
          offset = get_dword ();
          if (rec_idx < rec_len)
            {
              length = get_dword ();
              printf ("offset: %#lx [@%#x], length: %#lx",
                      offset, location, length);
              if (rec_idx < rec_len)
                {
                  get_string (name);
                  printf (" ");
                  show_string (name);
                }
            }
          else
            printf ("offset: %#lx [@%#x], no length",
                    offset, location);
          break;
        case 0x01:
        case 0x0f:
        case 0x1a:
        case 0x1d:
          switch (sst)
            {
            case 0x01:
              printf ("Proc ");
              break;
            case 0x0f:
              printf ("Entry ");
              break;
            case 0x1a:
              printf ("MemFunc ");
              break;
            case 0x1d:
              printf ("C++Proc ");
              break;
            default:
              abort ();
            }
          location = rec_idx - loc_base;
          offset = get_dword ();
          type = get_word ();
          length = get_dword ();
          prologue = get_word ();
          body = get_dword ();
          class_type = get_word ();
          near_far = get_byte ();
          if (sst == 0x1a || sst == 0x1d)
            show_enc ();
          else
            {
              get_string (name);
              show_string (name);
            }
          printf (", offset: %#lx [@%#x], type: #%lu\n",
                  offset, location, type);
          printf ("       length: %#lx, pro: %#lx, pro+body: %#lx, ",
                  length, prologue, body);
          switch (near_far)
            {
            case 0x00:
              printf ("16-bit near");
              break;
            case 0x04:
              printf ("16-bit far");
              break;
            case 0x08:
              printf ("32-bit near");
              break;
            case 0x0c:
              printf ("32-bit far");
              break;
            default:
              printf ("nf: %#lx", near_far);
              break;
            }
          if (sst == 0x1a)      /* Member function */
            printf ("\n       class type: #%lu", class_type);
          break;
        case 0x02:
          printf ("End");
          break;
        case 0x04:
          printf ("Auto ");
          offset = get_dword ();
          type = get_word ();
          get_string (name);
          show_string (name);
          printf (", offset: %#lx, type: #%lu", offset, type);
          break;
        case 0x05:
        case 0x1e:
          if (sst == 0x1e)
            printf ("C++Static ");
          else
            printf ("Static ");
          location = rec_idx - loc_base;
          offset = get_dword ();
          seg = get_word ();
          type = get_word ();
          if (sst == 0x1e)
            show_enc ();
          else
            {
              get_string (name);
              show_string (name);
            }
          printf (", offset: %#lx [@%#x], seg: %u [@%#x], type: #%lu",
                  offset, location, seg, location + 4, type);
          break;
        case 0x0b:
          printf ("CodeLabel ");
          location = rec_idx - loc_base;
          offset = get_dword ();
          near_far = get_byte ();
          get_string (name);
          printf ("offset: %#lx [@%#x]", offset, location);
          show_string (name);
          break;
        case 0x0c:
          printf ("With");
          break;
        case 0x0d:
          printf ("Reg ");
          type = get_word ();
          reg = get_byte ();
          get_string (name);
          show_string (name);
          printf (", ");
          switch (reg)
            {
            case 0x00:
              printf ("AL");
              break;
            case 0x01:
              printf ("CL");
              break;
            case 0x02:
              printf ("DL");
              break;
            case 0x03:
              printf ("BL");
              break;
            case 0x04:
              printf ("AH");
              break;
            case 0x05:
              printf ("CH");
              break;
            case 0x06:
              printf ("DH");
              break;
            case 0x07:
              printf ("BH");
              break;
            case 0x08:
              printf ("AX");
              break;
            case 0x09:
              printf ("CX");
              break;
            case 0x0a:
              printf ("DX");
              break;
            case 0x0b:
              printf ("BX");
              break;
            case 0x0c:
              printf ("SP");
              break;
            case 0x0d:
              printf ("BP");
              break;
            case 0x0e:
              printf ("SI");
              break;
            case 0x0f:
              printf ("DI");
              break;
            case 0x10:
              printf ("EAX");
              break;
            case 0x11:
              printf ("ECX");
              break;
            case 0x12:
              printf ("EDX");
              break;
            case 0x13:
              printf ("EBX");
              break;
            case 0x14:
              printf ("ESP");
              break;
            case 0x15:
              printf ("EBP");
              break;
            case 0x16:
              printf ("ESI");
              break;
            case 0x17:
              printf ("EDI");
              break;
            case 0x18:
              printf ("ES"); /* error in documentation */
              break;
            case 0x19:
              printf ("CS");
              break;
            case 0x1a:
              printf ("SS");
              break;
            case 0x1b:
              printf ("DS");
              break;
            case 0x1c:
              printf ("FS");
              break;
            case 0x1d:
              printf ("GS");
              break;
            case 0x20:
              printf ("DX:AX");
              break;
            case 0x21:
              printf ("ES:BX");
              break;
            case 0x22:
              printf ("IP");
              break;
            case 0x23:
              printf ("FLAGS");
              break;
            case 0x24:
              printf ("EFLAGS");
              break;
            case 0x80:
            case 0x81:
            case 0x82:
            case 0x83:
            case 0x84:
            case 0x85:
            case 0x86:
            case 0x87:
              printf ("ST(%lu)", reg - 0x80);
              break;
            default:
              printf ("unknown register: %#.2lx", reg);
            }
          break;
        case 0x0e:
          printf ("Constant");
          break;
        case 0x10:
          printf ("Skip");
          break;
        case 0x11:
          printf ("ChangSeg ");
          location = rec_idx - loc_base;
          seg = get_word ();
          reserved = get_word ();
          printf ("segment: %u [@%#x]", seg, location);
          printf (", reserved: %#lx", reserved);
          break;
        case 0x12:
          printf ("Typedef");
          break;
        case 0x13:
          printf ("Public");
          break;
        case 0x14:
          printf ("Member ");
          offset = get_dword ();
          printf ("offset: %#lx, name: ", offset);
          get_string (name);
          show_string (name);
          break;
        case 0x15:
          printf ("Based ");
          offset = get_dword ();
          type = get_word ();
          printf ("offset: %#lx, type: #%lu, name: ", offset, type);
          get_string (name);
          show_string (name);
          break;
        case 0x16:
          printf ("Tag ");
          type = get_word ();
          printf ("type: #%lu, name: ", type);
          get_string (name);
          show_string (name);
          break;
        case 0x17:
          printf ("Table");
          break;
        case 0x18:
          printf ("Map ");
          get_string (name);
          show_string (name);
          printf (" to external ");
          get_string (name);
          show_string (name);
          break;
        case 0x19:
          printf ("Class ");
          type = get_word ();
          printf ("type: #%lu, name: ", type);
          show_enc ();
          break;
        case 0x1b:
          printf ("AutoScoped ");
          offset = get_dword ();
          file = get_word ();
          line = get_dword ();
          type = get_word ();
          get_string (name);
          printf ("offset: %#lx, source file: %u, line: %lu, type: #%lu\n",
                  offset, file, line, type);
          printf ("    name: ");
          show_string (name);
          break;
        case 0x1c:
          printf ("StaticScoped ");
          offset = get_dword ();
          seg = get_word ();
          file = get_word ();
          line = get_dword ();
          type = get_word ();
          get_string (name);
          printf ("offset: %#lx, segment: %u, source file: %u, "
                  "line: %lu, type: %lu\n",
                  offset, seg, file, line, type);
          printf ("    name: ");
          show_string (name);
          break;
        case 0x40:
          printf ("CuInfo: ");
          compiler_id = get_byte ();
          switch (compiler_id)
            {
            case 0x01:
              printf ("C");
              break;
            case 0x02:
              printf ("C++");
              break;
            case 0x03:
              printf ("PL/X-86");
              break;
            case 0x04:
              printf ("PL/I");
              break;
            default:
              printf ("unknown compiler id: %#lx", compiler_id);
              break;
            }
          printf (", options: ");
          get_string (name);
          show_string (name);
          printf (", compiler date: ");
          get_string (name);
          show_string (name);
          printf ("\n    time stamp: ");
          get_mem (&ts, sizeof (ts));
          printf ("%d/%.2d/%.2d %.2d:%.2d:%.2d.%.2d",
                  ts.year, ts.month, ts.day, ts.hours,
                  ts.minutes, ts.seconds, ts.hundredths);
          break;
        default:
          printf ("unknown SST type %#.2x at %d", sst, start);
          break;
        }
      printf ("\n");
      rec_idx = next;
      rec_len = total_rec_len;

      if (hex_dump)
        {
          int len = rec_idx - start;
          printf ("  hexdump:\n");
          rec_idx = start;
          dump_block (len, 3, 1);
        }
    }
}



static void show_fid (void)
{
  int fid, index, i;
  byte str[256];
  dword d;

  fid = get_byte ();
  switch (fid)
    {
    case 0x80:
      printf ("(nil)");
      break;
    case 0x81:
      printf ("void");
      break;
    case 0x82:
      get_string (str);
      show_string (str);
      break;
    case 0x83:
      index = get_word ();
      switch (index)
        {
        case 0x80:
          printf ("signed char");
          break;
        case 0x81:
          printf ("signed short");
          break;
        case 0x82:
          printf ("signed long");
          break;
        case 0x84:
          printf ("unsigned char");
          break;
        case 0x85:
          printf ("unsigned short");
          break;
        case 0x86:
          printf ("unsigned long");
          break;
        case 0x88:
          printf ("float");
          break;
        case 0x89:
          printf ("double");
          break;
        case 0x8a:
          printf ("long double");
          break;
        case 0x97:
          printf ("void");
          break;
        case 0xa0:
          printf ("signed char *");
          break;
        case 0xa1:
          printf ("signed short *");
          break;
        case 0xa2:
          printf ("signed long *");
          break;
        case 0xa4:
          printf ("signed char *");
          break;
        case 0xa5:
          printf ("signed short *");
          break;
        case 0xa6:
          printf ("signed long *");
          break;
        case 0xa8:
          printf ("float *");
          break;
        case 0xa9:
          printf ("double *");
          break;
        case 0xaa:
          printf ("long double *");
          break;
        case 0xb7:
          printf ("void *");
          break;
        default:
          printf ("#%d", index);
          break;
        }
      break;
    case 0x85:
      i = get_word ();
      printf ("%u", i);
      break;
    case 0x86:
      d = get_dword ();
      printf ("%lu", d);
      break;
    case 0x88:
      i = get_byte ();
      if (i >= 0x80)
        i -= 0x80;
      printf ("%d", i);
      break;
    case 0x89:
      i = get_word ();
      if (i >= 0x8000)
        i -= 0x8000;
      printf ("%d", i);
      break;
    case 0x8a:
      d = get_dword ();
      printf ("%ld", d);
      break;
    case 0x8b:
      i = get_byte ();
      printf ("%u", i);
      break;
    default:
      fprintf (stderr, "unknown FID: %#.2x\n", fid);
      exit (2);
    }
}


static void list_protection (void)
{
  int flags;

  flags = get_byte ();
  switch (flags)
    {
    case 0x00:
      printf ("private");
      break;
    case 0x01:
      printf ("protected ");
      break;
    case 0x02:
      printf ("public");
      break;
    default:
      printf ("unknown protection: %#x", flags);
      break;
    }
}


static void list_types (void)
{
  int total_rec_len, type, qual, len, next, flags, start;
  int args, max_args, fields;
  dword size;

  flags = 0;                    /* Keep the optimizer happy */
  total_rec_len = rec_len;
  while (rec_idx < rec_len)
    {
      len = get_word ();
      next = rec_idx + len;
      start = rec_idx;
      type = get_byte ();
      qual = get_byte ();
      printf ("  #%d: ", debug_type_index++);
      if (next < total_rec_len)
        rec_len = next;
      switch (type)
        {
        case 0x40:
          printf ("Class: ");
          if (qual & 1)
            printf ("is_struct ");
          if (qual & ~1)
            printf ("(qual: %#x) ", qual);
          size = get_dword ();
          fields = get_word ();
          printf ("size: %#lx, %d items: ", size, fields);
          printf ("#%u, name: ", get_word());
          show_enc ();
          printf ("\n");
          dump_rest ();
          break;

        case 0x41:
          printf ("Base Class: ");
          if (qual & 1)
            printf ("virtual, ");
          if ((qual & ~1))
            printf ("(qual: %#x) ", qual);
          list_protection ();
          printf (", class: #%u, offset: ", get_word ());
          show_fid ();
          printf ("\n");
          dump_rest ();
          break;

        case 0x42:
          printf ("Friend: ");
          if (qual & 1)
            printf ("class ");
          else
            printf ("function ");
          if (qual & ~1)
            printf ("(qual: %#x) ", qual);
          printf ("type: #%u ", get_word ());
          printf (", name: ");
          show_enc ();
          printf ("\n");
          dump_rest ();
          break;

        case 0x43:
          printf ("Class Definition: ");
          if (qual != 0)
            printf ("(qual: %#x) ", qual);
          list_protection ();
          printf (", typedef #%u", get_word ());
          printf (", class #%u\n", get_word ());
          dump_rest ();
          break;

        case 0x45:
          printf ("Member function: ");
          if (qual & 0x01)
            printf ("static ");
          if (qual & 0x02)
            printf ("inline ");
          if (qual & 0x04)
            printf ("const ");
          if (qual & 0x08)
            printf ("volatile ");
          if (qual & 0x10)
            printf ("virtual ");
          if (qual & ~0x1f)
            printf ("(qual: %#x) ", qual);
          list_protection ();
          type = get_byte ();
          switch (type)
            {
            case 0:
              break;
            case 1:
              printf (", constructor");
              break;
            case 2:
              printf (", destructor");
              break;
            default:
              printf (", unknown type: %#x", type);
              break;
            }
          type = get_word ();
          printf (", type: #%u", type);
          if (qual & 0x10)
            {
              printf (", virtual table index: ");
              show_fid ();
            }
          printf ("\n    name: ");
          show_enc ();
          printf ("\n");
          dump_rest ();
          break;

        case 0x46:
          printf ("Class Member: ");
          if (qual & 0x01)
            printf ("static ");
          if (qual & 0x02)
            printf ("vtbl_ptr ");
          if (qual & 0x04)
            printf ("vbase_ptr ");
          if (qual & 0x08)
            printf ("const ");
          if (qual & 0x10)
            printf ("volatile ");
          if (qual & 0x20)
            printf ("self_ptr ");
          if ((qual & ~0x3f) != 0)
            printf ("(qual: %#x) ", qual);
          list_protection ();
          printf (", type: #%u, offset: ", get_word ());
          show_fid ();
          printf (",\n    static name: ");
          show_enc ();
          printf (", name: ");
          show_enc ();
          printf ("\n");
          dump_rest ();
          break;

        case 0x48:
          printf ("Reference: ");
          if (qual != 0)
            printf ("(qual: %#x) ", qual);
          printf ("type: #%u\n", get_word ());
          dump_rest ();
          break;

        case 0x49:
          printf ("Member Pointer: ");
          if (qual & 0x01)
            printf ("has vbases, ");
          if (qual & 0x02)
            printf ("has mult-inh, ");
          if (qual & 0x04)
            printf ("const ");
          if (qual & 0x08)
            printf ("volatile ");
          if (qual & ~0x0f)
            printf ("qual: %#x, ", qual);
          printf ("child type: #%u, ", get_word ());
          printf ("class type: #%u, ", get_word ());
          printf ("representation type: #%u\n", get_word ());
          dump_rest ();
          break;

        case 0x52:
          printf ("Set: ");
          if (qual != 0)
            printf ("qual: %#x, ", qual);
          printf ("type: ");
          show_fid ();
          printf (", name: ");
          show_fid ();
          printf ("\n");
          dump_rest ();
          break;

        case 0x54:
          printf ("Function: ");
          args = get_word ();
          max_args = get_word ();
          printf ("qual: %#x, %d argument%s (%d maximum), returns ",
                  qual, args, (args == 1 ? "" : "s"), max_args);
          show_fid ();
          printf (",\n    argument list: ");
          show_fid ();
          printf ("\n");
          dump_rest ();
          break;

        case 0x57:
          printf ("Stack: ");
          if (qual & 0x01)
            printf ("32-bit ");
          if (qual & 0x02)
            printf ("far ");
          if (qual & ~0x03)
            printf ("qual: %#x, ", qual);
          size = get_dword ();
          printf ("size: %#lx, name: ", size);
          show_fid ();
          printf ("\n");
          dump_rest ();
          break;

        case 0x5c:
          printf ("Bit string: ");
          if (qual & 1)
            printf ("varying, ");
          if (qual & 2)
            printf ("signed, ");
          if (qual & 4)
            printf ("word alignment, ");
          if (qual & 8)
            printf ("display as value, ");
          if (qual & 0x10)
            printf ("descriptor, ");
          if (qual & ~0x1f)
            printf ("qual: %#x, ", qual);
          printf ("offset: %d, size: ", get_byte ());
          show_fid ();
          printf ("\n");
          dump_rest ();
          break;

        case 0x5d:
          printf ("User defined type: ");
          if (qual != 0)
            printf ("qual: %#x, ", qual);
          printf ("type: ");
          show_fid ();
          printf (", name: ");
          show_fid ();
          printf ("\n");
          dump_rest ();
          break;

        case 0x6f:
          printf ("Subrange: ");
          if (qual != 0)
            printf ("qual: %#x, ", qual);
          printf ("type: ");
          show_fid ();
          printf (", start: ");
          show_fid ();
          printf (", end: ");
          show_fid ();
          printf (", name: ");
          show_fid ();
          printf ("\n");
          dump_rest ();
          break;

        case 0x72:
          printf ("Code Label: ");
          if (qual & 1)
            printf ("32-bit ");
          else
            printf ("16-bit ");
          if (qual & 2)
            printf ("far");
          else
            printf ("near");
          if (qual & ~3)
            printf (" (qual: %#x)", qual);
          printf ("\n");
          dump_rest ();
          break;

        case 0x78:
          printf ("Array: ");
          if (qual & 1)
            printf ("col-maj ");
          else
            printf ("row-maj ");
          if (qual & 2)
            printf ("(packed) ");
          if (qual & 4)
            printf ("desc provided) ");
          if (qual & ~7)
            printf ("(qual: %#x) ", qual);
          size = get_dword ();
          printf ("size: %#lx, bounds type: ", size);
          show_fid ();
          printf (", elements type: ");
          show_fid ();
          printf (", name: ");
          show_fid ();
          printf ("\n");
          dump_rest ();
          break;

        case 0x79:
          printf ("Structure: ");
          if (qual != 0)
            printf ("(qual: %#x) ", qual);
          size = get_dword ();
          fields = get_word ();
          printf ("size: %#lx, %d fields: ", size, fields);
          show_fid ();
          printf (", names: ");
          show_fid ();
          printf (", tag: ");
          show_fid ();
          printf ("\n");
          dump_rest ();
          break;

        case 0x7a:
          printf ("Pointer: ");
          show_fid ();
          if (rec_idx < rec_len)
            {
              printf (", name: ");
              show_fid ();
            }
          printf ("\n");
          dump_rest ();
          break;

        case 0x7b:
          printf ("Enum: ");
          if (qual != 0)
            printf ("(qual: %#x) ", qual);
          printf ("type: ");
          show_fid ();
          printf (", values: ");
          show_fid ();
          printf (", minimum: ");
          show_fid ();
          printf (", maximum: ");
          show_fid ();
          printf (", name: ");
          show_fid ();
          printf ("\n");
          dump_rest ();
          break;

        case 0x7f:
          printf ("List");
          switch (qual)
            {
            case 0x01:
              printf (" (field types)");
              break;
            case 0x02:
              printf (" (field names and offsets)");
              break;
            case 0x03:
              printf (" (enumeration)");
              break;
            case 0x04:
              printf (" (arguments)");
              break;
            }
          printf (":\n");
          while (rec_idx < rec_len)
            {
              printf ("    ");
              if (qual == 0x04)
                flags = get_byte ();
              show_fid ();
              switch (qual)
                {
                case 0x02:
                  printf (", offset: ");
                  show_fid ();
                  break;
                case 0x03:
                  printf (", value: ");
                  show_fid ();
                  break;
                case 0x04:
                  if (flags & 0x01)
                    printf (" by value");
                  else
                    printf (" by address");
                  if (flags & 0x02)
                    printf (", descriptor provided");
                  if (flags & ~0x03)
                    printf (" (flags: %#x)", flags);
                  break;
                }
              printf ("\n");
            }
          dump_rest ();
          break;

        default:
          printf ("unknown complex type: %#.2x\n", type);
          dump_rest ();
          break;
        }
      rec_idx = next;
      rec_len = total_rec_len;

      if (hex_dump)
        {
          int len = rec_idx - start;
          printf ("  hexdump:\n");
          rec_idx = start;
          dump_block (len, 3, 1);
        }
    }
}


static void list_ledata (void)
{
  int len;
  dword offset, seg;
  char *name;

  show_record ("LEDATA");

  seg = get_index ();
  offset = get_word_or_dword ();
  len = rec_len - rec_idx;

  printf (" ");
  show_seg (seg);
  printf (" offset: %#lx length: %#x\n", offset, len);
  name = lname_list[segment_list[seg-1].name-1];
  if (strcmp (name, "$$SYMBOLS") == 0 && list_debug
      && debug_info == debug_hll && hll_style >= 3)
    list_symbols ();
  else if (strcmp (name, "$$TYPES") == 0 && list_debug
           && debug_info == debug_hll && hll_style >= 3)
    list_types ();
  else
    dump_rest ();
}


static void list_block (int level)
{
  dword rep_count, block_count;
  int indent, len;

  indent = (level + 1) * 2;
  rep_count = get_word_or_dword ();
  block_count = get_word ();
  if (block_count == 0)
    {
      len = get_byte ();
      printf ("%*sRepeat count: %lu, data length: %d\n",
              indent, "", rep_count, len);
      dump_block (len, indent + 2, 0);
    }
  else
    {
      printf ("%*sRepeat count: %lu, block count: %lu\n",
              indent, "", rep_count, block_count);
      while (block_count > 0)
        {
          list_block (level + 1);
          --block_count;
        }
    }
}


static void list_lidata (void)
{
  dword offset, seg;

  show_record ("LIDATA");

  seg = get_index ();
  offset = get_word_or_dword ();

  printf (" ");
  show_seg (seg);
  printf (" offset: %#lx length: %#x\n", offset, rec_len - rec_idx);
  while (rec_len - rec_idx >= 4)
    list_block (0);
  dump_rest ();
}


static void list_target (int method)
{
  switch (method)
    {
    case 0:
      printf ("T0 (seg ");
      show_seg (get_index ());
      printf (")");
      break;
    case 1:
      printf ("T1 (group ");
      show_group (get_index ());
      printf (")");
      break;
    case 2:
      printf ("T2 (ext ");
      show_ext (get_index ());
      printf (")");
      break;
    default:
      printf ("T%d (index #%d)", method & 3, get_index ());
                break;
    }
}


static void list_frame (int method)
{
  switch (method)
    {
    case 0:
      printf ("F0 (seg ");
      show_seg (get_index ());
      printf (")");
      break;
    case 1:
      printf ("F1 (group ");
      show_group (get_index ());
      printf (")");
      break;
    case 2:
      printf ("F2 (ext");
      show_ext (get_index ());
      printf (")");
      break;
    case 4:
      printf ("F4 (cur seg)");
      break;
    case 5:
      printf ("F5 (target)");
      break;
    default:
      printf ("F%d", method);
      if (method <= 2)
        printf (" (index #%d)", get_index ());
      break;
    }
}


static void list_fixupp (void)
{
  int first, locat, method, offset, fix_data;

  show_record ("FIXUPP");
  printf (":\n");
  while (rec_idx < rec_len)
    {
      first = get_byte ();
      if (first & 0x80)
        {
          printf ("  FIXUP ");
          if (first & 0x40)
            printf ("seg-rel, ");
          else
            printf ("self-rel, ");
          locat = (first >> 2) & 0x0f;
          switch (locat)
            {
            case 0:
              printf ("LOW-8");
              break;
            case 1:
              printf ("OFFSET-16");
              break;
            case 2:
              printf ("SEL-16");
              break;
            case 3:
              printf ("FAR-16:16");
              break;
            case 4:
              printf ("HIGH-8");
              break;
            case 5:
              printf ("OFFSET-16(LR)");
              break;
            case 9:
              printf ("OFFSET-32");
              break;
            case 11:
              printf ("FAR-16:32");
              break;
            case 13:
              printf ("OFFSET-32(LR)");
              break;
            default:
              printf ("unknown location: %d", locat);
              break;
            }
          offset = get_byte ();
          offset |= (first & 3) << 8;
          printf (", offset: %#x, ", offset);
          fix_data = get_byte ();
          if (fix_data & 0x80)
            printf ("frame thread %d", (fix_data >> 4) & 3);
          else
            list_frame ((fix_data >> 4) & 7);
          if (fix_data & 0x08)
            printf (", target thread %d P=%d",
                    fix_data & 3, (fix_data >> 2) & 1);
          else
            {
              printf (", ");
              list_target (fix_data & 3);
            }
          if (!(fix_data & 0x04))
            printf (", disp: %#lx", get_word_or_dword ());
        }
      else
        {
          method = (first >> 2) & 7;
          printf ("  THREAD %d ", first & 3);
          if (first & 0x40)
            {
              list_frame (method);
              printf (", P=%d", (first >> 4) & 1);
            }
          else
            list_target (method & 3);
        }
      printf ("\n");
    }
}


static void list_hll_linnum (void)
{
  int group, segment;
  dword addr, line, stmt, lst_line, first_col, num_cols;
  word src_line, index;
  byte reserved;
  byte name[256];

  group = get_index ();
  segment = get_index ();
  printf (": group: %d, segment: ", group);
  show_seg (segment);
  if (!linnum_started)
    {
      printf (", ");
      line = get_word ();
      if (line != 0)
        printf ("must_be_zero: %ld, ", line);
      linnum_entry_type = get_byte ();
      switch (linnum_entry_type)
        {
        case 0x00:
          printf ("source file");
          break;
        case 0x01:
          printf ("listing file");
          break;
        case 0x02:
          printf ("source & listing file");
          break;
        case 0x03:
          printf ("file names table");
          break;
        case 0x04:
          printf ("path table");
          break;
        default:
          printf ("undefined entry type: %#x", linnum_entry_type);
          break;
        }
      reserved = get_byte ();
      printf (", reserved=%#x\n", reserved);
      line_count = get_word ();
      segment = get_word ();
      names_length = get_dword ();
      printf ("%d line%s, segment: %u, name table length: %#lx",
              line_count, (line_count == 1 ? "" : "s"), segment, names_length);
      linnum_started = TRUE;
      linnum_files = linnum_file_idx = 0;
    }
  printf ("\n");

  if (!linnum_files && (linnum_entry_type == 3 || linnum_entry_type == 4))
    {
      printf ("  No line numbers\n");
      line_count = 0;
    }
  else
    {
      while (rec_idx < rec_len && line_count != 0)
        {
          switch (linnum_entry_type)
            {
            case 0x00:
              src_line = get_word ();
              index = get_word ();
              addr = get_dword ();
              printf ("  Line %6u of file %u at 0x%.8lx", src_line, index, addr);
              break;
            case 0x01:
              lst_line = get_dword ();
              stmt = get_dword ();
              addr = get_dword ();
              printf ("  Line %6lu, statement %6lu at 0x%.8lx",
                      lst_line, stmt, addr);
              break;
            case 0x02:
              src_line = get_word ();
              index = get_word ();
              lst_line = get_dword ();
              stmt = get_dword ();
              addr = get_dword ();
              printf ("  Line %6u of file %u, ", src_line, index);
              printf ("listing line %6lu, statement %6lu at 0x%.8lx",
                      lst_line, stmt, addr);
              break;
            case 0x03:
            case 0x04:
              printf ("  No line numbers");
              break;
            default:
              printf ("  ????");
              break;
            }
          printf ("\n");
          --line_count;
        }
    }
  if (line_count == 0)
    linnum_started = FALSE;

  /* Path table comes next, ignored... */

  /* Note: this will fail if the name table is split */

  if (rec_idx < rec_len)
    {
      if (!linnum_files/*|| (linnum_entry_type != 3 && linnum_entry_type != 4)*/)
        {
          first_col = get_dword ();
          num_cols = get_dword ();
          linnum_files = get_dword ();
          printf ("  first column: %lu, columns: %lu, number of files: %d\n",
                  first_col, num_cols, linnum_files);
          linnum_file_idx = 0;
        }

      for (; linnum_file_idx < linnum_files && rec_idx < rec_len; ++linnum_file_idx)
        {
          printf ("  #%d: ", linnum_file_idx + 1);
          get_string (name);
          show_string (name);
          printf ("\n");
        }

      linnum_started = (linnum_file_idx < linnum_files);
    }
}


static void list_modend (FILE *f)
{
  int type, end_data;
  long pos;

  show_record ("MODEND");
  printf (": ");
  type = get_byte ();
  if (type & 0x80)
    printf ("main ");
  if (type & 0x40)
    {
      if (type & 0x01)
        printf ("rel-");
      printf ("start: ");
      end_data = get_byte ();
      if (end_data & 0x80)
        printf ("frame thread %d", (end_data >> 4) & 3);
      else
        list_frame ((end_data >> 4) & 7);
      if (end_data & 0x08)
        printf (", target thread %d P=%d",
                end_data & 3, (end_data >> 2) & 1);
      else
        {
          printf (", ");
          list_target (end_data & 3);
        }
      if (!(end_data & 0x04))   /* This bit must always be clear! */
        printf (", disp: %#lx", get_word_or_dword ());
    }
  printf ("\n");
  dump_rest ();
  if (page_size == 0)
    done = TRUE;
  else
    {
      pos = ftell (f);
      if (pos % page_size != 0)
        fseek (f, page_size * (1 + pos / page_size), SEEK_SET);
    }
}


static int list_libend (FILE *f)
{
  byte dblock[512];
  unsigned dblockno, offset, bn;
  int bucket, i, len;

  done = TRUE;
  printf ("LIBEND\n");
  fseek (f, dict_offset, SEEK_SET);
  for (dblockno = 0; dblockno < dict_blocks; ++dblockno)
    {
      printf ("Dictionary block %u: ", dblockno);
      if (fread (dblock, 512, 1, f) != 1)
        {
          putchar ('\n');
          return -1;
        }
      if (dblock[37] == 0xff)
        printf ("full\n");
      else
        printf ("free space at 0x%.3x\n", dblock[37] * 2);
      for (bucket = 0; bucket < 37; ++bucket)
        if (dblock[bucket] != 0)
          {
            offset = dblock[bucket] * 2;
            printf ("  Bucket %2u: offset=0x%.3x", bucket, offset);
            if (offset < 38)
              putchar ('\n');
            else
              {
                len = dblock[offset];
                ++offset;
                if (offset + len + 2 > 512)
                  printf (" (record extends beyond end of block)\n");
                else
                  {
                    printf (" name=\"");
                    for (i = 0; i < len; ++i)
                      show_char (dblock[offset++]);
                    bn = dblock[offset++];
                    bn |= dblock[offset++] << 8;
                    printf ("\" block=%u\n", bn);
                  }
              }
          }
    }
  return 0;
}


static void list_omf (const char *fname)
{
  FILE *f;
  struct omf_rec rec;

  f = fopen (fname, "rb");
  if (f == NULL)
    {
      perror (fname);
      exit (2);
    }
  lname_count = 0; lname_alloc = 0; lname_list = NULL;
  segment_count = 0; segment_alloc = 0; segment_list = NULL;
  group_count = 0; group_alloc = 0; group_list = NULL;
  ext_count = 0; ext_alloc = 0; ext_list = NULL;
  pub_count = 0;
  debug_type_index = 512; page_size = 0; done = FALSE;
  debug_info = debug_default; hll_style = 0; linnum_started = FALSE;
  do
    {
      int skip_hexdump = 0;
      if (show_addr || hex_dump)
        rec_pos = ftell (f);
      if (fread (&rec, sizeof (rec), 1, f) != 1)
        goto failure;
      rec_type = rec.rec_type;
      rec_len = rec.rec_len;
      rec_idx = 0;
      if (rec_len > sizeof (rec_buf))
        {
          fprintf (stderr, "%s: Record too long", fname);
          exit (2);
        }
      if (fread (rec_buf, rec_len, 1, f) != 1)
        goto failure;
      /*...check checksum...*/
      --rec_len;
      switch (rec_type)
        {
        case LIBHDR:
          list_libhdr ();
          break;
        case LIBEND:
          if (list_libend (f) != 0)
            goto failure;
          break;
        case THEADR:
          list_theadr ();
          break;
        case COMENT:
          list_coment ();
          break;
        case MODEND:
        case MODEND|REC32:
          list_modend (f);
          break;
        case EXTDEF:
          list_extdef ();
          break;
        case TYPDEF:
          show_record ("TYPDEF");
          printf ("\n");
          dump_rest ();
          break;
        case PUBDEF:
        case PUBDEF|REC32:
          list_pubdef ();
          break;
        case LINNUM:
        case LINNUM|REC32:
          show_record ("LINNUM");
          if (debug_info == debug_hll)
            list_hll_linnum ();
          else
            {
              printf ("\n");
              dump_rest ();
            }
          break;
        case LNAMES:
          list_lnames ();
          break;
        case SEGDEF:
        case SEGDEF|REC32:
          list_segdef ();
          break;
        case GRPDEF:
          list_grpdef ();
          break;
          break;
        case FIXUPP:
        case FIXUPP|REC32:
          list_fixupp ();
          break;
        case LEDATA:
        case LEDATA|REC32:
          list_ledata ();
          skip_hexdump = 1;
          break;
        case LIDATA:
        case LIDATA|REC32:
          list_lidata ();
          skip_hexdump = 1;
          break;
        case COMDEF:
          list_comdef ();
          break;
        case COMDAT:
        case COMDAT|REC32:
          show_record ("COMDAT");
          printf ("\n");
          dump_rest ();
          skip_hexdump = 1;
          break;
        case ALIAS:
          list_alias ();
          break;
        default:
          printf ("Unknown record type at %ld: %.2x\n",
                  ftell (f) - (sizeof (rec) + rec_len + 1), rec.rec_type);
          dump_rest ();
          skip_hexdump = 1;
          break;
        }

      /* hex dump? */
      if (hex_dump && !skip_hexdump)
        {
          printf (" hexdump: type=0x%02x  len=0x%03x (%d) crc=%02x\n",
                  rec.rec_type, rec.rec_len, rec.rec_len,
                  rec_buf[rec.rec_len-1]);
          rec_len = rec.rec_len - 1; /* skip crc */
          rec_idx = 0;
          dump_block (rec_len, 2, 1);
        }
    } while (!done);
  fclose (f);
  if (lname_list != NULL)
    free (lname_list);
  return;

failure:
  if (ferror (f))
    perror (fname);
  else
    fprintf (stderr, "%s: Unexpected end of file", fname);
  exit (2);
}


static void usage (void)
{
  fputs ("listomf " VERSION INNOTEK_VERSION " -- "
         "Copyright (c) 1993-1996 by Eberhard Mattes\n\n"
         "Usage: listomf [-a] [-d] <input_file>\n\n"
         "Options:\n"
         "  -a    Show addresses of records\n"
         "  -d    Don't interpret $$TYPES and $$SYMBOLS segments\n"
         "  -x    Do hex dump of it after the intepretation.\n",
         stderr);
  exit (1);
}


int main (int argc, char *argv[])
{
  int c;

  opterr = 0;
  while ((c = getopt (argc, argv, "adx")) != EOF)
    switch (c)
      {
      case 'a':
        show_addr = TRUE;
        break;
      case 'd':
        list_debug = FALSE;
        break;
      case 'x':
        hex_dump = TRUE;
        break;
      default:
        usage ();
      }

  if (argc - optind != 1)
    usage ();

#ifdef __EMX__
  if (_isterm (1))
    setvbuf (stdout, NULL, _IOLBF, BUFSIZ);
  else
    setvbuf (stdout, NULL, _IOFBF, BUFSIZ);
#endif /* __EMX__ */

  list_omf (argv[optind]);
  return 0;
}
