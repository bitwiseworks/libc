/* uaddmem.c (emx+gcc) -- Copyright (c) 1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdlib.h>
#include <stddef.h>
#include <umalloc.h>
#include <sys/builtin.h>        /* For <sys/fmutex.h> */
#include <sys/fmutex.h>
#include <emx/umalloc.h>

Heap_t _uaddmem (Heap_t h, void *memory, size_t size, int clean)
{
  Heap_t r;

  _um_heap_lock (h);
  r = _um_addmem_nolock (h, memory, size, clean);
  _um_heap_unlock (h);
  return r;
}
