/** $Id: sharedpm.h 3643 2008-05-18 12:46:41Z bird $ */
/** @file
 *
 * LIBC Shared Process Management.
 *
 * Copyright (c) 2004-2005 knut st. osmundsen <bird@anduin.net>
 * Copyright (c) 2004 nickk
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

#ifndef __InnoTekLIBC_sharedpm_h__
#define __InnoTekLIBC_sharedpm_h__

#include <sys/cdefs.h>
#include <sys/types.h>
#include <sys/signal.h>
#include <sys/ipc.h>


__BEGIN_DECLS

/** The name of the shared semaphore protecting the memory. */
#define SPM_MUTEX_NAME          "\\SEM32\\INNOTEKLIBC-V1.3"

/** The name of the shared memory. */
#define SPM_MEMORY_NAME         "\\SHAREMEM\\INNOTEKLIBC-V1.3"

/** The timeout for accessing the shared mem semaphore. */
#define SPM_MUTEX_TIMEOUT       (30*1000)

/** Allocate 512KB if OBJ_ANY isn't supported.
 * If it is supported we allocated the double amount. */
#define SPM_MEMORY_SIZE         (512*1024)

/** The defined max size of a process.
 * A good bit is reserved for the future here.
 *
 * Theoretically speaking, OS/2 cannot entertain more than a 1000 processes.
 * So, even if we reserve a good bit here it's not gonna cause any trouble.
 */
#define SPM_PROCESS_SIZE        (256)

/**
 * The SPM version.
 */
#define SPM_VERSION             0x00010003


/**
 * Process termination reason.
 */
typedef enum __LIBC_EXIT_REASON
{
    /** No reason. */
    __LIBC_EXIT_REASON_NONE = 0,
    /** Normal exit. */
    __LIBC_EXIT_REASON_EXIT = 1,
    /** OS/2 hard error. */
    __LIBC_EXIT_REASON_HARDERROR = 2,
    /** Trap (i.e. 16-bit?). */
    __LIBC_EXIT_REASON_TRAP = 3,
    /** Kill process. */
    __LIBC_EXIT_REASON_KILL = 4,
    /** Exception. */
    __LIBC_EXIT_REASON_XCPT = 5,
    /** Signal base. */
    __LIBC_EXIT_REASON_SIGNAL_BASE = 32,
    /** Signal max. */
    __LIBC_EXIT_REASON_SIGNAL_MAX = 256,
    /** Hack to ensure that this will be a 32-bit int. */
    __LIBC_EXIT_REASON_MAKE_INT32 = 0x7fffffff
} __LIBC_EXIT_REASON;


/** Pointer to child termination data. */
typedef struct __LIBC_SPMCHILDNOTIFY *__LIBC_PSPMCHILDNOTIFY;

/**
 * Child termination notification data.
 */
typedef struct __LIBC_SPMCHILDNOTIFY
{
    /** Pointer to the next node in the list. */
    __LIBC_PSPMCHILDNOTIFY volatile pNext;
    /** Structure size. */
    unsigned                        cb;
    /** Process Id. */
    pid_t                           pid;
    /** Process Group Id. */
    pid_t                           pgrp;
    /** Exit code. */
    int                             iExitCode;
    /** Reason code. */
    __LIBC_EXIT_REASON              enmDeathReason;
} __LIBC_SPMCHILDNOTIFY;


/**
 * Process state.
 */
typedef enum __LIBC_SPMPROCSTAT
{
    /** The process is free. */
    __LIBC_PROCSTATE_FREE = 0,
    /** The process is waiting for a chile to be created and snatch all the
     * inherited goodies from it.
     * If not caught before DosExecPgm/DosStartSession returns
     * it will be freed by the disappointed parent.
     */
    __LIBC_PROCSTATE_EMBRYO = 1,
    /** The process is running. */
    __LIBC_PROCSTATE_ALIVE = 2,
    /** The process is dead but other guys are still using it's data. */
    __LIBC_PROCSTATE_ZOMBIE = 3,
    /** Reserved process state. */
    __LIBC_PROCSTATE_RESERVED = 4,
    /** The maximum state number (exclusive). */
    __LIBC_PROCSTATE_MAX = 5,
    /** Hack to ensure that this will be a 32-bit int. */
    __LIBC_PROCSTATE_MAKE_INT32 = 0x7fffffff
} __LIBC_SPMPROCSTAT;


/** Pointer to (queued) signal. */
typedef struct __libc_SPMSignal *__LIBC_PSPMSIGNAL;

/**
 * Signal (queued).
 */
typedef struct __libc_SPMSignal
{
    /** Structure size. */
    unsigned                    cb;
    /** Pointer to the next signal. */
    __LIBC_PSPMSIGNAL volatile  pNext;
    /** Sender process.
     * This is used to decrement the cSigsSent count of the sender. */
    pid_t                       pidSender;
    /** Signal info. */
    siginfo_t                   Info;
} __LIBC_SPMSIGNAL;


