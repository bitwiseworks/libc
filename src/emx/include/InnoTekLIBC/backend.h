/* $Id: backend.h 3914 2014-10-24 14:01:38Z bird $ */
/** @file
 * LIBC - Backend header.
 */

/*
 * Copyright (c) 2004-2014 knut st. osmundsen <bird-srcspam@anduin.net>
 *
 * This file is part of InnoTek LIBC.
 *
 * InnoTek LIBC is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * InnoTek LIBC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with InnoTek LIBC; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef __InnoTekLIBC_backend_h__
#define __InnoTekLIBC_backend_h__

#include <sys/cdefs.h>
#include <sys/types.h>
#include <sys/_timeval.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <signal.h>
#include <emx/io.h>
#include <stdarg.h>


__BEGIN_DECLS

#ifndef __LIBC_THREAD_DECLARED
#define __LIBC_THREAD_DECLARED
typedef struct __libc_thread *__LIBC_PTHREAD, **__LIBC_PPTHREAD;
#endif
struct statfs;
struct stat;


/** @defgroup __libc_Back_thread   LIBC Backend - Threads
 * @{ */

/**
 * Initiatlize a new thread structure.
 *
 * @param   pThrd       Pointer to the thread structure.
 * @param   pParentThrd Pointer to the thread structure for the parent thread.
 *                      If NULL and thread id is 1 then inherit from parent process.
 *                      If NULL and thread is not null or no record of parent then
 *                      use defaults.
 */
void __libc_Back_threadInit(__LIBC_PTHREAD pThrd, const __LIBC_PTHREAD pParentThrd);

/**
 * Called before the thread structure is freed so the backend
 * can cleanup its members.
 *
 * @param   pThrd   Pointer to the thread in  question.
 */
void __libc_Back_threadCleanup(__LIBC_PTHREAD pThrd);

/**
 * Called in the context of the newly started thread to register
 * exception handler and to do other init stuff.
 *
 * @param   pExpRegRec  Exception handler registration record on the stack.
 *                      To be used for any exception handler registration.
 */
void __libc_Back_threadStartup(void *pExpRegRec);

/**
 * Called in the context of the thread which is to be terminated to
 * unregister exception handler and to do other final term stuff.
 *
 * @param   pExpRegRec  Exception handler registration record on the stack.
 *                      To be used for any exception handler registration.
 * @remark  This is called after __libc_Back_threadCleanup().
 * @remark  It is not called by thread which calls _endthread(), nor for the
 *          main thread.
 */
void __libc_Back_threadEnd(void *pExpRegRec);

/**
 * Suspend execution of the current thread for a given number of nanoseconds
 * or till a signal is received.
 *
 * @returns 0 on success.
 * @returns -EINVAL if the pReqTS is invalid.
 * @returns -EINTR if the interrupted by signal.

 * @param   ullNanoReq      Time to sleep, in nano seconds.
 * @param   pullNanoRem     Where to store remaining time (also nano seconds).

 * @remark  For relativly small sleeps this api temporarily changes the thread
 *          priority to timecritical (that is, if it's in the normal or idle priority
 *          classes) to increase precision. This means that if a signal or other
 *          asyncronous event is executed, it will be executed at wrong priority.
 *          It also means that if such code changes the priority it will be undone.
 */
int __libc_Back_threadSleep(unsigned long long ullNanoReq, unsigned long long *pullNanoRem);

/** @} */


/** @defgroup __libc_Back_fs   LIBC Backend - File System
 * @{ */

/**
 * Get the statistics for the filesystem which pszPath is located on.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   pszPath     The path to somewhere in the filesystem.
 * @param   pStatFs     Where to store the obtained information.
 */
int __libc_Back_fsStat(const char *pszPath, struct statfs *pStatFs);

/**
 * Get file system statistics
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   fh          The filehandle of any file within the mounted file system.
 * @param   pStatFs     Where to store the statistics.
 */
int __libc_Back_fsStatFH(int fh, struct statfs *pStatFs);

/**
 * Get the statistics for all the mounted filesystems.
 *
 * @returns Number of returned statfs structs on success.
 * @returns Number of mounted filesystems on success if paStatFS is NULL
 * @returns Negative error code (errno.h) on failure.
 * @param   paStatFs    Where to to store the statistics.
 * @param   cStatFS     Number of structures the array pointed to by paStatFs can hold.
 * @param   fFlags      Flags, currently ignored.
 */
int __libc_Back_fsStats(struct statfs *paStatFs, unsigned cStatFs, unsigned fFlags);

/**
 * Schedules all file system buffers for writing.
 *
 * See sync() for OS/2 limitations.
 */
void __libc_Back_fsSync(void);

/**
 * Query filesystem configuration information by path.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   pszPath     Path to query info about.
 * @param   iName       Which path config variable to query.
 * @param   plValue     Where to return the value.
 * @sa      __libc_Back_ioPathConf, fpathconf, pathconf, sysconf.
 */
int __libc_Back_fsPathConf(const char *pszPath, int iName, long *plValue);

/**
 * Resolves the path into an canonicalized absolute path.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   pszPath     The path to resolve.
 * @param   pszBuf      Where to store the resolved path.
 * @param   cchBuf      Size of the buffer.
 * @param   fFlags      Combination of __LIBC_BACKFS_FLAGS_RESOLVE_* defines.
 */
