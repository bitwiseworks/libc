/* defshrink.c (emx+gcc) -- Copyright (c) 1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdlib.h>
#include <stddef.h>
#include <umalloc.h>
#include <sys/builtin.h>        /* For <sys/fmutex.h> */
#include <sys/fmutex.h>
#include <emx/umalloc.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_HEAP
#include <InnoTekLIBC/logstrict.h>

void _um_default_shrink (Heap_t h, void *memory, size_t old_size,
                         size_t *new_size)
{
  LIBCLOG_ENTER("h=%p memory=%p old_size=%d new_size=%p:{%d}\n", (void *)h, memory, old_size, (void *)new_size, *new_size);
  void *cur;

  cur = sbrk (0);
  if (_UM_ADD (memory, old_size) != cur
      || sbrk ((int)(*new_size - old_size)) == (void *)-1)
    *new_size = old_size;
  LIBCLOG_RETURN_MSG_VOID("*new_size=%d\n", *new_size);
}
