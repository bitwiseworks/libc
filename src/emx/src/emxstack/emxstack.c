/* emxstack.c -- Fix the stack size
   Copyright (c) 1994-1998 Eberhard Mattes

This file is part of emxstack.

emxstack is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

emxstack is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with emxstack; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */


#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <getopt.h>
#include <sys/dirtree.h>
#include "defs.h"

/* How to open a file.  This type is used for my_open(). */

enum open_mode
{
  open_read, open_read_write
};

/* A file.  F is the stream.  NAME is the name of the file. */

struct file
{
  FILE *f;
  const char *name;
};


/* Mode of operation. */

static enum op
{
  op_none,                      /* No command specified */
  op_show,                      /* Display stack size */
  op_check,                     /* Check whether fix is required */
  op_fix,                       /* Fix stack size */
  op_update,                    /* Update stack size */
  op_force                      /* Set stack size */
} operation;

/* This variable is 2 if verbose output is requested (-v), 1 by
   default, and 0 if emxstack should be quiet (-q). */

static int verbosity;

/* New stack size. */

static long new_stack_size;

/* Minimum stack size. */

static long min_stack_size;


/* Combine two 16-bit words (low word L and high word H) into a 32-bit
   word. */

#define COMBINE(L,H) ((L) | (long)((H) << 16))

/* Round up to the next integral multiple of the page size. */

#define ROUND_PAGE(X)  ((((X) - 1) & ~0xfff) + 0x1000)


static void error (const char *fmt, ...) NORETURN2;


/* Print an error message and abort.  This function is called like
   printf() but never returns.  The message will be prefixed with
   "emxbind: " and a newline will be added at the end. */

static void error (const char *fmt, ...)
{
  va_list arg_ptr;

  va_start (arg_ptr, fmt);
  fprintf (stderr, "emxstack: ");
  vfprintf (stderr, fmt, arg_ptr);
  fputc ('\n', stderr);
  exit (2);
}


/* Print an error message and abort.  This function is called like
   printf() but never returns.  The message will be prefixed with the
   file name NAME and a newline will be added at the end. */

static void file_error (const char *fmt, const char *name, ...)
{
  va_list arg_ptr;

  va_start (arg_ptr, name);
  fprintf (stderr, "%s: ", name);
  vfprintf (stderr, fmt, arg_ptr);
  fputc ('\n', stderr);
  exit (2);
}


/* Display a warning message.  If the -c or -f commands are used
   without -v, the message is supressed. */

static void file_warning (const char *fmt, const char *name, ...)
{
  va_list arg_ptr;

  if (verbosity >= 2 || (operation != op_check && operation != op_fix))
    {
      va_start (arg_ptr, name);
      fprintf (stderr, "%s: ", name);
      vfprintf (stderr, fmt, arg_ptr);
      fputc ('\n', stderr);
    }
}


/* Read SIZE bytes from the file F to the buffer DST.  If there aren't
   SIZE bytes at the current location of the read/write pointer, an
   error message will be displayed and the program will be
   terminated. */

static void my_read (void *dst, size_t size, struct file *f)
{
  if (fread (dst, 1, size, f->f) != size)
    {
      if (ferror (f->f))
        file_error ("%s", f->name, strerror (errno));
      else
        file_error ("End of file reached", f->name);
    }
}


/* Write SIZE bytes from SRC to the file F. */

static void my_write (const void *src, size_t size, struct file *f)
{
  if (fwrite (src, 1, size, f->f) != size)
    file_error ("%s", f->name, strerror (errno));
}


/* Open the file named NAME and let F reference that file.  OMODE is
   the open mode.  The file is always opened in binary mode. */

static void my_open (struct file *f, const char *name, enum open_mode omode)
{
  const char *ms;

  switch (omode)
    {
    case open_read:
      ms = "rb";
      break;
    case open_read_write:
      ms = "r+b";
      break;
    default:
      ms = NULL;
      break;
    }
  f->name = name;
  f->f = fopen (name, ms);
  if (f->f == NULL)
    file_error ("%s", name, strerror (errno));
}


