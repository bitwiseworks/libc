/* $Id: b_ioFHToPath.c 2326 2005-09-26 02:36:28Z bird $ */
/** @file
 *
 * LIBC Backend - __libc_Back_fhToPath().
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
#include "libc-alias.h"
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <emx/io.h>
#include <InnoTekLIBC/backend.h>
#define __LIBC_LOG_GROUP    __LIBC_LOG_GRP_BACKEND
#include <InnoTekLIBC/logstrict.h>

#define INCL_DOSPROFILE
#define INCL_DOSERRORS
#define INCL_FSMACROS
#include <os2emx.h>



/**
 * Try resolve a filehandle to a path.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   fh          The file handle.
 * @param   pszPath     Where to store the path.
 * @param   cchPath     The size of he buffer pointed to by pszPath.
 */
int __libc_Back_ioFHToPath(int fh, char *pszPath, size_t cchPath)
{
    LIBCLOG_ENTER("fh=%d pszPath=%p cchPath=%d\n", fh, (void *)pszPath, cchPath);

    /*
     * Get the filehandle structure and check that's is an ordinary file.
     */
    __LIBC_PFH  pFH;
    int rc = __libc_FHEx(fh, &pFH);
    if (rc)
        LIBCLOG_ERROR_RETURN(rc, "ret %d - Invalid filehandle %d\n", rc, fh);

    if (pFH->pszNativePath)
    {
        size_t cch = strlen(pFH->pszNativePath) + 1;
        if (cch <= cchPath)
        {
            memcpy(pszPath, pFH->pszNativePath, cch);
            LIBCLOG_RETURN_INT(0);
        }

        memcpy(pszPath, pFH->pszNativePath, cchPath);
        pszPath[cchPath - 1] = '\0';
        LIBCLOG_ERROR_RETURN(-EOVERFLOW, "ret -EOVERFLOW - cch=%d cchPath=%d '%s'\n", cch, cchPath, pFH->pszNativePath);
    }


    /*
     * Now query the SFN number for this file.
     */

    /*
     * Query the filename fromthe SFN entry.
     */

    /** @todo implement this api. */
    LIBCLOG_ERROR_RETURN(-ENOSYS, "ret -ENOSYS - Not implemented!\n");
}

