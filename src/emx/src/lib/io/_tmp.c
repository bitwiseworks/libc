/* _tmp.c (emx+gcc) */

#include "libc-alias.h"
#include <sys/builtin.h>        /* For <sys/fmutex.h> */
#include <sys/fmutex.h>
#include <emx/startup.h>
#include "_tmp.h"


/*******************************************************************************
*   Global Variables                                                           *
*******************************************************************************/
int _tmpidx = IDX_LO;

/** This semaphore protects _tmpidx. */
static _fmutex _tmpidx_fmutex;

/*******************************************************************************
*   Internal Functions                                                         *
*******************************************************************************/
static void _init1_tmp(void);


_CRT_INIT1(_init1_tmp)

/** Initialize the semaphore -- this function will be called by
   _CRT_init() via the __crtinit1__ set vector. */
CRT_DATA_USED
static void _init1_tmp(void)
{
    static void *hack = (void *)_init1_tmp;
    hack = hack;
    _fmutex_checked_create(&_tmpidx_fmutex, 0);
}


void _tmpidx_lock(void)
{
    _fmutex_checked_request(&_tmpidx_fmutex, _FMR_IGNINT);
}

void _tmpidx_unlock(void)
{
    _fmutex_checked_release(&_tmpidx_fmutex);
}
