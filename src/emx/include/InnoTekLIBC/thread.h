/* $Id: thread.h 2254 2005-07-17 12:25:44Z bird $ */
/** @file
 *
 * LIBC Thread Handling.
 *
 * Copyright (c) 2004 knut st. osmundsen <bird-srcspam@anduin.net>
 *
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

#ifndef __InnoTekLIBC_thread_h__
#define __InnoTekLIBC_thread_h__

/*******************************************************************************
*   Defined Constants And Macros                                               *
*******************************************************************************/
/** Maximum number of TLS variables supported by LIBC.
 * The current limit (128) is higher than for instance WinNT. But if
 * you think we're wasting space, look at the page padding of the thread
 * structure... */
#define __LIBC_TLS_MAX      128


/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include <sys/cdefs.h>
#include <time.h>                       /* struct tm; */
#include <signal.h>


/*******************************************************************************
*   Structures and Typedefs                                                    *
*******************************************************************************/
struct _uheap;

/**
 * sigwait,sigwaitinfo, sigtimedwait data.
 */
typedef volatile struct __libc_thread_sigwait
{
    /** Done waitin' indicator.*/
    volatile int    fDone;
    /** The signals we're waiting for. */
    sigset_t        SigSetWait;
    /** The where to return signal info. */
    siginfo_t       SigInfo;
} __LIBC_THREAD_SIGWAIT, *__LIBC_PTHREAD_SIGWAIT;


/**
 * sigsuspend data.
 */
typedef volatile struct __libc_thread_sigsuspend
{
    /** Done waitin' indicator.*/
    volatile int    fDone;
} __LIBC_THREAD_SIGSUSPEND, *__LIBC_PTHREAD_SIGSUSPEND;


/**
 * Signal notification callback function.
 *
 * This is a notification which can be used to correct the state of
 * a system object before any user code is executed.
 *
 * The semaphore lock is hold and signals are all on hold, so be very careful with waitin
 * on other semphores and stuff like that. Crashing is totally forbidden. :-)
 *
 * @param   iSignalNo       The signal number.
 * @param   pvUser          The user argument.
 */
typedef void __LIBC_FNSIGCALLBACK(int iSignalNo, void *pvUser);
/** Pointer to a signal callback function. */
typedef __LIBC_FNSIGCALLBACK *__LIBC_PFNSIGCALLBACK;


/**
 * Per thread structure for LIBC.
 *
 * This structure contains buffers and variables which in a single threaded
 * LIBC would be static.
 *
 * Members most frequently used have been put together at the front.
 */
