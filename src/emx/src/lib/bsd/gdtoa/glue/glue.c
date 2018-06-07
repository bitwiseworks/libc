/*
 * Machine-independent glue to integrate David Gay's gdtoa
 * package into libc.
 *
 * $FreeBSD: src/lib/libc/gdtoa/glue.c,v 1.2 2003/06/21 08:20:14 das Exp $
 */

#ifdef __EMX__

#include <386/builtin.h>
#include <sys/fmutex.h>
_fmutex __libc_gdtoa_locks[2] = {0};

#else

#include <pthread.h>

pthread_mutex_t __gdtoa_locks[] = {
	PTHREAD_MUTEX_INITIALIZER,
	PTHREAD_MUTEX_INITIALIZER
};
#endif
