/* fnexplod.c (emx+gcc) -- Copyright (c) 1992-1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <emx/syscalls.h>


char **_fnexplode (const char *mask)
{
  char **list, **tmp, *p, *q;
  int a, n, i;
  struct _find find;
  struct stat st;
  char name[MAXNAMLEN];

  /* Avoid expanding a directory to all the files and directories it
     contains. */

  if (stat (mask, &st) == 0 && S_ISDIR (st.st_mode))
    return NULL;

  _strncpy (name, mask, sizeof (name));
  p = _getname (name);
  list = NULL; n = 0; a = 0;
  if (__findfirst (mask, 0x10, &find) != 0)
    return NULL;
  do
    {
      if (   find.szName[0] != '.'
          || (   find.szName[1] != '\0'
              && (find.szName[1] != '.' || find.szName[2] != '\0')))
        {
          if (n + 1 >= a)
            {
              a += 32;
              tmp = realloc (list, a * sizeof (char **));
              if (tmp == NULL)
                {
                  errno = ENOMEM;
                  goto failure;
                }
              list = tmp;
            }
          strcpy (p, find.szName);
          _fnlwr2 (name, name);
          q = strdup (name);
          if (q == NULL)
            {
              errno = ENOMEM;
              goto failure;
            }
          list[n++] = q;
        }
    } while (__findnext (&find) == 0);
  if (list != NULL)
    list[n] = NULL;
  return list;

failure:
  if (list != NULL)
    {
      for (i = 0; i < n; ++i)
        free (list[i]);
      free (list);
    }
  return NULL;
}


void _fnexplodefree (char **list)
{
  int i;

  for (i = 0; list[i] != NULL; ++i)
    free (list[i]);
  free (list);
}