typedef struct __libc_thread
{
    /** errno value. (Comes first, see errnofun.s.) */
    int             iErrNo;
    /** Default tiled heap. */
    struct _uheap * pTiledHeap;
    /** Default regular heap. */
    struct _uheap * pRegularHeap;
    /** Reference count. */
    volatile unsigned cRefs;
    /** Thread Id. */
    unsigned        tid;
    /** Pointer to next thread in the list. */
    struct __libc_thread   *pNext;
    /** New TLS variable array. */
    void           *apvTLS[__LIBC_TLS_MAX];
    /** Old TLS variable. */
    void           *pvThreadStoreVar;

    /** The nesting depth of the default logger. */
    unsigned        cDefLoggerDepth;
    /** Buffer used by asctime() and indirectly by ctime() . (Adds two 2 bytes for padding). */
    char            szAscTimeAndCTimeBuf[26+2];
    /** Buffer used by gmtime() and localtime(). */
    struct tm       GmTimeAndLocalTimeBuf;
    /** Buffer used by tmpnam(). */
    char            szTmpNamBuf[16];
    /** Current posistion of strtok(). */
    char           *pszStrTokPos;
    /** Buffer used by strerror() to format unknown error values. */
    char            szStrErrorBuf[28];
    /** Buffer used by _getvol(). */
    char            szVolLabelBuf[12];
    /** Buffer used by ttyname(). */
    char            szTTYNameBuf[32];

    /** Pending signals.
     * Protected by the signal semaphore. */
    sigset_t        SigSetPending;
    /** Blocked signals.
     * Protected by the signal semaphore. */
    sigset_t        SigSetBlocked;
    /** Old Blocked signals. Used by sigsuspend().
     * sigsuspend() sets this and fSigSetBlockedOld. When a signal is to be
     * delivered this member will be pushed on the stack for use on an eventual
     * return. fSigSetBlockOld will be clared.
     * Protected by the signal semaphore. */
    sigset_t        SigSetBlockedOld;

    /** Signals queued for delivery on this thread.
     * Protected by the signal semaphore. */
    struct
    {
        struct SignalQueued *pHead;
        struct SignalQueued *pTail;
    }               SigQueue;

    /** Alternate signal stack block address.
     * Protected by the signal semaphore. */
    void           *pvSigStack;
    /** Alternate signal stack block size.
     * Protected by the signal semaphore. */
    size_t          cbSigStack;

    /** @defgroup   libc_threadstruct_flags     Thread Flags
     * @todo checkup access safety here!
     * @{ */
    /** If set SigSetBlockedOld should be used used to restore SigSetBlocked
     * when returning from a signal handler.  */
    unsigned        fSigSetBlockedOld : 1;
    /** If set the stack block pointed to by pvSigStack is in use. */
    unsigned        fSigStackActive : 1;
    /** If set the thread is internal.
     * This means the thread will not receive signals. */
    unsigned        fInternalThread : 1;
    /** @} */

    /** Flags whether or not the thread is being forced to evaluate its
     * pending signals.
     *
     * All updates of this variable must be done atomically and when owning
     * the signal semaphore. The atomically requirement is because it's being
     * read without owning the semaphore.
     */
    volatile unsigned   fSigBeingPoked;

    /** The millisecond timestamp of the last signal.
     * This is used to detect system call interruptions (select). The function will clear
     * before doing the system call and evaluate it when the call returns. signals.c will set it
     * when ever a thread enters for processing a signal asynchronously. */
    volatile unsigned long      ulSigLastTS;
    /** Callback on signal/exception. */
    __LIBC_PFNSIGCALLBACK       pfnSigCallback;
    /** User argument to signal/exception callback. */
    void                       *pvSigCallbackUser;

    /** Thread status, chiefly used for the u member of the thread structure. */
    volatile enum enmLIBCThreadStatus
    {
        /** The thread status must be queried from the OS. */
        enmLIBCThreadStatus_unknown = 0,
        /** The thread status must be queried from the OS. */
        enmLIBCThreadStatus_startup,
        /** The thread is in a sigwait(), sigwaitinfo(), or sigtimedwait() call. */
        enmLIBCThreadStatus_sigwait,
        /** The thread is in a sigsuspend() call. */
        enmLIBCThreadStatus_sigsuspend,
    }               enmStatus;

    /** Data used in certain thread states.
     * Use the enmStatus field to determin which way to read the data items here.
     */
    union
    {
        /** enmLIBCThreadStatus_startup:    Begin Thread Arguments. */
        struct __libc_thread_u_startup
        {
            /** Thread argument. */
            void    *pvArg;
            /** Thread routine. */
            void   (*pfnStart)(void *pvArg);
        } startup;

        /** enmLIBCThreadStatus_sigwait:    Thread blocked in sigwait(), sigwaitinfo() or sigtimedwait(). */
        __LIBC_PTHREAD_SIGWAIT      pSigWait;
        /** enmLIBCThreadStatus_sigsuspend: Thread blocked in sigsuspend(). */
        __LIBC_PTHREAD_SIGSUSPEND   pSigSuspend;
    } u;

    /** Data used by the backends. */
    union __libc_backend_data
    {
        struct __libc_sys
        {
            /** Directory find data entry.
             * Used by __findfirst() and __findnext(). */
            struct find_data
            {
                /** Directory handle. HDIR_CREATE if no session opened. */
                unsigned long   hdir;
                /** Type of buffer content. FIL_STANDARDL or FIL_STANDARD,
                 * i.e. FILEFINDBUF4 or FILEFINDBUF4L. */
                unsigned long   fType;
                /** Number of files left in the buffer. */
                unsigned long   cFiles;
                /** Pointer to the next entry. Don't test on this, test on cFiles! */
                const char     *pchNext;
                /** Buffer. */
                char            achBuffer[2048];
            } fd;
        } sys;
    } b;


} __LIBC_THREAD;