/** Inheritance FH Bundle Type.
 * @{ */
/** Termination bundle. */
#define __LIBC_SPM_INH_FHB_TYPE_END         (0)
/** Standard bundle. */
#define __LIBC_SPM_INH_FHB_TYPE_STANDARD    (0xe0 | (sizeof(unsigned) + sizeof(ino_t) + sizeof(dev_t) + sizeof(unsigned)))
/** Directory bundle. */
#define __LIBC_SPM_INH_FHB_TYPE_DIRECTORY   (0xc0 | (sizeof(unsigned) + sizeof(ino_t) + sizeof(dev_t) + sizeof(unsigned) + sizeof(unsigned)))
/** Socket bundle, using the BSD 4.4 backend. */
#define __LIBC_SPM_INH_FHB_TYPE_SOCKET_44   (0xa0 | (sizeof(unsigned) + sizeof(unsigned short)))
/** Socket bundle, using the BSD 4.3 backend. */
#define __LIBC_SPM_INH_FHB_TYPE_SOCKET_43   (0x80 | (sizeof(unsigned) + sizeof(unsigned short)))
/*#define __LIBC_SPM_INH_FHB_TYPE_            (0x60 | (sizeof(unsigned) + ))*/
/*#define __LIBC_SPM_INH_FHB_TYPE_            (0x40 | (sizeof(unsigned) + ))*/
/*#define __LIBC_SPM_INH_FHB_TYPE_            (0x20 | (sizeof(unsigned) + ))*/
/*#define __LIBC_SPM_INH_FHB_TYPE_            (0x00 | (sizeof(unsigned) + ))*/
/** Get the per file handle size from the bundle type. */
#define __LIBC_SPM_INH_FHB_SIZE(type)   ((type) & 0x1f)
/** @} */

/**
 * SPM Inheritance File Handle Bundle Header.
 */
#pragma pack(1)
typedef struct __libc_SPMInhFHBHdr
{
    /** Bundle type. */
    unsigned char   uchType;
    /** Count of handles in this bundle. */
    unsigned char   cHandles;
    /** Start handle number. */
    unsigned short  StartFH;
} __LIBC_SPMINHFHBHDR;
#pragma pack()
/** Pointer to SPM filehandle bundle header. */
typedef __LIBC_SPMINHFHBHDR *__LIBC_PSPMINHFHBHDR;

/**
 * SPM standard filehandle inherit bundle
 * This is used for OS/2 filehandles which only needs flags
 * transfered.
 */
#pragma pack(1)
typedef struct __libc_SPMInhFH
{
    /** The common bundle header. */
    __LIBC_SPMINHFHBHDR Hdr;
    /** Array Hdr.cHandles entries. */
    struct
    {
        /** The flags. */
        unsigned            fFlags;
        /** The inode number. */
        ino_t               Inode;
        /** The device number. */
        dev_t               Dev;
        /** String table offset of the native path. */
        unsigned            offNativePath : 24;
        unsigned            u8Reserved : 8;
    } aHandles[1];
} __LIBC_SPMINHFHBSTD;
#pragma pack()
/** Pointer to SPM standard filehandle inherit bundle. */
typedef __LIBC_SPMINHFHBSTD *__LIBC_PSPMINHFHBSTD;

/**
 * SPM directory filehandle inherit bundle.
 */
#pragma pack(1)
typedef struct __libc_SPMInhFHDir
{
    /** The common bundle header. */
    __LIBC_SPMINHFHBHDR Hdr;
    /** Array of flags of Hdr.cHandles entries. */
    struct
    {
        /** The flags. */
        unsigned            fFlags;
        /** The inode number. */
        ino_t               Inode;
        /** The device number. */
        dev_t               Dev;
        /** String table offset of the native path. */
        unsigned            offNativePath : 24;
        /** Set if this path is in the unix tree. */
        unsigned            fInUnixTree : 1;
        unsigned            u7Reserved : 7;
        /** The current position. */
        unsigned            uCurEntry;
    } aHandles[1];
} __LIBC_SPMINHFHBDIR;
#pragma pack()
/** Pointer to SPM directory filehandle inherit bundle. */
typedef __LIBC_SPMINHFHBDIR *__LIBC_PSPMINHFHBDIR;

/**
 * SPM socket filehandle inherit bundle.
 *
 * This is used for both BSD 4.3 and 4.4 sockets. The data needed
 * here is the flags and the socket number.
 */
#pragma pack(1)
typedef struct __libc_SPMInhFHSocket
{
    /** The common bundle header. */
    __LIBC_SPMINHFHBHDR Hdr;
    /** Array of flags of Hdr.cHandles entries. */
    struct
    {
        /** The file handle flags. */
        unsigned        fFlags;
        /** The socket number. */
        unsigned short  usSocket;
    } aHandles[1];
} __LIBC_SPMINHFHBSOCK;
#pragma pack()
/** Pointer to SPM socket filehandle inherit bundle. */
typedef __LIBC_SPMINHFHBSOCK *__LIBC_PSPMINHFHBSOCK;