/* Close the file F if it is open. */

static void my_close (struct file *f)
{
  if (fflush (f->f) != 0 || fclose (f->f) != 0)
    file_error ("%s", f->name, strerror (errno));
}


/* Set the read/write pointer of the file F to location POS. */

static void my_seek (struct file *f, long pos)
{
  if (fseek (f->f, pos, SEEK_SET) != 0)
    file_error ("%s", f->name, strerror (errno));
}


/* Return the length of the file F.  Note that this function moves the
   read/write pointer to the end of the file. */

static long my_size (struct file *f)
{
  long n;

  if (fseek (f->f, 0L, SEEK_END) != 0 || (n = ftell (f->f)) < 0)
    error ("seek failed (`%s')", f->name);
  return n;
}


/* Tell the user how to use this program. */

static void usage (void)
{
  fputs ("emxstack " VERSION INNOTEK_VERSION " -- "
         "Copyright (c) 1994-1995 by Eberhard Mattes\n\n", stderr);
  fputs ("Usage: emxstack <command> [<options>] <file>...\n", stderr);
  fputs ("\nCommands:\n", stderr);
  fputs ("-c        Check whether stack size should be fixed\n", stderr);
  fputs ("-d        Display stack size\n", stderr);
  fputs ("-f        Fix stack size for emx 0.9\n", stderr);
  fputs ("-s<n>     Set stack size to <n> Kbyte\n", stderr);
  fputs ("-u<n>     Update stack size to <n> Kbyte (if less than <n> Kbyte)\n",
         stderr);
  fputs ("\nOptions:\n", stderr);
  fputs ("-p        Act on all files in PATH\n", stderr);
  fputs ("-q        Be quiet\n", stderr);
  fputs ("-v        Be verbose\n", stderr);
  exit (1);
}


/* Process the file F. */