#ifndef __LIBC_THREAD_DECLARED
#define __LIBC_THREAD_DECLARED
typedef struct __libc_thread *__LIBC_PTHREAD, **__LIBC_PPTHREAD;
#endif


/**
 * Thread Termination Callback Registration Record. (cool name, right)
 * For use with __libc_ThreadRegisterTermCallback().
 */
typedef struct __libc_ThreadTermCbRegRec
{
    /** This member must be initialized to NULL. */
    struct __libc_ThreadTermCbRegRec   *pNext;
    /** Flags field reserved for future use.
     * Must be initalized to ZERO.  */
    unsigned                            fFlags;
    /**
     * The callback function.
     *
     * @param pRegRec   Registration record which the callback was registered with.
     * @param fFlags    Reserved for future use. Will always be zero when fFlags
     *                  in the RegRec is zero.
     */
    void                              (*pfnCallback)(struct __libc_ThreadTermCbRegRec *pRegRec, unsigned fFlags);
} __LIBC_THREADTERMCBREGREC, *__LIBC_PTHREADTERMCBREGREC;


/*******************************************************************************
*   Global Variables                                                           *
*******************************************************************************/
__BEGIN_DECLS
/** Pointer to the TLS ULONG (OS/2 rules) which will point to the LIBC thread
 * structure for the current thread.
 * The TLS ULONG is allocated by __init_dll(). The thread structure it points to
 * is allocated on demand. */
extern __LIBC_PPTHREAD  __libc_gpTLS;
__END_DECLS


/*******************************************************************************
*   External Functions                                                         *
*******************************************************************************/
__BEGIN_DECLS
/**
 * Get the thread structure for the current thread.
 *
 * Will automatically allocate a thread structure if such is not yet done
 * for the thread.
 *
 * @returns pointer to current thread struct.

 * @remark  No reference counting here, current thread have a permanent
 *          reference to it self.
 * @remark  This API is considered to be internal to LIBC and is thus not
 *          exposed in the shared library version of LIBC. Please don't call it.
 *          External LIBs should use the __libc_TLS*() API.
 */
#define __libc_threadCurrent()   (*__libc_gpTLS ? *__libc_gpTLS : __libc_threadCurrentSlow())


/**
 * Get the thread structure for the current thread.
 *
 * Used by the __libc_threadCurrent() macro for allocating a thread structure for the
 * current thread when such doesn't exist.
 *
 * @returns pointer to current thread struct.

 * @remark  No reference counting here, current thread have a permanent
 *          reference to it self.
 * @remark  This API is considered to be internal to LIBC and is thus not
 *          exposed in the shared library version of LIBC. Please don't call it.
 *          External LIBs should use the __libc_TLS*() API.
 */
__LIBC_PTHREAD __libc_threadCurrentSlow(void);


/**
 * Get the thread structure for the current thread.
 *
 * Do not create anything automatically.
 *
 * @returns pointer to current thread struct.
 * @returns NULL if not initiated.
 *
 * @remark  No reference counting here, current thread have a permanent
 *          reference to it self.
 * @remark  This API is considered to be internal to LIBC and is thus not
 *          exposed in the shared library version of LIBC. Please don't call it.
 *          External LIBs should use the __libc_TLS*() API.
 */
#define __libc_threadCurrentNoAuto()   (__libc_gpTLS ? *__libc_gpTLS : NULL)


