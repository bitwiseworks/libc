/* $Id: logstrict.h 2436 2005-11-13 11:35:02Z bird $ */
/** @file
 *
 * InnoTek LIBC - Debug Logging and Strict Checking Features.
 *
 * InnoTek Systemberatung GmbH
 *
 * Copyright (c) 2004 InnoTek Systemberatung GmbH
 * Copyright (c) 2004-2005 knut st. osmundsen <bird-srcspam@anduin.net>
 * Author: knut st. osmundsen <bird-srcspam@anduin.net>
 *
 * All Rights Reserved
 *
 */

#ifndef __InnoTekLIBC_LOG_H__
#define __InnoTekLIBC_LOG_H__

/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include <stdarg.h>
#include <sys/cdefs.h>
#include <sys/types.h>                  /* size_t */
#include <sys/param.h>                  /* NULL */

/** @defgroup   __libc_log      Debug Logging and Strict Checking Features
 *
 * The logging feature is not accessible unless DEBUG_LOGGING is #defined.
 *
 * The strict checking feature is not accessible unless __LIBC_STRICT is #defined.
 *
 * The user of this feature must #define __LIBC_LOG_GROUP to a valid group
 * number before including this file.
 *
 * The user may also #define __LIBC_LOG_INSTANCE if it doesn't want to use
 * the default logging device.
 *
 * Note that all everything but the main logger & strict macros are using the
 * default prefix to avoid too much namespace pollution. The main logger and
 * strict macros are not prefixed with the double underscore because that
 * would make the code less readable (and mean more typing which is painful).
 *
 * @{
 */


/*******************************************************************************
*   Defined Constants And Macros                                               *
*******************************************************************************/
/**
 * The user may also #define __LIBC_LOG_INSTANCE if it doesn't want to use
 * the default logging device.
 */
#ifndef __LIBC_LOG_INSTANCE
#define __LIBC_LOG_INSTANCE NULL
#endif


/**
 * The user of this feature must #define __LIBC_LOG_GROUP to a valid group
 * number before including this file.
 */
#ifndef __LIBC_LOG_GROUP
#define __LIBC_LOG_GROUP    0
#error "__LIBC_LOG_GROUP must be defined before including InnoTekLIBC/log.h"
#endif


/** Macro to, in release mode too, log a generic message within a function. */
#define LIBCLOG_REL(...) \
    __libc_LogMsg(~0, __LIBC_LOG_INSTANCE, __LIBC_LOG_GROUP, __PRETTY_FUNCTION__, __VA_ARGS__)


/** Macro to log a function entry. */
#ifdef DEBUG_LOGGING
#define LIBCLOG_ENTER(...) \
    unsigned __libclog_uEnterTS__ = __libc_LogEnter(__LIBC_LOG_INSTANCE, __LIBC_LOG_GROUP, __PRETTY_FUNCTION__, __VA_ARGS__)
#else
#define LIBCLOG_ENTER(...)      //...
#endif

/** Macro to log a generic message within a function entered by LIBCLOG_ENTER(). */
#ifdef DEBUG_LOGGING
#define LIBCLOG_MSG(...) \
    __libc_LogMsg(__libclog_uEnterTS__, __LIBC_LOG_INSTANCE, __LIBC_LOG_GROUP, __PRETTY_FUNCTION__, __VA_ARGS__)
#else
#define LIBCLOG_MSG(...)        ((void)0)
#endif

/** Macro to log a generic message within a function. */
#ifdef DEBUG_LOGGING
#define LIBCLOG_MSG2(...) \
    __libc_LogMsg(~0, __LIBC_LOG_INSTANCE, __LIBC_LOG_GROUP, __PRETTY_FUNCTION__, __VA_ARGS__)
#else
#define LIBCLOG_MSG2(...)       ((void)0)
#endif

/** Macro to log an user error within a function entered by LIBCLOG_ENTER(). */
#ifdef DEBUG_LOGGING
#define LIBCLOG_ERROR(...) \
    __libc_LogError(__libclog_uEnterTS__, __LIBC_LOG_INSTANCE, __LIBC_LOG_GROUP, __PRETTY_FUNCTION__, __FILE__, __LINE__, __VA_ARGS__)
#else
#define LIBCLOG_ERROR(...)      ((void)0)
#endif

/** Macro to log an user error within a function. */
#ifdef DEBUG_LOGGING
#define LIBCLOG_ERROR2(...) \
    __libc_LogError(~0, __LIBC_LOG_INSTANCE, __LIBC_LOG_GROUP, __PRETTY_FUNCTION__, __FILE__, __LINE__, __VA_ARGS__)
#else
#define LIBCLOG_ERROR2(...)     ((void)0)
#endif

/** Macro to check for and log an user error within a function entered by LIBCLOG_ENTER(). */
#ifdef DEBUG_LOGGING
#define LIBCLOG_ERROR_CHECK(expr, ...) \
    (!(expr) ? __libc_LogError(__libclog_uEnterTS__, __LIBC_LOG_INSTANCE, __LIBC_LOG_GROUP, __PRETTY_FUNCTION__, __FILE__, __LINE__, __VA_ARGS__) : ((void)0) )