int __libc_Back_fsPathResolve(const char *pszPath, char *pszBuf, size_t cchBuf, unsigned fFlags);
/** Flags for __libc_Back_fsPathResolve().
 * @{ */
#define __LIBC_BACKFS_FLAGS_RESOLVE_FULL        0
/** Resolves and verfies the entire path, but it's ok if the last component
 * does not exist. */
#define __LIBC_BACKFS_FLAGS_RESOLVE_FULL_MAYBE  0x01
/** Get the native path instead, no unix root translations. */
#define __LIBC_BACKFS_FLAGS_RESOLVE_NATIVE      0x10
/** Direct buffer mode for testing purposes.  */
#define __LIBC_BACKFS_FLAGS_RESOLVE_DIRECT_BUF  0x8000
/** @} */


/**
 * Changes the default drive of the process.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   chDrive     New default drive.
 */
int __libc_Back_fsDriveDefaultSet(char chDrive);

/**
 * Gets the default drive of the process.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   pchDrive    Where to store the default drive.
 */
int __libc_Back_fsDriveDefaultGet(char *pchDrive);

/**
 * Sets or change the unixroot of the current process.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   pszNewRoot  The new root.
 */
int __libc_Back_fsDirChangeRoot(const char *pszNewRoot);

/**
 * Gets the current directory of the process on a
 * specific drive or on the current one.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   pszPath     Where to store the path to the current directory.
 *                      This will be prefixed with a drive letter if we're
 *                      not in the unix tree.
 * @param   cchPath     The size of the path buffer.
 * @param   chDrive     The drive letter of the drive to get it for.
 *                      If '\0' the current dir for the current drive is returned.
 * @param   fFlags      Flags for skipping drive letter and slash.
 */
int __libc_Back_fsDirCurrentGet(char *pszPath, size_t cchPath, char chDrive, int fFlags);

/** Flags for __libc_Back_fsDirCurrentGet().
 * @{ */
#define __LIBC_BACK_FSCWD_NO_DRIVE          1
#define __LIBC_BACK_FSCWD_NO_ROOT_SLASH     2
/** @} */

/**
 * Changes the current directory of the process.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   pszPath     Path to the new current directory of the process.
 * @param   fDrive      Force a change of the current drive too.
 */
int __libc_Back_fsDirCurrentSet(const char *pszPath, int fDrive);

/**
 * Changes the current directory of the process.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   fh          The handle of an open directory.
 * @param   fDrive      Force a change of the current drive too.
 */
int __libc_Back_fsDirCurrentSetFH(int fh, int fDrive);

/**
 * Creates a new directory.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   pszPath     Path of the new directory.
 * @param   Mode        Permissions on the created directory.
 */
int __libc_Back_fsDirCreate(const char *pszPath, mode_t Mode);

/**
 * Removes a new directory.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   pszPath     Path to the directory which is to be removed.
 */
int __libc_Back_fsDirRemove(const char *pszPath);

/**
 * Creates a symbolic link.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   pszTarget   The target of the symlink link.
 * @param   pszSymlink  The path to the symbolic link to create.
 */
int __libc_Back_fsSymlinkCreate(const char *pszTarget, const char *pszSymlink);

/**
 * Reads the content of a symbolic link.
 *
 * This is weird interface as it will return a truncated result if not
 * enough buffer space. It is also weird in that there is no string
 * terminator.
 *
 * @returns Number of bytes returned in pachBuf.
 * @returns Negative error code (errno.h) on failure.
 * @param   pszPath     The path to the symlink directory.
 * @param   pachBuf     Where to store the symlink value.
 * @param   cchBuf      Size of buffer.
 */
int __libc_Back_fsSymlinkRead(const char *pszPath, char *pachBuf, size_t cchBuf);

/**
 * Stats a symbolic link.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   pszPath     Path to the file to stat. If this is a symbolic link
 *                      the link it self will be stat'ed.
 * @param   pStat       Where to store the file stats.
 */
int __libc_Back_fsSymlinkStat(const char *pszPath, struct stat *pStat);

/**
 * Sets the file access mode of a symlink.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   pszPath The path to the file to set the mode of.
 * @param   Mode    The filemode.
 */
int __libc_Back_fsSymlinkModeSet(const char *pszPath, mode_t Mode);

/**
 * Sets the file times of a symlink.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   pszPath The path to the file to set the mode of.
 * @param   paTimes Two timevalue structures. If NULL the current time is used.
 */
int __libc_Back_fsSymlinkTimesSet(const char *pszPath, const struct timeval *paTimes);

/**
 * Changes the ownership and/or group, without following any final symlink.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   pszPath     Path to the file to modify ownership of.  If this is a
 *                      symbolic link, the link it self will modified.
 * @param   uid         The user id of the new owner, pass -1 to not change it.
 * @param   gid         The group id of the new group, pass -1 to not change it.
 */
int __libc_Back_fsSymlinkOwnerSet(const char *pszPath, uid_t uid, gid_t gid);

/**
 * Stats a file.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   pszPath     Path to the file to stat.
 * @param   pStat       Where to store the file stats.
 */
int __libc_Back_fsFileStat(const char *pszPath, struct stat *pStat);

