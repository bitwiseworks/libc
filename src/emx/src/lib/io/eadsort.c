/* eadsort.c (emx+gcc) -- Copyright (c) 1993-1995 by Eberhard Mattes */

#include "libc-alias.h"
#define INCL_DOSFILEMGR
#include <os2.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/ead.h>
#include "ea.h"

static int _ead_compare (const void *x1, const void *x2)
{
  return strcmp ((*((PFEA2 *)x1))->szName, ((*(PFEA2 *)x2))->szName);
}


void _ead_sort (_ead ead)
{
  if (ead->count >= 2)
    qsort (ead->index, ead->count, sizeof (ead->index[0]), _ead_compare);
}
