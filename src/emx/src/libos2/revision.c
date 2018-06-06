/* revision.c (emx+gcc) -- Copyright (c) 1993-1999 by Eberhard Mattes */

#define INCL_REXXSAA
#include <os2emx.h>

static char const revision[] = "60";

ULONG emx_revision (PCSZ name, LONG argc, const RXSTRING *argv,
                    PCSZ queuename, PRXSTRING retstr);

ULONG emx_revision (PCSZ name, LONG argc, const RXSTRING *argv,
                    PCSZ queuename, PRXSTRING retstr)
{
  int i;

  if (argc != 0)
    return 1;
  for (i = 0; revision[i] != 0; ++i)
    retstr->strptr[i] = revision[i];
  retstr->strlength = i;
  return 0;
}