/**
 * Gets the file stats for a file by filehandle.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure. The content
 *          of *pStat is undefined.
 * @param   fh      Handle to file.
 * @param   pStat   Where to store the stats.
 */
int __libc_Back_fsFileStatFH(int fh, struct stat *pStat);

/**
 * Sets the file access mode of a file.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   pszPath The path to the file to set the mode of.
 * @param   Mode    The filemode.
 */
int __libc_Back_fsFileModeSet(const char *pszPath, mode_t Mode);

/**
 * Sets the file access mode of a file by filehandle.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   fh      Handle to file.
 * @param   Mode    The filemode.
 */
int __libc_Back_fsFileModeSetFH(int fh, mode_t Mode);

/**
 * Sets the file the times of a file.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   pszPath The path to the file to set the times of.
 * @param   paTimes Two timevalue structures. If NULL the current time is used.
 */
int __libc_Back_fsFileTimesSet(const char *pszPath, const struct timeval *paTimes);

/**
 * Sets the file the times of a file by filehandle.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   fh      Handle to file.
 * @param   paTimes Two timevalue structures. If NULL the current time is used.
 */
int __libc_Back_fsFileTimesSetFH(int fh, const struct timeval *paTimes);

/**
 * Changes the ownership and/or group, following all symbolic links.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   pszPath     Path to the file to modify ownership of.
 * @param   uid         The user id of the new owner, pass -1 to not change it.
 * @param   gid         The group id of the new group, pass -1 to not change it.
 */
int __libc_Back_fsFileOwnerSet(const char *pszPath, uid_t uid, gid_t gid);

/**
 * Changes the ownership and/or group.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   fh          Handle to file.
 * @param   uid         The user id of the new owner, pass -1 to not change it.
 * @param   gid         The group id of the new group, pass -1 to not change it.
 */
int __libc_Back_fsFileOwnerSetFH(int fh, uid_t uid, gid_t gid);

/**
 * Renames a file or directory.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   pszPathOld      Old file path.
 * @param   pszPathNew      New file path.
 *
 * @remark OS/2 doesn't preform the deletion of the pszPathNew atomically.
 */
int __libc_Back_fsRename(const char *pszPathOld, const char *pszPathNew);

/**
 * Unlinks a file, directory, symlink, dev, pipe or socket.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   pszPath         Path to the filesystem file/dir/symlink/whatever to remove.
 */
int __libc_Back_fsUnlink(const char *pszPath);


/** @defgroup __libc_Back_io   LIBC Backend - I/O and File Management.
 * @{ */

/**
 * Opens or creates a file.
 *
 * @returns Filehandle to the opened file on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   pszFile     Path to the file.
 * @param   fLibc       The LIBC open flags (O_*).
 * @param   fShare      The share flags (SH_*).
 * @param   cbInitial   Initial filesize.
 * @param   Mode        The specified permission mask.
 * @param   ppFH        Where to store the LIBC filehandle structure which was created
 *                      for the opened file.
 */
int __libc_Back_ioFileOpen(const char *pszFile, unsigned fLibc, int fShare, off_t cbInitial, mode_t Mode, PLIBCFH *ppFH);

/**
 * Change the current position of a file stream and get the new position.
 *
 * @returns new file offset on success.
 * @returns Negative error code (errno) on failure.
 * @param   hFile       File handle to preform seek operation on.
 * @param   off         Offset to seek to.
 * @param   iMethod     The seek method. SEEK_CUR, SEEK_SET or SEEK_END.
 */
off_t __libc_Back_ioSeek(int hFile, off_t off, int iMethod);

/**
 * Sets the size of an open file.
 *
 * When expanding a file the contents of the allocated
 * space is undefined.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   fh      Handle to the file which size should be changed.
 * @param   cbFile  The new filesize.
 * @param   fZero   If set any new allocated file space will be
 *                  initialized to zero.
 */
int __libc_Back_ioFileSizeSet(int fh, __off_t cbFile, int fZero);

/**
 * Reads directory entries from an open directory.
 *
 * @returns Number of bytes read.
 * @returns Negative error code (errno.h) on failure.
 *
 * @param   fh      The file handle of an open directory.
 * @param   pvBuf   Where to store the directory entries.
 *                  The returned data is a series of dirent structs with
 *                  variable name size. d_reclen must be used the offset
 *                  to the next struct (from the start of the current one).
 * @param   cbBuf   Size of the buffer.
 * @param   poff    Where to store the lseek offset of the first entry.
 *
 */
ssize_t __libc_Back_ioDirGetEntries(int fh, void *pvBuf, size_t cbBuf, __off_t *poff);

/**
 * File Control.
 * 
 * Deals with file descriptor flags, file descriptor duplication and locking.
 * 
 * @returns 0 on success and *piRet set.
 * @returns Negated errno on failure and *piRet set to -1.
 * @param   fh          File handle (descriptor).
 * @param   iRequest    Which file file descriptior request to perform.
 * @param   iArg        Argument which content is specific to each
 *                      iRequest operation.
 * @param   prc         Where to store the value which upon success is
 *                      returned to the caller.
 */
int __libc_Back_ioFileControl(int fh, int iRequest, intptr_t iArg, int *prc);

