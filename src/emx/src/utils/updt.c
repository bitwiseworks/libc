/* updt.c -- Copy a file if required
   Copyright (c) 1994-1995 by Eberhard Mattes

This file is part of emx.

emx is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

emx is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with emx; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <io.h>
#include <sys/types.h>
#include <sys/stat.h>

#define FALSE 0
#define TRUE  1

int main (int argc, char *argv[]);
static void usage (void);

static void usage (void)
{
  fputs ("Usage: updt [-t] [-v] <source_file> <target_file>\n", stderr);
  exit (1);
}


int main (int argc, char *argv[])
{
  FILE *source_file, *target_file;
  char *source_fname, *target_fname;
  struct stat source_stat, target_stat;
  int c1, c2, c, opt_t, opt_v;

  opt_t = FALSE; opt_v = FALSE;
  while ((c = getopt (argc, argv, "tv")) != EOF)
    {
      switch (c)
        {
        case 't':
          opt_t = TRUE;
          break;
        case 'v':
          opt_v = TRUE;
          break;
        default:
          usage();
        }
    }
  if (argc - optind != 2)
    usage ();
  source_fname = argv[optind+0];
  target_fname = argv[optind+1];
  source_file = fopen (source_fname, "rb");
  if (source_file == NULL)
    {
      fprintf (stderr, "updt: cannot open source file `%s'\n", source_fname);
      return 1;
    }
  target_file = fopen (target_fname, "rb");
  if (target_file != NULL)
    {
      if (opt_t)
        {
          if (stat (source_fname, &source_stat) != 0)
            {
              fprintf (stderr, "updt: cannot access `%s'\n", source_fname);
              return 1;
            }
          if (stat (target_fname, &target_stat) != 0)
            {
              fprintf (stderr, "updt: cannot access `%s'\n", target_fname);
              return 1;
            }
          if (target_stat.st_mtime >= source_stat.st_mtime)
            {
              if (opt_v)
                fputs ("updt: source not newer than target "
                       "-- not copying\n", stderr);
              return 0;
            }
          if (opt_v)
            fputs ("updt: source newer than target "
                   "-- copying\n", stderr);
        }
      else
        {
          for (;;)
            {
              c1 = getc (source_file);
              c2 = getc (target_file);
              if (c1 != c2 || c1 == EOF)
                break;
            }
          if (c1 == EOF && c2 == EOF && !ferror (source_file) &&
              !ferror (target_file))
            {
              if (opt_v)
                fputs ("updt: files match -- not copying\n", stderr);
              return 0;
            }
          if (opt_v)
            fputs ("updt: files don't match -- copying\n", stderr);
        }
      (void)fclose (target_file);
    }
  else if (opt_v)
    fputs ("updt: target does not exist -- copying\n", stderr);
  rewind (source_file);
  target_file = fopen (target_fname, "wb");
  if (target_file == NULL)
    {
      fprintf (stderr, "updt: cannot open target file `%s'\n", target_fname);
      return 1;
    }
  for (;;)
    {
      c1 = getc (source_file);
      if (c1 == EOF)
        break;
      if (putc (c1, target_file) == EOF)
        break;
    }
  if (ferror (source_file) || fclose (source_file) != 0)
    {
      fprintf (stderr, "updt: cannot read source file `%s'\n", source_fname);
      return 1;
    }
  if (ferror (target_file) || fflush (target_file) != 0 ||
      fclose (target_file) != 0)
    {
      fprintf (stderr, "updt: cannot write target file `%s'\n",
               target_fname);
      return 1;
    }
  return 0;
}
