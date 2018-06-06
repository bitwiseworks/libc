/* trealloc.c (emx+gcc) -- Copyright (c) 1996 by Eberhard Mattes */

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

void *_trealloc (void *block, size_t new_size)
{
  LIBCLOG_ENTER("block=%p new_size=%d\n", block, new_size);
  _UM_MT_DECL

  if (_UM_DEFAULT_TILED_HEAP == NULL)
    _um_init_default_tiled_heap ();
  void *pvRet;
  if (block == NULL)
    pvRet = _tmalloc (new_size);
  else
    pvRet = _um_realloc (block, new_size, 4, _UMFI_TILED);
  if (pvRet || !new_size)
    LIBCLOG_RETURN_P(pvRet);
  LIBCLOG_ERROR_RETURN_P(pvRet);
}