/**
 * File Control operation - OS/2 standard handle.
 * 
 * @returns 0 on success.
 * @returns OS/2 error code or negated errno on failure.
 * 
 * @param   pFH         Pointer to the handle structure to operate on.
 * @param   fh          It's associated filehandle.
 * @param   iRequest    Which file file descriptior request to perform.
 * @param   iArg        Argument which content is specific to each
 *                      iRequest operation.
 * @param   prc         Where to store the value which upon success is
 *                      returned to the caller.
 */
int __libc_Back_ioFileControlStandard(__LIBC_PFH pFH, int fh, int iRequest, intptr_t iArg, int *prc);
    
/**
 * Try resolve a filehandle to a path.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   fh          The file handle.
 * @param   pszPath     Where to store the native path.
 * @param   cchPath     The size of he buffer pointed to by pszPath.
 */
int __libc_Back_ioFHToPath(int fh, char *pszPath, size_t cchPath);

/**
 * Query filesystem configuration information by file handle.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   fh          The handle to query config info about.
 * @param   iName       Which path config variable to query.
 * @param   plValue     Where to return the configuration value.
 * @sa      __libc_Back_fsPathConf, fpathconf, pathconf, sysconf.
 */
int __libc_Back_ioPathConf(int fh, int iName, long *plValue);

/** @} */

/** @} */


/** @defgroup __libc_Back_ldr   LIBC Backend - Loader
 * @{ */

/** Special handle that's returned when passing NULL as library name to
 * __libc_Back_ldrOpen. */
#define __LIBC_BACK_LDR_GLOBAL  ((void *)(intptr_t)-2)

/**
 * Opens a shared library.
 *
 * @returns 0 on success.
 * @returns Native error number.
 * @param   pszLibrary      Name of library to load.
 * @param   fFlags          Flags - ignored.
 * @param   ppvModule       Where to store the handle.
 * @param   pszError        Where to store error information.
 * @param   cchError        Size of error buffer.
 */
int  __libc_Back_ldrOpen(const char *pszLibrary, int fFlags, void **ppvModule, char *pszError, size_t cchError);

/**
 * Finds a symbol in an open shared library.
 *
 * @returns 0 on success.
 * @returns Native error number.
 * @param   pvModule        Module handle returned by __libc_Back_ldrOpen();
 * @param   pszSymbol       Name of the symbol we're to find in pvModule.
 * @param   ppfn            Where to store the symbol address.
 */
int __libc_Back_ldrSymbol(void *pvHandle,  const char *pszSymbol, void **ppfn);

/**
 * Closes a shared library.
 *
 * @returns 0 on success.
 * @returns Native error number.
 * @param   pvModule        Module handle returned by __libc_Back_ldrOpen();
 */
int  __libc_Back_ldrClose(void *pvModule);


/** @} */





/** @defgroup __libc_Back_misc   LIBC Backend - Miscellaneous
 * @{ */

/**
 * Gets the system load averages.
 * The load is the average values of ready and running threads(/processes)
 * over the last 1, 5 and 15 minuttes.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   pardAvgs    Where to store the samples.
 * @param   cAvgs       Number of samples to get. Max is 3.
 * @remark  See OS/2 limitations in getloadavg().
 */
int __libc_Back_miscLoadAvg(double *pardAvgs, unsigned cAvgs);

/** @} */


/** @defgroup __libc_Back_Signals   LIBC Backend - Signals and Exceptions
 * @{ */

#if defined(END_OF_CHAIN) && defined(INCL_DOSEXCEPTIONS)
/**
 * The LIBC Sys Backend exception handler.
 *
 * @returns XCPT_CONTINUE_SEARCH or XCPT_CONTINUE_EXECUTION.
 * @param   pXcptRepRec     Report record.
 * @param   pXcptRegRec     Registration record.
 * @param   pCtx            Context record.
 * @param   pvWhatEver      Not quite sure what this is...
 */
ULONG _System __libc_Back_exceptionHandler(PEXCEPTIONREPORTRECORD       pXcptRepRec,
                                           PEXCEPTIONREGISTRATIONRECORD pXcptRegRec,
                                           PCONTEXTRECORD               pCtx,
                                           PVOID                        pvWhatEver);
#endif

/** @} */


/** @defgroup __libc_Back_MMan      LIBC Backend - Memory Management
 * @{ */

/**
 * Change the memory protection attributes of a range of pages.
 * This function supports the crossing of object boundaries and works
 * on any memory the native apis works on.
 *
 * @returns Negative error code (errno.h) on failure.
 * @param   pv      Pointer to first page - page aligned!
 * @param   cb      Size of the ranage - page aligned!
 * @param   fFlags  The PROT_* flags to replace the current flags with.
 */
int __libc_Back_mmanProtect(void *pv, size_t cb, unsigned fFlags);

/** @} */


/** @defgroup   __libc_Back_signal      LIBC Backend - Signals
 * @{
 */

/** @defgroup __libc_Back_signalRaise_return    __libc_back_signalRaise() returns.
 * These are only valid for positive return values.
 * @{ */
/** Try restart any interrupted system call. */
#define __LIBC_BSRR_RESTART     0x01
/** Go ahead interrupt system call in progress. */
#define __LIBC_BSRR_INTERRUPT   0x02
/** If set execution should be resumed. */
#define __LIBC_BSRR_CONTINUE    0x10
/** If set execution should not be resumed but the signal should be passed
 * on to the system. */
