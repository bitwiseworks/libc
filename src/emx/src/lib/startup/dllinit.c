/* $Id: dllinit.c 2297 2005-08-21 09:30:00Z bird $ */
/** @file
 * Default DLL init and termination code.
 */

#include <emx/startup.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_INITTERM
#include <InnoTekLIBC/logstrict.h>


/**
 * This is the typical DLL startup code.
 *
 * @returns 1 on success.
 * @returns 0 on failure.
 * @param   hmod    Handle of this DLL.
 * @param   fFlag   0 init.
 *                  1 term.
 * @remark  This function is called from dll0.asm.
 */
unsigned long _System _DLL_InitTerm(unsigned long hmod, unsigned long fFlag)
{
    LIBCLOG_ENTER("hmod=%#lx flag=%ld\n", hmod, fFlag);
    switch (fFlag)
    {
        case 0:
            if (_CRT_init() != 0)
                break;
            __ctordtorInit();
            LIBCLOG_RETURN_INT(1);
        case 1:
            __ctordtorTerm();
            _CRT_term();
            LIBCLOG_RETURN_INT(1);
    }
    LIBCLOG_ERROR_RETURN_INT(0);
}

