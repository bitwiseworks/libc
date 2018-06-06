/* fnlwr.c (emx+gcc) -- Copyright (c) 1992-1996 by Eberhard Mattes */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static char cache_curdrive = 0;
static char cache_filesys[26] =
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

#define DIRSEP_P(c) ((c) == '\\' || (c) == '/')

void _fnlwr2 (char *name, const char *base)
{
  char c, dr[3], fs[16];
  int i;

  /* Do not translate UNC paths to lower case. */

  if (DIRSEP_P (base[0]) && DIRSEP_P (base[1]) && !DIRSEP_P (base[2]))
    return;

  c = _fngetdrive (base);
  if (c == 0)
    {
      if (cache_curdrive == 0)
        cache_curdrive = _getdrive ();
      c = cache_curdrive;
    }
  else if (name == base)
    name[0] = (char)(c - 'A' + 'a');
  i = c - 'A';
  if (cache_filesys[i] == 0)
    {
      dr[0] = c;
      dr[1] = ':';
      dr[2] = 0;
      if (_filesys (dr, fs, sizeof (fs)) != 0)
        return;
      if (strcmp (fs, "FAT") == 0 || strcmp (fs, "CDFS") == 0)
        cache_filesys[i] = 'u';     /* upper-casing */
      else
        cache_filesys[i] = 'p';     /* case-preserving, perhaps */
    }
  if (cache_filesys[i] == 'u')
    strlwr (name);
}


void _fnlwr (char *name)
{
  _fnlwr2 (name, name);
}


void _rfnlwr (void)
{
  int i;

  for (i = 0; i < 26; ++i)
    cache_filesys[i] = 0;
  cache_curdrive = 0;
}


void _sfnlwr (const char *name)
{
  cache_curdrive = _fngetdrive (name);
  if (cache_curdrive == 0)
    cache_curdrive = _getdrive ();
}