/**
 * The SPM fs inherit data, part 1.
 * The FS data is stored in two sections for legacy reasons.
 */
typedef struct __libc_SPMInhFS
{
    /** In Unix Tree global. */
    int         fInUnixTree;
    /** Size of the unix root. Only set if there's an official root. */
    size_t      cchUnixRoot;
    /** The current unix root if cchUnixRoot is non-zero. */
    char        szUnixRoot[1];
} __LIBC_SPMINHFS;
/** Pointer to FS inherit data. */
typedef __LIBC_SPMINHFS *__LIBC_PSPMINHFS;

/**
 * SPM fs inherit data, part two.
 */
typedef struct __libc_SPMInhFS2
{
    /** The size of this structure. */
    unsigned    cb;
    /** The umask value. */
    unsigned    fUMask;
} __LIBC_SPMINHFS2;
/** Pointer to FS inherit data, part two. */
typedef __LIBC_SPMINHFS2 *__LIBC_PSPMINHFS2;

/**
 * SPM signal inherit data.
 */
typedef struct __libc_SPMInhSig
{
    /** The size of this structure. */
    unsigned    cb;
    /** Mask of signals which should be ignored. */
    sigset_t    SigSetIGN;
} __LIBC_SPMINHSIG;
/** Pointer to Signal inherit data. */
typedef __LIBC_SPMINHSIG *__LIBC_PSPMINHSIG;



/**
 * Inherit structure.
 *
 * All the data it contains is allocated in one block heap block!
 */
typedef struct __libc_SPMInherit
{
    /** Size of the inherit structure. */
    size_t                  cb;
    /** Pointer to the filehandle part.
     * This is a succession of bundles terminating with a _END one. */
    __LIBC_PSPMINHFHBHDR    pFHBundles;
    /** Pointer to the file system part. If NULL default values are assumed. */
    __LIBC_PSPMINHFS        pFS;
    /** Pointer to the signal part. If NULL default values are assumed. */
    __LIBC_PSPMINHSIG       pSig;
    /** Pointer to strings (filenames++).
     * All the strings are NULL terminated and referenced by offset. */
    char                   *pszStrings;
    /** More file system stuff. */
    __LIBC_PSPMINHFS2       pFS2;
} __LIBC_SPMINHERIT;
/** Pointer to inherit data. */
typedef __LIBC_SPMINHERIT *__LIBC_PSPMINHERIT;


/** Pointer to per process data structure. */
typedef struct __libc_SPMProcess *__LIBC_PSPMPROCESS;

/**
 * Per Process Data.
 */
