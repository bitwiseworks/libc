/* emxomfar.c -- Manage OMF libraries (.LIB files)
   Copyright (c) 1992-1998 Eberhard Mattes

This file is part of emxomfar.

emxomfar is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

emxomfar is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with emxomfar; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/omflib.h>


#define FALSE 0
#define TRUE  1

/* The name of the original library file. */
static char *lib_fname;

/* The name of the new library file. */
static char *new_fname;

/* This is the command character. */
static char cmd;

/* This flag is non-zero if the v option is given. */
static char verbose;

/* This flag is non-zero if the c option is given. */
static char silent;

/* OMFLIB descriptor of the original library. */
static struct omflib *lib;

/* OMFLIB descriptor of the new library. */
static struct omflib *new_lib;


/* Tell the user how to run this program. */

static void usage (void)
{
  fprintf (stderr, "emxomfar " VERSION VERSION_DETAILS
           "\nCopyright (c) 1992-1996 by Eberhard Mattes\n" VERSION_COPYRIGHT "\n\n");
  fprintf (stderr, "Usage: emxomfar [-p#] <command> <library_file> [<module>]...\n");
  fprintf (stderr, "\nCommands:\n");
  fprintf (stderr, "  d    delete module from library\n");
  fprintf (stderr, "  r    replace modules in library\n");
  fprintf (stderr, "  t    list table of contents\n");
  fprintf (stderr, "  x    extract modules from library\n");
  fprintf (stderr, "\nOptions (append to command):\n");
  fprintf (stderr, "  c    no warning when creating new library\n");
  fprintf (stderr, "  v    verbose output\n");
  fprintf (stderr, "\nOptions (prededing command):\n");
  fprintf (stderr, "  -p#  set page size\n");
  exit (1);
}


/* This function is passed to omflib_pubdef_walk to list the public
   symbols. */

static int list_pubdef (const char *name, char *error)
{
  printf ("  %s\n", name);
  return 0;
}


/* Perform the requested action for the module NAME. */

static void do_mod (const char *name)
{
  char error[512], c;
  char mod_name[256];
  int page;

  c = 'a'; page = 0;
  omflib_module_name (mod_name, name);
  if (lib != NULL)
    {
      page = omflib_find_module (lib, name, error);
      if (cmd == 'r')
        {
          if (page != 0)
            {
              c = 'r';
              if (omflib_mark_deleted (lib, name, error) != 0)
                {
                  fprintf (stderr, "emxomfar: %s\n", error);
                  exit (2);
                }
            }
        }
      else if (page == 0)
        {
          fprintf (stderr, "emxomfar: Module not found: %s\n", name);
          return;
        }
    }
  switch (cmd)
    {
    case 't':
      if (verbose)
        {
          printf("%s (page %d):\n", mod_name, page);
          if (omflib_pubdef_walk (lib, page, list_pubdef, error) != 0)
            fprintf (stderr, "emxomfar: %s\n", error);
          fputchar ('\n');
        }
      else
        printf ("%s\n", mod_name);
      break;
    case 'x':
      if (verbose)
        printf ("x - %s\n", name);
      if (omflib_extract (lib, name, error) != 0)
        {
          fprintf (stderr, "emxomfar: %s\n", error);
          exit (2);
        }
      break;
    case 'r':
      if (verbose)
        printf ("%c - %s\n", c, name);
      if (omflib_add_module (new_lib, name, error) != 0)
        {
          fprintf (stderr, "emxomfar: %s(%s): %s\n", new_fname, name, error);
          exit (2);
        }
      break;
    case 'd':
      if (verbose)
        printf ("d - %s\n", mod_name);
      if (omflib_mark_deleted (lib, name, error) != 0)
        fprintf (stderr, "emxomfar: %s\n", error);
      break;
    }
}



/**
 * Calculate the expected page size for a library based on the
 * parameters which is specified.
 *
 * @returns 
 * @param   papszFiles  Pointer to a vector of files.
 * @param   chCmd       The command we're processing.
 * @remark  Only applies when creating a new library.
 */
