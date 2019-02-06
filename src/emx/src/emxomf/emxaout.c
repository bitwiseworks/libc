/* emxaout.c -- Convert OS/2-style OMF object files to GNU-style a.out
                object files
  Copyright (c) 1994-1998 Eberhard Mattes

This file is part of emxaout.

emxaout is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

emxaout is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with emxaout; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */


#pragma GCC diagnostic ignored "-Wunused-but-set-variable"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <getopt.h>
#include <sys/param.h>
#include <sys/emxload.h>
#include <sys/omflib.h>
#include <ar.h>
#include <a_out.h>

#define GROW_DEF_STATIC(NAME,TYPE) \
  static struct grow NAME##_grow; \
  static TYPE *NAME
#define GROW_SETUP(NAME,INCR) \
  (NAME = NULL, grow_init (&NAME##_grow, &NAME, sizeof (*(NAME)), INCR))
#define GROW_FREE(NAME) \
  grow_free (&NAME##_grow)
#define GROW_COUNT(NAME) \
  NAME##_grow.count
#define GROW_ADD(NAME) \
  (grow_by (&NAME##_grow, 1), NAME##_grow.count++)

/* Insert private header files. */

#include "defs.h"               /* Common definitions */
#include "grow.h"               /* Growing objects */

#define SEG_OTHER       0
#define SEG_CODE        1
#define SEG_DATA        2
#define SEG_BSS         3
#define SEG_SET         4
#define SEG_DEBUG       5

struct segment
{
  int name;
  dword size;
  byte *data;
  int type;
};

struct group
{
  int name;
};

struct pubdef
{
  char *name;
  dword offset;
  int group;
  int seg;
  int frame;
};

struct extdef
{
  char *name;
  int sym;
  dword size;                   /* Non-zero for COMDEF */
};

struct symbol
{
  char *name;
  struct nlist n;
};

struct thread
{
  int method;
  int index;
};


/* Prototypes for public functions. */

void error (const char *fmt, ...) NORETURN2 ATTR_PRINTF (1, 2);
void error2 (const char *fmt, ...) NORETURN2 ATTR_PRINTF (1, 2);
void *xmalloc (size_t n);
void *xrealloc (void *ptr, size_t n);
char *xstrdup (const char *s);


/* The name of the current input file. */
static const char *inp_fname;

/* The name of the current output file. */
static const char *out_fname = NULL;

/* The current input file. */
static FILE *inp_file;

/* The current output file. */
static FILE *out_file = NULL;

/* Assume that the .obj file contains leading underscores -- don't add
   extra underscores. */
static int underscores = FALSE;

static int rec_type;
static int rec_len;
static int rec_idx;
static byte rec_buf[MAX_REC_SIZE+8];

static int group_flat;

static int seg_code;
static int seg_data;
static int seg_bss;

static int cur_seg;
static dword cur_off;

static struct thread frame_threads[4];
static struct thread target_threads[4];

GROW_DEF_STATIC (lnames, char *);
GROW_DEF_STATIC (groups, struct group);
GROW_DEF_STATIC (segments, struct segment);
GROW_DEF_STATIC (pubdefs, struct pubdef);
GROW_DEF_STATIC (extdefs, struct extdef);
GROW_DEF_STATIC (symbols, struct symbol);
GROW_DEF_STATIC (trelocs, struct relocation_info);
GROW_DEF_STATIC (drelocs, struct relocation_info);


/* If there is an output file, close and delete it. */

static void cleanup (void)
{
  if (out_file != NULL)
    {
      fclose (out_file);
      remove (out_fname);
    }
}


/* Display an error message on stderr, delete the output file and
   quit.  This function is invoked like printf(): FMT is a
   printf-style format string, all other arguments are formatted
   according to FMT.  The message should not end with a newline. The
   exit code is 2. */

void error (const char *fmt, ...)
{
  va_list arg_ptr;

  va_start (arg_ptr, fmt);
  fprintf (stderr, "emxaout: ");
  vfprintf (stderr, fmt, arg_ptr);
  fputc ('\n', stderr);
  cleanup ();
  exit (2);
}


/* Display an error message for the current input file on stderr,
   delete the output file and quit.  This function is invoked like
   printf(): FMT is a printf-style format string, all other arguments
   are formatted according to FMT.  The message should not end with a
   newline. The exit code is 2. */

void error2 (const char *fmt, ...)
{
  va_list arg_ptr;

  va_start (arg_ptr, fmt);
  fprintf (stderr, "%s[%ld+%d]: ", inp_fname,
           ftell (inp_file) - (sizeof (struct omf_rec) + rec_len + 1),
           rec_idx);
  vfprintf (stderr, fmt, arg_ptr);
  fputc ('\n', stderr);
  cleanup ();
  exit (2);
}


/* Allocate N bytes of memory.  Quit on failure.  This function is
   used like malloc(), but we don't have to check the return value. */

void *xmalloc (size_t n)
{
  void *p;

  p = malloc (n);
  if (p == NULL && n != 0)
    error ("Out of memory");
  return p;
}


/* Change the allocation of PTR to N bytes.  Quit on failure.  This
   function is used like realloc(), but we don't have to check the
   return value. */

void *xrealloc (void *ptr, size_t n)
{
  void *p;

  p = realloc (ptr, n);
  if (p == NULL)
    error ("Out of memory");
  return p;
}


/* Create a duplicate of the string S on the heap.  Quit on failure.
   This function is used like strdup(), but we don't have to check the
   return value. */

char *xstrdup (const char *s)
{
  char *p;

  p = xmalloc (strlen (s) + 1);
  strcpy (p, s);
  return p;
}

#if 0 /* not used */
/* Display a warning message on stderr (and do not quit).  This
   function is invoked like printf(): FMT is a printf-style format
   string, all other arguments are formatted according to FMT. */

static void warning (const char *fmt, ...)
{
  va_list arg_ptr;

  va_start (arg_ptr, fmt);
  fprintf (stderr, "emxaout warning: ");
  vfprintf (stderr, fmt, arg_ptr);
  fputc ('\n', stderr);
}
#endif

/* Display some hints on using this program, then quit. */

static void usage (void)
{
  puts ("emxaout " VERSION VERSION_DETAILS
        "\nCopyright (c) 1994-1996 by Eberhard Mattes\n" VERSION_COPYRIGHT "\n");
  puts ("Usage:");
  puts ("  emxaout [-u] [-o <output_file>] <input_file>");
  puts ("\nOptions:");
  puts ("  -u                 Don't add leading underscores");
  puts ("  -o <output_file>   Write output to <output_file>");
  exit (1);
}


/* Create the output file. */

static void open_output (void)
{
  out_file = fopen (out_fname, "wb");
  if (out_file == NULL)
    error ("Cannot create output file `%s'", out_fname);
}


/* Close the output file.  Display an error message and quit on
   failure. */

static void close_output (void)
{
  if (fflush (out_file) != 0 || fclose (out_file) != 0)
    {
      out_file = NULL;
      error ("Write error on output file `%s'", out_fname);
    }
  out_file = NULL;
}


/* Define or build the name of the output file.  If DST_FNAME is not
   NULL, use it as name of the output file.  Otherwise, build the name
   from INP_FNAME (the input file name) and EXT (the extension). */

static void make_out_fname (const char *dst_fname, const char *inp_fname,
                            const char *ext)
{
  static char tmp[MAXPATHLEN+1];

  if (dst_fname == NULL)
    {
      if (strlen (inp_fname) + strlen (ext) > MAXPATHLEN)
        error ("File name `%s' too long", inp_fname);
      strcpy (tmp, inp_fname);
      _remext (tmp);
      strcat (tmp, ext);
      out_fname = tmp;
    }
  else
    out_fname = dst_fname;
}


static void get_mem (void *dst, int len)
{
  if (rec_idx + len > rec_len)
    error2 ("String beyond end of record");
  memcpy (dst, rec_buf + rec_idx, len);
  rec_idx += len;
}


static void get_string (byte *dst)
{
  int len;

  if (rec_idx >= rec_len)
    error2 ("String beyond end of record");
  len = rec_buf[rec_idx++];
  get_mem (dst, len);
  dst[len] = 0;
}


static int get_index (void)
{
  int result;

  if (rec_idx >= rec_len)
    error2 ("Index beyond end of record");
  result = rec_buf[rec_idx++];
  if (result & 0x80)
    {
      if (rec_idx >= rec_len)
        error2 ("Index beyond end of record");
      result = ((result & 0x7f) << 8) | rec_buf[rec_idx++];
    }
  return result;
}


static word get_dword (void)
{
  dword result;

  if (rec_idx + 4 > rec_len)
    error2 ("Dword beyond end of record");
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
    error2 ("Word beyond end of record");
  result = rec_buf[rec_idx++];
  result |= rec_buf[rec_idx++] << 8;
  return result;
}


static word get_byte (void)
{
  if (rec_idx + 1 > rec_len)
    error2 ("Byte beyond end of record");
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
      error2 ("Unknown COMDAT length prefix");
    }
}


static void init_module (void)
{
  int i;

  GROW_SETUP (lnames, 8);
  GROW_SETUP (groups, 8);
  GROW_SETUP (segments, 8);
  GROW_SETUP (pubdefs, 16);
  GROW_SETUP (extdefs, 16);
  GROW_SETUP (symbols, 16);
  GROW_SETUP (trelocs, 32);
  GROW_SETUP (drelocs, 32);

  group_flat = -1;
  seg_code = -1; seg_data = -1; seg_bss = -1;
  cur_seg = -1;

  for (i = 0; i < 4; ++i)
    frame_threads[i].method = target_threads[i].method = -1;
}


static void term_module (void)
{
  int i;

  for (i = 0; i < GROW_COUNT (lnames); ++i)
    free (lnames[i]);
  GROW_FREE (lnames);

  GROW_FREE (groups);

  for (i = 0; i < GROW_COUNT (segments); ++i)
    if (segments[i].data != NULL)
      free (segments[i].data);
  GROW_FREE (segments);

  for (i = 0; i < GROW_COUNT (pubdefs); ++i)
    free (pubdefs[i].name);
  GROW_FREE (pubdefs);

  for (i = 0; i < GROW_COUNT (extdefs); ++i)
    free (extdefs[i].name);
  GROW_FREE (extdefs);

  for (i = 0; i < GROW_COUNT (symbols); ++i)
    if (symbols[i].name != NULL)
      free (symbols[i].name);
  GROW_FREE (symbols);

  GROW_FREE (trelocs);
  GROW_FREE (drelocs);
}


/* Convert an LNAMES record.

   LNAMES record:
   �
   �1   String length n
   �n   Name
   �

   Name indices are assigned sequentially. */

static void conv_lnames (void)
{
  byte string[256];
  int i;

  while (rec_idx < rec_len)
    {
      get_string (string);
      i = GROW_ADD (lnames);
      lnames[i] = xstrdup ((char *)string);
    }
}


/* Convert a GRPDEF record.

   GRPDEF record:
    1/2 Group name index
   �
   �1   0xff (use segment index)
   �1/2 Segment index
   �

   Group indices are assigned sequentially. */

static void conv_grpdef (void)
{
  int i, name, type, seg;
  const char *group_name;

  name = get_index ();
  i = GROW_ADD (groups);
  groups[i].name = name;

  if (name < 1 || name > GROW_COUNT (lnames))
    error2 ("GRPDEF: name index out of range");
  group_name = lnames[name - 1];

  if (strcmp (group_name, "FLAT") == 0)
    group_flat = i;

  while (rec_idx < rec_len)
    {
      type = get_byte ();
      if (type != 0xff)
        error2 ("GRPDEF: unknown type");
      seg = get_index ();
    }
}


static int is_set_seg (const char *name)
{
  return (name[0] == 'S' && name[1] == 'E' && name[2] == 'T'
          && (name[3] >= '1' && name[3] <= '3')
          && name[4] == '_' && name[5] == '_' && name[6] == '_'
          && (name[7] == 'C' || name[7] == 'D')
          && strcmp (name+8, "TOR_LIST__") == 0);
}


/* Convert a SEGDEF record.

   SEGDEF record:
    1   Segment attributes
    0/2 Frame number (present only if A=000)
    0/1 Offset (present only if A=000)
    2/4 Segment length (4 bytes for 32-bit SEGDEF record)
    1/2 Segment name index
    1/2 Segment class index
    1/2 Overlay name index

    The segment attributes byte contains the following fields:

    A (bits 5-7)        Alignment (101=relocatable, 32-bit alignment)
    C (bits 2-4)        Combination (010=PUBLIC, 101=STACK)
    B (bit 1)           Big (segment length is 64KB)
    P (bit 0)           USE32 */

static void conv_segdef (void)
{
  int i, type;
  int attributes, alignment, combination, frame, offset, name, class, overlay;
  dword length;
  const char *seg_name, *class_name;

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

  if (class < 1 || class > GROW_COUNT (lnames))
    error2 ("SEGDEF: class index out of range");
  class_name = lnames[class - 1];

  if (name < 1 || name > GROW_COUNT (lnames))
    error2 ("SEGDEF: name index out of range");
  seg_name = lnames[name - 1];

  type = SEG_OTHER;
  if (strcmp (class_name, "CODE") == 0)
    {
      if (is_set_seg (seg_name))
        type = SEG_SET;
      else
        type = SEG_CODE;
    }
  else if (strcmp (class_name, "DATA") == 0)
    type = SEG_DATA;
  else if (strcmp (class_name, "BSS") == 0)
    type = SEG_BSS;
  else if (strcmp (class_name, "DEBSYM") == 0)
    type = SEG_DEBUG;
  else if (strcmp (class_name, "DEBTYP") == 0)
    type = SEG_DEBUG;

  if (type != SEG_OTHER)
    for (i = 0; i < GROW_COUNT (segments); ++i)
      if (segments[i].type == type)
        switch (type)
          {
          case SEG_CODE:
            error2 ("Multiple code segments");
          case SEG_DATA:
            error2 ("Multiple data segments");
          case SEG_BSS:
            error2 ("Multiple bss segments");
          case SEG_SET:
          case SEG_DEBUG:
            break;
          }
  i = GROW_ADD (segments);
  segments[i].name = name;
  segments[i].size = length;
  segments[i].data = xmalloc (length);
  segments[i].type = type;
  memset (segments[i].data, 0, length);
  switch (type)
    {
    case SEG_CODE:
      seg_code = i;
      break;
    case SEG_DATA:
      seg_data = i;
      break;
    case SEG_BSS:
      seg_bss = i;
      break;
    }
}


/* Convert a PUBDEF record.

   PUBDEF record:
    1-2 Base group
    1-2 Base segment
    0/2 Base frame
   �
   �1   String length n
   �n   Public name
   �2/4 Public offset (4 bytes for 32-bit PUBDEF record)
   �1-2 Type index
   �

   The base frame field is present only if the base segment field is
   0. */

static void conv_pubdef (void)
{
  int i, type, group, seg, frame;
  dword offset;
  byte name[1+256];

  group = get_index ();
  seg = get_index ();
  if (seg == 0)
    frame = get_word ();
  else
    frame = 0;

  while (rec_idx < rec_len)
    {
      name[0] = '_';
      get_string (name+1);
      offset = get_word_or_dword ();
      type = get_index ();
      i = GROW_ADD (pubdefs);
      pubdefs[i].name   = xstrdup ((char *)(underscores ? name + 1 : name));
      pubdefs[i].group  = group;
      pubdefs[i].seg    = seg;
      pubdefs[i].frame  = frame;
      pubdefs[i].offset = offset;
    }
}


/* Convert an EXTDEF record.

   EXTDEF record:
   �
   �1   String length n
   �n   External name
   �1-2 Type index
   �

   Symbol indices are assigned sequentially by EXTDEF and COMDEF. */

static void conv_extdef (void)
{
  int i, type;
  byte name[1+256];

  while (rec_idx < rec_len)
    {
      name[0] = '_';
      get_string (name+1);
      type = get_index ();
      i = GROW_ADD (extdefs);
      extdefs[i].name = xstrdup ((char *)(underscores ? name + 1 : name));
      extdefs[i].sym  = 0;
      extdefs[i].size = 0;      /* EXTDEF */
    }
}


/* Convert an COMDEF record.

   COMDEF record:
   �
   �1   String length n
   �n   Communal name
   �1-2 Type index
   �1   Data type (0x61: FAR data, 0x62: NEAR data)
   �1-5 Communal length
   �

   The length is encoded in 1 to 5 bytes, depending on the value:

   0 through 0x7f       1 byte, containing the value
   0 through 0xffff     0x81, followed by 16-bit word
   0 through 0xffffff   0x84, followed by 24-bit word
   0 through 0xffffffff 0x88, followed by 32-bit word

   Symbol indices are assigned sequentially by EXTDEF and COMDEF. */

static void conv_comdef (void)
{
  int i, type;
  word data_type;
  dword comm_count, comm_len;
  byte name[1+256];

  while (rec_idx < rec_len)
    {
      name[0] = '_';
      get_string (name+1);
      type = get_index ();
      data_type = get_byte ();
      switch (data_type)
        {
        case 0x61:
          comm_count = get_commlen ();
          comm_len = get_commlen ();
          break;
        case 0x62:
          comm_count = 1;
          comm_len = get_commlen ();
          break;
        default:
          error2 ("COMDEF: unknown data type");
        }
      if (comm_count == 0 || comm_len == 0)
        error2 ("COMDEF: size is zero");
      i = GROW_ADD (extdefs);
      extdefs[i].name = xstrdup ((char *)(underscores ? name + 1 : name));
      extdefs[i].sym  = 0;
      extdefs[i].size = comm_count * comm_len;
    }
}


/* Convert a LEDATA record.

   LEDATA record:
    1-2 Segment index
    2/4 Enumerated data offset (4 bytes for 32-bit LEDATA record)
    n   Data bytes (n is derived from record length) */

static void conv_ledata (void)
{
  int len;
  dword offset, seg;

  seg = get_index ();
  offset = get_word_or_dword ();
  len = rec_len - rec_idx;

  if (seg < 1 || seg > GROW_COUNT (segments))
    error2 ("LEDATA: segment index out of range");

  cur_seg = -1;
  if (segments[seg-1].data != NULL)
    {
      cur_seg = seg - 1;
      cur_off = offset;
      if (segments[seg-1].type != SEG_DEBUG)
        {
          if (offset + len > segments[seg-1].size)
            error2 ("LEDATA: data beyond end of segment");
          else
            get_mem (segments[seg-1].data + offset, len);
        }
    }
}


/* Convert an iterated data block, for LIDATA.

   Iterated data block:
    2/4 Repeat count
    2   Block count (0: data bytes, otherwise nested block)
    n   nested block or data bytes */

static void conv_block (dword *poffset)
{
  dword rep_count, block_count, i, j;
  int len, saved_idx;

  rep_count = get_word_or_dword ();
  block_count = get_word ();
  if (rep_count == 0)
    error2 ("LIDATA: repeat count is zero");
  if (block_count == 0)
    {
      len = get_byte ();
      if (*poffset + len * rep_count > segments[cur_seg].size)
        error2 ("LIDATA: data beyond end of segment");
      get_mem (segments[cur_seg].data + *poffset, len);
      *poffset += len;
      for (i = 1; i < rep_count; ++i)
        {
          memcpy (segments[cur_seg].data + *poffset,
                  segments[cur_seg].data + *poffset - len, len);
          *poffset += len;
        }
    }
  else
    {
      saved_idx = rec_idx;
      for (i = 0; i < rep_count; ++i)
        {
          rec_idx = saved_idx;
          for (j = 0; j < block_count; ++j)
            conv_block (poffset);
        }
    }
}


/* Convert a LIDATA record.

   LIDATA record:
    1-2 Segment index
    2/4 Iterated data offset (4 bytes for 32-bit LIDATA record)
    n   Data blocks */

static void conv_lidata (void)
{
  dword offset, seg;

  seg = get_index ();
  offset = get_word_or_dword ();
  if (seg < 1 || seg > GROW_COUNT (segments))
    error2 ("LIDATA: segment index out of range");

  if (segments[seg-1].data != NULL)
    {
      cur_seg = seg - 1;
      cur_off = offset;
      if (segments[cur_seg].type != SEG_DEBUG)
        {
          while (rec_len - rec_idx >= 4)
            conv_block (&offset);
        }
    }
  cur_seg = -1;                 /* FIXUPP for LIDATA not supported */
}


/* Convert a FIXUPP record.  This is the single interesting function
   of this program.

   FIXUPP record:
   �
   �?   THREAD subrecord or FIXUP subrecord
   �

   THREAD subrecord:
    1   Flags
    0-2 Index (present only for FRAME methods F0, F1 and F2)

   The flags byte contains the following fields:

   0 (bit 7)            always 0 to indicate THREAD subrecord
   D (bit 6)            0=target thread, 1=frame thread
   0 (bit 5)            reserved
   Method (bits 2-4)    method (T0 through T6 and F0 through F6)
   Thred (bits 0-1)     thread number

   FIXUP subrecord:
    2   Locat
    0-1 Fix data
    0-2 Frame datum
    0-2 Target datum
    2/4 target displacement (4 bytes for 32-bit FIXUPP record)

    The first locat byte contains the following fields:

    1 (bit 7)           always 1 to indicate FIXUP subrecord
    M (bit 6)           1=segment-relative fixup, 0=self-relative fixup
    Location (bit 2-5)  Type of location to fix up:
                          0010=16-bit selector
                          0011=32-bit long pointer (16:16)
                          1001=32-bit offset
    Offset (bits 0-1)   Most significant bits of offset into LEDATA record

    The second locat byte contains the least significant bits of the
    offset into the LEDATA record.

    The Fix data byte contains the following fields:

    F (bit 7)           1=frame thread, 0=methods F0 through F5
    Frame (bits 4-6)    frame thread number (F=1) or frame method (F=0)
    T (bit 3)           1=target thread, 1=methods
    P (bit 2)           Bit 2 of target method
    Targt (bits 0-1)    target thread number (T=1) or target method (T=0) */

static void conv_fixupp (void)
{
  int i, first, locat, offset, fix_data, thred;
  int frame_method, frame_index, target_method, target_index;
  dword disp;
  struct relocation_info r;
  struct thread th;

  while (rec_idx < rec_len)
    {
      first = get_byte ();
      if (first & 0x80)
        {
          /* FIXUP subrecord */
          offset = get_byte ();
          offset |= (first & 3) << 8;
          fix_data = get_byte ();
          if (first & 0x40)
            r.r_pcrel = 0;
          else
            r.r_pcrel = 1;
          r.r_address = offset + cur_off;
          r.r_pad = 0;
          locat = (first >> 2) & 0x0f;
          if (fix_data & 0x80)
            {
              thred = (fix_data >> 4) & 3;
              frame_method = frame_threads[thred].method;
              frame_index = frame_threads[thred].index;
            }
          else
            {
              frame_method = (fix_data >> 4) & 7;
              frame_index = 0;
              if (frame_method <= 2)
                frame_index = get_index ();
            }
          if (fix_data & 0x08)
            {
              thred = fix_data & 3;
              target_method = target_threads[thred].method;
              target_index = target_threads[thred].index;
            }
          else
            {
              target_method = fix_data & 3;
              target_index = get_index ();
            }
          disp = 0;
          if (!(fix_data & 0x04))
            disp = get_word_or_dword ();

          if (cur_seg < 0)
            error2 ("FIXUPP: not preceded by LEDATA or COMDAT");

          /* Ignore all fixups for other segments, such as $$SYMBOLS
             (which may have SEL-16 fixups). */

          if (cur_seg == seg_code || cur_seg == seg_data)
            {
              switch (locat)
                {
                case 0:
                  error2 ("FIXUPP: LOW-8 fixup not supported");
                case 1:
                  error2 ("FIXUPP: OFFSET-16 fixup not supported");
                case 2:
                  error2 ("FIXUPP: SEL-16 fixup not supported");
                case 3:
                  error2 ("FIXUPP: FAR-16:16 fixup not supported");
                case 4:
                  error2 ("FIXUPP: HIGH-8 fixup not supported");
                case 5:
                  error2 ("FIXUPP: OFFSET-16(LR) fixup not supported");
                case 9:
                  r.r_length = 2;
                  break;
                case 11:
                  error2 ("FIXUPP: FAR-16:32 fixup not supported");
                case 13:
                  error2 ("FIXUPP: OFFSET-32(LR) fixup not supported");
                default:
                  error2 ("FIXUPP: unknown location");
                }

              switch (target_method)
                {
                case 0:         /* T0: SEGDEF */
                  r.r_extern = 0;
                  if (target_index == seg_code + 1)
                    r.r_symbolnum = N_TEXT;
                  else if (target_index == seg_data + 1)
                    {
                      r.r_symbolnum = N_DATA;
                      disp += (seg_code >= 0 ? segments[seg_code].size : 0);
                    }
                  else if (target_index == seg_bss + 1)
                    {
                      r.r_symbolnum = N_BSS;
                      disp += (seg_code >= 0 ? segments[seg_code].size : 0);
                      disp += (seg_data >= 0 ? segments[seg_data].size : 0);
                    }
                  else if (target_index < 1
                           || target_index > GROW_COUNT (segments))
                    error2 ("FIXUPP: invalid segment index");
                  else
                    error2 ("FIXUPP: Target segment %s not supported",
                            lnames[segments[target_index-1].name-1]);
                  break;
                case 2:         /* T2: EXTDEF */
                  r.r_extern = 1;
                  if (target_index < 1 || target_index > GROW_COUNT (extdefs))
                    error2 ("FIXUPP: EXTDEF index out of range");
                  r.r_symbolnum = target_index - 1;
                  break;
                default:
                  error2 ("FIXUPP: TARGET method %d not supported",
                          target_method);
                }

              switch (frame_method)
                {
                case 1:         /* F1: GRPDEF */
                  if (frame_index != group_flat + 1)
                    error2 ("FIXUPP: only group FLAT is supported "
                            "for method F1");
                  break;
                default:
                  error2 ("FIXUPP: FRAME method %d not supported",
                          frame_method);
                }

              if (cur_seg == seg_code)
                {
                  i = GROW_ADD (trelocs);
                  trelocs[i] = r;
                }
              else if (cur_seg == seg_data)
                {
                  i = GROW_ADD (drelocs);
                  drelocs[i] = r;
                }
              else
                abort ();

              if (r.r_address + 4 > segments[cur_seg].size)
                error2 ("FIXUPP: fixup beyond end of segment");
              *((dword *)(segments[cur_seg].data + r.r_address))
                += r.r_pcrel ? disp - (r.r_address + 4) : disp;
            }
        }
      else
        {
          /* THREAD subrecord */
          thred = first & 3;
          if (first & 0x40)
            {
              th.method = (first >> 2) & 7;
              th.index = 0;
              if (th.method <= 2)
                th.index = get_index ();
              frame_threads[thred] = th;
            }
          else
            {
              th.method = (first >> 2) & 3;
              th.index = get_index ();
              target_threads[thred] = th;
            }
        }
    }
}


/* Build the symbol table. */

static void make_symtab (void)
{
  int i, j;
  struct nlist n;
  char skip;

  /* Important: Write extdefs first, to be able to use EXTDEF indices
     as symbol numbers. */

  for (i = 0; i < GROW_COUNT (extdefs); ++i)
    {
      n.n_type = N_EXT;
      n.n_un.n_strx = 0;
      n.n_value = extdefs[i].size;
      n.n_other = 0;
      n.n_desc = 0;
      j = GROW_ADD (symbols);
      symbols[j].name = xstrdup (extdefs[i].name);
      symbols[j].n = n;
      extdefs[i].sym = j;
    }
  for (i = 0; i < GROW_COUNT (pubdefs); ++i)
    {
      skip = FALSE;
      if (pubdefs[i].group != group_flat + 1)
        skip = TRUE;
      else if (pubdefs[i].seg == seg_code + 1)
        n.n_type = N_TEXT | N_EXT;
      else if (pubdefs[i].seg == seg_data + 1)
        n.n_type = N_DATA | N_EXT;
      else
        skip = TRUE;
      if (!skip)
        {
          n.n_un.n_strx = 0;
          n.n_value = pubdefs[i].offset;
          n.n_other = 0;
          n.n_desc = 0;
          j = GROW_ADD (symbols);
          symbols[j].name = xstrdup (pubdefs[i].name);
          symbols[j].n = n;
        }
    }
}


/* Write the symbol table. */

static void write_symtab (void)
{
  int i;
  dword str_size;

  str_size = sizeof (dword);
  for (i = 0; i < GROW_COUNT (symbols); ++i)
    {
      if (symbols[i].name == NULL)
        symbols[i].n.n_un.n_strx = 0;
      else
        {
          symbols[i].n.n_un.n_strx = str_size;
          str_size += strlen (symbols[i].name) + 1;
        }
      fwrite (&symbols[i].n, sizeof (struct nlist), 1, out_file);
    }
  fwrite (&str_size, sizeof (dword), 1, out_file);
  for (i = 0; i < GROW_COUNT (symbols); ++i)
    if (symbols[i].name != NULL)
      fwrite (symbols[i].name, strlen (symbols[i].name) + 1, 1, out_file);
}


/* Convert a MODEND record. */

static void conv_modend (void)
{
  struct exec h;
  int i;

  make_symtab ();
  N_SET_MAGIC (h, OMAGIC);
  N_SET_MACHTYPE (h, 0);
  N_SET_FLAGS (h, 0);
  h.a_text = (seg_code >= 0 ? segments[seg_code].size : 0);
  h.a_data = (seg_data >= 0 ? segments[seg_data].size : 0);
  h.a_bss =  (seg_bss  >= 0 ? segments[seg_bss ].size : 0);
  h.a_syms = GROW_COUNT (symbols) * sizeof (struct nlist);
  h.a_entry = 0;
  h.a_trsize = GROW_COUNT (trelocs) * sizeof (struct relocation_info);
  h.a_drsize = GROW_COUNT (drelocs) * sizeof (struct relocation_info);
  fwrite (&h, sizeof (h), 1, out_file);
  if (seg_code >= 0)
    fwrite (segments[seg_code].data, segments[seg_code].size, 1, out_file);
  if (seg_data >= 0)
    fwrite (segments[seg_data].data, segments[seg_data].size, 1, out_file);
  for (i = 0; i < GROW_COUNT (trelocs); ++i)
    fwrite (&trelocs[i], sizeof (struct relocation_info), 1, out_file);
  for (i = 0; i < GROW_COUNT (drelocs); ++i)
    fwrite (&drelocs[i], sizeof (struct relocation_info), 1, out_file);
  write_symtab ();
}


static void omf_to_o (void)
{
  struct omf_rec rec;

  do
    {
      if (fread (&rec, sizeof (rec), 1, inp_file) != 1)
        goto failure;
      rec_type = rec.rec_type;
      rec_len = rec.rec_len;
      rec_idx = 0;
      if (rec_len > sizeof (rec_buf))
        {
          rec_len = -1;         /* See error2() */
          error2 ("Record too long");
        }
      if (fread (rec_buf, rec_len, 1, inp_file) != 1)
        goto failure;
      /*...check checksum...*/
      --rec_len;
      switch (rec_type)
        {
        case THEADR:
          break;

        case COMENT:
          break;

        case MODEND:
        case MODEND|REC32:
          conv_modend ();
          break;

        case EXTDEF:
          conv_extdef ();
          break;

        case TYPDEF:
          break;

        case PUBDEF:
        case PUBDEF|REC32:
          conv_pubdef ();
          break;

        case LINNUM:
        case LINNUM|REC32:
          break;

        case LNAMES:
          conv_lnames ();
          break;

        case SEGDEF:
        case SEGDEF|REC32:
          conv_segdef ();
          break;

        case GRPDEF:
          conv_grpdef ();
          break;

        case FIXUPP:
        case FIXUPP|REC32:
          conv_fixupp ();
          break;

        case LEDATA:
        case LEDATA|REC32:
          conv_ledata ();
          break;

        case LIDATA:
        case LIDATA|REC32:
          conv_lidata ();
          break;

        case COMDEF:
          conv_comdef ();
          break;

        case COMDAT:
        case COMDAT|REC32:
          break;

        default:
          error2 ("Unknown record type: %.2x\n", rec.rec_type);
        }
    } while (rec_type != MODEND && rec_type != (MODEND|REC32));
  return;

failure:
  if (ferror (inp_file))
    error ("%s: %s", inp_fname, strerror (errno));
  else
    error ("%s: Unexpected end of file", inp_fname);
}


/* Convert the OMF file SRC_FNAME to an a.out file named DST_FNAME.
   If DST_FNAME is NULL, the output file name is derived from
   SRC_FNAME. */

static void convert (const char *src_fname, const char *dst_fname)
{
  init_module ();

  inp_fname = src_fname;
  inp_file = fopen (inp_fname, "rb");
  if (inp_file == NULL)
    error ("Cannot open input file `%s'", inp_fname);

  make_out_fname (dst_fname, inp_fname, ".o");
  open_output ();
  omf_to_o ();
  close_output ();
  fclose (inp_file);

  term_module ();
}


/* Main function of emxaout.  Parse the command line and perform the
   requested actions. */

int main (int argc, char *argv[])
{
  int c, i;
  char *opt_o;

  /* Expand response files (@filename) and wildcard (*.o) on the
     command line. */

  _response (&argc, &argv);
  _wildcard (&argc, &argv);

  /* Set default values of some options. */

  opt_o = NULL;
  opterr = FALSE;

  /* Parse the command line options. */

  while ((c = getopt (argc, argv, "uo:")) != EOF)
    switch (c)
      {
      case 'o':
        if (opt_o != NULL)
          usage ();
        opt_o = optarg;
        break;
      case 'u':
        underscores = TRUE;
        break;
      default:
        usage ();
      }

  /* Check for non-option arguments. */

  if (argc - optind == 0)
    usage ();

  if (opt_o != NULL)
    {

      /* If the -o option is used, there must be exactly one input
         file name. */

      if (argc - optind != 1)
        usage ();
      convert (argv[optind], opt_o);
    }
  else
    {

      /* The -o option is not used. Convert all the files named on the
         command line. */

      for (i = optind; i < argc; ++i)
        convert (argv[i], NULL);
    }

  return 0;
}