#define __LIBC_BSRR_PASSITON    0x20
/** If set the passed in SIGQUEUED structure was used. */
#define __LIBC_BSRR_USED_QUEUED 0x40
/** @} */

/** @defgroup __libc_back_signalRaise_flags     __libc_back_signalRaise() flags.
 * @{ */
/** The signal is thread specific and must be delivered to the current thread. */
#define __LIBC_BSRF_THREAD      0x01
/** The signal was send from an unknown process. */
#define __LIBC_BSRF_EXTERNAL    0x02
/** The signal was generated by the hardware (i.e. CPUs and such). */
#define __LIBC_BSRF_HARDWARE    0x04
/** The signal should be queued. */
#define __LIBC_BSRF_QUEUED      0x08
/** @} */


/**
 * Raises a signal in the current process.
 *
 * @returns On success a flag mask out of the __LIBC_BSRR_* #defines is returned.
 * @returns On failure a negative error code (errno.h) is returned.
 * @param   iSignalNo           Signal to raise.
 * @param   pSigInfo            Pointer to signal info for this signal.
 *                              NULL is allowed.
 * @param   pvXcptOrQueued      Exception handler parameter list.
 *                              Or if __LIBC_BSRF_QUEUED is set, a pointer to locally malloced
 *                              SIGQUEUED node.
 * @param   fFlags              Flags of the #defines __LIBC_BSRF_* describing how to
 *                              deliver the signal.
 */
int __libc_Back_signalRaise(int iSignalNo, const siginfo_t *pSigInfo, void *pvXcptOrQueued, unsigned fFlags);

/**
 * Queue a signal.
 *
 * @returns 0 on success.
 * @returns -1 on failure, errno set.
 * @param   pid             The target process id.
 * @param   iSignalNo       Signal to queue.
 * @param   SigVal          The value to associate with the signal.
 */
int __libc_Back_signalQueue(pid_t pid, int iSignalNo, const union sigval SigVal);

/**
 * Send a signal to a process.
 *
 * Special case for iSignalNo equal to 0, where no signal is sent but permissions to
 * do so is checked.
 *
 * @returns 0 on if signal sent.
 * @returns -errno on failure.
 *
 * @param   pid         Process Id of the process which the signal is to be sent to.
 * @param   iSignalNo   The signal to send.
 *                      If 0 no signal is sent, but error handling is done as if.
 */
int __libc_Back_signalSendPid(pid_t pid, int iSignalNo);

/**
 * Sends a signal to a process group.
 *
 * Special case for iSignalNo equal to 0, where no signal is sent but permissions to
 * do so is checked.
 *
 * @returns 0 on if signal sent.
 * @returns -errno on failure.
 *
 * @param   pgrp        Process group (positive).
 *                      0 means the process group of this process.
 *                      1 means all process in the system. (not implemented!)
 * @param   iSignalNo   Signal to send to all the processes in the group.
 *                      If 0 no signal is sent, but error handling is done as if.
 */
int __libc_Back_signalSendPGrp(pid_t pgrp, int iSignalNo);

/**
 * sigaction worker; queries and/or sets the action for a signal.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno) on failure.
 * @param   iSignalNo   Signal number.
 * @param   pSigAct     Pointer to new signal action.
 *                      If NULL no update is done.
 * @param   pSigActOld  Where to store the old signal action.
 *                      If NULL nothing is attempted stored.
 */
int __libc_Back_signalAction(int iSignalNo, const struct sigaction *pSigAct, struct sigaction *pSigActOld);

/**
 * Change interrupt/restart system call properties for a signal.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno) on failure.
 * @param   iSignalNo   Signal number to change interrupt/restart
 *                      properties for.
 * @param   fFlag       If set Then clear the SA_RESTART from the handler action.
 *                      If clear Then set the SA_RESTART from the handler action.
 * @remark  The SA_RESTART flag is inherited when using signal().
 */
int __libc_Back_signalInterrupt(int iSignalNo, int fFlag);

/**
 * Changes and/or queries the alternative signal stack settings of a thread.
 *
 * @returns 0 on success.
 * @returns Negative error number (errno.h) on failure.
 * @param   pThrd       Thread which signal stack to change and/or query.
 * @param   pStack      New stack settings. (Optional)
 * @param   pOldStack   Old stack settings. (Optional)
 */
int __libc_Back_signalStack(__LIBC_PTHREAD pThrd, const stack_t *pStack, stack_t *pOldStack);

/**
 * Block or unblock signal deliveries of a thread.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno) on failure.
 * @param   pThrd       Thread to apply this to.
 * @param   iHow        Describes the action taken if pSigSetNew not NULL. Recognized
 *                      values are SIG_BLOCK, SIG_UNBLOCK or SIG_SETMASK.
 *
 *                      SIG_BLOCK means to or the sigset pointed to by pSigSetNew with
 *                          the signal mask for the current thread.
 *                      SIG_UNBLOCK means to and the 0 complement of the sigset pointed
 *                          to by pSigSetNew with the signal mask of the current thread.
 *                      SIG_SETMASK means to set the signal mask of the current thread
 *                          to the sigset pointed to by pSigSetNew.
 *
 * @param   pSigSetNew  Pointer to signal set which will be applied to the current
 *                      threads signal mask according to iHow. If NULL no change
 *                      will be made the the current threads signal mask.
 * @param   pSigSetOld  Where to store the current threads signal mask prior to applying
 *                      pSigSetNew to it. This parameter can be NULL.
 */
