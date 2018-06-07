/* eadcopy.c (emx+gcc) -- Copyright (c) 1993-1995 by Eberhard Mattes */

#include "libc-alias.h"
#define INCL_DOSFILEMGR
#include <os2.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <errno.h>
#include <sys/ead.h>
#include "ea.h"

int _ead_copy (_ead dst_ead, _ead src_ead, int src_index)
{
  int i;
  const FEA2 *s;

  if (src_index == 0)
    for (i = 1; i <= src_ead->count; ++i)
      {
        s = src_ead->index[i-1];
        if (_ead_add (dst_ead, s->szName, s->fEA, s->szName + s->cbName + 1,
                      s->cbValue) < 0)
          return -1;
      }
  else if (src_index < 1 || src_index > src_ead->count)
    {
      errno = EINVAL;
      return -1;
    }
  else
    {
      s = src_ead->index[src_index-1];
      if (_ead_add (dst_ead, s->szName, s->fEA, s->szName + s->cbName + 1,
                    s->cbValue) < 0)
        return -1;
    }
  return 0;
}
