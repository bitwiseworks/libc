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

#define CHILD1 "spawnvpe-1-child.exe"
#define CHILD2 "spawnvpe-2-child.sh"
#define CHILD3 "spawnvpe-3-child.cmd"

int main (void)
{
  printf ("fibGetEnv     = %08lx (%s\\0...)\n", fibGetEnv (), fibGetEnv ());
  printf ("fibGetCmdLine = %08lx (%s\\0...)\n", fibGetCmdLine (), fibGetCmdLine ());

  char *argc [] = { NULL };
  char *envv [] = { "LIBC_LOGGING=-all+process+initterm", NULL };
  int i;
  int rc;

  rc = spawnvpe (P_WAIT, CHILD1, argc, envv);

  printf ("spawnvpe(child1) = %d (%d %s)\n", rc, errno, strerror (errno));

  rc = spawnvpe (P_WAIT, CHILD2, argc, envv);

  printf ("spawnvpe(child2) = %d (%d %s)\n", rc, errno, strerror (errno));

  rc = spawnvpe (P_WAIT, CHILD3, argc, envv);

  printf ("spawnvpe(child3) = %d (%d %s)\n", rc, errno, strerror (errno));

  return rc;
}
