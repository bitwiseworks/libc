#ifndef _ERROR_H
#define _ERROR_H

#include <stdarg.h>
#include <stdio.h>

#ifdef	__cplusplus
extern "C" {
#endif

/* Print a message with `fprintf (stderr, FORMAT, ...)';
   if ERRNUM is nonzero, follow it with ": " and strerror (ERRNUM).
   If STATUS is nonzero, terminate the program with `exit (STATUS)'.  */

static inline void error (int __status, int __errnum, const char *__format, ...)
{
    va_list __args;
    fflush(stdout);
    va_start(__args, __format);
    vfprintf(stderr, __format, __args);
    if (__errnum)
        strerror(__errnum);
    va_end(__args);
    if (__status)
        exit(__status);
}

static inline void error_at_line (int __status, int __errnum, const char *__fname,
			   unsigned int __lineno, const char *__format, ...)
{
    va_list __args;
    fflush(stdout);
    va_start(__args, __format);
    fprintf(stderr, "%s(%d): ", __fname, __lineno);
    vfprintf(stderr, __format, __args);
    if (__errnum)
        strerror(__errnum);
    va_end(__args);
    if (__status)
        exit(__status);
}

/* If NULL, error will flush stdout, then print on stderr the program
   name, a colon and a space.  Otherwise, error will call this
   function without parameters instead.  */
extern void (*error_print_progname) (void);


/* This variable is incremented each time `error' is called.  */
extern unsigned int error_message_count;

/* Sometimes we want to have at most one error per line.  This
   variable controls whether this mode is selected or not.  */
extern int error_one_per_line;

#ifdef	__cplusplus
}
#endif

#endif /* error.h */