#else
#define LIBCLOG_ERROR_CHECK(expr, ...) ((void)0)
#endif

/** Macro to check for and log an user error within a function. */
#ifdef DEBUG_LOGGING
#define LIBCLOG_ERROR2_CHECK(expr, ...) \
    (!(expr) ? __libc_LogError(~0, __LIBC_LOG_INSTANCE, __LIBC_LOG_GROUP, __PRETTY_FUNCTION__, __FILE__, __LINE__, __VA_ARGS__) : ((void)0) )
#else
#define LIBCLOG_ERROR2_CHECK(expr, ...) ((void)0)
#endif

/** Macro to log a raw message. */
#ifdef DEBUG_LOGGING
#define LIBCLOG_RAW(string, maxlen) \
    __libc_LogRaw(__LIBC_LOG_INSTANCE, __LIBC_LOG_GROUP, string, maxlen)
#else
#define LIBCLOG_RAW(...)        ((void)0)
#endif

/** Macro to leave a function entered by LIBCLOG_ENTER(). */
#ifdef DEBUG_LOGGING
#define LIBCLOG_LEAVE(...) \
    __libc_LogLeave(__libclog_uEnterTS__, __LIBC_LOG_INSTANCE, __LIBC_LOG_GROUP, __PRETTY_FUNCTION__, __VA_ARGS__)
#else
#define LIBCLOG_LEAVE(...)      ((void)0)
#endif

/** Macro to log a custom message and return. */
#define LIBCLOG_RETURN_MSG(rc,...)      do { LIBCLOG_LEAVE(__VA_ARGS__); return (rc); } while (0)
/** Macro to log a custom message and return. */
#define LIBCLOG_RETURN_MSG_VOID(...)    do { LIBCLOG_LEAVE(__VA_ARGS__); return; } while (0)

/** Macro to log a void return and do the return. */
#define LIBCLOG_RETURN_VOID()           LIBCLOG_RETURN_MSG_VOID( "ret void\n")
/** Macro to log an int return and do the return. */
#define LIBCLOG_RETURN_INT(rc)          LIBCLOG_RETURN_MSG((rc), "ret %d (%#x)\n", (rc), (rc))
/** Macro to log an unsigned int return and do the return. */
#define LIBCLOG_RETURN_UINT(rc)         LIBCLOG_RETURN_MSG((rc), "ret %u (%#x)\n", (rc), (rc))
/** Macro to log an long int return and do the return. */
#define LIBCLOG_RETURN_LONG(rc)         LIBCLOG_RETURN_MSG((rc), "ret %ld (%#lx)\n", (rc), (rc))
/** Macro to log an unsigned long int return and do the return. */
#define LIBCLOG_RETURN_ULONG(rc)        LIBCLOG_RETURN_MSG((rc), "ret %lu (%#lx)\n", (rc), (rc))
/** Macro to log a pointer return and do the return. */
#define LIBCLOG_RETURN_P(rc)            LIBCLOG_RETURN_MSG((rc), "ret %p\n", (void*)(rc))


/** Macro to leave a function entered by LIBCLOG_ENTER() on user error. */
#ifdef DEBUG_LOGGING
#define LIBCLOG_ERROR_LEAVE(...) \
    __libc_LogErrorLeave(__libclog_uEnterTS__, __LIBC_LOG_INSTANCE, __LIBC_LOG_GROUP, __PRETTY_FUNCTION__, __FILE__, __LINE__, __VA_ARGS__)
#else
#define LIBCLOG_ERROR_LEAVE(...)      ((void)0)
#endif

/** Macro to log a user error and return. */
#define LIBCLOG_ERROR_RETURN(rc,...)          do { LIBCLOG_ERROR_LEAVE(__VA_ARGS__); return (rc); } while (0)
#define LIBCLOG_ERROR_RETURN_MSG              LIBCLOG_ERROR_RETURN
/** Macro to log a user error and return. */
#define LIBCLOG_ERROR_RETURN_MSG_VOID(...)    do { LIBCLOG_ERROR_LEAVE(__VA_ARGS__); return; } while (0)

/** Macro to log a void return user error and do the return. */
#define LIBCLOG_ERROR_RETURN_VOID()           LIBCLOG_ERROR_RETURN_MSG_VOID( "ret void\n")
/** Macro to log an int return user error and do the return. */
#define LIBCLOG_ERROR_RETURN_INT(rc)          LIBCLOG_ERROR_RETURN_MSG((rc), "ret %d (%#x)\n", (rc), (rc))
/** Macro to log an unsigned int return user error and do the return. */
#define LIBCLOG_ERROR_RETURN_UINT(rc)         LIBCLOG_ERROR_RETURN_MSG((rc), "ret %u (%#x)\n", (rc), (rc))
/** Macro to log an long int return user error and do the return. */
#define LIBCLOG_ERROR_RETURN_LONG(rc)         LIBCLOG_ERROR_RETURN_MSG((rc), "ret %ld (%#lx)\n", (rc), (rc))
/** Macro to log an unsigned long int return user error and do the return. */
#define LIBCLOG_ERROR_RETURN_ULONG(rc)        LIBCLOG_ERROR_RETURN_MSG((rc), "ret %lu (%#lx)\n", (rc), (rc))
/** Macro to log a pointer return user error and do the return. */
#define LIBCLOG_ERROR_RETURN_P(rc)            LIBCLOG_ERROR_RETURN_MSG((rc), "ret %p\n", (void*)(rc))

