/* memory.c -- Memory management
   Copyright (c) 1994-1996 by Eberhard Mattes

This file is part of emx.

emx is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

emx is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with emx; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.

As special exception, emx.dll can be distributed without source code
unless it has been changed.  If you modify emx.dll, this exception
no longer applies and you must remove this paragraph from all source
files for emx.dll.  */


#define INCL_DOSERRORS
#include <os2emx.h>
#include <sys/builtin.h>
#include <sys/fmutex.h>
#include <sys/rmutex.h>
#include <sys/uflags.h>
#include "emxdll.h"

/* This is the size of one object for emx.dll's private memory
   pool. */

#define PRIVATE_OBJECT_SIZE     0x100000

/* Up to this many objects of the above size will be allocated. */

#define MAX_PRIVATE_OBJECTS     16

/* This is the maximum number of memory objects registered with
   register_obj(). */

#define MAX_OBJECTS     (MAX_HEAP_OBJS + MAX_PRIVATE_OBJECTS)

/* This header precedes blocks allocated with DosSubAllocMem. */

struct subhdr
{
  void *object;
  ULONG size;
};

/* Base addresses of emx.dll's private memory objects. */

static void *private_objects[MAX_PRIVATE_OBJECTS];

/* Number of private memory objects. */

static int private_objects_count;

/* Mutex semaphore for protecting the heap. */

static _rmutex heap_rmutex;

/* Number of memory objects registered with register_obj(). */

static ULONG object_count;

/* Memory objects registered with register_obj(). */

static void *object_table[MAX_OBJECTS];


/* Register a memory object. */

static void register_obj (void *base)
{
  /* This cannot happen unless someone increases the number of
     allocate_obj_above() and register_obj() calls and forgets to
     update MAX_OBJECTS. */

  if (object_count >= MAX_OBJECTS)
    {
      oprintf ("Too many memory objects\r\n");
      quit (255);
    }
  object_table[object_count++] = base;
}


/* Allocate and register a memory object of SIZE bytes with allocation
   flags FLAGS.  The object must not be located below MIN_ADDR.
   Return a pointer to the memory object.  Return NULL on error. */

static void *allocate_obj_above (ULONG size, ULONG flags, ULONG min_addr)
{
  ULONG rc;
  void *base, *ret;

  rc = DosAllocMem (&base, size, flags);
  if (rc != 0)
    return NULL;
  if ((ULONG)base > min_addr)
    {
      register_obj (base);
      return base;
    }

  /* The memory object is located below MIN_ADDR.  Recurse until we
     get an appropriate object or until we run out of memory. */

  ret = allocate_obj_above (size, flags, min_addr);

  /* We got a suitable object.  Free the bad one. */

  DosFreeMem (base);
  return ret;
}


static void free_obj (void *p)
{
  ULONG i;

  for (i = 0; i < object_count; ++i)
    if (object_table[i] == p)
      break;
  if (i >= object_count)
    {
      oprintf ("Attempt to free unknown object %x\r\n", (ULONG)p);
      quit (255);
    }
  DosFreeMem (p);
  while (i < object_count)
    {
      object_table[i] = object_table[i+1];
      ++i;
    }
  --object_count;
}


/* Free all memory objects registered with register_obj(). */

void term_memory (void)
{
  while (object_count != 0)
    free_obj (object_table[0]);
}


/* Call DosSetMem.  If ERROR is -1, return for any error code.
   Otherwise, abort if the error code is nonzero and not equal to
   ERROR. */

ULONG setmem (ULONG base, ULONG size, ULONG flags, ULONG error)
{
  ULONG rc;

  if (debug_flags & DEBUG_SETMEM)
    oprintf ("Set memory: %.8x %.8x %.8x\r\n",
             (unsigned)base, (unsigned)size, (unsigned)flags);
  rc = DosSetMem ((void *)base, size, flags);
  if (rc != 0 && error != (ULONG)-1 && rc != error)
    {
      if (rc == 8)
        otext ("Out of swap space\r\n");
      else
        oprintf ("DosSetMem failed, error code = 0x%.8x\r\n", (unsigned)rc);
      quit (255);
    }
  return rc;
}