static void process_file (struct file *f)
{
  struct exe1_header h1;
  struct exe2_header h2;
  struct os2_header ho;
  struct object stack_obj, temp_obj;
  long off_new_exe, off_stack_obj, size;
  long stack_size, stack_kb;
  dword i;
  int uses_emx;
  byte buf[2];

  /* Read the DOS EXE header. */

  size = my_size (f);
  if (size < sizeof (h1))
    {
      file_warning ("Not an executable file", f->name);
      return;
    }
  my_seek (f, 0);
  my_read (&h1, sizeof (h1), f);
  if (h1.magic != 0x5a4d)
    {
      file_warning ("Not an executable file", f->name);
      return;
    }

  /* Read the second part of the DOS EXE header. */

  if (size < sizeof (h1) + sizeof (h2)
      || h1.reloc_ptr < sizeof (h1) + sizeof (h2))
    {
      file_warning ("DOS executable", f->name);
      return;
    }
  my_read (&h2, sizeof (h2), f);
  off_new_exe = COMBINE (h2.new_lo, h2.new_hi);

  /* Read the magic word of the new EXE header. */

  if (off_new_exe + 2 > size)
    {
      file_warning ("DOS executable", f->name);
      return;
    }
  my_seek (f, off_new_exe);
  my_read (buf, 2, f);
  if (!(buf[0] == 'L' && buf[1] == 'X'))
    {
      file_warning ("Not an LX executable file", f->name);
      return;
    }

  /* Read the LX header. */

  my_seek (f, off_new_exe);
  my_read (&ho, sizeof (ho), f);

  /* Skip dynamic link libraries; they don't have a stack object. */

  if (ho.mod_flags & 0x8000)
    {
      file_warning ("Dynamic link library", f->name);
      return;
    }

  /* Retrieve the stack size from the object-relative initial value of
     ESP.  We ignore the stack_size field of the LX header -- it is
     not set by emxbind and was not set in older versions of
     LINK386. */

  stack_size = ho.stack_esp;
  stack_kb = stack_size / 1024;

  /* Check whether the object number for ESP is valid. */

  if (ho.stack_obj < 1 || ho.stack_obj > ho.obj_count)
    file_error ("Invalid stack object number", f->name);

  /* Read the object record for the stack. */

  off_stack_obj = (off_new_exe + ho.obj_offset
                   + (ho.stack_obj - 1) * sizeof (struct object));
  my_seek (f, off_stack_obj);
  my_read (&stack_obj, sizeof (stack_obj), f);

  /* Scan the import module table to check whether the executable
     imports EMX.DLL, EMXLIBC.DLL, or EMXWRAP.DLL.  If at least one of
     these DLLs is imported, `uses_emx' is set to true. */

  uses_emx = 0;
  my_seek (f, off_new_exe + ho.impmod_offset);
  for (i = 0; i < ho.impmod_count; ++i)
    {
      byte len, name[255+1];

      my_read (&len, 1, f);
      if (len != 0)
        my_read (name, len, f);
      name[len] = 0;
      if (stricmp ((char *)name, "EMX") == 0 || stricmp ((char *)name, "EMXLIBC") == 0
          || stricmp ((char *)name, "EMXWRAP") == 0)
        {
          uses_emx = TRUE;
          break;
        }
    }

  /* Now perform the action requested by the user. */

  switch (operation)
    {
    case op_show:
      printf ("%s: %lu Kbyte\n", f->name, stack_kb);
      break;

    case op_check:
      if (!uses_emx)
        file_warning ("Does not use emx.dll", f->name);
      else if (stack_size <= 16384)
        printf ("%s: Needs to be fixed\n", f->name);
      else if (verbosity >= 2)
        printf ("%s: Does not need to be fixed\n", f->name);
      break;

    case op_fix:
      if (!uses_emx)
        {
          file_warning ("Does not use emx.dll", f->name);
          break;
        }
      /*NOBREAK*/

    case op_update:
      if (stack_size >= min_stack_size)
        {
          printf ("%s: Stack size is %lu Kbyte; not changed\n",
                  f->name, stack_kb);
          break;
        }
      /*NOBREAK*/

    case op_force:

      /* Changing the stack size when there are pages assigned to the
         stack object or any succeeding non-resource object is
         difficult as the page map must be changed.  Changing the
         stack size when there are any non-resource objects following
         the stack object is difficult or impossible as relocations
         may have to be changed or invented.  Therefore, we fail if
         there are pages assigned to the stack object or if any
         non-resource objects follow the stack object. */

      if (stack_obj.map_count != 0)
        {
          file_warning ("Stack object has page map entries"
                        " -- stack size not changed", f->name);
          return;
        }

      my_seek (f, off_stack_obj + sizeof (struct object));
      for (i = ho.stack_obj + 1; i <= ho.obj_count; ++i)
        {
          my_read (&temp_obj, sizeof (temp_obj), f);
          if (!(temp_obj.attr_flags & 0x08))
            {
              file_warning ("A non-resource object succeeds the stack object"
                            " -- stack size not changed", f->name);
              return;
            }
        }

      if (verbosity >= 2)
        printf ("%s: Stack size was %lu Kbyte\n", f->name, stack_kb);

      /* Update the initial value of ESP and the size of the stack
         object. */

      ho.stack_esp = new_stack_size;
      stack_obj.virt_size = ROUND_PAGE (new_stack_size);

      /* Set the stack_size field to zero -- it is not used. */

      ho.stack_size = 0;

      /* Write the modified LX header. */

      my_seek (f, off_new_exe);
      my_write (&ho, sizeof (ho), f);

      /* Write the modified stack object record. */

      my_seek (f, off_stack_obj);
      my_write (&stack_obj, sizeof (stack_obj), f);
      break;

    default:
      abort ();
    }
}


/* Process the file FNAME. */

