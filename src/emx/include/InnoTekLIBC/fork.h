/** $Id: fork.h 3803 2014-02-05 19:25:05Z bird $ */
/** @file
 *
 * InnoTek LIBC - Fork.
 *
 * Copyright (C) 2004 knut st. osmundsen
 * Copyright (C) 2004 nickk
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

#ifndef __InnoTekLIBC_fork_h__
#define __InnoTekLIBC_fork_h__

#include <sys/cdefs.h>
#include <sys/types.h>

__BEGIN_DECLS

/** Current fork version. */
#define __LIBC_FORK_VERSION             0x00020000

/** Current fork module version. */
#define __LIBC_FORK_MODULE_VERSION      0x00010000

/** Timeout on waiting semaphore in fork operations */
#define __LIBC_FORK_SEM_TIMEOUT         (30*1000)

/** Minimum fork buffer size. (512 KB) */
#define __LIBC_FORK_BUFFER_SIZE_MIN     (512*1024)
/** Maximum fork buffer size. (8 MB) */
#define __LIBC_FORK_BUFFER_SIZE_MAX     (8*1024*1024)

/** Fork header magic used for the szMagic field of __LIBC_FORKHEADER. */
#define __LIBC_FORK_HDR_MAGIC           "ForkHDR"


/**
 * For side values.
 */
typedef enum __LIBC_FORKCTX
{
    /** The callback is only called in the child context. */
    __LIBC_FORK_CTX_CHILD = 0,
    /** The callback is only called in the parent context. */
    __LIBC_FORK_CTX_PARENT = 1,
    /** The callback is called in both parent and child contexts. */
    __LIBC_FORK_CTX_BOTH = 2,
    /** The mask for the calling context values. */
    __LIBC_FORK_CTX_MASK = 0xf,
    /** Flag for pfnCompletionCallback to add a callback to the end of the
     * list so that it will be called last. */
    __LIBC_FORK_CTX_FLAGS_LAST = 0x1000
} __LIBC_FORKCTX;

/**
 * Fork callback operation.
 *
 * The order the values are declared is the order they are executed
 * during the fork. We reserve a good bunch of space for future
 * extensions.
 */
typedef enum __LIBC_FORKOP
{
    /** Called before DosExecPgm() to let everyone check if they agree
     * to forking now, and to line up critical stuff for execution
     * at the earliest possible moment in the child process.
     * The critical stuff can not exceed the size of the fork buffer.
     * This was called _CHECK_PARENT in the specs.
     */
    __LIBC_FORK_OP_EXEC_PARENT      = 0x10,
    /** Called during module registration in the child. (Meaning during
     * DosExecPgm except for the executable.)
     * If someone disagrees to forking or wanna do something before
     * datasegments are copied, this is the time.
     * This was called _CHECK_CHILD in the specs.
     */
    __LIBC_FORK_OP_EXEC_CHILD       = 0x20,
    /** Called after DosExecPgm() in the parent to do the main fork job.
     * Before calling callbacks pages are normally duplicated.
     */
    __LIBC_FORK_OP_FORK_PARENT      = 0x30,
    /** Called after __LIBC_FORK_OP_FORK_PARENT in the child.
     * This normally just calling callback.
     */
    __LIBC_FORK_OP_FORK_CHILD       = 0x40
} __LIBC_FORKOP;


/** pfnDuplicatePages Flags.
 * @{
 */
/** Copy only dirty pages.
 * We're using the DosQueryMemState() API to figure out which pages
 * are dirty or not. Mind the value! */
#define __LIBC_FORK_FLAGS_ONLY_DIRTY    0x00000000
/** Copy all pages.
 * This flag excludes __LIBC_FORK_FLAG_ONLY_DIRTY and visa versa.
 */
#define __LIBC_FORK_FLAGS_ALL           0x80000000
/** Mind and copy page attributes.
 * This flag indicates caution in respect to page attributes and that
 * page attributes should be duplicated in the child process.
 */
#define __LIBC_FORK_FLAGS_PAGE_ATTR     0x40000000
/** Have DosAllocMem flags.
 * If set the flags also contains (some of) the flags passed to DosAllocMem().
 */
#define __LIBC_FORK_FLAGS_ALLOC_FLAGS   0x20000000
/** The allocation DosAllocMem flag mask. */
#define __LIBC_FORK_FLAGS_ALLOC_MASK    0x00000fff
/** @} */


