/** $Id: namespace.h 2259 2005-07-17 13:43:12Z bird $ */
/** @file
 * Replacement for the BSD namespace wrapper.
 */
#ifndef _NAMESPACE_H_
#define _NAMESPACE_H_
#include "libc-alias.h"
/* kill these as they're handled by alias/aliasbsdfuncs.awk */
#define __weak_reference(a,b)   struct __hack
#define __make_alias(a,b)       struct __hack

#define err _err
#define warn _warn
#define getprogname _getprogname

#define _fsync      _STD(fsync)
#define _fstat      _STD(fstat)
#define _fcntl      _STD(fcntl)
#define _writev     _STD(writev)
#define _readv      _STD(readv)
#define _close      _STD(close)
#define _open       _STD(open)
#define _read       _STD(read)
#define _write      _STD(write)
#define _connect    connect
#define _socket     socket
#define __strtok_r  _STD(strtok_r)
#define _fpathconf  _STD(fpathconf)
#define _statfs     _STD(statfs)
#define _fstatfs    _STD(fstatfs)
#define nsdispatch  _nsdispatch
#define _sigprocmask _STD(sigprocmask)
#define _nanosleep   _STD(nanosleep)
#define _sleep       _STD(sleep)
#define __sleep      _STD(sleep)
#define _usleep      _STD(usleep)
#define feenableexcept __feenableexcept
#define fedisableexcept __fedisableexcept

#endif

