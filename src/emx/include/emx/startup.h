/* emx/startup.h (emx+gcc) */

#ifndef _EMX_STARTUP_H
#define _EMX_STARTUP_H

#include <sys/cdefs.h>

__BEGIN_DECLS

unsigned long _System _DLL_InitTerm(unsigned long, unsigned long);
extern int  _CRT_init(void);
extern void _CRT_term(void);
extern void __ctordtorInit(void);
extern void __ctordtorTerm(void);

extern void __ctordtorInit1(int *);
extern void __ctordtorTerm1(int *);

/** init and term vectors.
 * @{
 */
/** Array of CRT init functions. */
extern int __crtinit1__;
/** Array of CRT exit functions. */
extern int __crtexit1__;
/** Array of exception handlers something. */
extern int __eh_init__;
/** Array of exception handlers something. */
extern int __eh_term__;
/** Array of constructors. */
extern int __CTOR_LIST__;
/** Array of destructors. */
extern int __DTOR_LIST__;
/** @} */


/* argv[i][-1] contains some flag bits: */

#define _ARG_DQUOTE   0x01          /* Argument quoted (")                  */
#define _ARG_RESPONSE 0x02          /* Argument read from response file     */
#define _ARG_WILDCARD 0x04          /* Argument expanded from wildcard      */
#define _ARG_ENV      0x08          /* Argument from environment            */
#define _ARG_NONZERO  0x80          /* Always set, to avoid end of string   */

/* Arrange that FUN will be called by _CRT_init(). */

#define _CRT_INIT1(fun) __asm__ (".stabs \"___crtinit1__\", 23, 0, 0, _" #fun);

/* Arrange that FUN will be called by _CRT_term(). */

#define _CRT_EXIT1(fun) __asm__ (".stabs \"___crtexit1__\", 23, 0, 0, _" #fun);
 
#if __GNUC_PREREQ__(4,2) 
# define CRT_DATA_USED __attribute__((__used__)) 
#else 
# define CRT_DATA_USED
#endif 

extern char ** _org_environ;

__END_DECLS

#endif /* not _EMX_STARTUP_H */