/**
 * Get the thread structure for the thread specified by it's thread identification.
 *
 * Used for instance by signal handling to change the signal properties of another
 * thread.
 *
 * @returns Pointer to threads thread struct.
 *          The caller _must_ call __libc_threadRelease() when it's done using the
 *          thread structure.
 * @returns NULL if the thread wasn't found.
 * @param   tid     The Thread Id of the thread to find.
 * @remark  This API is considered to be internal to LIBC and is thus not
 *          exposed in the shared library version of LIBC. Please don't call it.
 */
__LIBC_PTHREAD __libc_threadLookup(unsigned tid);


/**
 * Get the thread structure for a thread selected by a custom callback function.
 *
 * @returns Pointer to the selected thread.
 *          The caller _must_ call __libc_threadRelease() when it's done using the
 *          thread structure.
 * @param   pfnCallback Function which will to the thread selection.
 *
 *                      Returns 1 if the current thread should be returned.
 *                      Returns 2 if the current thread should be returned immediately.
 *                      Returns 0 if the current best thread should remain unchanged.
 *                      Returns -1 if the enumeration should fail (immediately).
 *
 *                      pCur        The current thread.
 *                      pBest       The current best thread.
 *                      pvParam     User parameter.
 *
 * @param   pvParam     User Parameter.
 */
__LIBC_PTHREAD __libc_threadLookup2(int (pfnCallback)(__LIBC_PTHREAD pCur, __LIBC_PTHREAD pBest, void *pvParam), void *pvParam);


/**
 * Enumerates all the threads LIBC is aware of subjecting each of them to a
 * caller specified callback function.
 *
 * @returns 0 on success.
 * @returns -1 if pfnCallback returned -1.
 *
 * @param   pfnCallback Function which will to the thread selection.
 *
 *                      Returns 0 if the enmeration should continue.
 *                      Returns -1 if the enumeration should fail (immediately).
 *
 *                      pCur        The current thread.
 *                      pvParam     User parameter.
 *
 * @param   pvParam     User Parameter.
 */
int         __libc_threadEnum(int (pfnCallback)(__LIBC_PTHREAD pCur, void *pvParam), void *pvParam);


/**
 * Allocate and initialize a thread structure for a thread which is yet
 * to be created.
 *
 * The returned thread structure will have cRefs set to 1, thus
 * use __libc_threadDereference() to free it.
 *
 * @returns Pointer to thread structure.
 * @returns NULL on error. errno set.
 */
__LIBC_PTHREAD __libc_threadAlloc(void);


/**
 * Sets up the current thread to use the thread structure pThrd.
 *
 * @param   pThrd   Pointer to the thread structure this thread
 *                  should be using.
 */
void __libc_threadUse(__LIBC_PTHREAD pThrd);


/**
 * Dereferences a thread structure referenced by __libc_threadLookup() or
 * __libc_threadLookup2(), or allocated by __libc_threadAlloc().
 *
 * LIBC maintains reference counting on the thread structure so the thread
 * structure will not be freed by the thread it represent while someone else
 * is accessing it. However, the reference counting does not protect any of
 * the structures members from writes or reads, that's left to the users of
 * the members to synchronize between them.
 *
 * @returns pointer to threads thread struct.
 * @returns NULL if the thread wasn't found.
 * @param   pThrd   Pointer to thread structure returned by __libc_threadLookup(),
 *                  __libc_threadLookup2() or __libc_threadAlloc().
 * @remark  This API is considered to be internal to LIBC and is thus not
 *          exposed in the shared library version of LIBC. Please don't call it.
 */
void __libc_threadDereference(__LIBC_PTHREAD pThrd);


