/* theapmin.c (emx+gcc) -- Copyright (c) 1996-1998 by Eberhard Mattes */

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

int _theapmin (void)
{
  LIBCLOG_ENTER("\n");
  _UM_MT_DECL

  if (_UM_DEFAULT_TILED_HEAP == NULL)
    _um_init_default_tiled_heap ();
  int rc = _uheapmin (_UM_DEFAULT_TILED_HEAP);
  LIBCLOG_RETURN_INT(rc);
}