int __libc_Back_signalMask(__LIBC_PTHREAD pThrd, int iHow, const sigset_t * __restrict pSigSetNew, sigset_t * __restrict pSigSetOld);

/**
 * Wait for one or more signals and remove and return the first of them
 * to occur.
 *
 * Will return immediately if one of the signals is already pending. If more than
 * one signal is pending the signal with highest priority will be returned.
 *
 * @returns Signal number on success.
 * @returns Negative error code (errno) on failure.
 * @param   pSigSet     Signals to wait for.
 * @param   pSigInfo    Where to store the signal info for the signal
 *                      that we accepted.
 * @param   pTimeout    Timeout specification. If NULL wait for ever.
 */
int __libc_Back_signalWait(const sigset_t *pSigSet, siginfo_t *pSigInfo, const struct timespec *pTimeout);

/**
 * Suspends the current thread till a signal have been handled.
 *
 * @returns Negative error code (errno) on failure. (always fails)
 * @param   pSigSet     Temporary signal mask for the thread.
 */
int __libc_Back_signalSuspend(const sigset_t *pSigSet);

/**
 * Gets the set of signals which are blocked by the current thread and are
 * pending on the process or the calling thread.
 *
 * @returns 0 indicating success.
 * @returns Negative error code (errno) on failure.
 * @param   pSigSet     Pointer to signal set where the result is to be stored.
 */
int __libc_Back_signalPending(sigset_t *pSigSet);

/**
 * Queries and/or starts/stops a timer.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   iWhich      Which timer to get, any of the ITIMER_* #defines.
 *                      OS/2 only supports ITIMER_REAL.
 * @param   pValue      Where to store the value.
 *                      Optional. If NULL pOldValue must not be NULL.
 * @param   pOldValue   Where to store the old value.
 *                      Optional. If NULL pValue must not be NULL.
 */
int __libc_Back_signalTimer(int iWhich, const struct itimerval *pValue, struct itimerval *pOldValue);

/**
 * This is a hack to deal with potentially lost thread pokes.
 * 
 * For some reason or another we loose the async signal in some situations. It's 
 * been observed happening after/when opening files (fopen), but it's not known
 * whether this is really related or not.
 */
void __libc_Back_signalLostPoke(void);


/** @} */



/** @defgroup grp_Back_process  LIBC Backend - Process Management
 * @{ */

/**
 * Fork a child process pretty much identical to the calling process.
 * See SuS for full description of what fork() does and doesn't.
 *
 * @returns 0 in the child process.
 * @returns process identifier of the new child in the parent process. (positive, non-zero)
 * @returns Negative error code (errno.h) on failure.
 */
pid_t __libc_Back_processFork(void);

/**
 * Waits/polls for on one or more processes to change it's running status.
 *
 * @returns 0 on success, pSigInfo containing status info.
 * @returns Negated error code (errno.h) on failure.
 * @param   enmIdType   What kind of process specification Id contains.
 * @param   Id          Process specification of the enmIdType sort.
 * @param   pSigInfo    Where to store the result.
 * @param   fOptions    The WEXITED, WUNTRACED, WSTOPPED and WCONTINUED flags are used to
 *                      select the events to report. WNOHANG is used for preventing the api
 *                      from blocking. And WNOWAIT is used for peeking.
 * @param   pResUsage   Where to store the reported resources usage for the child.
 *                      Optional and not implemented on OS/2.
 */
int __libc_Back_processWait(idtype_t enmIdType, id_t Id, siginfo_t *pSigInfo, unsigned fOptions, struct rusage *pUsage);

/**
 * Gets the real user id of the current process.
 * @returns Real user id.
 */
uid_t __libc_Back_processGetUid(void);

/**
 * Gets the effective user id of the current process.
 * @returns Effective user id.
 */
uid_t __libc_Back_processGetEffUid(void);

/**
 * Sets the effective user id of the current process.
 * If the caller is superuser real and saved user id are also set.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno) on failure.
 * @param   uid         New effective user id.
 *                      For superusers this is also the new real and saved user id.
 */
int __libc_Back_processSetUid(uid_t uid);

/**
 * Sets the real, effective and saved user ids of the current process.
 * Unprivilegde users can only set them to the real user id, the
 * effective user id or the saved user id.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno) on failure.
 * @param   ruid    New real user id. Ignore if -1.
 * @param   euid    New effective user id. Ignore if -1.
 * @param   svuid   New Saved user id. Ignore if -1.
 */
int __libc_Back_processSetUidAll(uid_t ruid, uid_t euid, uid_t svuid);


/**
 * Gets the real group id of the current process.
 * @returns Real group id.
 */
gid_t __libc_Back_processGetGid(void);

/**
 * Gets the effective group id of the current process.
 * @returns Effective group id.
 */
gid_t __libc_Back_processGetEffGid(void);

/**
 * Sets the effective group id of the current process.
 * If the caller is superuser real and saved group id are also set.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno) on failure.
 */
int __libc_Back_processSetGid(gid_t gid);