/** Macro to log an typical int return which is an error if negative and otherwise a success. */
#define LIBCLOG_MIX_RETURN_INT(rc)            do { if (rc < 0) LIBCLOG_ERROR_RETURN_INT(rc); LIBCLOG_RETURN_INT(rc); } while (0)

/** Macro to log an typical int return which is an error if not zero and otherwise a success. */
#define LIBCLOG_MIX0_RETURN_INT(rc)           do { if (rc) LIBCLOG_ERROR_RETURN_INT(rc); LIBCLOG_RETURN_INT(rc); } while (0)


/** @defgroup   __libc_log_init_flags    Flags for __libc_LogInitEx
 *
 * @{
 */
/** Do not write the log header. */
#define __LIBC_LOG_INIT_NOHEADER    0x00000001
/** Do not write the log entry column headers to the log header. */
#define __LIBC_LOG_INIT_NOLEGEND    0x00000002
/** Use UTF-8 encoding in the log file. */
#define __LIBC_LOG_INIT_UTF8        0x00000004
/** @} */


/** @defgroup   __libc_log_flags    Message Flags (to be combined with group)
 *
 * In source files which uses the logger you can or these flags together
 * with a group specification in the __LIBC_LOG_GROUP #define.
 * @{
 */
/** Forces a flush of the output file after the message have been written. */
#define __LIBC_LOG_MSGF_FLUSH       0x00010000
/** Forces writing the message even if the respective log group is disabled. */
#define __LIBC_LOG_MSGF_ALWAYS      0x00020000
/** @} */


/** @defgroup   __libc_log_groups   Default Logging Groups
*
 * In source files which uses the default logger you must #define
 * __LIBC_LOG_GROUP to one of these defines.
 *
 * @{
 */
/** whatever. */
#define __LIBC_LOG_GRP_NOGROUP      0

/*-- LIBC --*/
/** Process APIs. */
#define __LIBC_LOG_GRP_PROCESS      1
/** Heap APIs. */
#define __LIBC_LOG_GRP_HEAP         2
/** File stream APIs. */
#define __LIBC_LOG_GRP_STREAM       3
/** Other I/O APIs. */
#define __LIBC_LOG_GRP_IO           4
/** String APIs. */
#define __LIBC_LOG_GRP_STRING       5
/** Locale APIs. */
#define __LIBC_LOG_GRP_LOCALE       6
/** Regular expression APIs. */
#define __LIBC_LOG_GRP_REGEX        7
/** Math APIs. */
#define __LIBC_LOG_GRP_MATH         8
/** Time APIs. */
#define __LIBC_LOG_GRP_TIME         9
/** BSD DB APIs. */
#define __LIBC_LOG_GRP_BSD_DB       10
/** GLIBC POSIX APIs. */
#define __LIBC_LOG_GRP_GLIBC_POSIX  11
/** Thread APIs. */
#define __LIBC_LOG_GRP_THREAD       12
/** Mutex Semaphores. */
#define __LIBC_LOG_GRP_MUTEX        13
/** Signal APIs and Events. */
#define __LIBC_LOG_GRP_SIGNAL       14
/** Environment APIs. */
#define __LIBC_LOG_GRP_ENV          15
/** Memory Manager APIs. */
#define __LIBC_LOG_GRP_MMAN         16
/** Load APIs. */
#define __LIBC_LOG_GRP_LDR          1 /** @todo fix me */

/** Backend SysV IPC APIs. */
#define __LIBC_LOG_GRP_BACK_IPC     17
/** Backend Thread APIs. */
#define __LIBC_LOG_GRP_BACK_THREAD  18
/** Backend Process APIs. */
#define __LIBC_LOG_GRP_BACK_PROCESS 19
/** Backend Signal APIs and Events. */
#define __LIBC_LOG_GRP_BACK_SIGNAL  20
/** Backend Memory Mananger APIs. */
#define __LIBC_LOG_GRP_BACK_MMAN    21
/** Backend Loader APIs. */
#define __LIBC_LOG_GRP_BACK_LDR     22
/** Backend Filesystem APIs. */
#define __LIBC_LOG_GRP_BACK_FS      23
/** Shared Process Database and LIBC Shared Memory APIs. */
#define __LIBC_LOG_GRP_BACK_SPM     24
/** Fork APIs. */
#define __LIBC_LOG_GRP_FORK         25
/** Backend IO APIs. */
#define __LIBC_LOG_GRP_BACK_IO      26
/** Init/Term APIs and Events. */
#define __LIBC_LOG_GRP_INITTERM     27
/** Backend APIs. */
#define __LIBC_LOG_GRP_BACKEND      28
/** Misc APIs. */
#define __LIBC_LOG_GRP_MISC         29
/** BSD Gen APIs. */
#define __LIBC_LOG_GRP_BSD_GEN      30
/** GLIBC Misc APIs. */
#define __LIBC_LOG_GRP_GLIBC_MISC   31

