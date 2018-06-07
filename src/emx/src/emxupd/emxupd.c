/* emxupd.c -- Update a DLL or EXE which is in use
   Copyright (c) 1996 Eberhard Mattes

This file is part of emxupd.

emxupd is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

emxupd is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with emxupd; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */


#define INCL_DOSERRORS
#define INCL_DOSFILEMGR
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <os2.h>

APIRET APIENTRY DosReplaceModule (PCSZ pszOld, PCSZ pszNew, PCSZ pszBackup);


static void usage (void)
{
  puts ("Usage: emxupd <source_file> <target_path>\n"
        "       emxupd -d <old_file>\n"
        "Options:\n"
        "  -d   Delete the file");
  exit (1);
}


static void replace_module (const char *old_path, const char *new_path)
{
  ULONG rc;

  /* First try to delete or copy the file the standard way.  Note that
     DosReplaceModule works only if the file is really in use. */

  if (new_path == NULL)
    rc = DosDelete (old_path);
  else
    rc = DosCopy (new_path, old_path, DCPY_EXISTING | DCPY_FAILEAS);

  /* If deleting or copying the file failed due to the module being in
     use, call DosReplaceModule. */

  if (rc == ERROR_SHARING_VIOLATION)
    {
      rc = DosReplaceModule (old_path, new_path, NULL);
      if (rc == 0 && new_path == NULL)
        rc = DosDelete (old_path);
    }

  /* Examine the return code. */

  switch (rc)
    {
    case 0:
      return;
    case ERROR_FILE_NOT_FOUND:
      puts ("File not found.");
      exit (2);
    case ERROR_PATH_NOT_FOUND:
      puts ("Path not found.");
      exit (2);
    case ERROR_MODULE_IN_USE:
      puts ("Module is in use.");
      exit (2);
    default:
      printf ("Operation failed, error code = %lu.\n", rc);
      exit (2);
    }
}


#define PATH_DELIM_P(c) ((c) == ':' || (c) == '\\' || (c) == '/')

static void update (const char *source_path, const char *target_path)
{
  ULONG rc;
  FILESTATUS3 info;
  size_t len;
  const char *last;
  char concat[CCHMAXPATH];

  /* If TARGET_PATH is a directory, append the last component of
     SOURCE_PATH to TARGET_PATH. */

  rc = DosQueryPathInfo (target_path, FIL_STANDARD, &info, sizeof (info));
  if (rc == 0 && (info.attrFile & FILE_DIRECTORY))
    {
      len = strlen (target_path);
      last = _getname (source_path);
      if (len != 0 && PATH_DELIM_P (target_path[len-1]))
        len -= 1;
      if (len + strlen (last) + 1 > sizeof (concat))
        {
          printf ("Path name too long\n");
          exit (2);
        }
      memcpy (concat, target_path, len);
      concat[len] = '\\';
      strcpy (concat + len + 1, last);
      target_path = concat;
    }

  /* Replace the module. */

  replace_module (target_path, source_path);
  printf ("\"%s\" replaced with \"%s\".\n", target_path, source_path);
}


static void delete (const char *old_path)
{
  replace_module (old_path, NULL);
  printf ("\"%s\" deleted.\n", old_path);
}


int main (int argc, char *argv[])
{
  int c;
  char delete_flag = FALSE;

  /* This check is not required as we use LINK386 for linking. */

  if (_osmode != OS2_MODE)
    exit (2);

  /* Parse the command line options. */

  while ((c = getopt (argc, argv, "d")) != -1)
    switch (c)
      {
      case 'd':
        delete_flag = 1;
        break;
      default:
        usage ();
      }

  if (delete_flag && argc - optind == 1)
    delete (argv[optind+0]);
  else if (!delete_flag && argc - optind == 2)
    update (argv[optind+0], argv[optind+1]);
  else
    usage ();
  return 0;
}
