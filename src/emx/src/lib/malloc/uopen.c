/* uopen.c (emx+gcc) -- Copyright (c) 1996-1997 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdlib.h>
#include <stddef.h>
#include <umalloc.h>
#include <sys/builtin.h>        /* For <sys/fmutex.h> */
#include <sys/fmutex.h>
#include <emx/umalloc.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_HEAP
#include <InnoTekLIBC/logstrict.h>

int _uopen (Heap_t h)
{
  LIBCLOG_ENTER("h=%p\n", (void *)h);
  if (h->magic != _UM_MAGIC_HEAP
      || h == _um_regular_heap || h == _um_tiled_heap)
    LIBCLOG_ERROR_RETURN_INT(-1);
  if (_fmutex_open (&h->fsem) != 0)
    LIBCLOG_ERROR_RETURN_INT(-1);
  LIBCLOG_RETURN_INT(0);
}
