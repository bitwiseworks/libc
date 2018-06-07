/* envargs.c (emx+gcc) -- Copyright (c) 1993-1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdlib.h>
#include <string.h>
#include <emx/startup.h>

void _envargs (int *argcp, char ***argvp, const char *name)
{
  char *str, *p, **args, **argv, **new_argv;
  int argc, i, n, len;

  str = getenv (name);
  if (str == NULL || *str == 0)
    return;
  str = strdup (str);
  if (str == NULL)
    return;
  args = _splitargs (str, &n);
  if (args == NULL || n == 0)
    return;
  argc = *argcp; argv = *argvp;
  new_argv = malloc ((argc + n + 1) * sizeof (char *));
  if (new_argv == NULL)
    return;
  new_argv[0] = argv[0];
  for (i = 0; i < n; ++i)
    {
      len = strlen (args[i]);
      p = malloc (len + 2);
      if (p == NULL)
        return;
      p[0] = _ARG_NONZERO|_ARG_ENV;
      memcpy (p + 1, args[i], len + 1);
      new_argv[1+i] = p + 1;
    }
  for (i = 1; i < argc; ++i)
    new_argv[n+i] = argv[i];
  new_argv[n+argc] = NULL;
  free (str);
  free (args);
  *argcp = argc + n;
  *argvp = new_argv;
}
