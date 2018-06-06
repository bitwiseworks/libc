/* sys/times.h (emx+gcc) */
/** @file
 * EMX
 * @changed bird: corrected prototype, added BSD type blocker and use BSD style C++ wrappers.
 */

#ifndef _SYS_TIMES_H
#define _SYS_TIMES_H

#include <sys/cdefs.h>
#include <sys/_types.h>

__BEGIN_DECLS
#if !defined (_CLOCK_T) && !defined(_CLOCK_T_DECLARED)
typedef __clock_t clock_t;
#define _CLOCK_T_DECLARED
#define _CLOCK_T
#endif

struct tms
{
    clock_t tms_utime;      /**< User mode CPU time. */
    clock_t tms_stime;      /**< Kernel mode CPU time. */
    clock_t tms_cutime;     /**< User mode CPU time for waited for children. */
    clock_t tms_cstime;     /**< Kernel mode CPU time for waited for children. */
};

clock_t times(struct tms *);
__END_DECLS

#endif /* not _SYS_TIMES_H */
