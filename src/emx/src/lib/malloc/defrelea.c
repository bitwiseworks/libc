/* defrelea.c (emx+gcc) -- Copyright (c) 1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdlib.h>
#include <stddef.h>
#include <umalloc.h>
#include <sys/builtin.h>        /* For <sys/fmutex.h> */
#include <sys/fmutex.h>
#include <emx/umalloc.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_HEAP
#include <InnoTekLIBC/logstrict.h>

void _um_default_release (Heap_t h, void *memory, size_t size)
{
  LIBCLOG_ENTER("h=%p memory=%p size=%d\n", (void *)h, memory, size);
  void *cur;

  cur = sbrk (0);
  if (_UM_ADD (memory, size) == cur)
    sbrk (-size);
  LIBCLOG_RETURN_VOID();
}
