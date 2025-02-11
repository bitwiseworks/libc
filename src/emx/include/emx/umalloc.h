/* emx/umalloc.h (emx+gcc) */

#ifndef _EMX_UMALLOC_H
#define _EMX_UMALLOC_H

#if defined (__cplusplus)
extern "C" {
#endif

/* include all we need here to make life easier. */
#include <stddef.h>
#include <umalloc.h>
#include <sys/builtin.h>
#include <sys/fmutex.h>
#include <sys/uflags.h>

/* =========================== Introduction =========================== */

/* This implementation of malloc() uses two different allocation
   schemes, one for small blocks, one for big blocks.  Small blocks
   are allocated from special big blocks, big blocks are made up of
   fixed-sized pages to reduce fragmentation.

   Glossary:

   heap         A set of non-contiguous segments along with various
                control information.  This is the entity passed by the
                user to _umalloc() etc.  Each heap starts with a
                struct _uheap.  Heap_t points to a heap.

   segment      A contiguous area of memory made up of pages and
                organized in lumps.  This is used for allocating big
                blocks and for allocating crates.  Each segment starts
                with a struct _um_seg; the rest of the segment is
                divided into pages and used for lumps.

   lump         A contiguous set of pages.  A lump is either free or
                allocated.  An allocated lump is either used for a
                user block or for a crate.  The first page of a lump
                starts with a struct _um_lump which contains control
                information about the lump.  The remaining bytes of
                the first page and the remaining pages of the lump
                make up a block.

   page         Smallest allocation unit in a segment.  Division into
                pages is used to reduce fragmentation.  The size of a
                page, _UM_PAGE_SIZE, must be a power of two.

   bucket       A set of free lumps of certain sizes.  Bucket i
                contains free lumps of SIZE(i) through SIZE(i+1)-1
                bytes, where SIZE(i) is a ower of two.  Buckets are
                implemented as doubly-linked lists which may span
                segments.  Each heap has its own set of buckets of
                various sizes.

   crate        A contiguous set of crumbs, all having the same size.
                Crates are allocated as lumps from the heap.  Lumps
                used for crates have state _UMS_CRATE.  Each crate
                starts with a struct _um_crate followed by an array of
                crumbs.

   crumb        Allocation unit in crates, used for allocation of
                small blocks.  All crumbs of a crate have the same
                size.  Each crumb starts with a struct _um_crumb.

   crateset     A linked list of crates and a doubly-linked list of
                free crumbs (which may span several crates), all
                crumbs having the same size.  Each heap has its own
                set of cratesets of various (small) sizes.

   block        Memory allocated from a heap.  malloc() returns a
                pointer to a block.  Each block is preceded by control
                information (header); the word immediately preceding a
                block points to the parent structure (segment or
                crate).  A block is either located in a lump or in a
                crumb, distinguished by the magic word pointed to by
                the parent pointer.

   header       The control information (struct _um_hdr) preceding a
                block.  At least, this is a pointer to the parent
                structure (segment or crate).  The low 3 bits of the
                pointer are used for flag bits, therefore the parent
                structure must be aligned on a 8-byte boundary (the
                low 3 bits of the pointer are set to zero).  The
                `size' member of the header is not valid for used
                crumbs (it's overlaid by the first word of the block).

   A different allocation scheme (crumbs) for small blocks is used for
   the following reasons:

   - The granularity (page size) of lumps is 32 bytes.  That's quite a
     lot of overhead for, say, 4-byte blocks

   - If the page size were decreased to 4 to minimize overhead, there
     wouldn't be enough space for keeping the two free list pointers
     -- free crumbs don't need the size member, therefore it can be
     overlaid with one of the pointers

   - Moreover, decreasing the page size would increase fragmentation
     of the heap

   - Allocating small blocks as lumps would increase fragmentation of
     the heap.

   The following diagram shows four crumbs (of 4 bytes each), #2 being
   free:

       crumb #0    crumb #1    crumb #2    crumb #3
       |<---12--->||<---12--->||<---12--->||<---12--->|
       |======||==||======||==||==========||======||==|
         hdr   blk   hdr   blk  _um_crumb    hdr    blk
       |          ||          |            |          |
       |=======...||=======...|            |=======...|
        _um_crumb   _um_crumb               _um_crumb

   Note that part of `struct _um_crumb' is overlaid by the block.  The
   entire `struct _um_crumb' is used for the free crumb.

   The following diagram shows four crumbs (of 8 bytes each), #2 being
   free:

       crumb #0        crumb #1        crumb #2        crumb #3
       |<-----16----->||<-----16----->||<-----16----->||<-----16----->|
       |======||======||======||======||==========|....|======||======|
         hdr     blk     hdr     blk    _um_crumb        hdr     blk
       |=======...|    |=======...|                    |=======...|
        _um_crumb       _um_crumb                       _um_crumb

   Again, part of `struct _um_crumb' is overlaid by the block.

   We could save another 4 bytes per crumb for block sizes >= 8 by not
   storing the size, which can be retrieved from the crate pointed to
   by the header.  Then, _msize() would return the rounded size.  The
   minimum crumb size is 12 bytes, therefore this wouldn't improve
   things for 4-byte blocks.

   The following diagram shows a used lump spanning two pages
   with _UM_LUMP_HEADER_SIZE == 8 and _UM_PAGE_SIZE == 32:

               32-byte boundary                32-byte boundary
               |                               |
       |<-------- page (32) --------->||<-------- page (32) --------->|
       |<- 8->|                       ||                              |
       |======||===================================|...............|==|
         hdr                block                       unused     size
       |              |
       |=======.......|
       struct _um_lump

   By `mis-aligning' the pages by _UM_LUMP_HEADER_SIZE we can align
   blocks on any power of two.  Note that part of `struct _um_lump' is
   overlaid by the block.

   The following diagram shows a free lump spanning two pages with
   _UM_LUMP_HEADER_SIZE == 8 and _UM_PAGE_SIZE == 32:

               32-byte boundary                32-byte boundary
               |                               |
       |<-------- page (32) --------->||<-------- page (32) --------->|
       |==============|............................................|==|
       struct _um_lump                  unused                     size

   Note that the entire `struct _um_lump' is used.

   */

/* ======================== Common definitions ======================== */

/* Magic numbers. */

#define _UM_MAGIC_CRATE 0xdd63eeed
#define _UM_MAGIC_SEG   0xdd73eeed
#define _UM_MAGIC_HEAP  0xdd68eeed

/* These flags are passed internally between the functions. */

#define _UMFI_TILED     0x0001  /* Create tiled block */
#define _UMFI_ZERO      0x0002  /* Zero the block if required */
#define _UMFI_CRATE     0x0004  /* Create a lump for a crate */
#define _UMFI_NOMOVE    0x0008  /* Expand block without moving */

/* These flags are stored in the parent pointer of lumps and crumbs.
   In consequence, we must align objects pointed to by those pointers
   (ie, segments and crates) on 8-byte boundaries. */

#define _UMF_MASK       0x07    /* Mask for all the following flags */
#define _UMF_RESERVED   0x04    /* Not yet used, _udump_allocated_delta() */

#define _UMS_MASK       0x03    /* Mask for the block status */
#define _UMS_FREE       0x00    /* The block is free (1) */
#define _UMS_USER       0x01    /* The block is used for a user block */
#define _UMS_CRATE      0x02    /* The block is used for a crate */
#define _UMS_RESERVED   0x03    /* Not yet used */

/* (1) In most cases which check the status for equality or inequality
       to _UMS_FREE.  Defining _UMS_FREE as zero makes these tests
       efficient. */

/* Segments and crates must be aligned on a _UM_PARENT_ALIGN byte
   boundary. */

#define _UM_PARENT_ALIGN        (_UMF_MASK + 1)

/* Turn an _umint into a pointer.  This is used for the parent pointer
   in the header. */

#define _PTR_FROM_UMINT(x,t)  ((t *)((x) & ~_UMF_MASK))

/* Turn a pointer into an _umint.  This is used for the parent pointer
   in the header; the flags will be set to zero if the pointer is
   properly aligned. */

#define _UMINT_FROM_PTR(p)    ((_umint)(p))

/* Return true iff P is suitably aligned for holding a tiled object of
   S bytes. */

#define _UM_TILE_ALIGNED(p,s) ((((unsigned long)(p) + (s)) & 0xffff) >= (s) \
                               || ((unsigned long)(p) & 0xffff) == 0)

