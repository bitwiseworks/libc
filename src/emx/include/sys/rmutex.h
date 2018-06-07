/* sys/rmutex.h (emx+gcc) */

/* This header requires <sys/builtin.h> and <sys/fmutex.h>. */

#ifndef _SYS_RMUTEX_H
#define _SYS_RMUTEX_H

#warning "_rmutex is obsoleted. use _fmutex!"

#define _rmutex             _fmutex
#define _rmutex_request     _fmutex_request
#define _rmutex_release     _fmutex_release
#define _rmutex_close       _fmutex_close
#define _rmutex_create      _fmutex_create
#define _rmutex_open        _fmutex_open
#define _rmutex_available   _fmutex_available
#define _rmutex_dummy       _fmutex_dummy
#define _rmutex_checked_request _fmutex_checked_request
#define _rmutex_checked_release _fmutex_checked_release
#define _rmutex_checked_close   _fmutex_checked_close
#define _rmutex_checked_create  _fmutex_checked_create
#define _rmutex_checked_open    _fmutex_checked_open


#endif /* not _SYS_RMUTEX_H */
