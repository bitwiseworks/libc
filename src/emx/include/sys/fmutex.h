/* sys/fmutex.h (emx+gcc) */

/* Fast mutex semaphores. */

/* This header requires <sys/builtin.h>. */

#ifndef _SYS_FMUTEX_H
#define _SYS_FMUTEX_H

#include <sys/cdefs.h>
#include <stdlib.h> /* need abort */
#include <InnoTekLIBC/FastInfoBlocks.h>
#include <386/builtin.h>

__BEGIN_DECLS

/* Constants for _fmutex.fs.  See _fmutex_available() for ordering. */

#define _FMS_AUTO_INITIALIZE    (-1)
#define _FMS_UNINIT       0
#define _FMS_AVAILABLE    1
#define _FMS_OWNED_SIMPLE 2
#define _FMS_OWNED_HARD   3

/* Constants for _fmutex_create() */

#define _FMC_SHARED         0x01
#define _FMC_MUST_COMPLETE  0x02
#define _FMC_DUMMY          0x04    /* internal */

/* Constants for _fmutex_request() */

#define _FMR_IGNINT     0x01
#define _FMR_NOWAIT     0x02

/* We cannot use __attribute__ ((__packed__)) because G++ does not
   support this. */

#pragma pack(1)
typedef struct
{
    /** Handle to event semaphore. */
    unsigned long           hev;
    /** Semaphore status. */
    volatile signed char    fs;
    /** Semaphore create flags. (_FMC_SHARED) */
    unsigned char           flags;
    /** padding the struct to 16 bytes. */
    unsigned char           padding[2];
    /** The (pid << 16) | tid of the owner. */
    volatile unsigned       Owner;
    /** Descriptive name of the mutex. */
    const char             *pszDesc;
} _fmutex;
#pragma pack()

#define _FMUTEX_INITIALIZER                         _FMUTEX_INITIALIZER_EX(0, __FILE__)
#define _FMUTEX_INITIALIZER_EX(fFlags, pszDesc)     { 0, _FMS_AUTO_INITIALIZE, fFlags, {0x42,0x42}, 0, pszDesc }


unsigned __fmutex_request_internal (_fmutex *, unsigned, signed char);
unsigned __fmutex_request_internal_must_complete (_fmutex *, unsigned);
unsigned __fmutex_release_internal (_fmutex *);
unsigned __fmutex_release_internal_must_complete (_fmutex *);


static __inline__ unsigned _fmutex_request (_fmutex *sem, unsigned flags)
{
    if (!(sem->flags & _FMC_MUST_COMPLETE))
    {
        signed char fs = __cxchg (&sem->fs, _FMS_OWNED_SIMPLE);
        if (fs == _FMS_AVAILABLE)
        {
            __atomic_xchg(&sem->Owner, fibGetTidPid());
            return 0;
        }
        else
            return __fmutex_request_internal (sem, flags, fs);
    }
    else
        return __fmutex_request_internal_must_complete (sem, flags);
}


static __inline__ unsigned _fmutex_release (_fmutex *sem)
{
    if (!(sem->flags & _FMC_MUST_COMPLETE))
    {
        signed char fs;
        __atomic_xchg(&sem->Owner, 0);
        fs = __cxchg(&sem->fs, _FMS_AVAILABLE);
        if (fs != _FMS_OWNED_HARD)
            return 0;
        else
            return __fmutex_release_internal (sem);
    }
    else
        return __fmutex_release_internal_must_complete (sem);
}

static __inline__ int _fmutex_available (_fmutex *sem)
{
    return sem->fs <= _FMS_AVAILABLE;
}


static __inline__ int _fmutex_is_owner (_fmutex *sem)
{
    return sem->fs > _FMS_AVAILABLE && sem->Owner == fibGetTidPid();
}

/**
 * Release a semaphore in the child process after the
 * semaphore was locked for the forking the parent.
 *
 * @param   pSem    Semaphore to unlock.
 */
static __inline__ void _fmutex_release_fork(_fmutex *pSem)
{
    pSem->fs = _FMS_AVAILABLE;
    pSem->Owner = 0;
}

unsigned _fmutex_create (_fmutex *, unsigned);
unsigned _fmutex_create2 (_fmutex *sem, unsigned, const char *);
unsigned _fmutex_open (_fmutex *);
unsigned _fmutex_close (_fmutex *);
void _fmutex_dummy (_fmutex *);

void _fmutex_checked_close(_fmutex *);
void _fmutex_checked_create(_fmutex *, unsigned);
void _fmutex_checked_create2(_fmutex *, unsigned, const char *);
void _fmutex_checked_open(_fmutex *);
void _fmutex_abort(_fmutex *, const char *);

static __inline__ void _fmutex_checked_release (_fmutex * sem)
{
    if (sem->Owner != fibGetTidPid())
        _fmutex_abort(sem, "release not owner");
    if (_fmutex_release (sem) != 0)
        _fmutex_abort(sem, "release");
}

static __inline__ void _fmutex_checked_request (_fmutex * sem, unsigned flags)
{
    if (_fmutex_request (sem, flags) != 0)
        _fmutex_abort(sem, "request");
}

__END_DECLS

#endif /* not _SYS_FMUTEX_H */

