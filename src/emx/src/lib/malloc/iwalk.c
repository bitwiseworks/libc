/* iwalk.c (emx+gcc) -- Copyright (c) 1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdlib.h>
#include <stddef.h>
#include <umalloc.h>
#include <sys/builtin.h>        /* For <sys/fmutex.h> */
#include <sys/fmutex.h>
#include <emx/umalloc.h>

#ifndef NDEBUG
int _um_walk_error (int x)
{
  return x;
}
#endif

static int _um_crate_walk (struct _um_crate *crate, size_t size, Heap_t h,
                           _um_callback2 *callback, void *arg)
{
  int i, j, ret;
  struct _um_crate *crate2;
  struct _um_crumb *crumb;
  size_t n, used;

  if (crate->magic != _UM_MAGIC_CRATE || crate->parent_heap != h)
    return _UM_WALK_ERROR (_HEAPBADBEGIN);
  h->scratch1 += 1;             /* Count crates */
  for (i = 0; i < h->n_cratesets; ++i)
    if (h->cratesets[i].crumb_size == crate->crumb_size)
      break;
  if (i >= h->n_cratesets)
    return _UM_WALK_ERROR (_HEAPBADBEGIN);
  j = 0;
  for (crate2 = h->cratesets[i].crate_head;
       crate2 != NULL && crate2->magic == _UM_MAGIC_CRATE;
       crate2 = crate2->next)
    {
      if (++j > h->n_crates)    /* Avoid infinite loop */
        return _UM_WALK_ERROR (_HEAPBADEND);
      if (crate2 == crate)
        break;
    }
  if (crate2 != crate)
    return _UM_WALK_ERROR (_HEAPBADBEGIN);
  if (crate->used > crate->max || crate->init > crate->max)
    return _UM_WALK_ERROR (_HEAPBADBEGIN);
  n = (sizeof (struct _um_crate)
       + crate->max * (crate->crumb_size + _UM_CRUMB_OVERHEAD));
  if (n != size)
    return _UM_WALK_ERROR (_HEAPBADBEGIN);

  used = 0;
  for (i = 0; i < crate->init; ++i)
    {
      crumb = _UM_CRUMB_BY_INDEX (crate, i);
      if (_UM_CRUMB_STATUS (crumb) == _UMS_FREE)
        ret = callback (h, crumb, crate->crumb_size,
                        _FREEENTRY, _HEAPOK, NULL, 0, arg);
      else
        {
          ret = callback (h, crumb, crumb->x.used.size,
                          _USEDENTRY, _HEAPOK, NULL, 0, arg);
          ++used;
        }
      if (ret != 0)
        return _UM_WALK_ERROR (ret);
    }
  if (used != crate->used)
    return _UM_WALK_ERROR (_HEAPBADEND);

#if 0
  /* Don't forget to update _check_crateset() if you enable this
     code. */
  for (i = crate->init; i < crate->max; ++i)
    {
      crumb = _UM_CRUMB_BY_INDEX (crate, i);
      ret = callback (h, crumb, crate->crumb_size, _FREEENTRY, _HEAPOK,
                      NULL, 0, arg);
      if (ret != 0)
        return _UM_WALK_ERROR (ret);
    }
#endif
  return _HEAPOK;
}


static int _um_seg_walk (struct _um_seg *seg, Heap_t h, 
                         _um_callback2 *callback, void *arg)
{
  int ret;
  struct _um_lump *lump, *next;
  char *start;

  if (seg->magic != _UM_MAGIC_SEG || seg->parent_heap != h)
    return _UM_WALK_ERROR (_HEAPBADBEGIN);

  start = _UM_ADD (seg, sizeof (struct _um_seg));
  start += _UM_ALIGN_DIFF (start + _UM_LUMP_HEADER_SIZE, _UM_PAGE_SIZE);
  if (seg->start != (void *)start
      || (((_umint)seg->end - (_umint)seg->start)
          > seg->size - ((char *)seg->start - (char *)seg->mem))
      || ((_umint)seg->end - (_umint)seg->start) % _UM_PAGE_SIZE != 0
      || (_umint)seg->zero_limit < (_umint)seg->start
      || (_umint)seg->zero_limit > (_umint)seg->end
      || (((_umint)seg->zero_limit - (_umint)seg->start)
          % _UM_PAGE_SIZE) != 0)
    return _UM_WALK_ERROR (_HEAPBADBEGIN);

  /* TDOO: Check seg->mem */

  lump = (struct _um_lump *)seg->start;
  ret = _HEAPOK;
  while ((unsigned long)lump < (unsigned long)seg->end)
    {
      if (_PTR_FROM_UMINT (lump->parent_seg, struct _um_seg) != seg)
        return _UM_WALK_ERROR (_HEAPBADNODE);
      next = _UM_NEXT_LUMP (lump);
      if ((_umint)next < (_umint)seg->start
          || (_umint)next > (_umint)seg->end)
        return _UM_WALK_ERROR (_HEAPBADEND);
      if (lump->size != ((_umint *)next)[-1])
        return _UM_WALK_ERROR (_HEAPBADNODE);

      /* Check for two adjacent free lumps -- that must not happen. */

      if (_UM_LUMP_STATUS (lump) == _UMS_FREE
          && !_UM_LAST_LUMP (seg, lump)
          && _UM_LUMP_STATUS (next) == _UMS_FREE)
        return _UM_WALK_ERROR (_HEAPBADNODE);

      if (_UM_LUMP_STATUS (lump) == _UMS_CRATE)
        ret = _um_crate_walk ((struct _um_crate *)_UM_BLOCK_FROM_LUMP (lump),
                              lump->size, h, callback, arg);
      else
        ret = callback (h, _UM_BLOCK_FROM_LUMP (lump),
                        lump->size, (_UM_LUMP_STATUS (lump) == _UMS_FREE
                                     ? _FREEENTRY : _USEDENTRY),
                        _HEAPOK, NULL, 0, arg);
      if (ret != _HEAPOK)
        return _UM_WALK_ERROR (ret);
      lump = next;
    }
  if ((void *)lump != seg->end)
    return _UM_WALK_ERROR (_HEAPBADEND);
  return _HEAPOK;
}

  
int _um_walk_no_lock (Heap_t h, _um_callback2 *callback, void *arg)

{
  int ret, initial_count, i;
  struct _um_seg *seg;

  ret = _HEAPEMPTY; initial_count = 0; i = 0; h->scratch1 = 0;
  for (seg = h->seg_head; seg != NULL; seg = seg->next)
    {
      if (++i > h->n_segments)  /* Avoid infinite loop */
        return _UM_WALK_ERROR (_HEAPBADEND);
      if (seg == h->initial_seg)
        ++initial_count;
      ret = _um_seg_walk (seg, h, callback, arg);
      if (ret != _HEAPOK)
        return ret;
    }
  if (i != h->n_segments)
    return _UM_WALK_ERROR (_HEAPBADEND);
  if (initial_count != 1)
    return _UM_WALK_ERROR (_HEAPBADBEGIN);
  return ret;
}