static int calc_pagesize(char **papszFiles, const char chCmd)
{
    unsigned    cFiles;
    size_t      cbFiles;
    int         cbPage;
    int         i;

    /* if it's not a replace command return default size. */
    if (chCmd != 'r')
        return 16;

    /*
     * Count the files and sum their sizes.
     */
    for (i = 0, cFiles = 0, cbFiles = 0; papszFiles[i]; i++)
    {
        struct stat s;
        cFiles++;
        if (!stat(papszFiles[i], &s))
            cbFiles += s.st_size;
        else
            cbFiles += 128 * 1024;      /* file not found? just guess a size, 128kb. */
    }

    /*
     * We'll need an approximation of this formula:
     * cbPage = (cbPage * cFiles + cbFiles) / 65535;
     *
     * Let's do the calculation assuming cbPage being 256.
     * We'll also add a few files (just for case that any of these were a library),
     * and a couple extra of bytes too.
     */
    cbFiles += cFiles * 4096;
    cFiles += 64;
    cbPage = (256 * cFiles + cbFiles) / 65536;
    for (i = 16; i < cbPage; )
        i <<= 1;

    if (verbose)
        printf("calculated page size %d\n", i);
    return i;
}

/* Cleanup, for atexit. */

static void cleanup (void)
{
  char error[512];

  if (new_fname != NULL)
    {
      if (new_lib != NULL)
        {
          omflib_close (new_lib, error);
          new_lib = NULL;
        }
      if (lib != NULL)
        {
          omflib_close (lib, error);
          lib = NULL;
        }
      remove (new_fname);
      if (lib_fname != NULL)
        rename (lib_fname, new_fname);
      new_fname = NULL;
    }
}


/* Entrypoint.  Parse the command line, open files and perform the
   requested action. */