struct __libc_ForkHandle;
struct __libc_ForkModule;
/** Pointer to fork handle. */
typedef struct __libc_ForkHandle *__LIBC_PFORKHANDLE;
/** Pointer to fork module. */
typedef struct __libc_ForkModule *__LIBC_PFORKMODULE;


/**
 * Fork packet header.
 *
 * The fork buffer is filled with packets which is sent to the
 * other context and sequentially unpacket there.
 *
 * This is version specific and noone should consider reading it
 * but the LIBC which is responsible for the fork().
 */
typedef struct __libc_ForkPkgHdr
{
    /** Magic / eyecatcher. Must be __LIBC_FORK_HDR_MAGIC. */
    char                    szMagic[8];

    /** Size of this packet (includes the header). */
    size_t                  cb;
    /** Type of header and content. */
    enum
    {
        /** This header terminates the buffer. */
        __LIBC_FORK_HDR_TYPE_END = 0,
        /** Next header. This is used to finish a run in one context
         * and give control to the other. */
        __LIBC_FORK_HDR_TYPE_NEXT = 1,
        /** Abort header. */
        __LIBC_FORK_HDR_TYPE_ABORT = 2,

        /** Duplicate pages header. */
        __LIBC_FORK_HDR_TYPE_DUPLICATE = 10,
        /** Invoke header. */
        __LIBC_FORK_HDR_TYPE_INVOKE = 11,
        /** Make sure this is a 32-bit. */
        __LIBC_FORK_HDR_TYPE_INT32_HACK = 0x7fffffff
    }                       enmType;

    /**
     * Type specific header data.
     */
    union
    {
        /**
         * End fork packet header
         */
        struct
        {
            /** End of packet. (exclusive) */
            char            achStart[1];
        } End;

        /**
         * Next fork packet header.
         */
        struct
        {
            /** End of packet. (exclusive) */
            char            achStart[1];
        } Next;

        /**
         * Abort fork packet header.
         */
        struct
        {
            /** Error code for errno. */
            int             err;
            /** End of packet. (exclusive) */
            char            achStart[1];
        } Abort;


        /**
         * Duplicate pages fork packet header.
         */
        struct
        {
            /** Duplication flags. */
            unsigned        fFlags;
            /** Start of data. */
            char            achStart[1];
        } Duplicate;

        /**
         * Invoke fork packet header.
         */
        struct
        {
            /** Function pointer. */
            int            (*pfn)(__LIBC_PFORKHANDLE pForkHandle, void *pvArg, size_t cbArg);
            /** Size argument. */
            size_t          cbArg;
            /** Start of argument data. (can be empty) */
            char            achStart[1];
        } Invoke;
    }                       u;
} __LIBC_FORKPKGHDR, *__LIBC_PFORKPKGHDR;



/**
 * The completion callback function.
 *
 * This is used to perform minor tasks (like cleanup) after the fork has completed.
 * At this point it's too late to talk to the other context, and it's too late to
 * make the fork fail (if it have already done so).
 *
 * @param   pvArg   User specified argument.
 * @param   rc      The fork result. This is 0 on success. On failure it is the
 *                  negative errno value.
 * @param   enmCtx  The context the completion callback function is called in.
 * @remark  The reason for not having a full round of callbacks at this point
 *          is that there is very little need for cleanup and it would be a waste
 *          of time to call all the callbacks.
 */
typedef void (*__LIBC_PFNCOMPLETIONCALLBACK)(void *pvArg, int rc, __LIBC_FORKCTX enmCtx);

/**
 * Completion callback entry.
 * These are placed at the end for the fork buffer.
 * @remark  This structure is version specific!
 */
typedef struct __libc_ForkCompletionCallback
{
    /** Pointer to function. */
    __LIBC_PFNCOMPLETIONCALLBACK    pfnCallback;
    /** The requested context(s). */
    __LIBC_FORKCTX                  enmContext;
    /** User argument. */
    void                           *pvArg;
} __LIBC_FORKCOMPLETIONCALLBACK, *__LIBC_PFORKCOMPLETIONCALLBACK;



/**
 * This handle starts the shared memory allocated by parent.
 *
 * The parent will publish this in pvForkHandle member of the
 * shared per process data it creates for the child before
 * doing DosExecPgm().
 */
