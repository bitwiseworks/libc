/* $Id: safe.h 3943 2014-12-28 23:31:07Z bird $ */
/** @file
 * Macros, prototypes and inline helpers.
 *
 * @copyright   Copyright (C) 2003-2015 knut st. osmundsen <bird-klibc-spam-xiv@anduin.net>
 * @licenses    MIT, BSD2, BSD3, BSD4, LGPLv2.1, LGPLv3, LGPLvFuture.
 */

#ifndef __safe_h__
#define __safe_h__


#include <stdlib.h>
#include <string.h>
void *  _lmalloc(unsigned);

/** Check if ptr is in high memory or not. */
#define SAFE_IS_HIGH(ptr)   ((unsigned)(ptr) >= 512*1024*1024)



/** Wrap a const string. */
#define SAFE_PCSZ(arg) \
    { \
        char *arg##_safe = (char *)arg; \
        if (SAFE_IS_HIGH(arg)) \
        { \
            int cch = strlen((char*)arg) + 1; \
            arg##_safe = _lmalloc(cch); \
            if (!arg##_safe) { rc = 8; goto l_##arg##_failed; } \
            memcpy(arg##_safe, (arg), cch); \
        } else do {} while (0)
/** Use the const string. */
#define SAFE_PCSZ_USE(arg) (PCSZ)arg##_safe
/** Cleanup a const string. */
#define SAFE_PCSZ_DONE(arg) \
        if (arg##_safe != (char *)arg) \
            free(arg##_safe); \
    } \
    l_##arg##_failed: do {} while (0)



/** Wrap an input only buffer. */
#define SAFE_INBUF(arg, len, type) \
    { \
        type *arg##_safe = (type *)arg; \
        if (SAFE_IS_HIGH(arg)) \
        { \
            arg##_safe = (type *)_lmalloc(len); \
            if (!arg##_safe) { rc = 8; goto l_##arg##_failed; } \
            memcpy(arg##_safe, (arg), len); \
        } else do {} while (0)
/** Use the safe input only buffer. */
#define SAFE_INBUF_USE(arg) arg##_safe
/** Cleanup an input only buffer. */
#define SAFE_INBUF_DONE(arg, len) \
        if (arg##_safe != arg) \
           free(arg##_safe); \
    } \
    l_##arg##_failed: do {} while (0)



/** Wrap an input/output buffer. */
#define SAFE_INOUTBUF(arg, len, type) \
    { \
        type *arg##_safe = arg; \
        if (SAFE_IS_HIGH(arg)) \
        { \
            arg##_safe = (type *)_lmalloc(len); \
            if (!arg##_safe) { rc = 8; goto l_##arg##_failed; } \
            memcpy(arg##_safe, (arg), len);\
        } else do {} while (0)
/** Use the safe input/output buffer. */
#define SAFE_INOUTBUF_USE(arg) arg##_safe
/** Cleanup an input/output buffer. */
#define SAFE_INOUTBUF_DONE(arg, len) \
        if (arg##_safe != arg) \
        { \
           memcpy(arg, arg##_safe, len); \
           free(arg##_safe); \
        } \
    } \
    l_##arg##_failed: do {} while (0)



/** Wrap the pointer to an input only type/struct. */
#define SAFE_INTYPE(arg, type) \
   { \
        type *arg##_free = NULL; \
        type *arg##_safe = (type *)arg; \
        if (SAFE_IS_HIGH(arg)) \
        { \
            if (sizeof(type) < 256) \
                arg##_safe = (type *)alloca(sizeof(type)); \
            else \
                arg##_free = arg##_safe = (type *)_lmalloc(sizeof(type)); \
            if (!arg##_safe) { rc = 8; goto l_##arg##_failed; } \
            *arg##_safe = *(arg); \
        } else do {} while (0)
/** Use the safe pointer to an input only type/struct. */
#define SAFE_INTYPE_USE(arg) arg##_safe
/** Cleanup after wrapping an input only type/struct. */
#define SAFE_INTYPE_DONE(arg) \
       if (arg##_free) \
          free(arg##_free); \
    } \
    l_##arg##_failed: do {} while (0)



/** Wrap the pointer to an input/output type/struct. */
#define SAFE_INOUTTYPE(arg, type) \
    { \
        type *arg##_free = NULL; \
        type *arg##_safe = (type *)arg; \
        if (SAFE_IS_HIGH(arg)) \
        { \
            if (sizeof(type) < 256) \
                arg##_safe = (type *)alloca(sizeof(type)); \
            else \
                arg##_free = arg##_safe = (type *)_lmalloc(sizeof(type)); \
            if (!arg##_safe) { rc = 8; goto l_##arg##_failed; } \
            *arg##_safe = *(arg); \
        } else do {} while (0)
/** Use the safe pointer to an input/output type/struct. */
#define SAFE_INOUTTYPE_USE(arg) arg##_safe
/** Cleanup after wrapping an input/output type/struct. */
#define SAFE_INOUTTYPE_DONE(arg) \
        if (arg##_safe != (arg)) \
          *(arg) = *arg##_safe; \
        if (arg##_free) \
           free(arg##_free); \
    } \
    l_##arg##_failed: do {} while (0)


#endif