typedef struct __libc_SPMProcess
{
    /**  0 - Pointer to the next process in the list.
     * Every process is in one or another list depending on their state. */
    volatile __LIBC_PSPMPROCESS pNext;
    /**  4 - Pointer to the previous process in the list. */
    volatile __LIBC_PSPMPROCESS pPrev;


    /** Core
     * @{ */
    /**  8 - Version of the SPM which created this process. */
    unsigned                    uVersion;
    /** 12 - Number of references to this process.
     * A process is not actually free till the reference count
     * reaches 0. */
    volatile unsigned           cReferences;
    /** 16 - State of this process. */
    volatile __LIBC_SPMPROCSTAT enmState;
    /** 20 - Process id. */
    pid_t                       pid;
    /** 24 - Process id of parent.
     * This might not match the OS/2 one since there are no notification
     * when a parent dies.  */
    pid_t                       pidParent;
    /** @} */


    /** User & Group
     * @{ */
    /** 28 - User id (also called Real User id). */
    uid_t                       uid;
    /** 32 - Effective user id. */
    uid_t                       euid;
    /** 36 - Saved user id. */
    uid_t                       svuid;
    /** 40 - Group id (also called Real Group id). */
    gid_t                       gid;
    /** 44 - Effecive group id. */
    gid_t                       egid;
    /** 48 - Saved group id. */
    gid_t                       svgid;
    /** 52 - Supplementary group ids. */
    gid_t                       agidGroups[16 /*NGROUPS_MAX*/];
    /** @} */


    /** Misc
     * @{ */
    /** 116 - Creation timestamp. */
    unsigned                    uTimestamp;
    /** 120 - The SPM open count of this process. */
    unsigned                    cSPMOpens;
    /** 124 - Indicates that the process is a full featured LIBC process.
     * Until this flag is set (atomically) it's not a good idea to
     * queue signals on the process since it won't have a signal handler
     * installed, and thus won't get to know about them.
     */
    volatile unsigned           fExeInited;
    /** 128 - Reserved for future use. Must be 0. */
    unsigned                    uMiscReserved;
    /** @} */


    /** Fork
     * @{  */
    /** 132 - Pointer to fork module list head. */
    void * volatile             pvModuleHead;
    /** 136 - Pointer to fork module list tail pointer. */
    void * volatile * volatile  ppvModuleTail;
    /** 140 - Pointer to fork handle which a child should use when spawned by fork(). */
    void                       *pvForkHandle;
    /** @} */


    /** Signals & Job Control
     * @{ */
    /** 144 - Incoming signal queue (FIFO).
     * For signals which aren't queable only one signal can be queued.
     */
    volatile __LIBC_PSPMSIGNAL  pSigHead;
    /** 148 - Number of signals send.
     * After _POSIX_SIGQUEUE_MAX signals only SIGCHLD will be allowed sent.
     */
    volatile unsigned           cSigsSent;
    /** 152 - Session Id. */
    pid_t                       sid;
    /** 156 - Process group. */
    pid_t                       pgrp;
    /** 160 - Reserved for future use. Must be 0. */
    unsigned                    uSigReserved1;
    /** 164 - Reserved for future use. Must be 0. */
    unsigned                    uSigReserved2;
    /** @} */


    /** Termination
     * @{ */
    /** 168 - Termination information which will be sent to the parent process. */
    __LIBC_PSPMCHILDNOTIFY      pTerm;
    /** 172 - List of child termination notifications. */
    __LIBC_PSPMCHILDNOTIFY volatile pChildNotifyHead;
    /** 176 - Pointer to the pointer where to insert the next child notification. */
    __LIBC_PSPMCHILDNOTIFY volatile * volatile ppChildNotifyTail;
    /** @} */

    /** 180 - Per Process Sockets Reference Counters. (Data is process local.) */
    uint16_t                   *pacTcpipRefs;
    /** 184 - The Process priority (unix). */
    int                         iNice;
    /** 188 - Reserved fields with default value 0. */
    unsigned                    auReserved[56 - 47];


    /** 224 - Number of possible pointers to shared memory starting at pInherit.
     * This means we can extend the structure with pointers for as long as we
     * want and still have older LIBC versions cleanup the mess. */
    unsigned                    cPoolPointers;
    /** 228 -  Pointer to data inherited from the parent process. */
    __LIBC_PSPMINHERIT volatile pInherit;
    /** 232 - Pointer to data inherited from the parent process when it's locked.
     * When locked pInherit is NULL and this member points to the data instead.
     * This prevents spmAlloc() from reclaiming it while it's in use. */
    __LIBC_PSPMINHERIT volatile pInheritLocked;

} __LIBC_SPMPROCESS;

/** The current value for __LIBC_SPMPROCESS::cPoolPointers. */
#define __LIBC_SPMPROCESS_POOLPOINTERS      2

/**
 * Pool chunk.
 */
typedef struct __libc_SPMPoolChunk
{
    /** Next block in the list of all blocks.. */
    struct __libc_SPMPoolChunk *pPrev;
    /** Previous block in the list of all blocks.. */
    struct __libc_SPMPoolChunk *pNext;
} __LIBC_SPMPOOLCHUNK, *__LIBC_PSPMPOOLCHUNK;

/**
 * Free pool chunk.
 */
typedef struct __libc_SPMPoolChunkFree
{
    /** Main list. */
    __LIBC_SPMPOOLCHUNK             core;
    /** Next block in list of free nodes. */
    struct __libc_SPMPoolChunkFree *pPrev;
    /** Previous block in list of free nodes. */
    struct __libc_SPMPoolChunkFree *pNext;
    /** Size of this block. */
    size_t                          cb;
} __LIBC_SPMPOOLCHUNKFREE, *__LIBC_PSPMPOOLCHUNKFREE;

/**
 * Pool chunk alignment.
 */
#define SPM_POOL_ALIGN(size)    (((size) + 7) & ~7)

/**
 * This structure contains the global TCP/IP socket data.
 */
typedef struct __libc_SPMTcpIp
{
    /** Size of the structure. */
    size_t                      cb;
    /** The number of elements in the acRefs array. */
    unsigned                    cSockets;
    /** Array containing the references counters for the sockets in the system. */
    uint16_t                    acRefs[32768];
} __LIBC_SPMTCPIP, *__LIBC_PSPMTCPIP;


/**
 * The structure contains the system load averages.
 */
typedef struct __libc_SPMLoadAvg
{
    /** Array of the three stamples.
     * The entries are for 1, 5 and 15 min averages respectively.
     *
     * The samples them selfs are stored as integers and not as a
     * double as the interface returns. The reason is that this is smaller
     * and it's what both BSD and Linux is doing. The fraction part is
     * the lower 11 bits (this is also identical to BSD and Linux).
     */
    uint32_t                    u32Samples[3];
    /** Timestamp of the last update. */
    unsigned                    uTimestamp;
} __LIBC_SPMLOADAVG;
/** Pointer to load averages. */
typedef __LIBC_SPMLOADAVG *__LIBC_PSPMLOADAVG;