typedef struct __libc_ForkHandle
{
    /** Fork version (__LIBC_FORK_VERSION). */
    unsigned                uVersion;
    /** Parent process, i.e. the one which have called fork(). */
    pid_t                   pidParent;
    /** Child process. Filled in by the child when it accept the fork challenge. */
    pid_t                   pidChild;
    /** Size of the shared memory. */
    size_t                  cb;

    /** Buffer protection semaphore. */
    unsigned long           hmtx;
    /** Parent's event sem user to communicate between parent and child. */
    unsigned long           hevParent;
    /** Child's event sem user to communicate between parent and child. */
    unsigned long           hevChild;
    /** Timeout for waiting above mentioned semaphores */
    unsigned long           cmsecTimeout;

    /** Pointer to the child side function responsible for performing
     * the fork operation.
     *
     * This is called by __libc_ForkRegisterModule() to do init processing of an
     * module and to kick of the fork() circus. The first time it's called
     * the fork buffer is processed and emptied. It will further call the
     * pfnAtFork callback for the given module and let it do init time
     * processing.
     *
     * When the fExecutable flag is set the fork operation starts and the function
     * will not return. On failure it will exit the process.
     *
     * @returns 0 on success.
     * @returns -errno on failure.
     * @param   pForkHandle     Fork handle.
     * @param   pModule         The module it's called for.
     * @param   fExecutable     Indicates that the module is the executable module
     *                          and that the main part of fork() can start.
     */
    int                   (*pfnDoFork)(__LIBC_PFORKHANDLE pForkHandle, __LIBC_PFORKMODULE pModule, int fExecutable);

    /** Reserved fields for future versions for fork.
     * Older versions all initializes them to zero, so newer LIBC
     * versions can check and see if the fields are in use.
     * The total number of elements in the header is 64, currently 9 are defined.*/
    unsigned                auReserved[64 - 9];



    /**
     * Duplicating a number of pages from pvStart to pvEnd.
     *
     * @returns 0 on success.
     * @returns appropriate negative error code (errno.h) on failure.
     *
     * @param   pForkHandle Handle of the current fork operation.
     * @param   pvStart     Pointer to start of the pages. Rounded down.
     * @param   pvEnd       Pointer to end of the pages. Rounded up.
     * @param   fFlags      __LIBC_FORK_FLAGS_* defines.
     */
    int (*pfnDuplicatePages)(__LIBC_PFORKHANDLE pForkHandle, void *pvStart, void *pvEnd, unsigned fFlags);

    /**
     * Invoke a function in the child process giving it an chunk of input.
     * The function is invoked the next time the fork buffer is flushed,
     * call pfnFlush() if the return code is desired.
     *
     * @returns 0 on success.
     * @returns appropriate negative error code (errno.h) on failure.
     * @param   pForkHandle Handle of the current fork operation.
     * @param   pfn         Pointer to the function to invoke in the child.
     *                      The function gets the fork handle, pointer to
     *                      the argument memory chunk and the size of that.
     *                      The function must return 0 on success, and non-zero
     *                      on failure.
     * @param   pvArg       Pointer to a block of memory of size cbArg containing
     *                      input to be copied to the child and given to pfn upon
     *                      invocation.
     */
    int (*pfnInvoke)(__LIBC_PFORKHANDLE pForkHandle, int (*pfn)(__LIBC_PFORKHANDLE pForkHandle, void *pvArg, size_t cbArg), void *pvArg, size_t cbArg);

    /**
     * Flush the fork() buffer. Meaning taking what ever is in the fork buffer
     * and let the child process it.
     * This might be desired to get the result of a pfnInvoke() in a near
     * synchornous way.
     *
     * @returns 0 on success.
     * @returns appropriate negative error code (errno.h) on failure.
     * @param   pForkHandle Handle of the current fork operation.
     */
    int (*pfnFlush)(__LIBC_PFORKHANDLE pForkHandle);

    /**
     * Register a fork() completion callback.
     *
     * Use this primitive to do post fork() cleanup.
     * The callbacks are executed first in the child, then in the parent. The
     * order is reversed registration order.
     *
     * @returns 0 on success.
     * @returns appropriate non-zero error code on failure.
     * @param   pForkHandle Handle of the current fork operation.
     * @param   pfnCallback Pointer to the function to call back.
     *                      This will be called when fork() is about to
     *                      complete (the fork() result is established so to
     *                      speak). A zero rc argument indicates success,
     *                      a non zero rc argument indicates failure.
     * @param   pvArg       Argument to pass to pfnCallback as 3rd argument.
     * @param   enmContext  __LIBC_FORK_CTX_CHILD, __LIBC_FORK_CTX_PARENT, or
     *                      __LIBC_FORK_CTX_BOTH. May be ORed with
     *                      __LIBC_FORK_CTX_FLAGS_LAST to add the callback to
     *                      the end of the list so that it will be called after
     *                      all other registered callbacks. 
     *
     * @remark  Use with care, the memory used to remember these is taken from the
     *          fork buffer.
     */
    int (*pfnCompletionCallback)(__LIBC_PFORKHANDLE pForkHandle, __LIBC_PFNCOMPLETIONCALLBACK pfnCallback,
                                 void *pvArg, __LIBC_FORKCTX enmContext);

    /** Reserved callbacks fields for future usage.
     * These are initialized with NULL untill defined. When defined callers
     * must first check if they are not NULL!
     * The total number of callbacks are 64, currently 4 are defined. */
    int (*apfn[64 - 4])(__LIBC_PFORKHANDLE pForkHandle);



    /** Data private to the fork implementation.
     * @{
     */
    /** Pointer to the bottom of the stack (the highest address / exclusive).
     * Note that this doesn't have to be the stack of thread one (like the child
     * initially runs on). For such cases the child will allocate the new stack
     * before relocating it self. */
    void                   *pvStackHigh;
    /** Pointer to the absolute top of the stack (the lowest possible address / inclusvie). */
    void                   *pvStackLow;
    /** Pointer to the current stack position. This pointer is specific to the LIBC version
     * doing the fork. Generally speaking it's address which fork() returns too. */
    void                   *pvStackRet;
    /** Flag indicating whether or not the stack needs to be allocated.
     * This is only set when fork() is called from threads other than 1. */
    int                     fStackAlloc;
    /** Pointer to the instruction starting the return sequence for __fork().
     * Pratically speaking, set esp=pvStackRet and jump here. */
    void                   *pvForkRet;

    /** Number of completion callbacks. */
    unsigned                cCompletionCallbacks;
    /** Pointer to the completion callback array.
     * This put at the end of the fork buffer and new entries are added to
     * the front. */
    __LIBC_PFORKCOMPLETIONCALLBACK  papfnCompletionCallbacks;
    /** Index of the next completion callback for the parent. */
    volatile unsigned       iCompletionCallbackParent;
    /** Index of the next completion callback for the child. */
    volatile unsigned       iCompletionCallbackChild;

    /** Pointer to the start of the fork buffer. */
    __LIBC_PFORKPKGHDR      pBuf;
    /** Size of the fork buffer.  */
    size_t                  cbBuf;
    /** Pointer to the current packet in the fork buffer.
     * In fill-up mode this points to the next unused packet.
     * In process mode this is a NULL pointer. */
    __LIBC_PFORKPKGHDR      pBufCur;
    /** Number of free bytes currently left in the fork buffer.
     * Initially subtracked is storage for a __LIBC_FORK_HDR_TYPE_END packet. */
    size_t                  cbBufLeft;
    /** If set the buffer is flushable.
     * This is used to detect flushes done before the child or parent is ready
     * to respond. */
    int                     fFlushable;

    /** The current fork stage.
     * The values relative to the time in the fork() of the stage. */
    enum
    {
        /** Pre-Exec stage. */
        __LIBC_FORK_STAGE_PRE_EXEC = 0,
        /** DosExecPgm stage. Starts rigth before DosExecPgm is called. */
        __LIBC_FORK_STAGE_EXEC,
        /** Child init stage. Start at first register module in child. */
        __LIBC_FORK_STAGE_INIT_CHILD,
        /** Child init stage done. Starts after calling exec child for the executable module. */
        __LIBC_FORK_STAGE_INIT_DONE,
        /** Child init done and DosExecPgm returned stage. */
        __LIBC_FORK_STAGE_EXEC_DONE,
        /** Fork parent stage. */
        __LIBC_FORK_STAGE_FORK_PARENT,
        /** Fork child stage. */
        __LIBC_FORK_STAGE_FORK_CHILD,
        /** Completion callbacks child stage. */
        __LIBC_FORK_STAGE_COMPLETION_CHILD,
        /** Completion callbacks parent stage. */
        __LIBC_FORK_STAGE_COMPLETION_PARENT,

        /** Make sure this is a 32-bit. */
        __LIBC_FORK_STAGE_INT32_HACK = 0x7fffffff
    }                       enmStage;

    /** @} */
} __LIBC_FORKHANDLE;



