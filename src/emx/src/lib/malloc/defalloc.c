/* defalloc.c (emx+gcc) -- Copyright (c) 1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdlib.h>
#include <stddef.h>
#include <ulimit.h>
#include <umalloc.h>
#include <sys/builtin.h>        /* For <sys/fmutex.h> */
#include <sys/fmutex.h>
#include <emx/umalloc.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_HEAP
#include <InnoTekLIBC/logstrict.h>

void *_um_default_alloc (Heap_t h, size_t *size, int *clean)
{
  LIBCLOG_ENTER("h=%p size=%p:{%d} clean=%p\n", (void *)h, (void *)size, *size, (void *)clean);
  void *p;
  size_t n;
  int saved_errno;
  long rest;

  /* Round to the next multiple of 64K. */

  n = (*size + 0xffff) & ~0xffff;

  /* If there aren't enough bytes left in the current heap object, add
     the rest of the current heap object to the heap.  _heap_expand()
     will call us again.  Save errno in case UL_OBJREST isn't
     implemented.  Let's hope that no other thread calls sbrk() while
     we do all this work.  (Due to locking of the heap, this can only
     happen by a direct call to sbrk().) */

  saved_errno = errno;
  rest = ulimit (UL_OBJREST, 0); /* The 2nd arg isn't required */
  errno = saved_errno;
  if (rest != -1 && rest != 0 && n > (size_t)rest)
    {
      if (*size <= (size_t)rest)
        {
          /* We can allocate enough memory if we don't round the size.
             So don't round.  Note that this usually does not happen
             as the increment is rounded to a multiple of 64K (see
             above) and the break value is usually a multiple of 64K.
             The break value is usually a multiple of 64K because
             _INITIAL_DEFAULT_HEAP_SIZE is a multiple of 64K and the
             increment is rounded to a multiple of 64K (see above).
             However, someone else might call sbrk() with an increment
             which is not a multiple of 64K. */

          n = *size;
        }
      else
        {
          /* sbrk() will start a new heap object if non-contiguous
             memory allocation is implemented.  Add the rest of the
             current heap object to the heap and postpone the
             requested memory allocation to the next call. */

          p = sbrk ((int)rest);
          if (p == (void *)-1)
            LIBCLOG_ERROR_RETURN_P(NULL);
          *size = (size_t)rest; *clean = !_BLOCK_CLEAN;
          LIBCLOG_RETURN_P(p);
        }
    }

  /* Allocate memory. */

  p = sbrk (n);
  if (p == (void *)-1)
    {
      n = *size;
      p = sbrk (n);
      if (p == (void *)-1)
        LIBCLOG_ERROR_RETURN_P(NULL);
    }
  *size = n; *clean = !_BLOCK_CLEAN;
  LIBCLOG_RETURN_P(p);
}
