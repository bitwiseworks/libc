/* defexpan.c (emx+gcc) -- Copyright (c) 1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdlib.h>
#include <stddef.h>
#include <umalloc.h>
#include <sys/builtin.h>        /* For <sys/fmutex.h> */
#include <sys/fmutex.h>
#include <emx/umalloc.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_HEAP
#include <InnoTekLIBC/logstrict.h>

int _um_default_expand (Heap_t h, void *base, size_t old_size,
                        size_t *new_size, int *clean)
{
  LIBCLOG_ENTER("h=%p base=%p old_size=%d new_size=%p:{%d} clean=%p\n",
                (void *)h, base, old_size, (void *)new_size, *new_size, (void *)clean);
  void *p;
  size_t n;

  if (_UM_ADD (base, old_size) != sbrk (0))
    LIBCLOG_RETURN_INT(0);
  n = (*new_size - old_size + 0xffff) & ~0xffff;
  p = sbrk (n);
  if (p == (void *)-1)
    {
      n = *new_size - old_size;
      p = sbrk (n);
      if (p == (void *)-1)
        LIBCLOG_RETURN_INT(0);
    }
  if (p != _UM_ADD (base, old_size))
    {
      sbrk (-n);
      LIBCLOG_RETURN_INT(0);
    }
  *new_size = old_size + n; *clean = !_BLOCK_CLEAN;
  LIBCLOG_RETURN_INT(1);
}
