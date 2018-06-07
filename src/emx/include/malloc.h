/* malloc.h,v 1.7 2004/09/14 22:27:34 bird Exp */
/** @file
 * EMX
 * @todo    Merge with stdlib.h.
 */

#ifndef _MALLOC_H
#define _MALLOC_H

#include <sys/cdefs.h>
#include <sys/_types.h>

#if defined (__cplusplus)
extern "C" {
#endif

#if !defined (_SIZE_T) && !defined (_SIZE_T_DECLARED)
#define _SIZE_T
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


void *calloc (size_t, size_t);
void free (void *);
void *malloc (size_t);
void *realloc (void *, size_t);


#if (!defined (__STRICT_ANSI__) && !defined (_POSIX_SOURCE)) || defined (_WITH_UNDERSCORE) || defined(__USE_EMX)

#if !defined (_HEAPOK)
#define _HEAPOK       0
#define _HEAPEMPTY    1
#define _HEAPBADBEGIN 2
#define _HEAPBADNODE  3
#define _HEAPBADEND   4
#define _HEAPBADROVER 5
#endif

void *_tcalloc (size_t, size_t);
void _tfree (void *);
int _theapmin (void);
void *_tmalloc (size_t);
void *_trealloc (void *, size_t);

void *_expand (void *, size_t);
int _heapchk (void);
int _heapmin (void);
int _heapset (unsigned);
int _heap_walk (int (*)(const void *, size_t, int, int,
    const char *, size_t));
size_t _msize (const void *);

#endif

void    *valloc(size_t);
void    *memalign(size_t, size_t);

#if defined (__cplusplus)
}
#endif

#endif /* not _MALLOC_H */