/* Allocate a block of memory of SIZE bytes from emx.dll's private
   memory pool.  The address of the block is stored to *MEM.  Return
   the OS/2 error code. */

ULONG private_alloc (void **mem, ULONG size)
{
  ULONG rc;
  int i;
  void *obj, *sub;

  rc = ERROR_DOSSUB_NOMEM;
  for (i = private_objects_count - 1; i >= 0; --i)
    {
      rc = DosSubAllocMem (private_objects[i], &sub,
                           size + sizeof (struct subhdr));
      if (rc != ERROR_DOSSUB_NOMEM)
        break;
    }
  if (rc == ERROR_DOSSUB_NOMEM && private_objects_count < MAX_PRIVATE_OBJECTS)
    {
      rc = DosAllocMem (&obj, PRIVATE_OBJECT_SIZE, PAG_READ | PAG_WRITE);
      if (rc != 0)
        return rc;
      register_obj (obj);
      rc = DosSubSetMem (obj,
                         DOSSUB_INIT | DOSSUB_SPARSE_OBJ | DOSSUB_SERIALIZE,
                         PRIVATE_OBJECT_SIZE);
      if (rc != 0)
        return rc;
      i = private_objects_count++;
      private_objects[i] = obj;
      rc = DosSubAllocMem (private_objects[i], &sub,
                           size + sizeof (struct subhdr));
    }
  if (rc != 0)
    return rc;
  ((struct subhdr *)sub)->size = size;
  ((struct subhdr *)sub)->object = private_objects[i];
  *mem = (void *)((char *)sub + sizeof (struct subhdr));
  return 0;
}


/* Allocate a block of memory of SIZE bytes from emx.dll's private
   memory pool.  Return the address of the block.  Abort on error. */

void *checked_private_alloc (ULONG size)
{
  void *mem;
  ULONG rc;

  rc = private_alloc (&mem, size);
  if (rc != 0)
    {
      oprintf ("private_alloc failed, error code = 0x%.8x\r\n", (unsigned)rc);
      quit (255);
    }
  return mem;
}


/* Deallocate a block of memory.  MEM must have been returned by
   checked_private_alloc() or private_alloc(). */

ULONG private_free (void *mem)
{
  struct subhdr *hdr = ((struct subhdr *)mem) - 1;
  return DosSubFreeMem (hdr->object, (void *)hdr,
                        hdr->size + sizeof (struct subhdr));
}


/* Initialize the heap of the application.  This function is called
   from _DLL_InitTerm(). */

void init_heap (void)
{
  _rmutex_create (&heap_rmutex, 0);
}


/* Expand the top heap object by INCR bytes.  INCR is a positive
   number.  Return the previous break address or 0 (error). */

static ULONG expand_heap_obj_by (ULONG incr)
{
  ULONG rc, old_brk, new_brk, addr, size, rest;

  old_brk = top_heap_obj->brk;
  new_brk = old_brk + incr;

  if (new_brk < top_heap_obj->base || new_brk > top_heap_obj->end)
    return 0;

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
      rc = setmem (addr, size, PAG_DEFAULT | PAG_COMMIT, (ULONG)-1);
      if (rc != 0)
        return 0;
    }
  top_heap_obj->brk = new_brk;
  return old_brk;
}


/* Shrink the top heap object by DECR bytes.  DECR is a positive
   number.  Return the previous break address or 0 (error). */

