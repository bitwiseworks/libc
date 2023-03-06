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
#include <string.h>
#include <stdlib.h>

#include <InnoTekLIBC/FastInfoBlocks.h>

int main (int argc, char *argv [], char *envp [])
{
  int arglen, envlen;
  int i;

  printf ("(child) fibGetEnv     = %08lx (%s\\0...)\n", fibGetEnv (), fibGetEnv ());
  printf ("(child) fibGetCmdLine = %08lx (%s\\0...)\n", fibGetCmdLine (), fibGetCmdLine ());

  arglen = 1;
  for (i = 0; i < argc; ++ i)
  {
    int len = strlen (argv [i]);
    printf ("argv[%d] = \"%.*s\"%s (%d)\n", i, 32, argv [i], (len > 32 ? "..." : ""), len);
    arglen += len + 1;
  }

  envp = environ;
  envlen = 1;
  for (i = 0; envp [i]; ++ i)
  {
    int len = strlen (envp [i]);
    printf ("envv[%d] = \"%.*s\"%s (%d)\n", i, 32, envp [i], (len > 32 ? "..." : ""), len);
    envlen += len + 1;
  }

  printf ("Total length of environment passed from the parent = %d\n", envlen);
  printf ("Total length of arguments passed from the parent   = %d\n", arglen);

  return 0;
}
