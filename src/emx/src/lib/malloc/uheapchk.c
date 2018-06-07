/* uheapchk.c (emx+gcc) -- Copyright (c) 1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdlib.h>
#include <stddef.h>
#include <umalloc.h>
#include <sys/builtin.h>        /* For <sys/fmutex.h> */
#include <sys/fmutex.h>
#include <emx/umalloc.h>

struct _um_check
{
  unsigned free;
  unsigned crates;
};

static int _walk_check (Heap_t h, const void *obj, size_t size, int flag,
                        int status, const char *fname, size_t lineno,
                        void *arg)
{
  struct _um_check *check = (struct _um_check *)arg;

  if (status == _HEAPOK)
    {
      if (flag == _USEDENTRY && (h->type & _HEAP_TILED)
          && !_UM_TILE_OK (obj, size))
        return _HEAPBADNODE;
      if (flag == _FREEENTRY)
        check->free += 1;
    }
  return status;
}


static int _check_bucket (Heap_t h, int bucket_num, struct _um_check *check)
{
  struct _um_lump *lump, *prev_lump;
  struct _um_seg *seg;
  size_t min_size, max_size, rsize, i;

  min_size = _UM_BUCKET_SIZE (bucket_num);
  max_size = bucket_num == _UM_BUCKETS - 1 ? _UM_MAX_SIZE : min_size * 2 - 1;

  prev_lump = NULL; i = 0;
  for (lump = h->buckets[bucket_num].head;
       lump != NULL; lump = lump->x.free.next)
    {
      if (++i > h->scratch1)    /* Avoid infinite loop */
        return _UM_WALK_ERROR (_HEAPBADEND);
      if (_UM_LUMP_STATUS (lump) != _UMS_FREE)
        return _UM_WALK_ERROR (_HEAPBADNODE);
      seg = _PTR_FROM_UMINT (lump->parent_seg, struct _um_seg);
      if (seg->magic != _UM_MAGIC_SEG)
        return _UM_WALK_ERROR (_HEAPBADNODE);
      if (seg->parent_heap != h)
        return _UM_WALK_ERROR (_HEAPBADNODE);
      if (lump->x.free.prev != prev_lump)
        return _UM_WALK_ERROR (_HEAPBADNODE);
      rsize = _UM_ROUND_LUMP (lump->size);
      if (rsize < min_size || rsize > max_size)
        return _UM_WALK_ERROR (_HEAPBADNODE);
      check->free += 1;
      prev_lump = lump;
    }
  if (h->buckets[bucket_num].tail != prev_lump)
    return _UM_WALK_ERROR (_HEAPBADNODE);
  return _HEAPOK;
}


static int _check_crateset (Heap_t h, int set_num, struct _um_check *check)
{
  struct _um_crateset *crateset;
  struct _um_crumb *crumb, *prev_crumb;
  struct _um_crate *crate;
  size_t i;

  crateset = &h->cratesets[set_num];
  prev_crumb = NULL; i = 0;
  for (crumb = crateset->crumb_head; crumb != NULL; crumb = crumb->x.free.next)
    {
      if (++i > h->max_crumbs)  /* Avoid infinite loop */
        return _UM_WALK_ERROR (_HEAPBADEND);
      if (_UM_CRUMB_STATUS (crumb) != _UMS_FREE)
        return _UM_WALK_ERROR (_HEAPBADNODE);
      crate = _PTR_FROM_UMINT (crumb->x.free.parent_crate, struct _um_crate);
      if (crate->magic != _UM_MAGIC_CRATE)
        return _UM_WALK_ERROR (_HEAPBADNODE);
      if (crate->parent_crateset != crateset)
        return _UM_WALK_ERROR (_HEAPBADNODE);
      if (crumb->x.free.prev != prev_crumb)
        return _UM_WALK_ERROR (_HEAPBADNODE);
      check->free += 1;
      prev_crumb = crumb;
    }
  if (crateset->crumb_tail != prev_crumb)
    return _UM_WALK_ERROR (_HEAPBADNODE);

  i = 0;
  for (crate = crateset->crate_head; crate != NULL; crate = crate->next)
    {
      if (++i > h->n_crates)
        return _UM_WALK_ERROR (_HEAPBADEND);
    }
  check->crates += i;

  /* Only the head of the crate list may have INIT < MAX. */

  if (crateset->crate_head != NULL)
    {
      for (crate = crateset->crate_head->next;
           crate != NULL; crate = crate->next)
        if (crate->init < crate->max)
          return _UM_WALK_ERROR (_HEAPBADNODE);
    }

  return _HEAPOK;
}


int _uheapchk (Heap_t h)
{
  int ret, i;
  struct _um_check pass1, pass2;

  if (h == NULL)
    return _HEAPEMPTY;
  if (h->magic != _UM_MAGIC_HEAP)
    return _HEAPBADBEGIN;

  _um_heap_lock (h);
  pass1.free = 0; pass1.crates = 0;
  pass2.free = 0; pass2.crates = 0;
  ret = _um_walk_no_lock (h, _walk_check, &pass1);
  pass1.crates = h->scratch1;

  if (ret == _HEAPOK)
    for (i = 0; i < _UM_BUCKETS; ++i)
      {
        h->scratch1 = pass1.free;
        ret = _check_bucket (h, i, &pass2);
        if (ret != _HEAPOK)
          break;
      }

  if (ret == _HEAPOK)
    for (i = 0; i < h->n_cratesets; ++i)
      {
        ret = _check_crateset (h, i, &pass2);
        if (ret != _HEAPOK)
          break;
      }

  if (pass1.free != pass2.free)
    ret = _UM_WALK_ERROR (_HEAPBADNODE);
  if (pass1.crates != h->n_crates || pass2.crates != h->n_crates)
    ret = _UM_WALK_ERROR (_HEAPBADEND);

  _um_heap_unlock (h);
  return ret;
}
