/* emxbind.c -- Create an .exe executable from an a.out executable
   Copyright (c) 1991-1997 Eberhard Mattes

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


#define EXTERN

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/moddef.h>
#include "defs.h"
#include "emxbind.h"


/* The name of the module definition file (set by the -d option) or
   NULL (if there is no module definition file).  If it's the empty
   string, the input file name with .def extension will be used. */

static char *opt_d = NULL;

/* The banner line of emxbind. */

static char *title = "emxbind " VERSION " -- "
                     "Copyright (c) 1991-1997 by Eberhard Mattes";

/* File names of various files.  inp_fname holds the name of the
   input executable, stub_fname the name of the DOS stub file and
   def_fname the name of the module definition file (-d option). */

static char inp_fname[FNAME_SIZE];
static char stub_fname[FNAME_SIZE];
static char def_fname[FNAME_SIZE];

/* These global variables have the same values as the argc and argv
   parameters, respectively, of main(). */

static int gargc;
static char **gargv;


/* Give the user a short reminder about how to call this program. */

static void usage (void)
{
  puts (title);
  puts ("\nUsage:");
  puts ("  emxbind [-b] [<options>] <stub>[.exe] <input> [<output>[.exe]]");
  puts ("  emxbind [-b] [<options>] [-o <output>[.exe]] <input>");
  puts ("  emxbind -e [<options>] <input>[.exe]");
  puts ("  emxbind -s [<options>] <input>[.exe]");
  puts ("\nCommands:");
  puts ("  -b          bind .exe (default)");
  puts ("  -e          set OS/2 .exe flags: -f, -p or -w");
  puts ("  -s          strip symbols");
  puts ("\nOptions:");
  puts ("  -f          application type: full screen (-b and -e)");
  puts ("  -p          application type: Presentation Manager (-b and -e)");
  puts ("  -w          application type: windowed (-b and -e)");
  puts ("  -q          be quiet");
  puts ("  -v          be verbose");
  puts ("  -s          strip symbols (-b only)");
  puts ("  -c[<core>]  add data from core dump file (-b only)");
  puts ("  -d[<def>]   read module definition file (-b only)");
  puts ("  -h<size>    set heap size for OS/2 (-b only)");
  puts ("  -k<size>    set stack size for OS/2 (-b only)");
  puts ("  -m<map>     write .map file (-b only)");
  puts ("  -r<res>     add resources (-b only)");
  exit (1);
}


/* A module statement is ignored: display a warning message if the
   verbosity level is high enough. */

static void def_ignored (const char *name)
{
  if (verbosity >= 2)
    printf ("emxbind: %s statement not supported (warning)\n", name);
}


/* Callback function for reading the module definition file.  Display
   a warning for ignored statements if the verbosity level is high
   enough.  Save data from the other statements. */

static int md_callback (struct _md *md, const _md_stmt *stmt, _md_token token,
                        void *arg)
{
  struct export exp;

  switch (token)
    {
    case _MD_BASE:
      def_ignored ("BASE");
      break;

    case _MD_CODE:
      def_ignored ("CODE");
      break;

    case _MD_DATA:
      def_ignored ("DATA");
      break;

    case _MD_DESCRIPTION:

      /* Save the description string for putting it into the
         non-resident name table. */

      description = xstrdup (stmt->descr.string);
      break;

    case _MD_EXETYPE:
      def_ignored ("EXETYPE");
      break;

    case _MD_EXPORTS:

      /* Add an export entry. */

      exp.ord = stmt->export.ordinal;
      exp.resident = (!exp.ord || (stmt->export.flags & _MDEP_RESIDENTNAME)) ? TRUE : FALSE;
      exp.entryname = xstrdup (stmt->export.entryname);
      exp.flags = stmt->export.flags;
      if (stmt->export.internalname[0] != 0)
        exp.internalname = xstrdup (stmt->export.internalname);
      else
        exp.internalname = xstrdup (stmt->export.entryname);
      exp.offset = 0;
      exp.object = 0;
      add_export (&exp);
      break;

    case _MD_HEAPSIZE:
      def_ignored ("HEAPSIZE");
      break;

    case _MD_IMPORTS:
      def_ignored ("IMPORTS");
      break;

    case _MD_LIBRARY:

      /* Create a DLL. Override the module name if specified. Save the 
         initialization and termination policies after choosing default 
         values for unspecified values. */

      dll_flag = TRUE;

      if (stmt->library.name[0] != 0)
        module_name = xstrdup (stmt->library.name);

      switch (stmt->library.init)
        {
        case _MDIT_GLOBAL:
          init_global = TRUE;
          break;
        case _MDIT_INSTANCE:
          init_global = FALSE;
          break;
        default:
          break;
        }
      switch (stmt->library.term)
        {
        case _MDIT_GLOBAL:
          term_global = TRUE;
          break;
        case _MDIT_INSTANCE:
          term_global = FALSE;
          break;
        default:
          break;
        }
      if (stmt->library.init != _MDIT_DEFAULT
          && stmt->library.term == _MDIT_DEFAULT)
        term_global = init_global;
      if (stmt->library.term != _MDIT_DEFAULT
          && stmt->library.init == _MDIT_DEFAULT)
        init_global = term_global;
      break;

    case _MD_NAME:

      /* Create a program.  Save the module name and the application
         type. */

      if (stmt->name.name[0] != 0)
        module_name = xstrdup (stmt->name.name);
      app_type = stmt->name.pmtype;
      break;

    case _MD_OLD:
      def_ignored ("OLD");
      break;

    case _MD_PROTMODE:
      def_ignored ("PROTMODE");
      break;

    case _MD_REALMODE:
      def_ignored ("REALMODE");
      break;

    case _MD_SEGMENTS:
      def_ignored ("SEGMENTS");
      break;

    case _MD_STACKSIZE:

      /* Check and save the stack size. */

      if (stmt->stacksize.size < 20*1024
          || stmt->stacksize.size > 512*1024*1024)
        error ("Invalid stack size in module definition file");
      stack_size = (stmt->stacksize.size + 0xfff) & ~0xfff;
      break;

    case _MD_STUB:

      /* Save the name of the stub. */

      if (!stmt->stub.none)
        _strncpy (stub_fname, stmt->stub.name, sizeof (stub_fname));
      break;

    case _MD_VIRTUAL:
    case _MD_PHYSICAL:
      error ("VIRTUAL DEVICE and PHYSICAL DEVICE statements not supported");

    case _MD_parseerror:

      /* Handle a syntax error. */

      error ("%s (line %ld of %s)", _md_errmsg (stmt->error.code),
             _md_get_linenumber (md), def_fname);
      break;

    default:
      abort ();
    }
  return 0;
}


