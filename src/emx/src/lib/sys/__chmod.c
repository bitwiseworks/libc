/* sys/chmod.c (emx+gcc) -- Copyright (c) 1992-1996 by Eberhard Mattes
                         -- Copyright (c) 2003-2004 by knut st. osmundsen */

/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include "libc-alias.h"
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#define INCL_FSMACROS
#include <os2emx.h>
#include <emx/syscalls.h>
#include "syscalls.h"
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_BACK_IO
#include <InnoTekLIBC/logstrict.h>
#include <InnoTekLIBC/pathrewrite.h>


int __chmod(const char *pszPath, int flag, int attr)
{
    LIBCLOG_ENTER("pszPath=%s flag=%#x attr=%#x\n", pszPath, flag, attr);
    int         rc;
    int         cch;
    FILESTATUS3 info;
    FS_VAR();

    /*
     * Rewrite the specified file path.
     */
    cch = __libc_PathRewrite(pszPath, NULL, 0);
    if (cch > 0)
    {
        char *pszRewritten = alloca(cch);
        if (!pszRewritten)
        {
            errno = ENOMEM;
            LIBCLOG_ERROR_RETURN_INT(-1);
        }
        cch = __libc_PathRewrite(pszPath, pszRewritten, cch);
        if (cch > 0)
            pszPath = pszRewritten;
        /* else happens when someone changes the rules between the two calls. */
        else if (cch < 0)
        {
            errno = EAGAIN;             /* non-standard, I'm sure. */
            LIBCLOG_ERROR_RETURN_INT(-1);
        }
    }

    /*
     * Validate input.
     */
    if (flag < 0 || flag > 1)
    {
        errno = EINVAL;
        LIBCLOG_ERROR_RETURN_INT(-1);
    }
    if (    (pszPath[0] == '/' || pszPath[0] == '\\')
        &&  (pszPath[1] == 'p' || pszPath[1] == 'P')
        &&  (pszPath[2] == 'i' || pszPath[2] == 'I')
        &&  (pszPath[3] == 'p' || pszPath[3] == 'P')
        &&  (pszPath[4] == 'e' || pszPath[4] == 'E')
        &&  (pszPath[5] == '/' || pszPath[5] == '\\'))
    {
        errno = ENOENT;
        LIBCLOG_ERROR_RETURN_INT(-1);
    }

    /*
     * Query current attributes.
     */
    FS_SAVE_LOAD();
    rc = DosQueryPathInfo((PCSZ)pszPath, FIL_STANDARD, &info, sizeof(info));
    if (rc != 0)
    {
        FS_RESTORE();
        _sys_set_errno(rc);
        LIBCLOG_ERROR_RETURN_INT(-1);
    }
    if (flag == 0)
    {
        FS_RESTORE();
        LIBCLOG_RETURN_INT((int)info.attrFile);
    }

    /*
     * Change attributes.
     */
    info.attrFile = attr;
    rc = DosSetPathInfo((PCSZ)pszPath, FIL_STANDARD, &info, sizeof(info), 0);
    FS_RESTORE();
    if (rc != 0)
    {
        _sys_set_errno(rc);
        LIBCLOG_ERROR_RETURN_INT(-1);
    }
    LIBCLOG_RETURN_INT(0);
}
