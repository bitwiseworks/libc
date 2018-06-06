/* fcloseal.c (emx+gcc) -- Copyright (c) 1990-1998 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdio.h>
#include <emx/io.h>

int _fcloseall (void)
{
  int i, n;
  char ok;
  struct streamvec *sv;

  n = 0; ok = 1;
  for (sv = _streamvec_head; sv != NULL; sv = sv->pNext)
    {
      for (i = 0; i < sv->cFiles; ++i)
        if ((sv->aFiles[i]._flags & (_IOOPEN | _IONOCLOSEALL)) == _IOOPEN)
          {
            if (fclose (&sv->aFiles[i]) == 0)
              ++n;
            else
              ok = 0;
          }
    }
  return (ok ? n : EOF);
}
