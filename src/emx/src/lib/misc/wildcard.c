/* wildcard.c (emx+gcc) -- Copyright (c) 1990-1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <emx/syscalls.h>
#include <klibc/startup.h>

#define WPUT(x) do { \
    if (new_argc >= new_alloc) \
      { \
        new_alloc += 20; \
        new_argv = (char **)realloc (new_argv, new_alloc * sizeof (char *)); \
        if (new_argv == NULL) \
            goto out_of_memory; \
      } \
    new_argv[new_argc++] = x; \
  } while (0)



void _wildcard (int *argcp, char ***argvp)
{
  int i, old_argc, new_argc, new_alloc;
  char **old_argv, **new_argv;
  char line[256], *p, *q;
  struct _find find;

  old_argc = *argcp; old_argv = *argvp;
  _rfnlwr ();
  for (i = 1; i < old_argc; ++i)
    if (old_argv[i] != NULL &&
        !(old_argv[i][-1] & (__KLIBC_ARG_DQUOTE | __KLIBC_ARG_RESPONSE | __KLIBC_ARG_ARGV | __KLIBC_ARG_SHELL)) &&
        strpbrk (old_argv[i], "?*") != NULL)
      break;
  if (i >= old_argc)
    return;                 /* do nothing */
  new_argv = NULL; new_alloc = 0; new_argc = 0;
  for (i = 0; i < old_argc; ++i)
    {
      if (i == 0 || old_argv[i] == NULL
          || (old_argv[i][-1] & (__KLIBC_ARG_DQUOTE | __KLIBC_ARG_RESPONSE | __KLIBC_ARG_ARGV | __KLIBC_ARG_SHELL))
          || strpbrk (old_argv[i], "?*") == NULL
          || __findfirst (old_argv[i], 0x10, &find) != 0)
        WPUT (old_argv[i]);
      else
        {
          line[0] = __KLIBC_ARG_NONZERO | __KLIBC_ARG_WILDCARD;
          strcpy (line+1, old_argv[i]);
          p = _getname (line + 1);
          do
            {
              if (   find.szName[0] != '.'
                  || (   find.szName[1] != '\0'
                      && (find.szName[1] != '.' || find.szName[2] != '\0')))
                {
                  strcpy (p, find.szName);
                  _fnlwr2 (p, line+1);
                  q = strdup (line);
                  if (q == NULL)
                    goto out_of_memory;
                  WPUT (q+1);
                }
            } while (__findnext (&find) == 0);
        }
    }
  WPUT (NULL); --new_argc;
  *argcp = new_argc; *argvp = new_argv;
  _rfnlwr ();
  return;

out_of_memory:
  fputs ("Out of memory while expanding wildcards\n", stderr);
  exit (255);
}