/*-- other libraries/APIs --*/
/** Socket APIs. */
#define __LIBC_LOG_GRP_SOCKET       32
/** Other TCP/IP APIs. */
#define __LIBC_LOG_GRP_TCPIP        33
/** iconv APIs. */
#define __LIBC_LOG_GRP_ICONV        34
/** Dynamic Library (libdl) APIs. */
#define __LIBC_LOG_GRP_DLFCN        35
/** Posix thread APIs. */
#define __LIBC_LOG_GRP_PTHREAD      36
/** Posix thread APIs. */
#define __LIBC_LOG_GRP_DOSEX        37
/** TLS APIs. */
#define __LIBC_LOG_GRP_TLS          38

/** @todo complete this */
#define __LIBC_LOG_GRP_MAX          38
/** @} */


/** @defgroup   __libc_log_strict   Strict Assertions
 * @{ */

/** Generic assertion.
 * @param expr  Boolean expression,
 */
#ifdef __LIBC_STRICT
#define LIBC_ASSERT(expr) ((expr) ? (void)0 \
    : __libc_LogAssert(__LIBC_LOG_INSTANCE, __LIBC_LOG_GROUP, __PRETTY_FUNCTION__, __FILE__, __LINE__, #expr, NULL))
#else
#define LIBC_ASSERT(expr) ((void)0)
#endif

/** Generic assertion failed.
 * (Yeah, this always fails.)
 */
#ifdef __LIBC_STRICT
#define LIBC_ASSERT_FAILED() __libc_LogAssert(__LIBC_LOG_INSTANCE, __LIBC_LOG_GROUP, __PRETTY_FUNCTION__, __FILE__, __LINE__, "0", NULL)
#else
#define LIBC_ASSERT_FAILED() ((void)0)
#endif

/** Assert that a memory buffer is readable.
 * @param pv    Pointer to buffer.
 * @param cb    Size of buffer.
 */
#ifdef __LIBC_STRICT
#define LIBC_ASSERT_MEM_R(pv, cb) (__libc_StrictMemoryR((pv), (cb)) ? (void)0 \
    : __libc_LogAssert(__LIBC_LOG_INSTANCE, __LIBC_LOG_GROUP, __PRETTY_FUNCTION__, __FILE__, __LINE__, #pv "; " #cb, \
                       "Memory buffer at %p of %d bytes isn't readable!\n", (pv), (cb)))
#else
#define LIBC_ASSERT_MEM_R(pv, cb) ((void)0)
#endif

/** Assert that a memory buffer is readable and writable.
 * @param pv    Pointer to buffer.
 * @param cb    Size of buffer.
 */
#ifdef __LIBC_STRICT
#define LIBC_ASSERT_MEM_RW(pv, cb) (__libc_StrictMemoryRW((pv), (cb)) ? (void)0 \
    : __libc_LogAssert(__LIBC_LOG_INSTANCE, __LIBC_LOG_GROUP, __PRETTY_FUNCTION__, __FILE__, __LINE__, #pv "; " #cb, \
                       "Memory buffer at %p of %d bytes isn't readable and writable!\n", (pv), (cb)))
#else
#define LIBC_ASSERT_MEM_RW(pv, cb) ((void)0)
#endif

/** Assert that a zero terminated string is readable.
 * @param psz    Pointer to buffer.
 */
#ifdef __LIBC_STRICT
#define LIBC_ASSERT_STR(psz) (__libc_StrictStringR((psz), ~0) ? (void)0 \
    : __libc_LogAssert(__LIBC_LOG_INSTANCE, __LIBC_LOG_GROUP, __PRETTY_FUNCTION__, __FILE__, __LINE__, #psz, \
                       "String at %p isn't readable!\n", (psz)))
#else
#define LIBC_ASSERT_STR(psz) ((void)0)
#endif

/** Assert that a zero terminated string with a maximum lenght is readable.
 * @param psz       Pointer to buffer.
 * @param cchMax    Max string length.
 */
#ifdef __LIBC_STRICT
#define LIBC_ASSERT_NSTR(psz, cchMax) (__libc_StrictStringR((psz), cchMax) ? (void)0 \
    : __libc_LogAssert(__LIBC_LOG_INSTANCE, __LIBC_LOG_GROUP, __PRETTY_FUNCTION__, __FILE__, __LINE__, #psz " " #cchMax, \
                       "String at %p of maximum %d bytes isn't readable!\n", (psz), (cchMax)))
#else
#define LIBC_ASSERT_NSTR(psz, cchMax) ((void)0)
#endif