/* Read the module definition file. */

static void read_def_file (void)
{
  struct _md *md;

  md = _md_open (def_fname);
  if (md == NULL)
    error ("cannot open `%s'", def_fname);
  _md_next_token (md);
  _md_parse (md, md_callback, NULL);
  _md_close (md);
}


/* Fetch a file name from the command line.  If there are no more
   arguments or if the current argument looks like an option, don't
   use an argument and use the default file name DEF instead.  If DEF
   is NULL, a file name must be provided.  If EXT is non-NULL, it is
   used as default extension.  This function will update optind if it
   uses an argument. */

static void file_name (char *dst, const char *ext, const char *def)
{
  if (optind < gargc && gargv[optind][0] != '-')
    {
      if (strlen (gargv[optind]) > FNAME_SIZE - 5)
        error ("file name too long");
      strcpy (dst, gargv[optind]);
      ++optind;
    }
  else if (def == NULL)
    error ("file name missing");
  else
      strcpy (dst, def);
  if (ext)
  {
    char *curext = _getext2 (dst);
    if (curext && *curext == '.')
      curext++;
    if (strcmp (curext, ext))
    {
      if (strlen (dst) > FNAME_SIZE - 5)
        error ("file name too long");
      strcat (dst, ".");
      strcat (dst, ext);
    }
    }
}


/* Parse the command line. */