/**
 * Callback function for registration using _FORK_PARENT1()
 * or _FORK_CHILD1().
 *
 * @returns 0 on success.
 * @returns positive errno on warning.
 * @returns negative errno on failure. Fork will be aborted.
 * @param   pForkHandle     Pointer to fork handle.
 * @param   enmOperation    Callback operation.
 */
typedef int (*__LIBC_PFNFORKCALLBACK)(__LIBC_PFORKHANDLE pForkHandle, __LIBC_FORKOP enmOperation);


/**
 * Callback structure.
 * This is defined as a statick in the
 */
typedef struct __libc_ForkCallback
{
    /* Initiator function, which fills pForkBuffer */
    __LIBC_PFNFORKCALLBACK  pfnCallback;
    /* Priority of this callback */
    unsigned                uPriority;
#if 0
    /** Pointer to self.
     * This is a hack to allow the callbacks to be static without gcc
     * optimizing them away as unused.
     */
    const struct __libc_ForkCallback *pSelf;
#endif
} __LIBC_FORKCALLBACK, *__LIBC_PFORKCALLBACK;


/** @def _FORK_DATA_USED
 * Internal macro for convincing GCC to keep our fork callback structures. */
#if __GNUC_PREREQ__(4,2)
# define _FORK_DATA_USED() __attribute__((__used__))
#else
# define _FORK_DATA_USED()
#endif