/** Generic assertion, custom message.
 * @param expr  Boolean expression,
 * @param ...   Custom error message.
 */
#ifdef __LIBC_STRICT
#define LIBC_ASSERTM(expr, ...) ((expr) ? (void)0 \
    : __libc_LogAssert(__LIBC_LOG_INSTANCE, __LIBC_LOG_GROUP, __PRETTY_FUNCTION__, __FILE__, __LINE__, #expr, \
                       __VA_ARGS__))
#else
#define LIBC_ASSERTM(expr, ...) ((void)0)
#endif

/** Generic assertion failed, custom message.
 * (Yeah, this always fails.)
 * @param ...   Custom error message.
 */
#ifdef __LIBC_STRICT
#define LIBC_ASSERTM_FAILED(...) __libc_LogAssert(__LIBC_LOG_INSTANCE, __LIBC_LOG_GROUP, __PRETTY_FUNCTION__, __FILE__, __LINE__, "0", __VA_ARGS__)
#else
#define LIBC_ASSERTM_FAILED(...) ((void)0)
#endif

/** Assert that a memory buffer is readable, custom message
 * @param pv    Pointer to buffer.
 * @param cb    Size of buffer.
 */
#ifdef __LIBC_STRICT
#define LIBC_ASSERTM_MEM_R(pv, cb, ...) (__libc_StrictMemoryR((pv), (cb)) ? (void)0 \
    : __libc_LogAssert(__LIBC_LOG_INSTANCE, __LIBC_LOG_GROUP, __PRETTY_FUNCTION__, __FILE__, __LINE__, #pv "; " #cb, \
                       __VA_ARGS__))
#else
#define LIBC_ASSERTM_MEM_R(pv, cb, ...) ((void)0)
#endif

/** Assert that a memory buffer is readable and writable, custom message
 * @param pv    Pointer to buffer.
 * @param cb    Size of buffer.
 */
#ifdef __LIBC_STRICT
#define LIBC_ASSERTM_MEM_RW(pv, cb, ...) (__libc_StrictMemoryRW((pv), (cb)) ? (void)0 \
    : __libc_LogAssert(__LIBC_LOG_INSTANCE, __LIBC_LOG_GROUP, __PRETTY_FUNCTION__, __FILE__, __LINE__, #pv "; " #cb, \
                       __VA_ARGS__))
#else
#define LIBC_ASSERTM_MEM_RW(pv, cb, ...) ((void)0)
#endif

/** Assert that a zero terminated string is readable, custom message
 * @param psz    Pointer to buffer.
 */
#ifdef __LIBC_STRICT
#define LIBC_ASSERTM_STR(psz, ...) (__libc_StrictStringR((psz), ~0) ? (void)0 \
    : __libc_LogAssert(__LIBC_LOG_INSTANCE, __LIBC_LOG_GROUP, __PRETTY_FUNCTION__, __FILE__, __LINE__, #psz, \
                       __VA_ARGS__))
#else
#define LIBC_ASSERTM_STR(psz, ...) ((void)0)
#endif

/** Assert that a zero terminated string with a maximum lenght is readable, custom message
 * @param psz       Pointer to buffer.
 * @param cchMax    Max string length.
 */
#ifdef __LIBC_STRICT
#define LIBC_ASSERTM_NSTR(psz, cchMax, ...) (__libc_StrictStringR((psz), cchMax) ? (void)0 \
    : __libc_LogAssert(__LIBC_LOG_INSTANCE, __LIBC_LOG_GROUP, __PRETTY_FUNCTION__, __FILE__, __LINE__, #psz " " #cchMax, \
                       __VA_ARGS__))
#else
#define LIBC_ASSERTM_NSTR(psz, cchMax, ...) ((void)0)
#endif

/** Extracts the group from the fGroupAndFlags argument. */
#define __LIBC_LOG_GETGROUP(fGroupAndFlags) ((fGroupAndFlags) & 0xffff)

/** @} */


/*******************************************************************************
*   Structures and Typedefs                                                    *
*******************************************************************************/
/** Logging group. */
typedef struct __libc_log_group
{
    /** Set if logging for the group is enabled, clear if it's disabled. */
    int             fEnabled;
    /** Group name */
    const char *    pszGroupName;
} __LIBC_LOGGROUP, *__LIBC_PLOGGROUP;

/** Ordered collection of logging groups. */
typedef struct __libc_log_groups
{
    /** Group index base. This value is subtracted from the group part of the
     * fFlagsAndGroups arguments to make an index into paGroups. */
    unsigned            uBase;
    /** Number of groups in the array. */
    unsigned            cGroups;
    /** Array of log groups. */
    __LIBC_PLOGGROUP    paGroups;
} __LIBC_LOGGROUPS, *__LIBC_PLOGGROUPS;


