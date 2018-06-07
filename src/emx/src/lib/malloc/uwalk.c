/* uwalk.c (emx+gcc) -- Copyright (c) 1996-2000 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdlib.h>
#include <stddef.h>
#include <umalloc.h>
#include <sys/builtin.h>        /* For <sys/fmutex.h> */
#include <sys/fmutex.h>
#include <emx/umalloc.h>

static int _um_walk_walk1 (Heap_t h, const void *obj, size_t size, int flag,
                           int status, const char *fname, size_t lineno,
                           void *arg)
{
  return (*(_um_callback1 **)arg)(obj, size, flag, status, fname, lineno);
}


int _uheap_walk (Heap_t h, _um_callback1 *callback)
{
  return _uheap_walk2 (h, _um_walk_walk1, &callback);
}