/* Return true iff the address P is suitable for holding a tiled
   object of S bytes. */

#define _UM_TILE_TILED(p,s) ((unsigned long)(p) + (s) <= 0x20000000)

/* Return true iff the address P can be used for a tiled object of S
   bytes. */

#define _UM_TILE_OK(p,s)      (_UM_TILE_ALIGNED ((p), (s)) \
                               && _UM_TILE_TILED ((p), (s)))

/* Compute the smallest number X such that P will be aligned on a A
   byte boundary if X is added (as a byte count) to P.  P can be
   either a pointer or an integer, A must be a power of two. */

#define _UM_ALIGN_DIFF(p,a) ((((unsigned long)(p) + (a) - 1) & ~((a) - 1)) \
                             - (unsigned long)(p))

/* Return true iff P is aligned on an A boundary.  A must be a power
   of two. */

#define _UM_IS_ALIGNED(p,a) (((unsigned long)(p) & ((a) - 1)) == 0)

/* Return a void pointer pointing N bytes past the start of the object
   pointed to be P.  In other words, add an integer to a pointer,
   interpreting the integer as byte count. */

#define _UM_ADD(p,n)  ((void *)((char *)(p) + (n)))

/* Set the status of a crumb or lump to S.  V is the `parent' member
   of the header. */

