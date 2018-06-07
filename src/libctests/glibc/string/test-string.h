/* Test and measure string and memory functions.
   Copyright (C) 1999, 2002, 2004 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Written by Jakub Jelinek <jakub@redhat.com>, 1999.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

typedef struct
{
  const char *name;
  void (*fn) (void);
  long test;
} impl_t;

#ifdef __EMX__ /* aout based */
#include <stdint.h>
extern uintptr_t __impls__;
asm (".stabs  \"___impls__\", 21, 0, 0, 0xffffffff");

#define IMPL(name, test) \
  impl_t tst_ ## name = { #name, (void (*) (void))name, test }; \
   asm (".stabs \"___impls__\", 25, 0, 0, _tst_" #name);
#else
extern impl_t __start_impls[], __stop_impls[];
#define IMPL(name, test) \
  impl_t tst_ ## name							\
  __attribute__ ((section ("impls"), aligned (sizeof (void *))))	\
    = { #name, (void (*) (void))name, test };
#endif

#ifdef TEST_MAIN

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#undef __USE_STRING_INLINES

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <error.h>
#include <errno.h>
#include <time.h>
#define GL(x) _##x
#define GLRO(x) _##x
#include <hp-timing.h>

/* bird added for bsd */
#ifndef TEMP_FAILURE_RETRY /* special GNU idea */
# define TEMP_FAILURE_RETRY(expression) \
  (__extension__							      \
    ({ long int __result;						      \
       do __result = (long int) (expression);				      \
       while (__result == -1L && errno == EINTR);			      \
       __result; }))
#endif
#ifndef HAVE_STRNLEN
static inline size_t strnlen(const char *psz, size_t cch)
{
    const char *psz2 = psz;
    while (cch > 0 && *psz)
        cch--, psz++;
    return psz - psz2;
}
#endif


# define TEST_FUNCTION test_main ()
# define TIMEOUT (4 * 60)
# define OPT_ITERATIONS 10000
# define OPT_RANDOM 10001
# define OPT_SEED 10002

unsigned char *buf1, *buf2;
int ret, do_srandom;
unsigned int seed;
size_t page_size;

hp_timing_t _dl_hp_timing_overhead;

# ifndef ITERATIONS
size_t iterations = 100000;
#  define ITERATIONS_OPTIONS \
  { "iterations", required_argument, NULL, OPT_ITERATIONS },
#  define ITERATIONS_PROCESS \
  case OPT_ITERATIONS:				\
    iterations = strtoul (optarg, NULL, 0);	\
    break;
#  define ITERATIONS iterations
# else
#  define ITERATIONS_OPTIONS
#  define ITERATIONS_PROCESS
# endif

# define CMDLINE_OPTIONS ITERATIONS_OPTIONS \
  { "random", no_argument, NULL, OPT_RANDOM },	\
  { "seed", required_argument, NULL, OPT_SEED },
# define CMDLINE_PROCESS ITERATIONS_PROCESS \
  case OPT_RANDOM:							\
    {									\
      int fdr = open ("/dev/urandom", O_RDONLY);			\
									\
      if (fdr < 0 || read (fdr, &seed, sizeof(seed)) != sizeof (seed))	\
	seed = time (NULL);						\
      if (fdr >= 0)							\
	close (fdr);							\
      do_srandom = 1;							\
      break;								\
    }									\
									\
  case OPT_SEED:							\
    seed = strtoul (optarg, NULL, 0);					\
    do_srandom = 1;							\
    break;

#define CALL(impl, ...)	\
  (* (proto_t) (impl)->fn) (__VA_ARGS__)

#ifdef __EMX__
#define FOR_EACH_IMPL(impl, notall) \
    for (uintptr_t *_ptrfirst = __impls__ == -1 ? &__impls__ - 1 : &__impls__; \
         _ptrfirst; \
         _ptrfirst = NULL) \
      for (uintptr_t *_ptr = *_ptrfirst == -2 ? _ptrfirst + 1 :  _ptrfirst + 2; \
           _ptr && *_ptr != NULL; \
           _ptr = *_ptrfirst == -2 /* emxomf */ || (_ptr - _ptrfirst < *_ptrfirst /* a.out */ ) ? _ptr + 1 : NULL) \
        for (impl_t *impl = *(impl_t **)_ptr; impl; impl = NULL) \
          if (!notall || impl->test)
#else
#define FOR_EACH_IMPL(impl, notall) \
  for (impl_t *impl = __start_impls; impl < __stop_impls; ++impl)	\
    if (!notall || impl->test)
#endif

#define HP_TIMING_BEST(best_time, start, end)	\
  do									\
    {									\
      hp_timing_t tmptime;						\
      HP_TIMING_DIFF (tmptime, start + _dl_hp_timing_overhead, end);	\
      if (best_time > tmptime)						\
	best_time = tmptime;						\
    }									\
  while (0)

static void
test_init (void)
{
  page_size = 2 * getpagesize ();
#ifdef MIN_PAGE_SIZE
  if (page_size < MIN_PAGE_SIZE)
    page_size = MIN_PAGE_SIZE;
#endif
#ifdef HAVE_MMAP
  buf1 = mmap (0, 2 * page_size, PROT_READ | PROT_WRITE,
	       MAP_PRIVATE | MAP_ANON, -1, 0);
  if (buf1 == MAP_FAILED)
    error (EXIT_FAILURE, errno, "mmap failed");
#else
  buf1 = memalign (getpagesize (), 2 * page_size);
  if (!buf1)
    error (EXIT_FAILURE, errno, "memalign failed");
#endif
  if (mprotect (buf1 + page_size, page_size, PROT_NONE))
    error (EXIT_FAILURE, errno, "mprotect failed");
#ifdef HAVE_MMAP
  buf2 = mmap (0, 2 * page_size, PROT_READ | PROT_WRITE,
	       MAP_PRIVATE | MAP_ANON, -1, 0);
  if (buf2 == MAP_FAILED)
    error (EXIT_FAILURE, errno, "mmap failed");
#else
  buf2 = memalign (getpagesize (), 2 * page_size);
  if (!buf2)
    error (EXIT_FAILURE, errno, "memalign failed");
#endif
  if (mprotect (buf2 + page_size, page_size, PROT_NONE))
    error (EXIT_FAILURE, errno, "mprotect failed");
  HP_TIMING_DIFF_INIT ();
  if (do_srandom)
    {
      printf ("Setting seed to 0x%x\n", seed);
      srandom (seed);
    }

  memset (buf1, 0xa5, page_size);
  memset (buf2, 0x5a, page_size);
}

#endif