/**
 * This is the header of the LIBC shared memory.
 */
typedef struct __libc_SPMHeader
{
    /**  0 - SPM version. (the one which initialized the memory)  */
    unsigned                            uVersion;

    /**  4 - Size of the shared memory. */
    size_t                              cb;
    /**  8 - Free memory. */
    volatile size_t                     cbFree;
    /** 12 - Start chunk of shared memory pool. */
    volatile __LIBC_PSPMPOOLCHUNK       pPoolHead;
    /** 16 - End chunk of shared memory pool. */
    volatile __LIBC_PSPMPOOLCHUNK       pPoolTail;
    /** 20 - Start free chunk in the shared memory pool. */
    volatile __LIBC_PSPMPOOLCHUNKFREE   pPoolFreeHead;
    /** 24 - End free chunk in the shared memory pool. */
    volatile __LIBC_PSPMPOOLCHUNKFREE   pPoolFreeTail;

    /** 28 - The size of one process block in this version of the shared memory. */
    size_t                              cbProcess;
    /** 32 - Array index by process state giving the head pointers
     * to the processes in that state. */
    volatile __LIBC_PSPMPROCESS         apHeads[__LIBC_PROCSTATE_MAX];
    /** 52 -List of free child (termination) notification structures. */
    volatile __LIBC_PSPMCHILDNOTIFY     pChildNotifyFreeHead;

    /** 56 - Pointer to the tcpip globals. */
    __LIBC_PSPMTCPIP                    pTcpip;
    /** 60 - Creator pid. */
    pid_t                               pidCreate;
    /** 64 - Creation timestamp. */
    __extension__ union
    {
#ifdef INCL_DOSDATETIME
        DATETIME                        dtCreate;
#endif
        char                            ach[16];
    };

    /** 80 - System Load Averages. */
    volatile __LIBC_SPMLOADAVG          LoadAvg;

    /** 96 - List of free signal structures. This will reduce
     * the time spent allocating a signal structure in most cases.
     * During SPM init 8 signal structures will be allocated and queued. */
    volatile __LIBC_PSPMSIGNAL          pSigFreeHead;
    /** 100 - Number of free signal structures in the list.
     * This is used to prevent the list from growing infinitly under stress.
     * When cSigFree reaches 32 unused signal structures will be freed instead
     * of put in the list. */
    volatile unsigned                   cSigFree;
    /** 104 - Number of active signals.
     * This is used to keep a reasonable limit of the number of active signal
     * structures so a process cannot exhaust the shared memory pool.
     * The maximum is defined by cSigMaxActive.
     */
    volatile unsigned                   cSigActive;
    /** 108 - The maximum number of active signals.
     * This is initialized by the SPM creator but can be adjusted later.
     */
    unsigned                            cSigMaxActive;
    /** 112 - Notification event semaphore.
     * This semaphore is signaled when an event occurs. An event is either a signal
     * or a child notification.
     * At the moment this isn't used by anyone, but it's being signaled just in case.
     */
    unsigned long                       hevNotify;

    /** 116 - Pointer to SysV Sempahore globals. */
    struct __libc_SysV_Sem             *pSysVSem;
    /** 120 - Pointer to SysV Shared Memory globals. */
    struct __libc_SysV_Shm             *pSysVShm;
    /** 124 - Pointer to SysV Message Queue globals. */
    struct __libc_SysV_Msg             *pSysVMsg;

    /*  128 - The rest of the block, up to cbProcess, is undefined in this version.
     * Future versions of LIBC may use this area assuming it's initalized with zeros.
     */
} __LIBC_SPMHEADER, *__LIBC_PSPMHEADER;


/**
 * SPM Exception handler registration record.
 */
typedef struct __LIBC_SPMXCPTREGREC
{
    __extension__ union
    {
#ifdef INCL_DOSEXCEPTIONS
        EXCEPTIONREGISTRATIONRECORD Core;
#endif
        void *apv[2];
    };
    unsigned                        uFutureStuff0;
    unsigned                        uFutureStuff1;
} __LIBC_SPMXCPTREGREC;
/** Pointer to SPM exception handler registration record. */
typedef __LIBC_SPMXCPTREGREC *__LIBC_PSPMXCPTREGREC;


/**
 * Gets the current process.
 *
 * @returns Pointer to the current process.
 * @returns NULL and errno on failure.
 * @remark  For this too __libc_spmRelease() must be called when done.
 */
__LIBC_PSPMPROCESS __libc_spmSelf(void);

/**
 * Gets the inherit data associated with the current process.
 * This call prevents it from being release by underrun handling.
 *
 * @returns Pointer to inherit data.
 *          The caller must call __libc_spmInheritRelease() when done.
 * @returns NULL and errno if no inherit data.
 */
__LIBC_PSPMINHERIT  __libc_spmInheritRequest(void);

/**
 * Releases the inherit data locked by the __libc_spmInheritRequest() call.
 *
 * @returns 0 on success.
 * @returns -1 and errno on failure.
 */