static void process_fname (const char *fname)
{
  struct file f;

  switch (operation)
    {
    case op_show:
    case op_check:
      my_open (&f, fname, open_read);
      break;

    case op_update:
    case op_force:
      my_open (&f, fname, open_read_write);
      break;

    default:
      abort ();
    }
  process_file (&f);
  my_close (&f);
}


/* Process all files in directory NAME. */

static void process_dir (const char *name)
{
  struct _dt_tree *tree;
  struct _dt_node *p;
  char path[512], *s;

  tree = _dt_read (name, "*.exe", 0);
  if (tree == NULL)
    {
      file_warning ("Cannot open directory", name);
      return;
    }
  strcpy (path, name);
  s = strchr (path, 0);
  if (s != path && strchr ("\\:/", s[-1]) == NULL)
    *s++ = '\\';
  for (p = tree->tree; p != NULL; p = p->next)
    if (!(p->attr & 0x10))
      {
        strcpy (s, p->name);
        process_fname (path);
      }
  _dt_free (tree);
}


/* Process all directories listed in LIST. */

static void process_env (const char *list)
{
  char dir[512];
  const char *end;
  int i;

  if (list == NULL || *list == 0)
    return;
  for (;;)
    {
      while (*list == ' ' || *list == '\t') ++list;
      if (*list == 0) break;
      end = list;
      while (*end != 0 && *end != ';') ++end;
      i = end - list;
      while (i>0 && (list[i-1] == ' ' || list[i-1] == '\t')) --i;
      if (i != 0)
        {
          memcpy (dir, list, i);
          dir[i] = 0;
          process_dir (dir);
        }
      if (*end == 0) break;
      list = end + 1;
    }
}


/* Set the mode of operation to X. */

static void set_op (enum op x)
{
  if (operation != op_none)
    error ("More than one operation specified");
  operation = x;
}


/* Get the new stack size from the argument of the option currently
   being parsed. */

static void get_new_stack_size (void)
{
  long n;
  char *e;

  errno = 0;
  n = strtol (optarg, &e, 0);
  if (errno != 0 || n < 20 || n > 512*1024 || e == optarg || *e != 0)
    error ("invalid stack size specified");
  new_stack_size = ROUND_PAGE (n * 1024);
}


/* The main function of emxstack. */

int main (int argc, char *argv[])
{
  int c;
  int path;

  /* Expand wildcards. */

  _wildcard (&argc, &argv);

  /* Set default values. */

  verbosity = 1; path = FALSE;
  new_stack_size = 0;
  min_stack_size = 0;
  operation = op_none;

  /* Let getopt() display error messages. */

  opterr = TRUE;
  optind = 0;

  /* Display usage information when called without arguments. */

  if (argc < 2 || strcmp (argv[1], "-?") == 0)
    usage ();

  /* Parse the command line options. */

  while ((c = getopt (argc, argv, "cdfpqs:u:v")) != EOF)
    switch (c)
      {
      case 'c':
        set_op (op_check);
        break;

      case 'd':
        set_op (op_show);
        break;

      case 'f':
        set_op (op_update);
        min_stack_size = 17 * 1024;
        new_stack_size = 8 * 1024 * 1024;
        break;

      case 'p':
        path = TRUE;
        break;

      case 's':
        set_op (op_force);
        get_new_stack_size ();
        min_stack_size = 0;
        break;

      case 'u':
        set_op (op_update);
        get_new_stack_size ();
        min_stack_size = new_stack_size;
        break;

      case 'q':
        verbosity = 0;
        break;

      case 'v':
        verbosity = 2;
        break;

      default:
        exit (1);
      }

  if (operation == op_none)
    error ("No operation given on the command line");

  if (path)
    {
      /* Use PATH. */

      if (optind < argc)
        error ("No files must be given on the command line if -p is used");
      process_env (getenv ("PATH"));
    }

  else
    {
      /* Process the remaining arguments as files. */

      int i;

      i = optind;
      if (i >= argc)
        error ("No files given on the command line");
      while (i < argc)
        process_fname (argv[i++]);
    }
  return 0;
}
