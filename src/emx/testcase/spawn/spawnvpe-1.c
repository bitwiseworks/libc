/*
 * spawnvpe() test program
 *
 * Copyright (C) 2023 Dmitriy Kuminov <coding@dmik.org>
 * Copyright (C) 2014 KO Myung-Hun <komh@chollian.net>
 *
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://www.wtfpl.net/ for more details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <process.h>

#include <InnoTekLIBC/FastInfoBlocks.h>

#define CHILD "spawnvpe-1-child.exe"

#define ENVC    20
#define ENVLEN  (4 * 1020)

#define ARGC    20
#define ARGLEN  (4 * 1021)

int main (void)
{
  printf ("fibGetEnv     = %08lx (%s\\0...)\n", fibGetEnv (), fibGetEnv ());
  printf ("fibGetCmdLine = %08lx (%s\\0...)\n", fibGetCmdLine (), fibGetCmdLine ());

  char *argv [ARGC + 1 + 1] = { CHILD };
  char *envv [ENVC + 1 + 1] = { "LIBC_LOGGING=-all+process+initterm" };
  int i;
  int rc;

  for (i = 1; i <= ARGC; ++ i)
  {
    argv [i] = calloc (1, ARGLEN + 1);
    memset (argv [i], '0' + i, ARGLEN);
  }
  argv [ARGC + 1] = NULL;

  for (i = 1; i <= ENVC; ++ i)
  {
    envv [i] = calloc (1, 6 + ENVLEN + 1);
    sprintf (envv [i], "ENV%02d=", i);
    memset (envv [i] + 6, '0' + i, ENVLEN);
  }
  envv [ENVC + 1] = NULL;

  rc = spawnvpe (P_WAIT, CHILD, argv, envv);

  for (i = 1; i <= ENVC; i++)
    free (envv [i]);

  for (i = 1; i <= ARGC; i++)
    free (argv [i]);

  printf ("Total length of environment passed to the child = %d\n",
          ENVC >= 0 ? (strlen (envv [0]) + 1 + ENVC * (6 + ENVLEN + 1) + 1) : 1);
  printf ("Total length of arguments passed to the child   = %d\n",
          ARGC >= 0 ? (strlen (argv [0]) + 1 + ARGC * (ARGLEN + 1) + 1) : 1);
  printf ("spawnvpe() = %d (%d %s)\n", rc, errno, strerror (errno));

  return rc;
}