#define _UM_SET_STATUS(v,s) ((v) = ((v) & ~_UMS_MASK) | (s))

/* _uheap_walk() applies the _UM_WALK_ERROR macro when returning an
   error.  Set a breakpoint in _um_walk_error() to find out more about
   the error with a debugger. */

#ifdef NDEBUG
#define _UM_WALK_ERROR(x) x
#else
#define _UM_WALK_ERROR(x) _um_walk_error(x)
int _um_walk_error (int);
#endif

/* _umint represents a pointer as unsigned integer. */

typedef unsigned long _umint;

/* _umagic holds a magic number. */

typedef unsigned long _umagic;

/* Declarations for forward references. */

struct _uheap;

/* Special assert that releases the heap lock if triggered */

#if defined (NDEBUG)
#define _um_heap_maybe_unlock(h)  ((void)0)
#define _um_assert(exp, h) ((void)0)
#define _UM_ASSERT_HEAP_PARAM(h)
#define _UM_ASSERT_HEAP_ARG(h)
#else
#define _um_heap_maybe_unlock(h) \
  (h && _fmutex_is_owner (&h->fsem) ? _fmutex_release(&h->fsem) : (void)0)
#define _um_assert(exp, h) ((exp) ? (void)0 : \
  _um_heap_maybe_unlock (h), assert (exp))
#define _UM_ASSERT_HEAP_PARAM(h) , Heap_t h
#define _UM_ASSERT_HEAP_ARG(h) , h
#endif


/* ============================== Header ============================== */

/* The control information immediately preceding free and allocated
   blocks must have the following layout (`size' is not required for
   free crumbs, but is required for free lumps): */

struct _um_hdr
{
  _umint size;                  /* Valid if the block is allocated */
  _umint parent;                /* Use _PTR_FROM_UMINT() to get the pointer */
};

#define _HDR_FROM_BLOCK(p)      (((struct _um_hdr *)(p)) - 1)

/* Return the status of the crumb P. */

#define _UM_HDR_STATUS(p)       ((p)->parent & _UMS_MASK)


/* ========================= Crates and crumbs ======================== */

/* Subheaps (crates) for blocks of constant size.  All the blocks
   allocated from a crate have the same rounded size.  Each crate has
   a header followed by an array of crumbs.  Initially, crumbs are
   allocated directly from the array, starting at index INIT.  This is
   done only for the most recently allocated crate as this is the only
   one which has free crumbs at INIT.  Deallocating a crumb will put
   it into a list of free crumbs.  If that list is non-empty, crumbs
   will be allocated from that list. */

#define _UM_MAX_CRATE_SIZE      8000

struct _um_crumb
{
  union
  {
    struct
    {
      struct _um_crumb *prev;   /* Cf. struct _um_hdr (size) */
      _umint parent_crate;      /* Cf. struct _um_hdr (parent) */
      struct _um_crumb *next;
    } free;
    struct
    {
      _umint size;              /* Cf. struct _um_hdr (size) */
      _umint parent_crate;      /* Cf. struct _um_hdr (parent) */
      char data;
    } used;
  } x;
};

