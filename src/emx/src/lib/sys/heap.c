/* sys/heap.c (emx+gcc) -- Copyright (c) 1996 by Eberhard Mattes */

#include "libc-alias.h"
#define INCL_DOSERRORS
#define INCL_FSMACROS
#define INCL_EXAPIS
#include <os2emx.h>
#include <errno.h>
#include <sys/uflags.h>
#include <emx/syscalls.h>
#include "syscalls.h"
#define  __LIBC_LOG_GROUP __LIBC_LOG_GRP_HEAP
#include <InnoTekLIBC/logstrict.h>


/* Allocate a memory object of SIZE bytes which is not located below
   MIN_ADDR.  Return the address of the memory object.  Return 0 on
   error. */

static ULONG alloc_above (ULONG size, ULONG min_addr)
{
  ULONG ret;
  void *p;
  FS_VAR();

  FS_SAVE_LOAD();
  if (DosAllocMemEx (&p, size, PAG_READ | PAG_WRITE | OBJ_FORK) != 0)
    {
      FS_RESTORE();
      return 0;
    }
  if ((ULONG)p > min_addr)
    {
      FS_RESTORE();
      return (ULONG)p;
    }

  /* The memory object is located below MIN_ADDR.  Recurse until we
     get an appropriate object or until we run out of memory. */

  ret = alloc_above (size, min_addr);

  /* We got a suitable object.  Free the bad one. */

  DosFreeMemEx (p);
  FS_RESTORE();
  return ret;
}


/* Expand the top heap object by INCR bytes.  INCR is a positive
   number.  Return the previous break address or 0 (error). */

ULONG _sys_expand_heap_obj_by (ULONG incr)
{
  LIBCLOG_ENTER("incr=%ld\n", incr);
  ULONG rc, old_brk, new_brk, addr, size, rest;

  old_brk = _sys_top_heap_obj->brk;
  new_brk = old_brk + incr;

  if (new_brk < _sys_top_heap_obj->base || new_brk > _sys_top_heap_obj->end)
    LIBCLOG_RETURN_ULONG(0UL);

  addr = old_brk;
  size = incr;
  if (addr & 0xfff)
    {
      rest = 0x1000 - (addr & 0xfff);
      if (rest >= size)
        size = 0;
      else
        {
          size -= rest;
          addr += rest;
        }
    }
  if (size != 0)
    {
      FS_VAR();
      FS_SAVE_LOAD();
      rc = DosSetMem ((void *)addr, size, PAG_DEFAULT | PAG_COMMIT);
      FS_RESTORE();
      if (rc != 0)
        LIBCLOG_RETURN_ULONG(0UL);
    }
  _sys_top_heap_obj->brk = new_brk;
  LIBCLOG_RETURN_ULONG(old_brk);
}


/* Shrink the top heap object by DECR bytes.  DECR is a positive
   number.  Return the previous break address or 0 (error). */

ULONG _sys_shrink_heap_obj_by (ULONG decr)
{
  LIBCLOG_ENTER("decr=%ld\n", decr);
  ULONG rc, old_brk, new_brk, addr, high;

  old_brk = _sys_top_heap_obj->brk;
  new_brk = old_brk - decr;

  if (new_brk < _sys_top_heap_obj->base || new_brk > _sys_top_heap_obj->end)
    LIBCLOG_RETURN_ULONG(0UL);

  addr = (new_brk + 0xfff) & ~0xfff;
  high = (_sys_top_heap_obj->brk + 0xfff) & ~0xfff;
  if (high > addr)
    {
      FS_VAR();
      FS_SAVE_LOAD();
      rc = DosSetMem ((void *)addr, high - addr, PAG_DECOMMIT);
      FS_RESTORE();
      if (rc != 0)
        LIBCLOG_RETURN_ULONG(0UL);
    }
  _sys_top_heap_obj->brk = new_brk;
  LIBCLOG_RETURN_ULONG(old_brk);
}


/* Expand the heap by INCR bytes.  INCR is a positive number.  Return
   the base address of the expansion or 0 (error). */