int  __libc_spmInheritRelease(void);

/**
 * Frees the inherit data of this process.
 * This is called when the executable is initialized.
 */
void  __libc_spmInheritFree(void);

/**
 * Create an embryo related to the current process.
 *
 * @returns pointer to the embryo process.
 *          The allocated process must be released by the caller.
 * @returns NULL and errno on failure.
 * @param   pidParent   The parent pid (i.e. this process).
 */
__LIBC_PSPMPROCESS __libc_spmCreateEmbryo(pid_t pidParent);

/**
 * Wait for a embryo to become a live process and complete 
 * inheriting (file handles / sockets issues).
 *
 * @returns non-zero if the process has started.
 * @param   pEmbryo         The embry process.
 */
int __libc_spmWaitForChildToBecomeAlive(__LIBC_PSPMPROCESS pEmbryo);

/**
 * Searches for a process given by pid.
 *
 * @returns Pointer to the desired process on success.
 * @returns NULL and errno on failure.
 * @param   pid         Process id to search for.
 * @param   enmState 	The state of the process.
 * @remark  Call __libc_spmRelease() to release the result.
 */
__LIBC_PSPMPROCESS __libc_spmQueryProcess(pid_t pid);

/**
 * Searches for a process with a given pid and state.
 *
 * @returns Pointer to the desired process on success.
 * @returns NULL and errno on failure.
 * @param   pid         Process id to search for.
 * @param   enmState 	The state of the process.
 * @remark  Call __libc_spmRelease() to release the result.
 */
__LIBC_PSPMPROCESS __libc_spmQueryProcessInState(pid_t pid, __LIBC_SPMPROCSTAT enmState);

/**
 * Process enumeration callback function.
 *
 * @returns 0 to continue the enumeration.
 * @returns Non-zero to stop the enumeration. The enumeration
 *          api will return this same value.
 *
 * @param   pProcess    The current process.
 * @param   pvUser      The user argument.
 * @remark  It is *not* allowed to terminate the thread or process, do long jumps
 *          or anything else which causes the enumeration function not to release
 *          the lock.
 *          It is not allowed to do expensive stuff either as it will harm other
 *          processes in the system which want to access the shared process facility!
 */
typedef int __LIBC_FNSPNENUM(__LIBC_PSPMPROCESS pProcess, void *pvUser);
/** Pointer to an process enumeration callback function. */
typedef __LIBC_FNSPNENUM *__LIBC_PFNSPNENUM;

/**
 * Enumerates all alive processes in a group.
 *
 * @returns 0 on success.
 * @returns -ESRCH if the process group wasn't found.
 * @returns -EINVAL if pgrp is negative.
 * @returns Whatever non-zero value the pfnCallback function returns to stop the enumeration.
 *
 * @param   pgrp            The process group id. 0 is an alias for the process group the caller belongs to.
 * @param   pfnCallback     Callback function.
 * @param   pvUser          User argument to the callback.
 */
int __libc_spmEnumProcessesByPGrp(pid_t pgrp, __LIBC_PFNSPNENUM pfnCallback, void *pvUser);

/**
 * Enumerates all alive processes owned by a user.
 *
 * @returns 0 on success.
 * @returns -ESRCH if the process group wasn't found.
 * @returns -EINVAL if uid is negative.
 * @returns Whatever non-zero value the pfnCallback function returns to stop the enumeration.
 *
 * @param   uid             The process user id.
 * @param   pfnCallback     Callback function.
 * @param   pvUser          User argument to the callback.
 */
int __libc_spmEnumProcessesByUser(uid_t uid, __LIBC_PFNSPNENUM pfnCallback, void *pvUser);

/**
 * Release reference to the given process.
 *
 * @returns 0 on success.
 * @returns -1 on failure.
 * @param   pProcess    Pointer to process to release.
 */
int __libc_spmRelease(__LIBC_PSPMPROCESS pProcess);

/**
 * Checks if the calling process can see the specfied one.
 *
 * @returns 0 if it can see it.
 * @returns -ESRCH (or other approriate error code) if it cannot.
 *
 * @param   pProcess    The process in question.
 */
int __libc_spmCanSee(__LIBC_PSPMPROCESS pProcess);

/**
 * Checks if we are a system-wide super user.
 *
 * @returns 0 if we are.
 * @returns -EPERM if we aren't.
 */
int __libc_spmIsSuperUser(void);

/**
 * Checks if the caller can modify the specified process.
 *
 * @returns 0 if we can modify it.
 * @returns -EPERM if we cannot modify it.
 * @param   pProcess    The process in question.
 */
int __libc_spmCanModify(__LIBC_PSPMPROCESS pProcess);

/**
 * Checks if the calling process is a member of the specified group.
 *
 * @returns 0 if member.
 * @returns -EPERM if not member.
 * @param   gid     The group id in question.
 */
