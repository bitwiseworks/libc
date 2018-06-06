/* calloc.c (emx+gcc) -- Copyright (c) 1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdlib.h>
#include <stddef.h>
#include <umalloc.h>
#include <sys/builtin.h>        /* For <sys/fmutex.h> */
#include <sys/fmutex.h>
#include <emx/umalloc.h>
#include <InnoTekLIBC/thread.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_HEAP
#include <InnoTekLIBC/logstrict.h>

void *_STD(calloc) (size_t count, size_t size)
{
  LIBCLOG_ENTER("count=%d size=%d\n", count, size);
  _UM_MT_DECL
  Heap_t    heap_reg = _UM_DEFAULT_REGULAR_HEAP;

  if (heap_reg == NULL)
    heap_reg = _um_init_default_regular_heap ();
  void *pv = _ucalloc (heap_reg, count, size);
  if (pv)
      LIBCLOG_RETURN_P(pv);
  LIBCLOG_ERROR_RETURN_P(NULL);
}