struct _um_crate
{
  /* This must be the first word.  It's used for consistency checking
     and for distinguishing crates from segments. */

  _umagic magic;

  struct _um_crateset *parent_crateset;

  /* PARENT_HEAP points to the heap this crate belongs to.  For
     instance, we need this for locking the heap in free(). */

  struct _uheap *parent_heap;

  /* NEXT points to the next crate of the same size of the same heap.
     This pointer is rarely needed, so a simply-linked list is
     sufficient. */

  struct _um_crate *next;

  /* All crumbs in this crate have this size. */

  size_t crumb_size;

  /* Maximum number of crumbs in this crate. */

  unsigned max;

  /* Crumbs 0 through INIT-1 have been initialized, the free ones are
     in the fre list.  Crumbs INIT through MAX-1 are available and
     uninitialized (they are not in the free list). */

  unsigned init;

  /* Number of used crumbs in this crate */

  unsigned used;
};

/* This is the overhead added to a block in a crumb. */

#define _UM_CRUMB_OVERHEAD     offsetof (struct _um_crumb, x.used.data)

/* Return a pointer to the Ith crumb of the crate P. */

#define _UM_CRUMB_BY_INDEX(p,i) \
    ((struct _um_crumb *) \
     (_UMINT_FROM_PTR (p) + sizeof (struct _um_crate) \
      + (i) * ((p)->crumb_size + _UM_CRUMB_OVERHEAD)))

/* Return a pointer to the crumb containing block P. */

#define _UM_CRUMB_FROM_BLOCK(p) \
  ((struct _um_crumb *)((char *)(p) \
                        - offsetof (struct _um_crumb, x.used.data)))

/* Return a pointer to the block in crumb P. */

#define _UM_BLOCK_FROM_CRUMB(p) ((void *)&(p)->x.used.data)

/* Return the status of the crumb P. */

#define _UM_CRUMB_STATUS(p)  ((p)->x.used.parent_crate & _UMS_MASK)

/* Set the status of the crumb P to S. */

#define _UM_CRUMB_SET_STATUS(p,s) \
    _UM_SET_STATUS ((p)->x.used.parent_crate, (s))


/* Note: the most recently allocated crate is at the head of the crate
   list (ie, it's pointed to by crate_head).  The most recently
   allocated crate is the only one which has INIT < MAX. */

struct _um_crateset
{
  size_t crumb_size;
  struct _um_crate *crate_head;
  struct _um_crumb *crumb_head;
  struct _um_crumb *crumb_tail;
};


/* ======================== Segments and lumps ======================== */

/* Each lump starts with a `struct _um_lump'.  The block (of a used
   lump) starts at the DATA member, which is overlaid by NEXT and PREV
   in free lumps.  The block is followed by an _umint which repeats
   the size.

   IMPORTANT:

       offsetof (struct _um_lump, x.used.data)

   must divide the page size to make alignment work!  Additional
   members (say, for debugging) must be inserted at the beginning of
   the structure, preceding the SIZE member. */

struct _um_lump
{
  _umint size;                  /* Cf. struct _um_hdr */
  _umint parent_seg;            /* Cf. struct _um_hdr */
  union
  {
    struct
    {
      struct _um_lump *next;
      struct _um_lump *prev;
    } free;
    struct
    {
      char data;
    } used;
  } x;
};


/* Each segment starts with a `struct _um_seg'.  Note that this
   structure might be preceded by padding for proper alignment.  See
   MEM. */

struct _um_seg
{
  /* This must be the first word.  It's used for consistency checking
     and for distinguishing crates from segments. */

  _umagic magic;

  /* PARENT_HEAP points to the heap this segment belongs to. */

  struct _uheap *parent_heap;

  /* NEXT points to the next segment of the same heap.  This pointer
     is rarely needed, so a simply-linked list is sufficient. */

  struct _um_seg *next;

  /* SIZE is the size of the memory area containing this segment.
     SIZE is relative to MEM (see below). */

  size_t size;