/*******************************************************************************
*   External Functions                                                         *
*******************************************************************************/
__BEGIN_DECLS
/**
 * Returns a default log directory for LIBC logging API.
 *
 * This is a directory where logger instances create log files by default. See
 * __libc_LogInitEx for more info.
 *
 * The default log directory is selected from the environment in the following
 * order (the respective environment variable must be defined and be non-empty,
 * otherwise the next path will be selected):
 *
 * 1. %LOGFILES%\app
 * 2. %UNIXROOT%\var\log\app
 * 3. Root of boot drive
 *
 * If a path pointed to by the environment variable does not actually exist, or
 * if a specified subdirectory in it cannot be created, path 3 (which always
 * exists) will be used as a fallback. For logger instances using the default
 * log directory this means that log files will be created (in the root
 * directory) even if there is a failure or misconfiguration of the system
 * directory structure.
 *
 * Note that the returned string does not have a trailing backslash unless it's
 * a root directory.
 */
extern const char *__libc_LogGetDefaultLogDir(void);

/**
 * Create a logger.
 *
 * This is equivalent to calling __libc_LogInitEx(NULL, fFlags, pGroups, NULL, pszFilenameFormat, ...).
 *
 * @returns Pointer to a logger instance on success.
 * @returns NULL on failure. errno is set.
 * @param   fFlags              Combination of __LIBC_LOG_INIT_* flags or zero.
 * @param   pGroups             Pointer to a table of logging groups used for this
 *                              logger instance.
 * @param   pszFilenameFormat   Format string for making up the log filename.
 * @param   ...                 Arguments to the format string.
 */
extern void *__libc_LogInit(unsigned fFlags, __LIBC_PLOGGROUPS pGroups, const char *pszFilenameFormat, ...) __printflike(3, 4);

/**
 * Create a logger (extended version).
 *
 * Each logger has an origin, i.e. an entity that emits log entries. It is
 * specified in the pszOrigin argument and usually is a symbolic library or
 * application name. Depending on flags and other arguments, it may be used as
 * part of the log file name or in log entries themselves to distinguish from
 * other log entities. The origin string must not contain symbols that are not
 * valid in filenames (e.g. slashes, colons, angle braces etc.).
 *
 * By default, log entries are written to a file with a name made up using the
 * pszFilenameFormat argument which is created in a directory returned by
 * __libc_LogGetDefaultLogDir (check it for more info on directory selection).
 *
 * Normally, pszFilenameFormat should not provide any path information. If it
 * does and making up results in an absolute path specification, the described
 * log directory selection will be turned off. If the path is not absolute, the
 * file will be created relative to the selected log directory. However, if any
 * component of its path does not exist, it will not be created and this
 * function will fail.
 *
 * If pszFilenameFormat is NULL, then a default filename will be made up using
 * the following format: "TTTTTTTT-PPPP-EXE[-ORIGIN].log" where TTTTTTTT is a
 * value of QSV_TIME_LOW (the number of seconds since 01.01.1970) in hex, PPPP
 * is the current PID in hex, EXE is the current executable name (w/o extension)
 * and ORIGIN is the value of pszOrigin unless it's NULL in which case ORIGIN is
 * omitted. It is always recommended to use the default filename in order to
 * avoid filename clashes between different processes and applications which is
 * especially important when many processes and applications write logs to the
 * same directory.
 *
 * The environment variable passed in pszEnvVar allows to change the destination
 * of logging at runtime and recognizes the following values:
 *
 * CURDIR - the log file will be created relative to the current directory.
 * STDOUT - log entries will be written to the standard output stream.
 * STDERR - log entries will be written to the standard error stream.
 *
 * Setting the destination to STDOUT or STDERR will have the following impact on
 * the way how the logger works:
 *
 * - The log file header will not be written.
 * - The TID field of each log entry will be prefixed with a PID value in hex
 *   and the GROUP field will be prefixed with an ORIGIN value if it's not NULL.
 *   This is to distinguish between different processes writing to the same
 *   stream.
 *
 * It's best to name the environment variable similarly to the environment
 * variable passed to __libc_LogGroupInit. E.g., if you pass "MYAPP_LOGGING" to
 * __libc_LogGroupInit, pass something like "MYAPP_LOGGING_OUTPUT" to
 * __libc_LogInitEx.
 *
 * Passing NULL in pszEnvVar is also supported and will force log files to be
 * always created in the current directory without a possibility to change the
 * destination to stdout/stderr at runtime (for backward compatibility).
 *
 * @returns Pointer to a logger instance on success.
 * @returns NULL on failure. errno is set.
 * @param   pszOrigin           Log origin or NULL.
 * @param   fFlags              Combination of __LIBC_LOG_INIT_* flags or zero.
 * @param   pGroups             Pointer to a table of logging groups used for this
 *                              logger instance.
 * @param   pszEnvVar           Name of the environment variable.
 *                              This is taken from the initial environment of the process
 *                              and not from the current!!
 * @param   pszFilenameFormat   Format string for making up the log filename.
 * @param   ...                 Arguments to the format string.
 */
extern void *__libc_LogInitEx(const char *pszOrigin, unsigned fFlags, __LIBC_PLOGGROUPS pGroups, const char *pszEnvVar, const char *pszFilenameFormat, ...) __printflike(5, 6);