/**
 * Sets the real, effective and saved group ids of the current process.
 * Unprivilegde users can only set them to the real group id, the
 * effective group id or the saved group id.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno) on failure.
 * @param   rgid    New real group id. Ignore if -1.
 * @param   egid    New effective group id. Ignore if -1.
 * @param   svgid   New Saved group id. Ignore if -1.
 */
int __libc_Back_processSetGidAll(gid_t rgid, gid_t egid, gid_t svgid);

/**
 * Gets the session id of the current process.
 * @returns Session id.
 * @returns Negated errno on failure.
 * @param   pid     Process to get the process group for.
 *                  Use 0 for the current process.
 */
pid_t __libc_Back_processGetSid(pid_t pid);

/**
 * Gets the process group of the specfied process.
 * @returns Process group.
 * @returns Negated errno on failure.
 * @param   pid     Process to get the process group for.
 *                  Use 0 for the current process.
 */
pid_t __libc_Back_processGetPGrp(pid_t pid);

/**
 * Gets the most favourable priority of a process, group of processes
 * or all processed owned by a user.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   iWhich      PRIO_PROCESS, PRIO_PGRP, or PRIO_USER.
 * @param   idWho       Id of the type specified by iWhich. 0 means the current process/pgrp/user.
 * @param   piPrio      Where to store the priority.
 */
int __libc_Back_processGetPriority(int iWhich, id_t idWho, int *piPrio);

/**
 * Sets the priority of a process, a group of processes
 * or all processed owned by a user.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   iWhich      PRIO_PROCESS, PRIO_PGRP, or PRIO_USER.
 * @param   idWho       Id of the type specified by iWhich. 0 means the current process/pgrp/user.
 * @param   iPrio       The new priority.
 */
int __libc_Back_processSetPriority(int iWhich, id_t idWho, int iPrio);


/** When this flag is set, the exec / spawn backend will handle hash bang scripts. */
extern int __libc_Back_gfProcessHandleHashBangScripts;
/** When this flag is set, the exec / spawn backend will handle PC batch scripts. */
extern int __libc_Back_gfProcessHandlePCBatchScripts;

/**
 * Gets the default shell for functions like system() and popen().
 *
 * @returns 0 on success, negative error number on failure.
 * @param   pszShell        Where to put the path to the shell.
 * @param   cbShell         The size of the buffer @a pszShell points to.
 * @param   poffShellArg    Where to return the offset into @a pszShell of the
 *                          first argument.  The system() and popen() calls has
 *                          traditionally not included the path to /bin/sh.
 * @param   pszCmdLineOpt   Where to put the shell option for specifying a
 *                          command line it should execute.
 * @param   cbCmdLineOpt    The size of the buffer @a pszCmdLineOpt points to.
 */
int __libc_Back_processGetDefaultShell(char *pszShell, size_t cbShell, size_t *poffShellArg,
                                       char *pszCmdLineOpt, size_t cbCmdLineOpt);

/** @} */


/** @defgroup grp_Back_time  LIBC Backend - Time Management
 * @{ */

/**
 * Gets the current high-resolution timestamp as nanoseconds.
 *
 * @returns nanosecond timestamp.
 */
hrtime_t __libc_Back_timeHighResNano(void);

/** @} */


/** @defgroup grp_Back_sysvipc LIBC Backend - SysV IPC
 * @{ */

/**
 * sysget syscall.
 */
int __libc_Back_sysvSemGet(key_t key, int nsems, int semflg);

/**
 * semop syscall.
 */
int __libc_Back_sysvSemOp(int semid, struct sembuf *sops_user, size_t nsops);

/**
 * semctl syscall
 */
int __libc_Back_sysvSemCtl(int semid, int semnum, int cmd, union semun real_arg);


/**
 * shmget.
 */
int __libc_Back_sysvShmGet(key_t key, size_t size, int shmflg);

/**
 * shmat.
 */
int __libc_Back_sysvShmAt(int shmid, const void *shmaddr, int shmflg, void **ppvActual);

/**
 * shmdt.
 */
int __libc_Back_sysvShmDt(const void *shmaddr);

/**
 * shmctl.
 */
int __libc_Back_sysvShmCtl(int shmid, int cmd, struct shmid_ds *bufptr);

/** @} */


/** @defgroup grp_Back_safesem LIBC Backend - Internal Signal-Safe Semaphores.
 * @{ */

/**
 * Safe Mutex Semaphore structure.
 *
 * For shared semaphores this structure must be in shared memory so all users
 * actually use the very same structure.
 */
typedef struct __LIBC_SAFESEMMTX
{
#ifdef __OS2__
    /** Mutex handle. */
    unsigned long       hmtx;
#endif
    /** Set if the semaphore is shared. */
    unsigned            fShared;
} __LIBC_SAFESEMMTX;
/** Pointer to a SAFESEM Mutex structure. */
typedef __LIBC_SAFESEMMTX *__LIBC_PSAFESEMMTX;

/**
 * Creates a safe mutex sem.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   pmtx        Pointer to the semaphore structure to initialize.
 * @param   fShared     Set if the semaphore should be sharable between processes.
 */
int __libc_Back_safesemMtxCreate(__LIBC_PSAFESEMMTX pmtx, int fShared);

/**
 * Opens a shared safe mutex sem.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   pmtx        Pointer to the semaphore structure.
 */
