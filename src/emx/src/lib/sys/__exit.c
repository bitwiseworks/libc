/* sys/exit.c (emx+gcc) -- Copyright (c) 1992-1993 by Eberhard Mattes
                           Copyright (c) 2004 knut st. osmundsen */

#include "libc-alias.h"
#define INCL_FSMACROS
#include <os2emx.h>
#include <emx/syscalls.h>
#include <InnoTekLIBC/sharedpm.h>
/*******************************************************************************
*   Structures and Typedefs                                                    *
*******************************************************************************/
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_INITTERM
#include <InnoTekLIBC/logstrict.h>


void __exit(int rc)
{
    LIBCLOG_ENTER("rc=%d\n", rc);
    FS_VAR();

    /*
     * Update the SPM data base with the right exit reason and code.
     */
    __libc_spmTerm(__LIBC_EXIT_REASON_EXIT, rc);

    /*
     * Actually exit the process.
     */
    FS_SAVE_LOAD();
    for (;;)
        DosExit(EXIT_PROCESS, rc < 0 || rc > 255 ? 255 : rc);
    LIBCLOG_MSG("shut up\n");
}
