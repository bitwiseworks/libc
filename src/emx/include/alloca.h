/* alloca.h,v 1.2 2004/09/14 22:27:31 bird Exp */
/** @file
 * EMX
 */

#ifndef _ALLOCA_H
#define _ALLOCA_H

#include <sys/cdefs.h>
#include <stddef.h>

__BEGIN_DECLS
#if __GNUC__ >= 2 || defined(__INTEL_COMPILER)
#undef  alloca	/* some GNU bits try to get cute and define this on their own */
#define alloca(sz) __builtin_alloca(sz)
#elif defined(lint)
void	*alloca(size_t);
#endif
__END_DECLS


#endif /* not _ALLOCA_H */
