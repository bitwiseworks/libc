/* sys/syscalls.h (emx+gcc) -- Copyright (c) 1992-1996 by Eberhard Mattes
   sys/filefind.c (libc)    -- Copyright (c) 2003 by Andrew Zabolotny
   sys/filefind.c (libc)    -- Copyright (c) 2003 by Knut St. Osmundsen
 */

#include "libc-alias.h"
#include <sys/signal.h>
#include <sys/types.h>
#include <InnoTekLIBC/sharedpm.h>

#ifndef __LIBC_THREAD_DECLARED
#define __LIBC_THREAD_DECLARED
struct __libc_thread;
typedef struct __libc_thread *__LIBC_PTHREAD, **__LIBC_PPTHREAD;
#endif

#if !defined (NULL)
#define NULL ((void *)0)
#endif

#if !defined (INIT)
#define EXTERN extern
#define INIT(x)
#endif

#define BEGIN do {
#define END   } while (0)

#if !defined (_OS2EMX_H)
typedef unsigned long ULONG;
#endif

/* This macro is used for accessing FDATE or FTIME values as
   USHORT. */

#define XUSHORT(x) (*(USHORT *)&(x))

/* Maximum number of heap objects (16 = 512 / 32). */

#define MAX_HEAP_OBJS   16

/* This structure describes one heap object. */

struct heap_obj
{
  ULONG base;                   /* Base address */
  ULONG end;                    /* End address */
  ULONG brk;                    /* Address of first unused byte */
};

EXTERN unsigned _sys_uflags INIT (0);
EXTERN unsigned long _sys_clock0_ms INIT (0);
EXTERN long _sys_ino INIT (0x100000);
EXTERN int _sys_pid;
EXTERN int _sys_ppid;

/** Virtual address limit and high memory indicator.
 *
 * Zero means limit is 512MB.
 * Non zero means more that 512MB. The value is then the size of then the user
 * address space size in bytes.
 *
 * Initiated by __init_dll().
 */
EXTERN unsigned long    _sys_gcbVirtualAddressLimit;

/* The top heap object.  This points into _sys_heap_objs[] or is NULL.
   While this variable is NULL, no memory has been allocated. */

EXTERN struct heap_obj *_sys_top_heap_obj;

/* This array holds information on all the heap objects.  The heap
   objects are managed in LIFO fashion. */

EXTERN struct heap_obj  _sys_heap_objs[MAX_HEAP_OBJS];

/* This is the number of heap objects. */

EXTERN unsigned         _sys_heap_obj_count;

/* This variable can be initialized by the application to control the
   size of the heap object(s). */

extern unsigned         _sys_heap_size;


#if defined (_OS2EMX_H)

/** Test if a DOS/OS2 file time is zero. */
#define FTIMEZEROP(x) (*(PUSHORT)&(x) == 0)
/** Test if a DOS/OS2 file date is zero. */
#define FDATEZEROP(x) (*(PUSHORT)&(x) == 0)

long _sys_p2t(FTIME t, FDATE d);

#endif /* _OS2EMX_H */

void _sys_get_clock (unsigned long *ms);

/** @group Heap stuff.
 * @{ */
ULONG _sys_expand_heap_obj_by (ULONG incr);
ULONG _sys_expand_heap_by (ULONG incr, ULONG sbrk_model);
ULONG _sys_shrink_heap_to (ULONG new_brk);
ULONG _sys_shrink_heap_by (ULONG decr, ULONG sbrk_model);
ULONG _sys_shrink_heap_obj_by (ULONG decr);
#ifdef _SYS_FMUTEX_H
/** This mutex semaphore protects the heap. */
EXTERN _fmutex          _sys_heap_fmutex;
/** Mutex semaphore protecting the list of high memory big blocks. */
EXTERN _fmutex          _sys_gmtxHimem;
#endif
/** @} */


/** @group Init Functions
 * @{ */
extern void             __init(int fFlags);
extern int              __init_dll(int fFlags, unsigned long hmod);
extern void /*volatile*/_sys_init_ret(void *stack) __attribute__((__noreturn__));
extern int              _sys_init_environ(const char *pszEnv);
extern void             _sys_init_largefileio(void);
extern int              __libc_fhInit(void);
/** @} */

/** @defgroup grp_sys_inherit   Inherit Function
 * @{ */
__LIBC_PSPMINHFHBHDR    __libc_fhInheritPack(size_t *pcb, char **ppszStrings, size_t *pcbStrings);
void                    __libc_fhInheritDone(void);
/** @} */

/** @group Term Functions
 * @{ */
extern int              _sys_term_filehandles(void);
/** @} */

/** @group Error code and errno Functions.
 * @{ */
extern void             _sys_set_errno(unsigned long rc);
extern int              __libc_native2errno(unsigned long rc);
extern int              __libc_back_errno2native(int rc);
/** @} */

/** @group Exec, Spawn and Fork stuff.
 * @{ */
#ifdef _SYS_FMUTEX_H
EXTERN _fmutex          __libc_gmtxExec INIT({0});
#endif
void                    __libc_fhExecDone(void);
/** @} */