ULONG shrink_heap_obj_by (ULONG decr)
{
  ULONG rc, old_brk, new_brk, addr, high;

  old_brk = top_heap_obj->brk;
  new_brk = old_brk - decr;

  if (new_brk < top_heap_obj->base || new_brk > top_heap_obj->end)
    return 0;

  addr = (new_brk + 0xfff) & ~0xfff;
  high = (top_heap_obj->brk + 0xfff) & ~0xfff;
  if (high > addr)
    {
      rc = setmem (addr, high - addr, PAG_DECOMMIT, (ULONG)-1);
      if (rc != 0)
        return 0;
    }
  top_heap_obj->brk = new_brk;
  return old_brk;
}


/* Expand the heap by INCR bytes.  INCR is a positive number.  Return
   the base address of the expansion or 0 (error). */

static ULONG expand_heap_by (ULONG incr, ULONG sbrk_model)
{
  unsigned old_obj_count;
  ULONG base, size;

  old_obj_count = heap_obj_count;

  /* Allocate the first object if not yet done. */

  if (heap_obj_count == 0)
    {
      size = heap_obj_size;
      if (size == 0)
        {
          /* Apparently emx_init has not been called.  This happens if
             emx.dll is used by a DLL which is used by a process which
             does not use emx.dll.  Use the default heap size of
             32MB. */

          size = HEAP_OBJ_SIZE;
        }
      if (incr > size)
        size = (incr + 0xffff) & ~0xffff;

      base = (ULONG)allocate_obj_above (size, PAG_READ | PAG_WRITE, 0);
      if (base == 0)
        return 0;
      heap_objs[0].base = base;
      heap_objs[0].brk = base;
      heap_objs[0].end = base + size;
      heap_obj_count = 1;
      top_heap_obj = &heap_objs[0];
    }

  /* Now we have at least one heap object.  Check for arithmetic
     overflow. */

  if (top_heap_obj->brk + incr < top_heap_obj->base)
    {
      /* Overflow.  If we've just allocated the first heap object,
         deallocate it again unless memory must be allocated
         contiguously. */

      if (old_obj_count == 0 && sbrk_model != _UF_SBRK_CONTIGUOUS)
        {
          free_obj ((void *)heap_objs[0].base);
          heap_obj_count = 0;
          top_heap_obj = NULL;
        }
      return 0;                 /* Failure */
    }

  /* Check whether we need another heap object.  Allocate one if we
     need one and are allowed to allocate one.  This should not happen
     if we just allocated the first one above. */

  if (top_heap_obj->brk + incr > top_heap_obj->end)
    {
      /* We need another heap object.  Fail if we are not allowed to
         allocate non-contiguous memory or if we already have the
         maximum number of heap objects. */

      if (sbrk_model == _UF_SBRK_CONTIGUOUS || heap_obj_count >= MAX_HEAP_OBJS)
        return 0;

      /* Allocate at least heap_obj_size (or 32MB if uninitialized)
         bytes.  The new object must be located above the currently
         top one. */

      size = heap_obj_size != 0 ? heap_obj_size : HEAP_OBJ_SIZE;
      if (incr > size)
        size = (incr + 0xffff) & ~0xffff;

      for (;;)
        {
          if (sbrk_model == _UF_SBRK_ARBITRARY)
            base = (ULONG)allocate_obj_above (size, PAG_READ | PAG_WRITE, 0);
          else
            base = (ULONG)allocate_obj_above (size, PAG_READ | PAG_WRITE,
                                              top_heap_obj->end);
          if (base != 0)
            break;

          /* If we're out of virtual address space, halve the size and
             try again until allocation succeeds.  Of course, don't
             attempt to allocate less than INCR bytes. */

          size /= 2;
          if (size < incr)
            return 0;
        }

      top_heap_obj = &heap_objs[heap_obj_count++];
      top_heap_obj->base = base;
      top_heap_obj->brk = base;
      top_heap_obj->end = base + size;
    }

  /* Now top_heap_obj points to an object which has enough space.
     Commit memory as required.  expand_heap_obj() returns the top
     object's break address, which is the base address of the
     expansion as an object might have been added. */

  return expand_heap_obj_by (incr);
}


/* Shrink the heap to the new break address NEW_BRK.  Return the
   previous break address or 0 (error). */