/**
 * Parses the given environment variable and sets the group
 * flags accordingly.
 *
 * The environment variable is a sequence of group idendifiers with
 * a prefix which determins whether or not that group is enabled.
 * A special group 'all' can be used to address all groups.
 *
 * If the environment variable is not present no changes will be
 * performed.
 *
 * @param   pGroups     Pointer to groups to init.
 * @param   pszEnvVar   Name of the environment variable.
 *                      This is taken from the initial environment of the process
 *                      and not from the current!!
 */
extern void __libc_LogGroupInit(__LIBC_PLOGGROUPS pGroups, const char *pszEnvVar);

/**
 * Terminate (or close if you like) a logger instance.
 * This means flushing any buffered messages and writing a termination
 * message before closing the log file.
 *
 * @returns 0 on succes.
 * @returns -1 on failure, error is set.
 * @param   pvInstance      Logger instance.
 */
extern int   __libc_LogTerm(void *pvInstance);

/**
 * Returns 1 if this log instance's output is set to the standard output or
 * standard input, and 0 otherwise.
 *
 * @returns 1 for TRUE and 0 for FALSE.
 * @param   pvInstance      Logger instance.
 */
extern int   __libc_LogIsOutputToConsole(void *pvInstance);

/**
 * Returns the origin of this log instance.
 *
 * @returns The origin string (may be NULL).
 * @param   pvInstance      Logger instance.
 */
extern const char *__libc_LogGetOrigin(void *pvInstance);

/**
 * Output an enter function log message.
 * An enter message is considered to be one line and is appended a newline if
 * none was given.
 *
 * @returns Current timestamp.
 * @param   pvInstance      Logger instance. If NULL the message goes to the
 *                          default log instance.
 * @param   fGroupAndFlags  Logging group and logging flags.
 * @param   pszFunction     Name of the function which was entered.
 * @param   pszFormat       Format string to display arguments.
 * @param   ...             Arguments to the format string.
 */
extern unsigned __libc_LogEnter(void *pvInstance, unsigned fGroupAndFlags, const char *pszFunction, const char *pszFormat, ...) __printflike(4, 5);

/**
 * Output a leave function log message.
 *
 * A leave message is considered to be one line and is appended a newline if
 * none was given.
 *
 * @param   uEnterTS        The timestamp returned by LogEnter.
 * @param   pvInstance      Logger instance. If NULL the message goes to the
 *                          default log instance.
 * @param   fGroupAndFlags  Logging group and logging flags.
 * @param   pszFunction     Name of the function which was entered.
 * @param   pszFormat       Format string to display the result.
 * @param   ...             Arguments to the format string.
 */
extern void     __libc_LogLeave(unsigned uEnterTS, void *pvInstance, unsigned fGroupAndFlags, const char *pszFunction, const char *pszFormat, ...) __printflike(5, 6);

/**
 * Output a leave function log message upon a user error.
 *
 * The function may breakpoint if configured to do so. A leave message is
 * considered to be one line and is appended a newline if none was given.
 *
 * @param   uEnterTS        The timestamp returned by LogEnter.
 * @param   pvInstance      Logger instance. If NULL the message goes to the
 *                          default log instance.
 * @param   fGroupAndFlags  Logging group and logging flags.
 * @param   pszFunction     Name of the function which was entered.
 * @param   pszFile         Source filename.
 * @param   uLine           Line number.
 * @param   pszFormat       Format string to display the result.
 * @param   ...             Arguments to the format string.
 */
extern void     __libc_LogErrorLeave(unsigned uEnterTS, void *pvInstance, unsigned fGroupAndFlags, const char *pszFunction, const char *pszFile, unsigned uLine, const char *pszFormat, ...) __printflike(7, 8);

/**
 * Output a log message.
 *
 * A log message is considered to be one line and is appended a newline if
 * none was given.
 *
 * @param   uEnterTS        The timestamp returned by LogEnter.
 * @param   pvInstance      Logger instance. If NULL the message goes to the
 *                          default log instance.
 * @param   fGroupAndFlags  Logging group and logging flags.
 * @param   pszFunction     Name of the function which was entered.
 * @param   pszFormat       Format string for the message to log.
 * @param   ...             Arguments to the format string.
 */
extern void     __libc_LogMsg(unsigned uEnterTS, void *pvInstance, unsigned fGroupAndFlags, const char *pszFunction, const char *pszFormat, ...) __printflike(5, 6);

/**
 * Output a log user error message.
 *
 * This may raise a breakpoint exception if configured so. A log message is
 * considered to be one line and is appended a newline if none was given.
 *
 * @param   uEnterTS        The timestamp returned by LogEnter.
 * @param   pvInstance      Logger instance. If NULL the message goes to the
 *                          default log instance.
 * @param   fGroupAndFlags  Logging group and logging flags.
 * @param   pszFunction     Name of the function which was entered.
 * @param   pszFile         Source filename.
 * @param   uLine           Line number.
 * @param   pszFormat       Format string for the message to log.
 * @param   ...             Arguments to the format string.
 */