static void get_args (void)
{
  int c;
  char *q;
  const char *opt_o = NULL;

  /* At least one argument, the command option, must be supplied. */

  if (gargc <= 1)
    usage ();

  /* Don't let getopt() report errors, use `-' to introduce options. */

  opterr = FALSE;
//optswchar = "-";

  /* No command option has been seen yet. */

  mode = 0;

  /* Examine the initial (emxbind) options, one by one. */

  while ((c = getopt (gargc, gargv,
                      "bc::d::efh:k:m:o:pqr:svw")) != -1)
    {
      switch (c)
	{
	case 's':
          /* -s after -b selects 'strip on bind' mode.
             -s without -b selects the 'strip in a executable' mode */
          if (mode == 'b')
          {
            opt_s = TRUE;
            break;
          }
	case 'b':
	case 'e':
	  if (mode != 0)
            error ("multiple commands specified");
	  mode = c;
	  break;
	case 'c':
	  if (opt_c != NULL)
            error ("multiple core files");
	  opt_c = (optarg != NULL ? optarg : "");
	  break;
	case 'd':
	  if (opt_d != NULL)
            error ("multiple module definition files");
	  opt_d = (optarg != NULL ? optarg : "");
	  break;
	case 'f':
	  opt_f = TRUE;
	  break;
	case 'h':
          errno = 0;
	  heap_size = strtol (optarg, &q, 0);
          if (errno != 0 || heap_size < 0 || heap_size > 512 || *q != 0)
            error ("invalid heap size");
          heap_size *= 1024*1024;
	  break;
	case 'k':
          errno = 0;
	  stack_size = strtol (optarg, &q, 0);
          if (errno != 0 || *q != 0 ||
              (stack_size < 20 || stack_size > 512*1024))
            error ("invalid stack size");
          stack_size *= 1024;
          stack_size = (stack_size + 0xfff) & ~0xfff;
	  break;
	case 'm':
	  if (opt_m != NULL)
            error ("multiple map files");
	  opt_m = optarg;
	  break;
	case 'o':
	  if (opt_o != NULL)
            error ("multiple output files");
	  opt_o = optarg;
	  break;
	case 'p':
	  opt_p = TRUE;
	  break;
	case 'q':
	  verbosity = 0;
	  break;
	case 'r':
	  opt_r = optarg;
	  break;
	case 'v':
	  verbosity = 2;
	  break;
	case 'w':
	  opt_w = TRUE;
	  break;
	default:
	  error ("invalid option");
	}
    }

  /* The default command option is -b. */

  if (mode == 0)
    mode = 'b';

  /* Complain if an illegal combination of options is used. */

  if (opt_o != NULL && mode != 'b')
    error ("-o option can be used only with -b command");
  if (opt_c != NULL && mode != 'b')
    error ("-c option can be used only with -b command");
  if ((opt_f || opt_p || opt_w) && mode != 'b' && mode != 'e')
    error ("-f, -p and -w options can be used only with -b and -e commands");
  if (opt_f + opt_p + opt_w > TRUE)
    error ("more than one of -f, -p and -w options given");
  if (mode == 'e' && !(opt_p || opt_f || opt_w))
    error ("-e command requires -f, -p or -w option");
  if (opt_d != NULL && mode != 'b')
    error ("-d option can be used only with -b command");

  /* If the -c option is used without argument, "core" will be used as
     core dump file. */

  if (opt_c != NULL && *opt_c == 0)
    opt_c = "core";

  /* Set the default path to the DOS stub */
  _execname (stub_fname, sizeof (stub_fname));
  strcpy (_getname (stub_fname), "os2stub.bin");

  /* If the -E option is not given and the name is not given by the
     EMXBIND_DLL environment variable, "emx.dll" will be used. */

  /* Parse the rest of the command line, depending on the command
     option. */

  switch (mode)
  {
    case 'b':
      if (gargc - optind <= 1 || gargv[optind+1][0] == '-')
        ;
      else if (opt_o != NULL)
        error ("Too many file names specified");
      else
	file_name (stub_fname, "exe", NULL);
      file_name (inp_fname, NULL, NULL);
      if (opt_d != NULL)
	{
	  if (opt_d[0] == 0)
	    {
	      _strncpy (def_fname, inp_fname, sizeof (def_fname) - 4);
	      _remext (def_fname);
	    }
	  else
	    _strncpy (def_fname, opt_d, sizeof (def_fname) - 4);
	  _defext (def_fname, "def");
          read_def_file ();
	}
      file_name (out_fname, (dll_flag ? "dll" : "exe"),
                 (opt_o != NULL ? opt_o : inp_fname));
      if (stricmp (inp_fname, out_fname) == 0)
        error ("The input and output files have the same name");
      break;

    case 'e':
    case 's':
      file_name (inp_fname, "exe", NULL);
      break;
  }

  /* Complain if there are any unprocessed arguments left. */

  if (optind < gargc)
    error ("too many arguments");
}


static void cleanup (void)
{
  my_remove (&out_file);
}


/* Open the files, depending on the command option.  The module
   definition file (-d option) is not handled here. */

static void open_files (void)
{
  atexit (cleanup);
  switch (mode)
    {
    case 'b':
      my_open (&inp_file, inp_fname, open_read);
      my_open (&stub_file, stub_fname, open_read);
      if (opt_c != NULL)
	my_open (&core_file, opt_c, open_read);
      if (opt_r != NULL)
	my_open (&res_file, opt_r, open_read);
      my_open (&out_file, out_fname, create_write);
      break;
    case 'e':
    case 's':
      my_open (&inp_file, inp_fname, open_read_write);
      break;
    }
}


/* Close all the open files. */

static void close_files (void)
{
  my_close (&out_file);
  my_close (&inp_file);
  my_close (&stub_file);
  my_close (&core_file);
  my_close (&res_file);
}


/* This is the main function.  Parse the command line and perform the
   action requested by the user. */

int main (int argc, char *argv[])
{
  /* Setup global variables for referencing the command line arguments. */

  gargc = argc; gargv = argv;

  /* Parse the command line. */

  get_args ();

  /* Display the banner line unless suppressed. */

  if (verbosity > 0) puts (title);

  /* If emxl.exe was located automatically by emxbind, display the
     path name of emxl.exe. */

  if (verbosity >= 2)
    printf ("Loader program for DOS: %s\n", stub_fname);

  /* Open the files. */

  open_files ();

  /* Further processing depends on the command. */

  cmd (mode);

  /* Close the files. */

  close_files ();
  return 0;
}
