/* touch.c -- Update the time stamp of files
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


#include <sys/types.h>
#include <sys/utime.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <fcntl.h>
#include <getopt.h>

#define FALSE 0
#define TRUE  1

static void usage (void)
{
  fputs ("Usage: touch [-c] [-r <ref_file>] <files>\n\n"
         "Options:\n"
         "  -c             Don't create files\n"
         "  -r <ref_file>  Use time and date of <ref_file>\n", stderr);
  exit (1);
}


int main (int argc, char *argv[])
{
  int c, i, h, rc;
  int dont_create;
  const char *ref_fname;
  struct utimbuf *ut;
  char *s;

  _response (&argc, &argv);
  _wildcard (&argc, &argv);
  if (argc < 2)
    usage ();
  dont_create = FALSE; ref_fname = NULL; opterr = FALSE;
  while ((c = getopt (argc, argv, "cr:")) != -1)
    switch (c)
      {
      case 'c':
        dont_create = TRUE;
        break;
      case 'r':
        ref_fname = optarg;
        break;
      default:
        usage ();
      }
  if (optind >= argc)
    usage ();

  if (ref_fname == NULL)
    ut = NULL;
  else
    {
      struct stat st;
      static struct utimbuf ut_ref;

      if (stat (ref_fname, &st) != 0)
        {
          perror (ref_fname);
          exit (1);
        }
      ut_ref.actime = st.st_atime;
      ut_ref.modtime = st.st_mtime;
      ut = &ut_ref;
    }

  rc = 0;
  for (i = optind; i < argc; ++i)
    {
      s = argv[i];
      if (utime (s, ut) != 0 && !dont_create)
        {
          h = open (s, O_CREAT|O_EXCL|O_WRONLY|O_BINARY, S_IREAD|S_IWRITE);
          if (h < 0)
            {
              perror (s);
              rc = 1;
            }
          else
            {
              close (h);
              if (ut != NULL && utime (s, ut) != 0)
                {
                  perror (s);
                  rc = 1;
                }
            }
        }
    }
  return rc;
}