/** @def _FORK_PARENT1
 * This macro is used to setup automatic fork callbacks for
 * the parent context. The callbacks are processed per module.
 *
 * @param   uPriority       Priority of the callback.
 *                          Callbacks are sorted descendingly by priority before
 *                          execution.
 * @param   pfnCallback     Callback function. See __LIBC_PFNFORKCALLBACK for prototype.
 */
#define _FORK_PARENT1(uPriority, pfnCallback) \
    static const __LIBC_FORKCALLBACK _FORK_DATA_USED() __fork_parent1_##pfnCallback = { pfnCallback, uPriority + (unsigned)(&__fork_parent1_##pfnCallback - &__fork_parent1_##pfnCallback) }; \
    __asm__ (".stabs \"___fork_parent1__\", 23, 0, 0, ___fork_parent1_" #pfnCallback);

/** @def _FORK_CHILD1
 * This macro is used to setup automatic fork callbacks for
 * the child context. The callbacks are processed per module.
 *
 * @param   uPriority       Priority of the callback.
 *                          Callbacks are sorted descendingly by uPriority before
 *                          execution.
 * @param   pfnCallback     Callback function. See __LIBC_PFNFORKCALLBACK for prototype.
 */
#define _FORK_CHILD1(uPriority, pfnCallback) \
    static const __LIBC_FORKCALLBACK _FORK_DATA_USED() __fork_child1_##pfnCallback  = { pfnCallback, uPriority + (unsigned)(&__fork_child1_##pfnCallback - &__fork_child1_##pfnCallback) }; \
    __asm__ (".stabs \"___fork_child1__\",  23, 0, 0, ___fork_child1_" #pfnCallback );


/**
 * Fork module structure.
 *
 * This is defined in crt0 and dll0 when -Zfork is given to gcc during link.
 */
typedef struct __libc_ForkModule
{
    /** Fork module version (__LIBC_FORK_MODULE_VERSION). */
    unsigned                    uVersion;
    /** Fork callback function.
     * Initialize to _atfork_callback() by crt0 and dll0.
     *
     * This callback is responsible for the main forking of the module. By default this
     * means copying the dirty pages in the datasegment to the child, and to process callbacks.
     * The user can override the behaviour by defining it's own _atfork_callback().
     */
    int                       (*pfnAtFork)(__LIBC_PFORKMODULE pModule, __LIBC_PFORKHANDLE pForkHandle, __LIBC_FORKOP enmOperation);
    /** Pointer to the _FORK_PARENT1 set vector. */
    __LIBC_PFORKCALLBACK       *papParent1;
    /** Pointer to the _FORK_CHILD1 set vector. */
    __LIBC_PFORKCALLBACK       *papChild1;
    /** Data segment base address. */
    void                       *pvDataSegBase;
    /** Data segment end address (exclusive?). */
    void                       *pvDataSegEnd;
    /** Flags, see the __LIBC_FORKMODULE_FLAGS_*. Some of these are runtime flags. */
    unsigned                    fFlags;
    /** Next ForkModule in the list.
     * The head is in the process structure stored in the LIBC shared process
     * management so multiple LIBC versions can share the same module lists. */
    __LIBC_PFORKMODULE          pNext;
    /** Reserved space to make it 32 byte aligned - must be zero. */
    unsigned                    auReserved[8];
} __LIBC_FORKMODULE;


/** Fork Module flags.
 * @{ */
/** Indicates that the module is an executable. */
#define __LIBC_FORKMODULE_FLAGS_EXECUTABLE      0x00000001
/** Indicates that the module already has been deregistered (libc termination). */
#define __LIBC_FORKMODULE_FLAGS_DEREGISTERED    0x00010000
/** @} */


/**
 * Register a forkable module. Called by crt0 and dll0.
 *
 * The call links pModule into the list of forkable modules
 * which is maintained in the process block.
 *
 * @returns 0 on normal process startup.
 *
 * @returns 1 on forked child process startup.
 *          The caller should respond by not calling any _DLL_InitTerm
 *          or similar constructs. If fExecutable the call will not
 *          return in this situation.
 *
 * @returns negative on failure.
 *          The caller should return from the dll init returning FALSE.
 *          If called from crt0 the function will it self call DosExit.
 *
 * @param   pModule     Pointer to the fork module structure for the
 *                      module which is to registered.
 * @param   fExecutable Indicator that the call origins from crt0.s and
 *                      the final forking should start. The function
 *                      will not return if this flag is set and the process
 *                      was forked.
 *
 */
int __libc_ForkRegisterModule(__LIBC_PFORKMODULE pModule, int fExecutable);


/**
 * Deregister a forkable module. Called by dll0.
 *
 * The call links pModule out of the list of forkable modules
 * which is maintained in the process block.
 *
 * @param   pModule     Pointer to the fork module structure for the
 *                      module which is to registered.
 */
void __libc_ForkDeregisterModule(__LIBC_PFORKMODULE pModule);


/**
 * Called multiple times during fork() both in the parent and the child.
 *
 * This default LIBC implementation will:
 *      1) schedule the data segment for duplication.
 *      2) do ordered LIBC fork() stuff.
 *      3) do unordered LIBC fork() stuff, _CRT_FORK1 vector.
 *
 * @returns 0 on success.
 * @returns appropriate negative errno on failure.
 * @returns appropriate positive errno as warning.
 * @param   pModule         Pointer to the module record which is being
 *                          processed.
 * @param   pForkHandle     Handle of the current fork operation.
 * @param   enmOperation    Which callback operation this is.
 *                          Any value can be used, the implementation
 *                          of this function must just respond to the
 *                          one it knows and return successfully on the
 *                          others.
 *                          Operations:
 *                              __LIBC_FORK_OP_CHECK_PARENT
 *                              __LIBC_FORK_OP_CHECK_CHILD
 *                              __LIBC_FORK_OP_FORK_PARENT
 *                              __LIBC_FORK_OP_FORK_CHILD
 */
int __libc_ForkDefaultModuleCallback(__LIBC_PFORKMODULE pModule, __LIBC_PFORKHANDLE pForkHandle, __LIBC_FORKOP enmOperation);


/**
 * At fork per module callback. This calls __libc_ForkDefaultModuleCallback().
 * See __libc_ForkDefaultModuleCallback() for more details.
 *
 * Can be overridden by user - just define it yourself and make sure
 * the linker uses your implementation.
 *
 * @returns 0 on success.
 * @returns appropriate negative errno on failure.
 * @returns appropriate positive errno as warning.
 * @param   pModule         Pointer to the module record which is being
 *                          processed.
 * @param   pForkHandle     Handle of the current fork operation.
 * @param   enmOperation    Which callback operation this is.
 *                          Any value can be used, the implementation
 *                          of this function must just respond to the
 *                          one it knows and return successfully on the
 *                          others.
 *                          Operations:
 *                              __LIBC_FORK_OP_CHECK_PARENT
 *                              __LIBC_FORK_OP_CHECK_CHILD
 *                              __LIBC_FORK_OP_FORK_PARENT
 *                              __LIBC_FORK_OP_FORK_CHILD
 */
int _atfork_callback(__LIBC_PFORKMODULE pModule, __LIBC_PFORKHANDLE pForkHandle, __LIBC_FORKOP enmOperation);

__END_DECLS

#endif /* !__InnoTekLIBC_fork_h__ */

