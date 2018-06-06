/* $Id: tcpip.c 1519 2004-09-27 02:15:07Z bird $ */
/** @file
 *
 * TCP/IP filehandle support.
 *
 * In order to properly support socket inheritance we need to be able
 * to clean up the sockets in a safe manner. Therefore we need to
 * resolve some TCP/IP socket function when we encounter sockets for
 * the first time.
 *
 * Cleanup of the open sockets is done by the SPM exit list handler.
 *
 *
 * Copyright (c) 2004 knut st. osmundsen <bird-srcspam@anduin.net>
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
#define INCL_DOSMODULEMGR
#include <os2emx.h>


#include <InnoTekLIBC/sharedpm.h>
#include <InnoTekLIBC/tcpip.h>
#include <InnoTekLIBC/errno.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_SOCKET
#include <InnoTekLIBC/logstrict.h>



#undef  __LIBC_LOG_GROUP
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_FORK

/**
 * Loads a given TCPIP dll during fork.
 * @returns 0 on success.
 * @returns negative errno on failure.
 * @param   hmodLoad    The handle of the module we're loading.
 * @param   pszDll      The DLL name.
 */
int __libc_tcpipForkLoadModule(HMODULE hmodLoad, const char *pszDll)
{
    LIBCLOG_ENTER("hmodLoad=%#lx pszDll=%p:{%s}\n", hmodLoad, (void *)pszDll, pszDll);
    int     rc;
    HMODULE hmod;
    char   *psz;

    /*
     * Try simple load.
     */
    rc = DosLoadModule(NULL, 0, (PCSZ)pszDll, &hmod);
    if (!rc && hmod == hmodLoad)
        LIBCLOG_RETURN_INT(0);

    /*
     * Failed, try get the full path to the module and retry.
     */
    if (rc)
        LIBCLOG_MSG("DosLoadModule(0,0,%s,) -> %d!\n", pszDll, rc);
    else
    {
        LIBCLOG_MSG("DosLoadModule(0,0,%s,) returns hmod=%#lx wanted %#lx\n", pszDll, hmod, hmodLoad);
        DosFreeModule(hmod);
    }
    psz = alloca(CCHMAXPATH);
    if (psz)
    {
        rc = DosQueryModuleName(hmodLoad, CCHMAXPATH, psz);
        if (!rc)
        {
            char szErr[16];
            rc = DosLoadModule((PSZ)&szErr[0], sizeof(szErr), (PCSZ)psz, &hmod);
            if (!rc)
                LIBCLOG_RETURN_INT(0);
            LIBC_ASSERTM_FAILED("DosLoadModule(,,'%s',) -> %d szErr=%.16s\n", psz, rc, szErr);
        }
        else
            LIBC_ASSERTM_FAILED("DosQueryModuleName(%#lx,,) -> %d\n", hmodLoad, rc);
        rc = -__libc_native2errno(rc);
    }
    else
    {
        LIBC_ASSERTM_FAILED("Failed to alloca() a path buffer!\n");
        rc = -ENOMEM;
    }

    LIBCLOG_RETURN_INT(rc);
}

