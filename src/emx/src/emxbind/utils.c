/* utils.c -- Utility functions
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
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <io.h>
#include "defs.h"
#include "emxbind.h"


static void out_of_mem (void) NORETURN2;


/* Print an error message and abort.  This function is called like
   printf() but never returns.  The message will be prefixed with
   "emxbind: " and a newline will be added at the end. */

void error (const char *fmt, ...)
{
  va_list arg_ptr;

  va_start (arg_ptr, fmt);
  fprintf (stderr, "emxbind: ");
  vfprintf (stderr, fmt, arg_ptr);
  fputc ('\n', stderr);
  exit (2);
}


/* Print an warning message. This function is called like  printf().
   The message will be prefixed with  "emxbind: warning: " and a
   newline will be added at the end. */

void warning (const char *fmt, ...)
{
  va_list arg_ptr;

  va_start (arg_ptr, fmt);
  fprintf (stderr, "emxbind: warning: ");
  vfprintf (stderr, fmt, arg_ptr);
  fputc ('\n', stderr);
}


/* Print an out of memory message and abort. */

static void out_of_mem (void)
{
  error ("out of memory");
}


/* Read SIZE bytes from the file F to the buffer DST.  If there aren't
   SIZE bytes at the current location of the read/write pointer, an
   error message will be displayed and the program will be
   terminated. */

void my_read (void *dst, size_t size, struct file *f)
{
  if (fread (dst, 1, size, f->f) != size)
    error ("cannot read `%s'", f->name);
}


/* Write SIZE bytes from SRC to the file F. */

void my_write (const void *src, size_t size, struct file *f)
{
  if (fwrite (src, 1, size, f->f) != size)
    error ("cannot write `%s'", f->name);
}


/* Open the file named NAME and let F reference that file.  OMODE is
   the open mode.  The file is always opened in binary mode. */

void my_open (struct file *f, const char *name, enum open_mode omode)
{
  const char *ms;

  f->del = FALSE;
  switch (omode)
    {
    case open_read:
      ms = "rb";
      break;
    case open_read_write:
      ms = "r+b";
      break;
    case create_write:
      ms = "wb";
      f->del = TRUE;
      break;
    default:
      ms = NULL;
      break;
    }
  f->name = name;
  f->f = fopen (name, ms);
  if (f->f == NULL)
    error ("cannot open `%s'", name);
  f->ok = TRUE;
}


/* Close the file F if it is open. */

void my_close (struct file *f)
{
  if (f->ok)
    {
      f->ok = FALSE;
      if (fflush (f->f) != 0)
        error ("cannot write `%s'", f->name);
      if (fclose (f->f) != 0)
        error ("cannot close `%s'", f->name);
    }
}


/* Close and remove the file F if it is open. */

void my_remove (struct file *f)
{
  if (f->ok)
    {
      f->ok = FALSE;
      fclose (f->f);
      remove (f->name);
    }
}


/* Set the read/write pointer of the file F to location POS. */

void my_seek (struct file *f, long pos)
{
  if (fseek (f->f, pos, SEEK_SET) != 0)
    error ("seek failed (`%s')", f->name);
}


/* Skip POS bytes of the file F.  The read/write pointer is moved by
   POS. */

void my_skip (struct file *f, long pos)
{
  if (fseek (f->f, pos, SEEK_CUR) != 0)
    error ("seek failed (`%s')", f->name);
}


/* Return the length of the file F.  Note that this function moves the
   read/write pointer to the end of the file. */

long my_size (struct file *f)
{
  long n;

  if (fseek (f->f, 0L, SEEK_END) != 0 || (n = ftell (f->f)) < 0)
    error ("seek failed (`%s')", f->name);
  return n;
}


/* Return the current location of the read/write pointer of the file
   F. */

long my_tell (struct file *f)
{
  long n;

  n = ftell (f->f);
  if (n < 0)
    error ("ftell() failed (`%s')", f->name);
  return n;
}


/* Truncate the file F.  All data beyond the current read/write
   pointer are discarded. */

void my_trunc (struct file *f)
{
  fflush (f->f);
  if (ftruncate (fileno (f->f), ftell (f->f)) != 0)
    error ("ftruncate failed (`%s')", f->name);
}


/* Read a null-terminated string from the file F to the buffer DST.
   There are SIZE bytes available at DST. */

void my_read_str (byte *dst, size_t size, struct file *f)
{
  int c;

  for (;;)
    {
      c = getc (f->f);
      if (c == EOF)
        error ("cannot read `%s'", f->name);
      if (size <= 0)
        error ("invalid string in `%s'", f->name);
      *dst++ = (byte)c;
      --size;
      if (c == 0)
	break;
    }
}


/* Let DST reference the file referenced by SRC.  If DST refers to a
   open file, it will be closed before doing this.  SRC will be
   closed. */

void my_change (struct file *dst, struct file *src)
{
  my_close (dst);
  *dst = *src;
  src->ok = src->del = FALSE;
  my_seek (dst, 0);
}


/* Return true iff the file NAME is readable. */

int my_readable (const char *name)
{
  return (access (name, 4) == 0);
}


/* Allocate N bytes of memory.  This function never returns NULL.
   When running out of memory, a message is displayed and the program
   is terminated. */

void *xmalloc (size_t n)
{
  void *q;

  q = malloc (n);
  if (q == NULL)
    out_of_mem ();
  return q;
}


/* Create a duplicate of the string P on the heap.  This function
   never returns NULL.  When running out of memory, a message is
   displayed and the program is terminated. */

char *xstrdup (const char *p)
{
  char *q;

  q = strdup (p);
  if (q == NULL)
    out_of_mem ();
  return q;
}


/* Reallocate the block of memory P to N bytes.  This function never
   returns NULL.  When running out of memory, a message is displayed
   and the program is terminated. */

void *xrealloc (void *p, size_t n)
{
  void *q;

  q = realloc (p, n);
  if (q == NULL)
    out_of_mem ();
  return q;
}


/* Return the string "s" if plural should be used for the number N.
   If N is 1, return the empty string. */

const char *plural_s (long n)
{
  return (n == 1 ? "" : "s");
}


/* Add SIZE bytes at SRC to the end of the growable buffer DST.
   Expand DST as required. */

void put_grow (struct grow *dst, const void *src, size_t size)
{
  if (dst->len + size > dst->size)
    {
      dst->size += 1024 * ((size + 1023) / 1024);
      dst->data = xrealloc (dst->data, dst->size);
    }
  memmove (dst->data + dst->len, src, size);
  dst->len += size;
}
