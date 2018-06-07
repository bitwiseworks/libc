/* mheap.c (emx+gcc) -- Copyright (c) 1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdlib.h>
#include <stddef.h>
#include <umalloc.h>
#include <sys/builtin.h>        /* For <sys/fmutex.h> */
#include <sys/fmutex.h>
#include <emx/umalloc.h>

Heap_t _mheap (const void *block)
{
  struct _um_hdr *hdr;
  _umagic *parent;

  if (block == NULL)
    return NULL;

  hdr = _HDR_FROM_BLOCK (block);
  if (_UM_HDR_STATUS (hdr) == _UMS_FREE)
    return NULL;
  parent = _PTR_FROM_UMINT (hdr->parent, _umagic);
  switch (*parent)
    {
    case _UM_MAGIC_CRATE:
      return ((struct _um_crate *)parent)->parent_heap;
    case _UM_MAGIC_SEG:
      return ((struct _um_seg *)parent)->parent_heap;
    default:
      return NULL;
    }
}
