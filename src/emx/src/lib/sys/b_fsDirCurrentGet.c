/* $Id: b_fsDirCurrentGet.c 2254 2005-07-17 12:25:44Z bird $ */
/** @file
 *
 * LIBC SYS Backend - getcwd.
 *
 * Copyright (c) 2004 knut st. osmundsen <bird@innotek.de>
 *
 *
 * This file is part of InnoTek LIBC.
 *
 * InnoTek LIBC is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * InnoTek LIBC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with InnoTek LIBC; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */


/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include "libc-alias.h"
#define INCL_FSMACROS
#include <os2emx.h>
#include "b_fs.h"
#include <string.h>
#include <errno.h>
#include <InnoTekLIBC/backend.h>
#include <InnoTekLIBC/libc.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_BACK_FS
#include <InnoTekLIBC/logstrict.h>


/*******************************************************************************
*   Internal Functions                                                         *
*******************************************************************************/
static void slashify(char *psz);

/**
 * Converts all slashes to the unix type.
 */
static void slashify(char *psz)
{
    for (;;)
    {
        psz = strchr(psz, '\\');
        if (!psz)
            break;
        *psz++ = '/';
    }
}


/**
 * Gets the current directory of the process on a
 * specific drive or on the current one.
 *
 * @note Differs from __libc_Back_fsDirCurrentGet in that it doesn't lock the
 * FS mutex assuming that the caller has locked it.
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
int __libc_back_fsDirCurrentGet(char *pszPath, size_t cchPath, char chDrive, int fFlags)
{
    LIBCLOG_ENTER("pszPath=%p cchPath=%d chDrive=%c fFlags=%d\n", (void *)pszPath, cchPath, chDrive ? chDrive : '0', fFlags);

    /*
     * Query the current drive.
     */
    ULONG   ul;
    ULONG   ulCurDrive;
    FS_VAR();
    FS_SAVE_LOAD();
    int rc = DosQueryCurrentDisk(&ulCurDrive, &ul);
    if (!rc)
    {
        /*
         * Query the native current directory path with drive letter.
         */
        ULONG   ulDrive = chDrive ? chDrive - (chDrive >= 'A' && chDrive <= 'Z' ? 'A' - 1 : 'a' - 1) : ulCurDrive;
        char    szNativePath[PATH_MAX];
        ULONG   cch = sizeof(szNativePath) - 3;
        rc = DosQueryCurrentDir(ulDrive, (PSZ)&szNativePath[3], &cch);
        if (!rc)
        {
            FS_RESTORE();
            /*
             * Make the drive stuff.
             */
            szNativePath[0] = ulDrive + 'A' - 1;
            szNativePath[1] = ':';
            szNativePath[2] = '\\';

            /*
             * Is this per chance in the unix tree?
             */
            char *pszSrc;
            if (!chDrive && __libc_gfInUnixTree)
            {
                pszSrc = &szNativePath[__libc_gcchUnixRoot];
                if (*pszSrc == '\0')
                {
                    pszSrc[0] = '/';
                    pszSrc[1] = '\0';
                }
                slashify(&szNativePath[2]);

                /*
                 * Check if someone have messed with the current directory...
                 */
                if (memicmp(&szNativePath[0], __libc_gszUnixRoot, __libc_gcchUnixRoot))
                {
                    LIBC_ASSERTM_FAILED("Current directory has been changed while in unixroot! unixroot=%s curdir=%s\n",
                                        __libc_gszUnixRoot, &szNativePath[0]);
                    __libc_gfInUnixTree = 0;
                    pszSrc = &szNativePath[0];
                }
            }
            else
            {
                if (!__libc_gfNoUnix)
                {
                    szNativePath[2] = '/';
                    slashify(&szNativePath[3]);
                }
                pszSrc = &szNativePath[0];
            }

            /*
             * Copy the result.
             */
            if (fFlags && pszSrc[1] == ':')
                pszSrc += 2;            /* drive */
            if ((fFlags & __LIBC_BACK_FSCWD_NO_ROOT_SLASH) && pszSrc[1])
                pszSrc++;           /* root slash */
            int cch = strlen(pszSrc) + 1;
            if (cch <= cchPath)
            {
                memcpy(pszPath, pszSrc, cch);
                LIBCLOG_RETURN_INT(0);
            }
            else
                LIBCLOG_ERROR_RETURN_INT(-ERANGE);
        }
        /* Native errors. */
    }

    FS_RESTORE();
    rc = -__libc_native2errno(rc);
    LIBCLOG_ERROR_RETURN_INT(rc);
}


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
int __libc_Back_fsDirCurrentGet(char *pszPath, size_t cchPath, char chDrive, int fFlags)
{
    /*
     * Lock the fs global state.
     */
    int rc = __libc_back_fsMutexRequest();
    if (rc)
        return rc;

    rc = __libc_back_fsDirCurrentGet(pszPath, cchPath, chDrive, fFlags);

    __libc_back_fsMutexRelease();
    return rc;
}
