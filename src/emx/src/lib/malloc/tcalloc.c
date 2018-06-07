/* tcalloc.c (emx+gcc) -- Copyright (c) 1996 by Eberhard Mattes */

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

void *_tcalloc (size_t count, size_t size)
{
    LIBCLOG_ENTER("count=%d size=%d\n", count, size);
  _UM_MT_DECL

  if (_UM_DEFAULT_TILED_HEAP == NULL)
    _um_init_default_tiled_heap ();
  void *pv = _utcalloc (_UM_DEFAULT_TILED_HEAP, count, size);
  if (pv)
    LIBCLOG_RETURN_P(pv);
  LIBCLOG_ERROR_RETURN_P(pv);
}
