/* ucreate.c (emx+gcc) -- Copyright (c) 1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdlib.h>
#include <stddef.h>
#include <umalloc.h>
#include <sys/builtin.h>        /* For <sys/fmutex.h> */
#include <sys/fmutex.h>
#include <emx/umalloc.h>

Heap_t _ucreate (void *memory, size_t size, int clean, unsigned type,
                 void *(*alloc_fun)(Heap_t, size_t *, int *),
                 void (*release_fun)(Heap_t, void *, size_t))
{
  return _ucreate2 (memory, size, clean, type,
                    alloc_fun, release_fun, NULL, NULL);
}