ULONG _sys_expand_heap_by (ULONG incr, ULONG sbrk_model)
{
  LIBCLOG_ENTER("incr=%ld sbrk_model=%ld\n", incr, sbrk_model);
  unsigned old_obj_count;
  ULONG base, size;

  old_obj_count = _sys_heap_obj_count;

  /* Allocate the first object if not yet done. */

  if (_sys_heap_obj_count == 0)
    {
      size = _sys_heap_size;
      if (incr > size)
        size = (incr + 0xffff) & ~0xffff;
      base = alloc_above (size, 0);
      if (base == 0)
        return 0;
      _sys_heap_objs[0].base = base;
      _sys_heap_objs[0].brk = base;
      _sys_heap_objs[0].end = base + size;
      _sys_heap_obj_count = 1;
      _sys_top_heap_obj = &_sys_heap_objs[0];
    }

  /* Now we have at least one heap object.  Check for arithmetic
     overflow. */

  if (_sys_top_heap_obj->brk + incr < _sys_top_heap_obj->base)
    {
      /* Overflow.  If we've just allocated the first heap object,
         deallocate it again unless memory must be allocated
         contiguously. */

      if (old_obj_count == 0 && sbrk_model != _UF_SBRK_CONTIGUOUS)
        {
          FS_VAR();
          FS_SAVE_LOAD();
          DosFreeMemEx ((void *)_sys_heap_objs[0].base);
          FS_RESTORE();
          _sys_heap_obj_count = 0;
          _sys_top_heap_obj = NULL;
        }
      LIBCLOG_RETURN_ULONG(0UL);  /* Failure */
    }

  /* Check whether we need another heap object.  Allocate one if we
     need one and are allowed to allocate one.  This should not happen
     if we just allocated the first one above. */

  if (_sys_top_heap_obj->brk + incr > _sys_top_heap_obj->end)
    {
      /* We need another heap object.  Fail if we are not allowed to
         allocate non-contiguous memory or if we already have the
         maximum number of heap objects. */

      if (sbrk_model == _UF_SBRK_CONTIGUOUS
          || _sys_heap_obj_count >= MAX_HEAP_OBJS)
        LIBCLOG_RETURN_ULONG(0UL);

      /* Allocate at least _sys_heap_size bytes.  The new object must
         be located above the currently top one. */

      size = _sys_heap_size;
      if (incr > size)
        size = (incr + 0xffff) & ~0xffff;

      for (;;)
        {
          if (sbrk_model == _UF_SBRK_ARBITRARY)
            base = alloc_above (size, 0);
          else
            base = alloc_above (size, _sys_top_heap_obj->end);
          if (base != 0)
            break;

          /* If we're out of virtual address space, halve the size and
             try again until allocation succeeds.  Of course, don't
             attempt to allocate less than INCR bytes. */

          size /= 2;
          if (size < incr)
            LIBCLOG_RETURN_ULONG(0UL);
        }

      _sys_top_heap_obj = &_sys_heap_objs[_sys_heap_obj_count++];
      _sys_top_heap_obj->base = base;
      _sys_top_heap_obj->brk = base;
      _sys_top_heap_obj->end = base + size;
    }

  /* Now _sys_top_heap_obj points to an object which has enough space.
     Commit memory as required.  _sys_expand_heap_obj() returns the
     top object's break address, which is the base address of the
     expansion as an object might have been added. */

  base = _sys_expand_heap_obj_by (incr);
  LIBCLOG_RETURN_ULONG(base);
}


/* Shrink the heap to the new break address NEW_BRK.  Return the
   previous break address or 0 (error). */