extern void     __libc_LogError(unsigned uEnterTS, void *pvInstance, unsigned fGroupAndFlags, const char *pszFunction, const char *pszFile, unsigned uLine, const char *pszFormat, ...) __printflike(7, 8);

/**
 * Output a raw log message.
 * Nothing is prepended. No newline is appended.
 *
 * @param   uEnterTS        The timestamp returned by LogEnter.
 * @param   pvInstance      Logger instance. If NULL the message goes to the
 *                          default log instance.
 * @param   fGroupAndFlags  Logging group and logging flags.
 * @param   pszFunction     Name of the function which was entered.
 * @param   pszString       Pointer to raw log message.
 * @param   cchMax          Maximum number of bytes to write.
 */
extern void     __libc_LogRaw(void *pvInstance, unsigned fGroupAndFlags, const char *pszString, unsigned cchMax);

/**
 * Dumps a byte block.
 *
 * @param   uEnterTS        The timestamp returned by LogEnter.
 * @param   pvInstance      Logger instance. If NULL the message goes to the
 *                          default log instance.
 * @param   fGroupAndFlags  Logging group and logging flags.
 * @param   pszFunction     Name of the function which was entered.
 * @param   pvData          Pointer to the bytes to dump.
 * @param   cbData          Number of bytes to dump.
 * @param   pszFormat       Format string for the message to log.
 * @param   ...             Arguments to the format string.
 */
extern void     __libc_LogDumpHex(unsigned uEnterTS, void *pvInstance, unsigned fGroupAndFlags, const char *pszFunction, void *pvData, unsigned cbData, const char *pszFormat, ...);

/**
 * Assertion helper.
 * Logs and displays (stderr) an assertion failed message.
 *
 * @param   pvInstance      Logger instance. If NULL the message goes to the
 *                          default log instance.
 * @param   pszFunction     Name of the function which was entered.
 * @param   pszFile         Source filename.
 * @param   uLine           Line number.
 * @param   pszExpression   Expression.
 * @param   pszFormat       Format string for the message to log.
 * @param   ...             Arguments to the format string.
 */
extern void     __libc_LogAssert(void *pvInstance, unsigned fGroupAndFlags,
                                 const char *pszFunction, const char *pszFile, unsigned uLine, const char *pszExpression,
                                 const char *pszFormat, ...) __printflike(7, 8);

/**
 * Special vsprintf implementation that supports extended format specifiers for logging purposes.
 *
 * Extended format specifiers are:
 * - %YT - prints current TID or PID:TID if pInst is forced to log to console (argument should be 0)
 * - %YG - prints log GROUP or ORIGIN:GROUP if pInst is forced to log to console (argument is a group number).
 * - %Zd - dumps memory in hex (argument is a pointer to memory block whose length is given in precision or width specs, by default 4 bytes).
 *
 * Note that it does not support the full set of standard format specifiers.
 *
 * @returns number of bytes formatted.
 * @param   pInst       Log instance (for %Y format extensions, may be NULL).
 * @param   pszBuffer   Where to put the the formatted string.
 * @param   cchBuffer   Size of the buffer.
 * @param   pszFormat   Format string.
 * @param   args        Argument list.
 */
extern int      __libc_LogVSNPrintf(void *pvInstance, char *pszBuffer, size_t cchBuffer, const char *pszFormat, va_list args);

/**
 * Special sprintf implementation that supports extended format specifiers for logging purposes.
 *
 * See __libc_LogVSNPrintf for more info.
 *
 * @returns number of bytes formatted.
 * @param   pInst       Log instance (for %Y format extensions, may be NULL).
 * @param   pszBuffer   Where to put the the formatted string.
 * @param   cchBuffer   Size of the buffer.
 * @param   pszFormat   Format string.
 * @param   ...         Format arguments.
 */
extern int      __libc_LogSNPrintf(void *pvInstance, char *pszBuffer, size_t cchBuffer, const char *pszFormat, ...);

/**
 * Validate a memory area for read access.
 * @returns 1 if readable.
 * @returns 0 if not entirely readable.
 * @param   pv      Pointer to memory area.
 * @param   cb      Size of memory area.
 */
extern int      __libc_StrictMemoryR(const void *pv, size_t cb);

/**
 * Validate a memory area for read & write access.
 * @returns 1 if readable and writable.
 * @returns 0 if not entirely readable and writable.
 * @param   pv      Pointer to memory area.
 * @param   cb      Size of memory area.
 */
extern int      __libc_StrictMemoryRW(void *pv, size_t cb);

/**
 * Validate a zero terminated string for read access.
 * @returns 1 if readable.
 * @returns 0 if not entirely readable.
 * @param   psz         Pointer to string.
 * @param   cchMax      Max string length. Use ~0 if to very all the
 *                      way to the terminator.
 */
extern int      __libc_StrictStringR(const char *psz, size_t cchMax);

__END_DECLS

/** @} */

#endif
