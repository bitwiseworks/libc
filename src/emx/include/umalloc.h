/* umalloc.h,v 1.4 2004/09/14 22:27:36 bird Exp */
/** @file
 * EMX.
 */


#ifndef _UMALLOC_H
#define _UMALLOC_H

#include <sys/cdefs.h>
#include <sys/_types.h>

__BEGIN_DECLS

#if !defined (_SIZE_T_DECLARED)
#define _SIZE_T_DECLARED
typedef __size_t size_t;
#endif

#if !defined (NULL)
#if defined (__cplusplus)
#define NULL 0
#else
#define NULL ((void *)0)
#endif
#endif

struct _uheap;
typedef struct _uheap *Heap_t;

typedef struct
{
  size_t _provided;
  size_t _used;
  size_t _tiled;
  size_t _shared;
  size_t _max_free;
  size_t _segments;
  size_t _crates;
  size_t _reserved1;
  size_t _reserved2;
  size_t _reserved3;
  size_t _reserved4;
  size_t _reserved5;
  size_t _reserved6;
  size_t _reserved7;
  size_t _reserved8;
  size_t _reserved9;
} _HEAPSTATS;

#define _HEAP_REGULAR   0x01
#define _HEAP_TILED     0x02
#define _HEAP_SHARED    0x04
#define _HEAP_HIGHMEM   0x08 /* advisory flag */

#define _BLOCK_CLEAN    1
#define _FORCE          1
#define _HEAP_MIN_SIZE  512
#define _RUNTIME_HEAP   _um_regular_heap

#if !defined (_HEAPOK)
#define _HEAPOK       0
#define _HEAPEMPTY    1
#define _HEAPBADBEGIN 2
#define _HEAPBADNODE  3
#define _HEAPBADEND   4
#define _HEAPBADROVER 5
#endif
#define _FREEENTRY    6
#define _USEDENTRY    7

extern Heap_t _um_regular_heap;

Heap_t _mheap (const void *);
Heap_t _uaddmem (Heap_t, void *, size_t, int);
void *_ucalloc (Heap_t, size_t, size_t);
int _uclose (Heap_t);
Heap_t _ucreate (void *, size_t, int, unsigned,
    void *(*)(Heap_t, size_t *, int *),
    void (*)(Heap_t, void *, size_t));
Heap_t _ucreate2 (void *, size_t, int, unsigned,
    void *(*)(Heap_t, size_t *, int *),
    void (*)(Heap_t, void *, size_t),
    int (*)(Heap_t, void *, size_t, size_t *, int *),
    void (*)(Heap_t, void *, size_t, size_t *));
Heap_t _udefault (Heap_t);
int _udestroy (Heap_t, int);
int _uheapchk (Heap_t);
int _uheapmin (Heap_t);
int _uheapset (Heap_t, unsigned);
unsigned _uheap_type (Heap_t);
int _uheap_walk (Heap_t, int (*)(const void *, size_t, int, int,
                                 const char *, size_t));
int _uheap_walk2 (Heap_t, int (*)(Heap_t, const void *, size_t, int, int,
                                  const char *, size_t, void *), void *);
void *_umalloc (Heap_t, size_t);
int _uopen (Heap_t);
int _ustats (Heap_t, _HEAPSTATS *);
void *_utcalloc (Heap_t, size_t, size_t);
Heap_t _utdefault (Heap_t);
void *_utmalloc (Heap_t, size_t);

__END_DECLS

#endif /* not _UMALLOC_H */