  /* ZERO_LIMIT is used to avoid zeroing memory for _ucalloc() in
     certain cases.  It is assumed that all pages at addresses >=
     ZERO_LIMIT are already zeroed. */

  void *zero_limit;

  /* START points to the first page of this segment. */

  void *start;

  /* END points to the (non-existing) page one past the last page of
     this segment. */

  void *end;

  /* Pointer to memory containing the segment.  This pointer is
     required because struct _um_seg must be aligned on a
     _UM_PARENT_ALIGN byte boundary. */

  void *mem;
};


/* A bucket of lumps is implemented as doubly-linked list. */

struct _um_bucket
{
  struct _um_lump *head;
  struct _um_lump *tail;
};

/* This is the maximum size of a lump. */

#define _UM_MAX_SIZE          0x40000000

/* This constant determines the size of the smallest bucket, bucket 0.
   Note that bucket numbers do not start at _UM_MIN_BUCKET! */

#define _UM_MIN_BUCKET        5

/* This constant determines the size of the biggest bucket.  Note that
   bucket numbes do not end at _UM_MAX_BUCKET! */

#define _UM_MAX_BUCKET        27        /* 128 MB */

/* This is the number of buckets.  Buckets are numbered 0 through
   _UM_BUCKETS-1. */

#define _UM_BUCKETS           (_UM_MAX_BUCKET - _UM_MIN_BUCKET + 1)

/* Bucket i contains free lumps with

        _UM_BUCKET_SIZE(i) <= SIZE < _UM_BUCKET_SIZE(i+1)

   where SIZE is the rounded size of the lumps.  The last bucket,
   _UM_BUCKETS-1, contains all free lumps of _UM_BUCKET_SIZE(i) bytes
   or more (there is no upper limit in the last bucket, except for
   _UM_MAX_SIZE). */

#define _UM_BUCKET_SIZE(i)    (1 << ((i) + _UM_MIN_BUCKET))

/* The page size must be a power of two and must be greater than or
   equal to _UM_LUMP_OVERHEAD. */

#define _UM_PAGE_SIZE         (1 << _UM_MIN_BUCKET)

/* This is the size of the header for a lump. */

#define _UM_LUMP_HEADER_SIZE  offsetof (struct _um_lump, x.used.data)

/* This is the overhead added to a block in a lump. */

#define _UM_LUMP_OVERHEAD     (_UM_LUMP_HEADER_SIZE + sizeof (_umint))

/* Compute the lump size from the block size.  The result is always a
   multiple of the page size. */

#define _UM_ROUND_LUMP(s)     (((s) + _UM_LUMP_OVERHEAD + _UM_PAGE_SIZE - 1) \
                               & ~(_UM_PAGE_SIZE-1))

/* Return true iff P is the first lump of segment S. */

#define _UM_FIRST_LUMP(s,p)     ((p) == (s)->start)

/* Return true iff P is the last lump of segment S. */

#define _UM_LAST_LUMP(s,p)      (_UM_NEXT_LUMP (p) == (s)->end)

/* Return a pointer to the lump immediately preceding the lump P.
   This macro must not be applied to the first lump of a segment. */

#define _UM_PREV_LUMP(p)    ((struct _um_lump *) \
                             ((char *)(p) \
                              - _UM_ROUND_LUMP (((_umint *)(p))[-1])))

/* Return a pointer to the lump immediately following the lump P.
   This macro can be applied to the last lump of a segment; in this
   case, the result should be equal the END member of the segment. */

#define _UM_NEXT_LUMP(p)    ((struct _um_lump *) \
                            ((char *)(p) + _UM_ROUND_LUMP ((p)->size)))

/* Return a pointer to the lump containing block P. */

#define _UM_LUMP_FROM_BLOCK(p) \
  ((struct _um_lump *)((char *)(p) \
                        - offsetof (struct _um_lump, x.used.data)))

/* Return a pointer to the block in lump P. */

#define _UM_BLOCK_FROM_LUMP(p) ((void *)&(p)->x.used.data)


/* Return the status of the lump P. */

#define _UM_LUMP_STATUS(p)      ((p)->parent_seg & _UMS_MASK)

/* Set the status of the lump P to S. */

#define _UM_LUMP_SET_STATUS(p,s) _UM_SET_STATUS ((p)->parent_seg, (s))


