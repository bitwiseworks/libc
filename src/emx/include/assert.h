/* assert.h,v 1.2 2004/09/14 22:27:31 bird Exp */
/** @file
 * EMX
 */

#include <sys/cdefs.h>

/*
 * Unlike other ANSI header files, <assert.h> may usefully be included
 * multiple times, with and without NDEBUG defined.
 */

#if defined (__cplusplus)
extern "C" {
#endif

#undef assert

#if defined (NDEBUG)
#define assert(exp) ((void)0)
#else
void _assert (const char *, const char *, unsigned);
#define assert(exp) ((exp) ? (void)0 : _assert (#exp, __FILE__, __LINE__))
#endif

#ifndef _ASSERT_H_
#define _ASSERT_H_

/*
 * Static assertions.  In principle we could define static_assert for
 * C++ older than C++11, but this breaks if _Static_assert is
 * implemented as a macro.
 *
 * C++ template parameters may contain commas, even if not enclosed in
 * parentheses, causing the _Static_assert macro to be invoked with more
 * than two parameters.
 */
#if __ISO_C_VISIBLE >= 2011 && !defined(__cplusplus)
#define	static_assert	_Static_assert
#endif

#endif /* !_ASSERT_H_ */

#if defined (__cplusplus)
}
#endif