int __libc_spmIsGroupMember(gid_t gid);

/**
 * Check if the caller can access the SysV IPC object as requested.
 *
 * @returns 0 if we can.
 * @returns -EPERM if we cannot.
 * @param   pPerm       The IPC permission structure.
 * @param   Mode        The access request. IPC_M, IPC_W or IPC_R.
 */
int __libc_spmCanIPC(struct ipc_perm *pPerm, mode_t Mode);

/**
 * Marks the current process (if we have it around) as zombie
 * or dead freeing all resources associated with it.
 *
 * @param   uReason     The OS/2 exit list type reason code.
 *                      This is only used if the current code is NONE.
 * @param   iExitCode   The unix exit code for this process.
 *                      This is only if the current code is 0.
 * @remark this might not be a sufficient interface for process termination but we'll see.
 */
void __libc_spmTerm(__LIBC_EXIT_REASON enmDeathReason, int iExitCode);

/**
 * Query about notifications from a specific child process.
 * The notifications are related to death, the cause and such.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   pid                 Process id.
 * @param   pNotify             Where to store the notification from the child.
 */
int __libc_spmQueryChildNotification(pid_t pid, __LIBC_PSPMCHILDNOTIFY pNotifyOut);

/**
 * Validates a process group id.
 *
 * @returns 0 if valid.
 * @returns -ESRCH if not valid.
 * @param   pgrp            Process group id to validate. 0 if the same as
 *                          the same as the current process.
 * @param   fOnlyChildren   Restrict the search to immediate children.
 */
int __libc_spmValidPGrp(pid_t pgrp, int fOnlyChildren);


/**
 * Identificators which can be obtained using __libc_spmGetId().
 */
typedef enum __LIBC_SPMID
{
    __LIBC_SPMID_PID = 0,
    __LIBC_SPMID_PPID,
    __LIBC_SPMID_SID,
    __LIBC_SPMID_PGRP,
    __LIBC_SPMID_UID,
    __LIBC_SPMID_EUID,
    __LIBC_SPMID_SVUID,
    __LIBC_SPMID_GID,
    __LIBC_SPMID_EGID,
    __LIBC_SPMID_SVGID
} __LIBC_SPMID;


/**
 * Gets the specified Id.
 *
 * @returns Requested Id.
 * @param   enmId       Identification to get.
 */
unsigned __libc_spmGetId(__LIBC_SPMID enmId);

/**
 * Sets the effective user id of the current process.
 * If the caller is superuser real and saved user id are also set.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno) on failure.
 * @param   uid         New effective user id.
 *                      For superusers this is also the new real and saved user id.
 */
int __libc_spmSetUid(uid_t uid);

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
int __libc_spmSetUidAll(uid_t ruid, uid_t euid, uid_t svuid);

/**
 * Sets the effective group id of the current process.
 * If the caller is superuser real and saved group id are also set.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno) on failure.
 */
int __libc_spmSetGid(gid_t gid);

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
int __libc_spmSetGidAll(gid_t rgid, gid_t egid, gid_t svgid);

/**
 * Locks the LIBC shared memory for short exclusive access.
 * The call must call __libc_spmUnlock() as fast as possible and make
 * no api calls until that is done.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 *
 * @param   pRegRec     Pointer to the exception handler registration record.
 * @param   ppSPMHdr    Where to store the pointer to the SPM header. Can be NULL.
 *
 * @remark  Don't even think of calling this if you're not LIBC!
 */
int __libc_spmLock(__LIBC_PSPMXCPTREGREC pRegRec, __LIBC_PSPMHEADER *ppSPMHdr);

/**
 * Unlock the LIBC shared memory after call to __libc_spmLock().
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 *
 * @param   pRegRec     Pointer to the exception handler registration record.
 *
 * @remark  Don't even think of calling this if you're not LIBC!
 */
int __libc_spmUnlock(__LIBC_PSPMXCPTREGREC pRegRec);

/**
 * Allocate memory from the LIBC shared memory.
 *
 * The SPM must be locked using __libc_spmLock() prior to calling this function!
 *
 * @returns Pointer to allocated memory on success.
 * @returns NULL on failure.
 *
 * @param   cbSize 	Size of memory to allocate.
 *
 * @remark  Don't think of calling this if you're not LIBC!
 */
void * __libc_spmAllocLocked(size_t cbSize);

/**
 * Free memory allocated by __libc_spmAllocLocked() or __libc_spmAlloc().
 *
 * The SPM must be locked using __libc_spmLock() prior to calling this function!
 *
 * @returns 0 on success.
 * @returns -1 and errno on failure.
 *
 * @param   pv      Pointer to memory block returned by __libc_SpmAlloc().
 *                  NULL is allowed.
 * @remark  Don't think of calling this if you're not LIBC!
 */
int __libc_spmFreeLocked(void *pv);