int main (int argc, char *argv[])
{
  char error[512], name[257], *s, new_cmd;
  int page;
  int i, n, create, page_size;
  char ext_fname[512];
  char bak_fname[512];

  /* Expand response files and wildcards on th command line. */

  _response (&argc, &argv);
  _wildcard (&argc, &argv);

  /* The default page size was 16, now we'll calculate one with 
     a minimum of 16. */

  page_size = 0;

  /* Set initial values. */

  verbose = FALSE; 
  silent  = FALSE; 
  cmd     = 0;

  /* Skip a leading dash, it's optional. */

  i = 1;
  s = i < argc ? argv[i++] : "";
  if (*s == '-')
    ++s;

  /* Parse the command and options. */

  for (;;)
    {
      while (*s != 0)
        {
          new_cmd = 0;
          switch (*s)
            {
            case 'v':
              verbose = TRUE;
              break;
            case 'c':
              silent = TRUE;
              break;
            case 's':
              if (cmd == 0)
                new_cmd = 's';
              break;
            case 'd':
            case 'r':
            case 't':
            case 'x':
              new_cmd = *s;
              break;
            case 'q':
              new_cmd = 'r';
              break;
            case 'u':
              if (cmd == 0)
                new_cmd = 'r';
              break;
            case 'l':
            case 'o':
              break;
            case 'p':
              ++s;
              page_size = strtol (s, &s, 10);
              if (page_size < 1 || *s != 0)
                usage ();
              --s;
              break;
            default:
              usage ();
            }
          ++s;
          if (new_cmd != 0)
            {
              if (cmd != 0)
                usage ();
              cmd = new_cmd;
            }
        }

      /* more options? */

      if (i >= argc)
        usage ();
      s = argv[i];
      if (*s != '-')
        break; /* not an option */
      
      s++;
      i++;
      if (*s == '-' && !s[1])
        break; /* end of options: '--' */
    }

  /* Complain if no command letter or library was given. */

  if (cmd == 0 || i >= argc)
    usage ();

  /* The s command is a no-op. */

  if (cmd == 's')
    return 0;

  /* Add the .lib default extension to the library name.  The result
     is in ext_fname. */

  _strncpy (ext_fname, argv[i++], sizeof (ext_fname) - 4);
  _defext (ext_fname, "lib");

  /* Initialize file names and OMFLIB descriptors. */

  lib_fname = ext_fname;
  lib = NULL;
  new_fname = NULL;
  new_lib = NULL;

  /* Make create non-zero iff we have to create a new library file. */

  create = (cmd == 'r' && access (lib_fname, 4) != 0);

  /* The r and d commands create a library file if it doesn't exist.
     If it does exist, a backup file is created. */

  if (cmd == 'r' || cmd == 'd')
    {
      if (create)
        {
          if (!silent)
            printf ("Creating library file `%s'\n", lib_fname);
          new_fname = lib_fname;
          lib_fname = NULL;
        }
      else
        {
          _strncpy (bak_fname, lib_fname, sizeof (bak_fname) - 4);
          _remext (bak_fname);
          _defext (bak_fname, "bak");
          if (stricmp (lib_fname, bak_fname) == 0)
            {
              fprintf (stderr, "Cannot update backup file\n");
              exit (1);
            }
          remove (bak_fname);
          if (rename (lib_fname, bak_fname) != 0)
            {
              perror (lib_fname);
              exit (2);
            }
          new_fname = lib_fname;
          lib_fname = bak_fname;
        }
    }

  /* If no new library file is to be created, the library file is
     supposed to exist.  Open it. */

  if (!create)
    {
      lib = omflib_open (lib_fname, error);
      if (lib == NULL)
        {
          fprintf (stderr, "emxomfar: %s: %s\n", lib_fname, error);
          exit (2);
        }
    }

  /* Create the output library for the r and d commands. */

  if (cmd == 'r' || cmd == 'd')
    {
      if (page_size <= 0)
        page_size = calc_pagesize(&argv[i], cmd);
      new_lib = omflib_create (new_fname, page_size, error);
      if (new_lib == NULL)
        {
          fprintf (stderr, "emxomfar: %s: %s\n", new_fname, error);
          exit (2);
        }
    }

  /* Close files when done or when aborting. */

  atexit (cleanup);

  /* Write header of new library. */

  if (cmd == 'r' || cmd == 'd')
    {
      if (omflib_header (new_lib, error) != 0)
        {
          fprintf (stderr, "emxomfar: %s\n", error);
          exit (2);
        }
    }

  if (i >= argc && !(cmd == 'r' || cmd == 'd'))
    {

      /* No arguments specified for the t and x commands: Apply
         do_mod() to all modules of the library. */

      n = omflib_module_count (lib, error);
      if (n == -1)
        {
          fprintf (stderr, "emxomfar: %s\n", error);
          exit (2);
        }
      for (i = 0; i < n; ++i)
        {
          if (omflib_module_info (lib, i, name, &page, error) != 0)
            fprintf (stderr, "emxomfar: %s\n", error);
          else
            do_mod (name);
        }
    }
  else
    {
      /* Apply do_mod() to all modules given on the command line. */

      while (i < argc)
        {
          if (!(cmd == 'd' && strcmp (argv[i], "__.SYMDEF") == 0))
            do_mod (argv[i]);
          ++i;
        }
    }

  /* Copy all the unmodified modules of the original library to the
     new library. */

  if (cmd == 'r' || cmd == 'd')
    {
      if ((!create && omflib_copy_lib (new_lib, lib, error) != 0)
          || omflib_finish (new_lib, error) != 0
          || omflib_close (new_lib, error) != 0)
        {
          fprintf (stderr, "emxomfar: %s\n", error);
          exit (2);
        }
      new_lib = NULL; new_fname = NULL;
    }

  /* Close the source library. */

  if (lib != NULL && omflib_close (lib, error) != 0)
    fprintf (stderr, "emxomfar: %s\n", error);
  return 0;
}
