/* assert.h,v 1.2 2004/09/14 22:27:31 bird Exp */
/** @file
 * EMX
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

#if defined (__cplusplus)
}
#endif