int __libc_Back_safesemMtxOpen(__LIBC_PSAFESEMMTX pmtx);

/**
 * Closes a shared safe mutex sem.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   pmtx        Pointer to the semaphore structure.
 */
int __libc_Back_safesemMtxClose(__LIBC_PSAFESEMMTX pmtx);

/**
 * Locks a mutex semaphore.
 *
 * @returns 0 on success.
 * @returns Negative errno on failure.
 * @param   pmtx        Pointer to the semaphore structure.
 */
int __libc_Back_safesemMtxLock(__LIBC_PSAFESEMMTX pmtx);

/**
 * Unlocks a mutex semaphore.
 *
 * @returns 0 on success.
 * @returns Negative errno on failure.
 * @param   pmtx        Pointer to the semaphore structure.
 */
int __libc_Back_safesemMtxUnlock(__LIBC_PSAFESEMMTX pmtx);


/**
 * Safe Event Semaphore structure.
 *
 * For shared semaphores this structure must be in shared memory so all users
 * actually use the very same structure.
 *
 * @remark  The event semaphore business is difficult because the lack of
 *          atomic mutex release + event wait apis in OS/2. We have to
 *          jump around the place to get this working nearly safly...
 */
typedef struct __LIBC_SAFESEMEV
{
#ifdef __OS2__
    /** The event semaphore. */
    unsigned long       hev;
#endif
    /** Number of threads which are supposed to be blocking on the above event semaphore. */
    uint32_t volatile   cWaiters;
    /** The mutex semaphore used to protect the event semaphore. */
    __LIBC_PSAFESEMMTX  pmtx;
    /** Set if the semaphore is shared. */
    unsigned            fShared;
} __LIBC_SAFESEMEV;
/** Pointer to a SAFESEM Event structure. */
typedef __LIBC_SAFESEMEV *__LIBC_PSAFESEMEV;

/**
 * Creates a safe event sem.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   pev         Pointer to the semaphore structure to initialize.
 * @param   pmtx        Pointer to the mutex semaphore which protects the event semaphore.
 * @param   fShared     Set if the semaphore should be sharable between processes.
 */
int __libc_Back_safesemEvCreate(__LIBC_PSAFESEMEV pev, __LIBC_PSAFESEMMTX pmtx, int fShared);

/**
 * Opens a shared safe event sem.
 *
 * The caller is responsible for opening the associated mutex
 * semaphore before calling this function.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   pev         Pointer to the semaphore structure to open.
 */
int __libc_Back_safesemEvOpen(__LIBC_PSAFESEMEV pev);

/**
 * Closes a shared safe mutex sem.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   pev         Pointer to the semaphore structure to close.
 */
int __libc_Back_safesemEvClose(__LIBC_PSAFESEMEV pev);

/**
 * Sleep on a semaphore.
 *
 * The caller must own the associated mutex semaphore. The mutex semaphore will
 * be released as we go to sleep and reclaimed when we wake up.
 *
 * The pfnComplete callback is used to correct state before signals are handled.
 * It will always be called be for this function returns, and it'll either be under
 * the protection of the signal mutex or the associated mutex (both safe sems).
 *
 * This is the most difficult thing we're doing in this API. On OS/2 we have
 * potential (at least theoretically) race conditions...
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 *
 * @param   pev         Pointer to the semaphore structure to sleep on.
 * @param   pfnComplete Function to execute on signal or on wait completion.
 * @param   pvUser      User argument to pfnComplete.
 */
int __libc_Back_safesemEvSleep(__LIBC_PSAFESEMEV pev, void (*pfnComplete)(void *pvUser), void *pvUser);

/**
 * Wakes up all threads sleeping on a given event semaphore.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   pev         Pointer to the semaphore structure to post.
 */
int __libc_Back_safesemEvWakeup(__LIBC_PSAFESEMEV pev);


/** @} */


/** @defgroup grp_Back_panic    LIBC Backend - Panic Routines
 * @{ */

/** The panic was caused by a signal. Drop the LIBC PANIC line. */
#define __LIBC_PANIC_SIGNAL         1
/** Don't set the SPM termination code / status. When set the caller is
 * responsible for doing this. */
#define __LIBC_PANIC_NO_SPM_TERM    2

/**
 * Print a panic message and dump/kill the process.
 *
 * @param   fFlags      A combination of the __LIBC_PANIC_* defines.
 * @param   pvCtx       Pointer to a context record if available. This is a PCONTEXTRECORD.
 * @param   pszFormat   User message which may contain %s and %x.
 * @param   ...         String pointers and unsigned intergers as specified by the %s and %x in pszFormat.
 */
void __libc_Back_panic(unsigned fFlags, void *pvCtx, const char *pszFormat, ...) __attribute__((__noreturn__));

/**
 * Print a panic message and dump/kill the process.
 *
 * @param   fFlags      A combination of the __LIBC_PANIC_* defines.
 * @param   pvCtx       Pointer to a context record if available. This is a PCONTEXTRECORD.
 * @param   pszFormat   User message which may contain %s and %x.
 * @param   args        String pointers and unsigned intergers as specified by the %s and %x in pszFormat.
 */
void __libc_Back_panicV(unsigned fFlags, void *pvCtx, const char *pszFormat, va_list args) __attribute__((__noreturn__));

/* @} */

__END_DECLS

#endif
