/* ustats.c (emx+gcc) -- Copyright (c) 1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdlib.h>
#include <stddef.h>
#include <umalloc.h>
#include <sys/builtin.h>        /* For <sys/fmutex.h> */
#include <sys/fmutex.h>
#include <emx/umalloc.h>

static int _walk_stats (Heap_t h, const void *obj, size_t size, int flag,
                        int status, const char *fname, size_t lineno,
                        void *arg)
{
  _HEAPSTATS *stats;

  stats = (_HEAPSTATS *)arg;
  if (status != _HEAPOK)
    return status;
  if (flag == _USEDENTRY)
    stats->_used += size;
  else if (size > stats->_max_free)
    stats->_max_free = size;
  return status;
}



int _ustats (Heap_t h, _HEAPSTATS *stats)
{
  struct _um_seg *seg;
  int ret;

  stats->_provided = 0;
  stats->_used = 0;
  stats->_tiled = 0;
  stats->_shared = 0;
  stats->_max_free = 0;
  stats->_segments = h->n_segments;
  stats->_crates = h->n_crates;
  stats->_reserved1 = 0;
  stats->_reserved2 = 0;
  stats->_reserved3 = 0;
  stats->_reserved4 = 0;
  stats->_reserved5 = 0;
  stats->_reserved6 = 0;
  stats->_reserved7 = 0;
  stats->_reserved8 = 0;
  stats->_reserved9 = 0;

  if (h == NULL)                /* Uninitialized _um_runtime_heap! */
    return 0;

  _um_heap_lock (h);
  if (h->type & _HEAP_TILED) stats->_tiled = 1;
  if (h->type & _HEAP_SHARED) stats->_shared = 1;

  for (seg = h->seg_head; seg != NULL; seg = seg->next)
    stats->_provided += (char *)seg->end - (char *)seg->start;
  ret = _um_walk_no_lock (h, _walk_stats, (void *)stats);

  _um_heap_unlock (h);
  return ret;
}
