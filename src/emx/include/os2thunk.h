/* os2thunk.h,v 1.4 2004/09/14 22:27:35 bird Exp */
/** @file
 * EMX
 */

#ifndef _OS2THUNK_H
#define _OS2THUNK_H

#include <sys/cdefs.h>

__BEGIN_DECLS

typedef unsigned long _far16ptr;

_far16ptr _libc_32to16 (void *ptr);
void *_libc_16to32 (_far16ptr ptr);

typedef union _thunk_u
{
    void *pv;
    unsigned short *pus;
    unsigned long *pul;
} _thunk_t;

unsigned long _libc_thunk1 (void *args, void *fun);

#define _THUNK_PASCAL_PROLOG(SIZE) \
  __extension__ ({ char _tb[(SIZE)+4]; _thunk_t _tu; _tu.pv  = _tb + sizeof (_tb); \
       *(unsigned long *)_tb = (SIZE);
#define _THUNK_PASCAL_CHAR(ARG)     _THUNK_PASCAL_SHORT (ARG)
#define _THUNK_PASCAL_SHORT(ARG)    (*--_tu.pus = (ARG))
#define _THUNK_PASCAL_LONG(ARG)     (*--_tu.pul = (ARG))
#define _THUNK_PASCAL_FLAT(ARG)     _THUNK_PASCAL_LONG (_libc_32to16 (ARG))
#define _THUNK_PASCAL_FAR16(ARG)    _THUNK_PASCAL_LONG (ARG)
#define _THUNK_PASCAL_FUNCTION(FUN) APIENTRY _16_##FUN
#define _THUNK_PASCAL_CALL(FUN)     _libc_thunk1 (_tb, (void *)(_16_##FUN)); })
#define _THUNK_PASCAL_CALLI(FUN)    _libc_thunk1 (_tb, (void *)(FUN)); })

#define _THUNK_C_PROLOG(SIZE) \
  __extension__ ({ char _tb[(SIZE)+4]; _thunk_t _tu; _tu.pv = _tb + sizeof (unsigned long); \
       *(unsigned long *)_tb = (SIZE);
#define _THUNK_C_CHAR(ARG)     _THUNK_C_SHORT (ARG)
#define _THUNK_C_SHORT(ARG)    (*_tu.pus++ = (ARG))
#define _THUNK_C_LONG(ARG)     (*_tu.pul++ = (ARG))
#define _THUNK_C_FLAT(ARG)     _THUNK_C_LONG (_libc_32to16 (ARG))
#define _THUNK_C_FAR16(ARG)    _THUNK_C_LONG (ARG)
#define _THUNK_C_FUNCTION(FUN) _16__##FUN
#define _THUNK_C_CALL(FUN)     _libc_thunk1 (_tb, (void *)(_16__##FUN)); })
#define _THUNK_C_CALLI(FUN)    _libc_thunk1 (_tb, (void *)(FUN)); })

#define _THUNK_PROLOG(SIZE)  _THUNK_PASCAL_PROLOG (SIZE)
#define _THUNK_CHAR(ARG)     _THUNK_PASCAL_CHAR (ARG)
#define _THUNK_SHORT(ARG)    _THUNK_PASCAL_SHORT (ARG)
#define _THUNK_LONG(ARG)     _THUNK_PASCAL_LONG (ARG)
#define _THUNK_FLAT(ARG)     _THUNK_PASCAL_FLAT (ARG)
#define _THUNK_FAR16(ARG)    _THUNK_PASCAL_FAR16 (ARG)
#define _THUNK_FUNCTION(FUN) _THUNK_PASCAL_FUNCTION (FUN)
#define _THUNK_CALL(FUN)     _THUNK_PASCAL_CALL (FUN)
#define _THUNK_CALLI(FUN)    _THUNK_PASCAL_CALLI (FUN)

#define MAKE16P(sel,off)   ((_far16ptr)((sel) << 16 | (off)))
#define MAKEP(sel,off)     _libc_16to32 (MAKE16P (sel, off))
#define SELECTOROF(farptr) ((SEL)((farptr) >> 16))
#define OFFSETOF(farptr)   ((USHORT)(farptr))

/* Return true iff the block of SIZE bytes at PTR does not cross a
   64Kbyte boundary. */

#define _THUNK_PTR_SIZE_OK(ptr,size) \
  (((ULONG)(ptr) & ~0xffff) == (((ULONG)(ptr) + (size) - 1) & ~0xffff))

/* Return true iff the structure pointed to by PTR does not cross a
   64KByte boundary. */

#define _THUNK_PTR_STRUCT_OK(ptr) _THUNK_PTR_SIZE_OK ((ptr), sizeof (*(ptr)))

__END_DECLS

#endif /* not _OS2THUNK_H */
