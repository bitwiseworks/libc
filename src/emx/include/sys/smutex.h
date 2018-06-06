/* sys/smutex.h (emx+gcc) */

/* Simple mutex semaphores.  These semaphores need not be created and
   destroyed, _smutex objects just have to be initialized to zero.
   _smutex semaphores are implemented with looping and DosSleep.
   Therefore, they should be used only if a collision is very
   unlikely, for instance, during initializing.  In all other cases,
   _fmutex or HMTX semaphores should be used. */

#ifndef _SYS_SMUTEX_H
#define _SYS_SMUTEX_H

#include <sys/builtin.h>

#if defined (__cplusplus)
extern "C" {
#endif

typedef signed char _smutex;


void __smutex_request_internal (__volatile__ _smutex *);


static __inline__ void _smutex_request (__volatile__ _smutex *sem)
{
  if (__cxchg (sem, 1) != 0)
    __smutex_request_internal (sem);
}

static __inline__ int _smutex_try_request (__volatile__ _smutex *sem)
{
  return __cxchg (sem, 1) == 0;
}


static __inline__ void _smutex_release (__volatile__ _smutex *sem)
{
  *sem = 0;
}


static __inline__ int _smutex_available (__volatile__ _smutex *sem)
{
  return *sem == 0;
}


#if defined (__cplusplus)
}
#endif

#endif /* not _SYS_SMUTEX_H */