/**
 * Allocate memory from the LIBC shared memory.
 *
 * @returns Pointer to allocated memory on success.
 * @returns NULL on failure.
 *
 * @param   cbSize 	Size of memory to allocate.
 *
 * @remark  Don't think of calling this if you're not LIBC!
 */
void * __libc_spmAlloc(size_t cbSize);

/**
 * Free memory allocated by __libc_SpmAlloc().
 *
 * @returns 0 on success.
 * @returns -1 and errno on failure.
 *
 * @param   pv      Pointer to memory block returned by __libc_SpmAlloc().
 *                  NULL is allowed.
 *
 * @remark  Don't think of calling this if you're not LIBC!
 */
int __libc_spmFree(void *pv);

/**
 * Register termination handler.
 *
 * This is a manual way of by passing a.out's broken weak symbols.
 */
void __libc_spmRegTerm(void (*pfnTerm)(void));


/**
 * Adds the first reference to a new socket.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   iSocket     The new socket.
 */
int     __libc_spmSocketNew(int iSocket);

/**
 * References a socket.
 *
 * @returns The new reference count.
 *          The low 16-bits are are the global count.
 *          The high 15-bits are are the process count.
 * @returns Negative error code (errno.h) on failure.
 * @param   iSocket     socket to reference.
 */
int     __libc_spmSocketRef(int iSocket);

/**
 * Dereferences a socket.
 *
 * @returns The new reference count.
 *          The low 16-bits are are the global count.
 *          The high 15-bits are are the process count.
 * @returns Negative error code (errno.h) on failure.
 * @param   iSocket     Socket to dereference.
 */
int     __libc_spmSocketDeref(int iSocket);

/**
 * Get the stored load average samples.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   pLoadAvg    Where to store the load average samples.
 * @param   puTimestamp Where to store the current timestamp.
 */
int     __libc_spmGetLoadAvg(__LIBC_PSPMLOADAVG  pLoadAvg, unsigned *puTimestamp);

/**
 * Get the stored load average samples.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   pLoadAvg    Where to store the load average samples.
 */
int     __libc_spmSetLoadAvg(const __LIBC_SPMLOADAVG *pLoadAvg);

/**
 * Marks the process as a full LIBC process.
 *
 * Up to this point it was just a process which LIBC happend to be loaded into.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 */
void    __libc_spmExeInited(void);

/**
 * Queues a signal on another process.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   pSigInfo    Signal to queue.
 * @param   pid         Pid to queue it on.
 * @param   fQueued     Set if the signal type is queued.
 */
int     __libc_spmSigQueue(int iSignalNo, siginfo_t *pSigInfo, pid_t pid, int fQueued);

/**
 * Callback which is called when a signal have been queued on a process.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   iSignalNo   The signal which have been queued.
 * @param   pProcess    Process which the signal was queued on.
 * @param   pvUser      User argument.
 */
typedef int (*__LIBC_PFNSPMSIGNALED)(int iSignalNo, const __LIBC_SPMPROCESS * pProcess, void *pvUser);

/**
 * Queues a signal on another process.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   iSignalNo   The signal to send. If 0 only permissions are checked.
 * @param   pSigInfo     Signal to queue. If NULL only permissions are checked.
 * @param   pgrp        Process group to queue a signal on.
 * @param   fQueued     Set if the signal type is queued.
 * @param   pfnCallback Pointer to callback function to post process signaled processes.
 *                      The callback must be _very_ careful. No crashing or blocking!
 * @param   pvUser      User argument specified to pfnCallback.
 */
int     __libc_spmSigQueuePGrp(int iSignalNo, siginfo_t *pSigInfo, pid_t pgrp, int fQueued, __LIBC_PFNSPMSIGNALED pfnCallback, void *pvUser);

/**
 * Get the signal set of pending signals.
 *
 * @returns Number of pending signals on success.
 * @returns 0 if no signals are pending.
 * @returns Negative error code (errno.h) on failure.
 * @param   pSigSet     Where to create the set of pending signals.
 */
int     __libc_spmSigPending(sigset_t *pSigSet);

/**
 * De-queues one or more pending signals of a specific type.
 *
 * @returns Number of de-queued signals on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   iSignalNo   Signal type to dequeue.
 * @param   paSignals   Where to store the signals.
 * @param   cSignals    Size of the signal array.
 * @param   cbSignal    Size of one signal entry.
 */
int     __libc_spmSigDequeue(int iSignalNo, siginfo_t *paSignals, unsigned cSignals, size_t cbSignal);


/**
 * Checks the SPM memory for trouble.
 *
 * @returns 0 on perfect state.
 * @returns -1 and errno on mutex failure.
 * @returns Number of failures if SPM is broken.
 * @param   fBreakpoint Raise breakpoint exception if a problem is encountered.
 * @param   fVerbose    Log everything.
 */
int     __libc_SpmCheck(int fBreakpoint, int fVerbose);

__END_DECLS

#endif /* __InnoTekLIBC_sharedpm_h__ */
