/** $Id: $ */
/** @file
 *
 * kLIBC - CRT init and termination code.
 *
 * Copyright (c) 1990-1998 by Eberhard Mattes
 * Copyright (c) 2004-2006 knut st. osmundsen <bird@innotek.de>
 *
 *
 * This file is part of kLIBC.
 *
 * kLIBC is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * kLIBC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with kLIBC; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */



/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include <errno.h>
#include <sys/builtin.h>
#include <emx/startup.h>
#define __LIBC_LOG_GROUP    __LIBC_LOG_GRP_INITTERM
#include <InnoTekLIBC/logstrict.h>


/*******************************************************************************
*   Global Variables                                                           *
*******************************************************************************/
/** The balance between _CRT_init and _CRT_term calls. */
static volatile int32_t gcCRTReferences = 0;


/**
 * Initializes the C runtime library.
 *
 * A _CRT_init() call should be matched by a _CRT_term() call as we are
 * keeping a count of the number of C runtime users.
 *
 * This function is normally called from _DLL_InitTerm and crt0.s.
 *
 * @returns 0 on success. -1 on failure.
 */
int _CRT_init(void)
{
    LIBCLOG_ENTER("\n");

    /*
     * On initialize once.
     */
    int32_t cRefs = __atomic_increment_s32(&gcCRTReferences);
    if (cRefs != 1)
        LIBCLOG_RETURN_MSG(0, "ret 0 (cRefs=%d)\n", cRefs);

    /*
     * Call the weak initializers.
     */
    __ctordtorInit1(&__crtinit1__);

    /*
     * ANSI X3.159-1989, 4.1.3: "The value of errno is zero at program startup..."
     *
     * The above code usually sets errno to EBADF, therefore we reset errno
     * to zero before calling main().
     */
    errno = 0;
    LIBCLOG_RETURN_INT(0);
}


/**
 * Terminates the C runtime library.
 *
 * A _CRT_term() call must be paired with a _CRT_init() call as we're
 * counting the number of C runtime users.
 *
 * This function is normally called from _DLL_InitTerm and crt0.s.
 */
void _CRT_term(void)
{
    LIBCLOG_ENTER("\n");
    int32_t cRefs = __atomic_decrement_s32(&gcCRTReferences);
    if (cRefs == 0)
    {
        /*
         * Call the weak terminators.
         */
        __ctordtorTerm1(&__crtexit1__);
    }
    else if (cRefs < 0)
        LIBCLOG_ERROR_RETURN_MSG_VOID("cRefs=%d\n", cRefs);
    LIBCLOG_RETURN_VOID();
}


/** @page Startup   Startup
 *
 * Quick description of how we startup a process which modules uses LIBC.
 *
 * The load first loads the DLLs of the process, calling their _DLL_InitTerm
 * entrypoints in order of dependencies. If you're using dynamic LIBC this
 * will be initiated first of the LIBC-based modules.
 *
 * LIBCxy.DLL:
 *      - dll0.s gets control and calls __init_dll in sys/__initdll.c
 *          - __init_dll calls __libc_HeapVote() to do the heap voting.
 *          - __init_dll initiates the _osminor and _osmajor globals.
 *          - __init_dll initiates _sys_gcbVirtualAddressLimit global.
 *          - __init_dll initiates _sys_pid and _sys_ppid globals.
 *          - __init_dll creates _sys_heap_fmutex, _sys_gmtxHimem and __libc_gmtxExec.
 *          - __init_dll then initiates __libc_gpTLS with an allocated TLS ULONG.
 *            The thread structure it self isn't initialized untill it's actually
 *            referenced, and when it is there is static structure for the first
 *            thread needing its per thread area.
 *          - __init_dll calls _sys_init_environ() which initializes environ and _org_environ.
 *              - _sys_init_environ() will call _hmalloc() thus initiating the high heap.
 *          - __init_dll calls __libc_spmSelf() which checks initiates SPM and gets
 *            any inherited properties.
 *          - __init_dll calls _sys_init_largefileio() which checks for LFN APIs.
 *          - __init_dll calls __libc_fhInit() which initiates filehandle table.
 *              - __libc_fhInit() will call _hmalloc() and a number of OS/2 APIs
 *                to figure out what handles was inherited.
 *          - __init_dll checks for LIBC_HOOK_DLLS and calls __libc_back_hooksInit()
 *                to process it if it is present in the environment.
 *          - __init_dll then intializes _sys_clock0_ms with the current MS count.
 *      - dll0.s calls _DLL_InitTerm in startup/dllinit.c.
 *          - _DLL_InitTerm calls _CRT_init() in startup/startup.c to initialize the CRT.
 *              - _CRT_init calls init_files() to initialize the file handle tables.
 *              - _CRT_init then call all the crt init functions in __crtinit1__.
 *          - _DLL_InitTerm calls __ctordtorInit() to initate any exception structures and
 *            construct static C++ objects.
 *      - dll0.s then returns to DOSCALL1.DLL and back to the kernel.
 *
 * Your.DLL:
 *      - dll0.s gets control and calls __init_dll() in sys/__initdll.c
 *          - __init_dll calls __libc_HeapVote() to do the heap voting.
 *          - __init_dll then returns since already done during LIBCxy.DLL init.
 *      - dll0.s calls _DLL_InitTerm in startup/dllinit.c.
 *          - _DLL_InitTerm calls _CRT_init() in startup/startup.c to initialize the CRT.
 *              - _CRT_init return at once since already done during LIBCxy.DLL init.
 *          - _DLL_InitTerm calls __ctordtorInit() to initate any exception structures and
 *            construct static C++ objects in _this_ DLL.
 *      - dll0.s then returns to DOSCALL1.DLL and back to the kernel.
 *
 * Your.exe:
 *      - crt0.s gets control and calls ___init_app in sys/386/appinit.s which forwards
 *        the call to __init() in sys/__init.c.
 *          - __init() calls __init_dll() to do common initiation.
 *              - __init_dll calls __libc_HeapVote() to do the heap voting.
 *              - __init_dll then returns since already done during LIBCxy.DLL init.
 *          - __init() parse the commandline to figure out how much stack to use for array
 *            and the argument copy.
 *          - __init() the allocates the stack space space, adding an exception handler struct
 *            and main() callframe.
 *          - __init() parse the commandline creating argv and it's strings using the
 *            allocated stack space.
 *          - __init() calls __libc_spmInheritFree() to release the inherit data (shared)
 *            associated with the current process.
 *          - __init() installs the exception handler.
 *          - __init() set the signal focus.
 *          - __init() then 'returns' thru the hack called ___init_ret in sys/386/appinit.s
 *      - crt0.s now gets control back with esp pointing to a complete callframe for main().
 *      - crt0.s calls _CRT_init() in startup/startup.c to initialize the CRT which returns
 *        immediately since since already done during LIBCxy.DLL init.
 *      - crt0.s the calls main().
 *      - crt0.s then calls exit() with the return value of main().
 *          - exit() will call all the registered at_exit functions.
 *          - exit() will call DosExit with the exit code and terminate the process.
 *
 * @todo update this with file handle changes!
 * @todo update this with fork and performance changes!
 */