static ULONG shrink_heap_to (ULONG new_brk)
{
  unsigned obj;
  ULONG old_brk;

  /* Find the heap object containing the new break address.  Fail if
     there is no such heap object.  Note that the new break address
     must not be beyond the heap object current break address, that
     is, we cannot shrink the heap (by deallocating objects) and grow
     a heap object in one step. */

  for (obj = 0; obj < heap_obj_count; ++obj)
    if (heap_objs[obj].base <= new_brk
        && heap_objs[obj].brk >= new_brk)
      break;

  if (obj >= heap_obj_count)
    return 0;

  /* We have at least one heap object, OBJ, so this is safe. */

  old_brk = top_heap_obj->brk;

  /* Free objects which are between the new break address and the old
     one.  Fail if there are such objects and _UF_SBRK_CONTIGUOUS is
     selected. */

  if (obj != heap_obj_count - 1)
    {
      if ((uflags & _UF_SBRK_MODEL) == _UF_SBRK_CONTIGUOUS)
        return 0;
      while (heap_obj_count - 1 > obj)
        {
          heap_obj_count -= 1;
          free_obj ((void *)heap_objs[heap_obj_count].base);
          heap_objs[heap_obj_count].base = 0;
        }
      top_heap_obj = &heap_objs[heap_obj_count-1];
    }

  if (new_brk == top_heap_obj->base)
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

      if (obj != 0 || (!first_heap_obj_fixed
                       && (uflags & _UF_SBRK_MODEL) != _UF_SBRK_CONTIGUOUS))
        {
          heap_obj_count -= 1;
          free_obj ((void *)heap_objs[heap_obj_count].base);
          heap_objs[heap_obj_count].base = 0;

          if (heap_obj_count == 0)
            top_heap_obj = NULL;
          else
            {
              top_heap_obj = &heap_objs[heap_obj_count-1];
              new_brk = top_heap_obj->brk;
            }
        }
    }

  /* Now decommit unused pages of the top heap object, if there is
     one. */

  if (heap_obj_count > 0
      && shrink_heap_obj_by (top_heap_obj->brk - new_brk) == 0)
    return 0;
  return old_brk;
}


/* Shrink the heap by DECR bytes.  DECR is a positive number.  Return
   the previous break address or 0 (error). */

ULONG shrink_heap_by (ULONG decr, ULONG sbrk_model)
{
  if (heap_obj_count == 0)
    return 0;
  if (top_heap_obj->brk - decr < top_heap_obj->base)
    return 0;
  return shrink_heap_to (top_heap_obj->brk - decr);
}


/* This function implements the __sbrk() system call. */

long do_sbrk (long incr)
{
  ULONG base;

  if (_rmutex_request (&heap_rmutex, _FMR_IGNINT) != 0)
    return -1;

  if (incr >= 0)
    base = expand_heap_by (incr, uflags & _UF_SBRK_MODEL);
  else
    base = shrink_heap_by (-incr, uflags & _UF_SBRK_MODEL);

  if (_rmutex_release (&heap_rmutex) != 0)
    return -1;
  return base != 0 ? (long)base : -1;
}


/* This function implements the __brk() system call. */

long do_brk (ULONG brkp)
{
  ULONG base;

  if (_rmutex_request (&heap_rmutex, _FMR_IGNINT) != 0)
    return -1;

  if (heap_obj_count == 0)
    base = 0;
  else if (brkp >= top_heap_obj->brk && brkp <= top_heap_obj->end)
    base = expand_heap_obj_by (brkp - top_heap_obj->brk);
  else if (brkp >= top_heap_obj->base && brkp < top_heap_obj->brk)
    base = shrink_heap_obj_by (top_heap_obj->brk - brkp);
  else
    base = shrink_heap_to (brkp);

  if (_rmutex_release (&heap_rmutex) != 0)
    return -1;
  return base != 0 ? 0 : -1;
}
