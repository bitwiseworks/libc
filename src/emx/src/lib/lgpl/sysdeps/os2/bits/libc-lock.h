/* libc-internal interface for mutex locks.  NPTL version.
   Copyright (C) 1996-2001, 2002, 2003, 2005 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public License as
   published by the Free Software Foundation; either version 2.1 of the
   License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#ifndef _BITS_LIBC_LOCK_H
#define _BITS_LIBC_LOCK_H 1

#include <InnoTekLIBC/thread.h>

#if 1 /* There is a hev leak problem with using fmutexes. */
#include <sys/smutex.h>

typedef _smutex                         __libc_lock_t;
#define __libc_lock_define(CLASS,NAME) \
  CLASS __libc_lock_t NAME;
#define __libc_lock_define_initialized(CLASS,NAME) \
  CLASS __libc_lock_t NAME = 0;
#define __libc_lock_init(NAME)          (NAME) = 0
#define __libc_lock_fini(NAME)          (NAME) = 0
#define __libc_lock_lock(NAME)          _smutex_request(&(NAME))
#define __libc_lock_trylock(NAME)       (!_smutex_try_request(&(NAME)))
#define __libc_lock_unlock(NAME)        _smutex_release(&(NAME))

#else

#include <sys/fmutex.h>

typedef _fmutex                         __libc_lock_t;
#define __libc_lock_define(CLASS,NAME) \
  CLASS __libc_lock_t NAME;
#define __libc_lock_define_initialized(CLASS,NAME) \
  CLASS __libc_lock_t NAME = _FMUTEX_INITIALIZER;
#define __libc_lock_init(NAME)          _fmutex_create(&(NAME), 0)
#define __libc_lock_fini(NAME)          _fmutex_close(&(NAME))
#define __libc_lock_lock(NAME)          _fmutex_request(&(NAME), 0)
#define __libc_lock_trylock(NAME)       _fmutex_request(&(NAME),  _FMR_NOWAIT)
#define __libc_lock_unlock(NAME)        _fmutex_release(&(NAME))
#endif




typedef struct __LIBC_LOCK      __libc_lock_recursive_t;
typedef struct __LIBC_RWLOCK    __libc_rwlock_t;
typedef struct __LIBC_LOCK      __rtld_lock_recursive_t;
typedef int __libc_key_t;

#define __libc_rwlock_define(CLASS,NAME) \
  CLASS __libc_rwlock_t NAME;
#define __libc_lock_define_recursive(CLASS,NAME) \
  CLASS __libc_lock_recursive_t NAME;
#define __rtld_lock_define_recursive(CLASS,NAME) \
  CLASS __rtld_lock_recursive_t NAME;

#define __libc_lock_define_initialized_recursive(CLASS,NAME) \
  CLASS __libc_lock_t NAME = NOT_IMPLEMENTED;
#define __libc_rwlock_define_initialized(CLASS,NAME) \
  CLASS __libc_rwlock_t NAME = NOT_IMPLEMENTED;
#define __rtld_lock_define_initialized_recursive(CLASS,NAME) \
  CLASS __rtld_lock_recursive_t NAME = NOT_IMPLEMENTED;
#define __rtld_lock_initialize(NAME) \
  NOT_IMPLEMENTED

#define __libc_rwlock_init(NAME)        NOT_IMPLEMENTED
#define __libc_lock_init_recursive(NAME) NOT_IMPLEMENTED

#define __libc_rwlock_fini(NAME)        NOT_IMPLEMENTED
#define __libc_lock_fini_recursive(NAME) NOT_IMPLEMENTED

#define __libc_rwlock_rdlock(NAME)      NOT_IMPLEMENTED
#define __libc_lock_lock_recursive(NAME) NOT_IMPLEMENTED

#define __libc_lock_trylock_recursive(NAME) NOT_IMPLEMENTED
#define __libc_rwlock_tryrdlock(NAME)   NOT_IMPLEMENTED
#define __rtld_lock_trylock_recursive(NAME) NOT_IMPLEMENTED

#define __libc_rwlock_unlock(NAME) NOT_IMPLEMENTED
#define __libc_lock_unlock_recursive(NAME) NOT_IMPLEMENTED

#define __rtld_lock_default_lock_recursive(lock) NOT_IMPLEMENTED
#define __rtld_lock_default_unlock_recursive(lock) NOT_IMPLEMENTED
#define __rtld_lock_lock_recursive(NAME) NOT_IMPLEMENTED
#define __rtld_lock_unlock_recursive(NAME) NOT_IMPLEMENTED

#define __libc_once_define(CLASS, NAME)            NOT_IMPLEMENTED
#define __libc_once(ONCE_CONTROL, INIT_FUNCTION)    NOT_IMPLEMENTED
#define __libc_cleanup_region_start(DOIT, FCT, ARG) NOT_IMPLEMENTED
#define __libc_cleanup_region_end(DOIT)     NOT_IMPLEMENTED
#define __libc_cleanup_end(DOIT)            NOT_IMPLEMENTED
#define __libc_cleanup_routine(fct, arg)    NOT_IMPLEMENTED
#define __libc_cleanup_push(fct, arg)       NOT_IMPLEMENTED
#define __libc_cleanup_pop(execute)         NOT_IMPLEMENTED
#define __libc_key_create(KEY, DESTRUCTOR)  NOT_IMPLEMENTED
#define __libc_getspecific(KEY)             NOT_IMPLEMENTED
#define __libc_setspecific(KEY, VALUE)      NOT_IMPLEMENTED

#endif	/* bits/libc-lock.h */