/**
 * Register a thread destruction callback.
 *
 * This will be called when one thread is terminating normally, i.e. calling
 * _endthread() or returning from it's thread function.
 * When LIBC implements pthreads basics any new non-abnormal thread exit will
 * cause a callback too.
 *
 * @param   pRegRec     Pointer to thread registration record.
 *                      This must be initialized as described in the documation of
 *                      the structure. After calling this API the memory must not
 *                      be touched or freed by the application. It is not possible
 *                      to unregister a callback at present.
 *
 * @remark  We might wanna extend the API at a later point for calling back
 *          at abnormal termination and such. Such extensions will be done
 *          using the fFlags member of __LIBC_THREADTERMCBREGREC and the fFlags
 *          parameter to the callback.
 */
int     __libc_ThreadRegisterTermCallback(__LIBC_PTHREADTERMCBREGREC pRegRec);

/**
 * Internal API which is called by a thread exit to work the registered callbacks.
 *
 * Not called for thread 1.
 *
 * @param   fFlags  Reserved for termination reasons.
 *                  Zero means normal exit, no other codes have been defined.
 */
void    __libc_threadTermination(unsigned fFlags);


/** @group InnoTek LIBC Thread Local Storage
 * @{
 */

/**
 * Allocates a TLS entry.
 *
 * @returns index of the allocated TLS index.
 * @returns -1 on failure. errno set.
 */
int     __libc_TLSAlloc(void);

/**
 * Frees a TLS entry allocated by __libc_TLSAlloc().
 *
 * @returns 0 on success.
 * @returns -1 on failure. errno set.
 * @param   iIndex      Value returned by __libc_TLSAlloc().
 */
int     __libc_TLSFree(int iIndex);

/**
 * Get the value stored in an allocated TLS entry.
 *
 * @returns value in given TLS entry.
 * @returns NULL on failure with errno set.
 * @param   iIndex      Value returned by __libc_TLSAlloc().
 */
void *  __libc_TLSGet(int iIndex);

/**
 * Set the value stored in an allocated TLS entry.
 *
 * @returns 0 on success.
 * @returns -1 on failure. errno set.
 * @param   iIndex      Value returned by __libc_TLSAlloc().
 * @param   pvValue     Value to store.
 */
int     __libc_TLSSet(int iIndex, void *pvValue);

/**
 * Register a thread termination destructor for an TLS entry.
 *
 * The destructor function will be called when a thread terminates
 * in a normal fashion and the TLS entry iIndex of that thread is
 * not NULL.
 *
 * There will be no callbacks in thread 1.
 *
 * @returns 0 on succces.
 * @returns -1 on failure. errno set.
 * @param   iIndex          Value returned by __libc_TLSAlloc().
 * @param   pfnDestructor   Callback function. Use NULL to unregister a previously
 *                          registered destructor.
 *
 *                          It's pvValue argument is the non-zero value in the
 *                          TLS entry for the thread it's called on.
 *
 *                          It's fFlags argument is reserved for future use, it will
 *                          always be zero when the fFlags parameter to this API is zero.
 *
 * @param   fFlags          Flags reserved for future use. At the moment
 *                          only ZERO is allowed.
 *
 * @remark  The application is not allowed to call __libc_TLSFree() for iIndex when calling
 *          this function. The result from doing that is undefined.
 */
int     __libc_TLSDestructor(int iIndex, void (*pfnDestructor)(void *pvValue, int iIndex, unsigned fFlags), unsigned fFlags);


/**
 * Get pointer to the destructor function registered for the given TLS entry.
 *
 * @returns NULL if invalid entry, errno set.
 * @returns NULL if no entry registered.
 * @returns Pointer to destructor if registered.
 *
 * @param   iIndex          Value returned by __libc_TLSAlloc().
 * @param   pfFlags         Where to store the flags supplied to __libc_TLSDestructor().
 *                          NULL is ok.
 */
void (*__libc_TLSGetDestructor(int iIndex, unsigned *pfFlags))(void *, int, unsigned);

/* fix later */
int __libc_back_threadCreate(void (*pfnStart)(void *), unsigned cbStack, void *pvArg, int fInternal);

/** @} */

__END_DECLS

#endif