/* =============================== Heaps =============================== */

/* This is the initial size of the default heap.  It should be a
   multiple of 64K, see _default_alloc_fun(). */

#define _INITIAL_DEFAULT_HEAP_SIZE      65536

/* This is the maximum number of cratesets. */

#define _UM_MAX_CRATESETS       5

/* Each thread has its own default heap. */

#define _UM_DEFAULT_REGULAR_HEAP pThrd->pRegularHeap
#define _UM_DEFAULT_TILED_HEAP   pThrd->pTiledHeap
#define _UM_MT_DECL              __LIBC_PTHREAD pThrd = __libc_threadCurrent();

/* Note that _um_regular_heap is declared in <umalloc.h>.
   the others aren't, so we declare them here. */

extern Heap_t _um_tiled_heap;
extern Heap_t _um_high_heap;
extern Heap_t _um_low_heap;


/* This structure is stored at the start of a heap; it's pointed to by
   Heap_t. */

struct _uheap
{
  /* Magic number. */

  _umagic magic;

  /* Type flags passed to _ucreate(). */

  unsigned type;

  /* Functions for expanding the heap and for shrinking the heap. */

  void *(*alloc_fun)(Heap_t, size_t *, int *);
  void (*release_fun)(Heap_t, void *, size_t);
  int (*expand_fun)(Heap_t, void *, size_t, size_t *, int *);
  void (*shrink_fun)(Heap_t, void *, size_t, size_t *);

  /* SEG_HEAD is the head of the segment list. */

  struct _um_seg *seg_head;

  /* INITIAL_SEG points to the initial segment created by _ucreate()
     in the same memory area as this structure.  The initial segment
     is never deallocated. */

  struct _um_seg *initial_seg;

  /* INITIAL_SEG_SIZE is the original size of the initial segment.  We
     need INITIAL_SEG_SIZE for deallocating memory contiguously added
     to the end of the initial segment. */

  size_t initial_seg_size;

  /* This is the number of segments.  That value could be computed
     from the segment list.  However, we use it to avoid an infinite
     loop in _uheapchk(). */

  size_t n_segments;

  /* This is the number of crates.  We use this number to avoid an
     infinite loop in _uheapchk(). */

  size_t n_crates;

  /* This is an upper limit on the number of free crumbs.  It's just
     the maximum number of crumbs all the crates can hold.  We use
     this number to avoid an infinite loop in _uheapchk(). */

  size_t max_crumbs;

  /* This array holds the control structures for the buckets. */

  struct _um_bucket buckets[_UM_BUCKETS];

  /* This is the number of createsets.  A heap can have up to
     _UM_MAX_CREATESETS createsets. */

  int n_cratesets;

  /* This array holds the control structures for the createsets. */

  struct _um_crateset cratesets[_UM_MAX_CRATESETS];

  /* Fast mutex semaphore for locking the heap.  There's no point in
     using an HMTX; if the owner dies, we can't continue anyway
     because the heap is probably not in a consistent state. */

  _fmutex fsem;

  /* This member is used for various purposes by _uheapchk():
     _seg_walk() counts the number of crates in SCRATCH1.
     _check_bucket() uses SCRATCH1 as upper limit for the number of
     free lumps. */

  size_t scratch1;
};


/* Defines types for walker functions. */

typedef int _um_callback1 (const void *, size_t, int, int, const char *,
                           size_t);

typedef int _um_callback2 (Heap_t, const void *, size_t, int, int,
                           const char *, size_t, void *);



/* Prototypes for internal functions. */

Heap_t _um_addmem_nolock (Heap_t, void *, size_t, int);
void *_um_alloc_no_lock (Heap_t, size_t, size_t, unsigned);
void _um_crumb_free_maybe_lock (struct _um_crate *, struct _um_crumb *, int);
void *_um_default_alloc (Heap_t, size_t *, int *);
int _um_default_expand (Heap_t, void *, size_t, size_t *, int *);
void _um_default_release (Heap_t, void *, size_t);
void _um_default_shrink (Heap_t, void *, size_t, size_t *);
void _um_free_maybe_lock (void *, int);
Heap_t _um_init_default_regular_heap (void);
Heap_t _um_init_default_tiled_heap (void);
void *_um_lump_alloc (Heap_t, size_t, size_t, unsigned);
void _um_lump_coalesce_free (Heap_t, struct _um_lump *, struct _um_seg *,
    size_t);