ULONG _sys_shrink_heap_to (ULONG new_brk)
{
  LIBCLOG_ENTER("new_brk=%ld\n", new_brk);
  unsigned obj;
  ULONG old_brk;

  /* Find the heap object containing the new break address.  Fail if
     there is no such heap object.  Note that the new break address
     must not be beyond the heap object current break address, that
     is, we cannot shrink the heap (by deallocating objects) and grow
     a heap object in one step. */

  for (obj = 0; obj < _sys_heap_obj_count; ++obj)
    if (_sys_heap_objs[obj].base <= new_brk
        && _sys_heap_objs[obj].brk >= new_brk)
      break;

  if (obj >= _sys_heap_obj_count)
    return 0;

  /* We have at least one heap object, OBJ, so this is safe. */

  old_brk = _sys_top_heap_obj->brk;

  /* Free objects which are between the new break address and the old
     one.  Fail if there are such objects and _UF_SBRK_CONTIGUOUS is
     selected. */

  if (obj != _sys_heap_obj_count - 1)
    {
      if ((_sys_uflags & _UF_SBRK_MODEL) == _UF_SBRK_CONTIGUOUS)
        LIBCLOG_RETURN_ULONG(0UL);
      while (_sys_heap_obj_count - 1 > obj)
        {
          FS_VAR();
          _sys_heap_obj_count -= 1;
          FS_SAVE_LOAD();
          DosFreeMemEx ((void *)_sys_heap_objs[_sys_heap_obj_count].base);
          FS_RESTORE();
          _sys_heap_objs[_sys_heap_obj_count].base = 0;
        }
      _sys_top_heap_obj = &_sys_heap_objs[_sys_heap_obj_count-1];
    }

  if (new_brk == _sys_top_heap_obj->base)
    {
      /* The top object is shrunk to zero bytes, deallocate it and
         bump NEW_BRK back to the previous object.  If
         _UF_SBRK_CONTIGUOUS is selected and the (only) object is
         shrunk to 0 bytes, it will be kept anyway to avoid breaking
         programs which depend on the object keeping its address:

            p = sbrk (1000);
            sbrk (-1000);
            assert (sbrk (1000) == p);

         The assertion should not fail.  However, programs which
         select _UF_SBRK_MONOTONOUS or _UF_SBRK_ARBITRARY should be
         prepared for failure of the assertion.  Consider the
         following code:

            sbrk (1000);
            p = sbrk (0);
            sbrk (-1000);
            brk (p);

         Here, brk() may fail due to deallocation of the top
         object. */

      if (obj != 0 || (_sys_uflags & _UF_SBRK_MODEL) != _UF_SBRK_CONTIGUOUS)
        {
          FS_VAR();
          _sys_heap_obj_count -= 1;
          FS_SAVE_LOAD();
          DosFreeMemEx ((void *)_sys_heap_objs[_sys_heap_obj_count].base);
          FS_RESTORE();
          _sys_heap_objs[_sys_heap_obj_count].base = 0;

          if (_sys_heap_obj_count == 0)
            _sys_top_heap_obj = NULL;
          else
            {
              _sys_top_heap_obj = &_sys_heap_objs[_sys_heap_obj_count-1];
              new_brk = _sys_top_heap_obj->brk;
            }
        }
    }

  /* Now decommit unused pages of the top heap object, if there is
     one. */

  if (_sys_heap_obj_count > 0
      && _sys_shrink_heap_obj_by (_sys_top_heap_obj->brk - new_brk) == 0)
      LIBCLOG_RETURN_ULONG(0UL);
  LIBCLOG_RETURN_ULONG(old_brk);
}


/* Shrink the heap by DECR bytes.  DECR is a positive number.  Return
   the previous break address or 0 (error). */

ULONG _sys_shrink_heap_by (ULONG decr, ULONG sbrk_model)
{
  LIBCLOG_ENTER("decr=%ld sbrk_model=%ld\n", decr, sbrk_model);
  ULONG ulRet;
  if (_sys_heap_obj_count == 0)
    LIBCLOG_RETURN_ULONG(0UL);
  if (_sys_top_heap_obj->brk - decr < _sys_top_heap_obj->base)
    LIBCLOG_RETURN_ULONG(0UL);
  ulRet = _sys_shrink_heap_to (_sys_top_heap_obj->brk - decr);
  LIBCLOG_RETURN_ULONG(ulRet);
}