void _um_lump_free_maybe_lock (struct _um_seg *, struct _um_lump *, int);
void _um_lump_link_heap (Heap_t, struct _um_lump *);
void _um_lump_make_free (Heap_t, struct _um_lump *, struct _um_seg *, size_t);
void *_um_realloc (void *, size_t, size_t, unsigned);
Heap_t _um_seg_addmem (Heap_t, struct _um_seg *, void *, size_t);
Heap_t _um_seg_setmem (Heap_t, struct _um_seg *, void *, size_t, int);
void _um_lump_unlink_bucket (struct _um_bucket *, struct _um_lump *);
void _um_lump_unlink_heap (Heap_t, struct _um_lump *);
int _um_walk_no_lock (Heap_t, _um_callback2 *, void *);
void _um_abort (const char *, ...)  __attribute__((__noreturn__));


/** @group Low Memory Heap Routines.
 * Intended for internal LIBC use. First call to any of these routines
 * will initialize the heap.
 * @{ */
void *  _lmalloc(size_t);
void *  _lcalloc(size_t, size_t);
void *  _lrealloc(void *, size_t);
Heap_t  _linitheap(void);
/** @} */

/** @group High Memory Heap Routines.
 * Intended for internal LIBC use. First call to any of these routines
 * will initialize the heap.
 * @{ */
void *  _hmalloc(size_t);
void *  _hcalloc(size_t, size_t);
void *  _hrealloc(void *, size_t);
Heap_t  _hinitheap(void);
char *  _hstrdup(const char *);
void *  __libc_HimemDefaultAlloc(Heap_t Heap, size_t *pcb, int *pfClean);
void    __libc_HimemDefaultRelease(Heap_t Heap, void *pv, size_t cb);
#if 0
int     __libc_HimemDefaultExpand(Heap_t Heap, void *pvBase, size_t cbOld, size_t *pcbNew, int *pfClean);
void    __libc_HimemDefaultShrink(Heap_t Heap, void *pvBase, size_t cbOld, size_t *pcbNew);
#endif
int     __libc_HasHighMem(void);
/** @} */


/** @group Default Heap Voting.
 * @{ */
void    __libc_HeapVote(int fDefaultHeapInHighMem);
void    __libc_HeapEndVoting(void);
int     __libc_HeapGetResult(void);
/** @} */



/* Inline functions. */

static __inline__ void _um_heap_lock (Heap_t h)
{
  _fmutex_checked_request (&h->fsem, _FMR_IGNINT);
}


static __inline__ void _um_heap_unlock (Heap_t h)
{
  _fmutex_checked_release (&h->fsem);
}


static __inline__ void _um_crumb_unlink (struct _um_crateset *crateset,
                                         struct _um_crumb *crumb)
{
  if (crumb->x.free.prev == NULL)
    crateset->crumb_head = crumb->x.free.next;
  else
    crumb->x.free.prev->x.free.next = crumb->x.free.next;
  if (crumb->x.free.next == NULL)
    crateset->crumb_tail = crumb->x.free.prev;
  else
    crumb->x.free.next->x.free.prev = crumb->x.free.prev;
}


static __inline__ void _um_lump_set_size (struct _um_lump *lump, size_t size)
{
  lump->size = size;            /* Must be done first */
  ((_umint *)_UM_NEXT_LUMP (lump))[-1] = size;
}


static inline int _um_find_bucket (size_t rsize)
{
  int bucket = __fls (rsize) - 1 - _UM_MIN_BUCKET;
  if (bucket < 0)
    bucket = 0;
  else if (bucket >= _UM_BUCKETS)
    return _UM_BUCKETS - 1;
#if defined(__LIBC_STRICT) || defined(DEBUG)
  if (   _UM_BUCKET_SIZE (bucket) > rsize
      || _UM_BUCKET_SIZE (bucket+1) <= rsize)
    _um_abort ("_um_find_bucked: rsize=%x\n", rsize);
#endif
  return bucket;
}


#if defined (__cplusplus)
}
#endif

#endif /* not _EMX_UMALLOC_H */
