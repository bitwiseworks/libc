/* $Id: hooks.c 2021 2005-06-13 02:16:10Z bird $ */
/** @file
 *
 * LIBC SYS Backend - hooks.
 *
 * Copyright (c) 2005 knut st. osmundsen <bird@innotek.de>
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

/** @page pg_hooks  Hooks
 *
 * Presently this is a simple feature to load and call dlls as specified in
 * the LIBC_HOOK_DLLS environment variable.
 *
 *
 * @subsection  LIBC_HOOK_DLLS
 *
 * The format is <dllname>@<export>[!<type>]. LIBC will load <dllname>,
 * resolve <export>and call <export> according to the convention of <type>.
 * Single of double quotes can be applied to the <dllname>. If multiple hooks
 * is desired, they shall be separated by the semicolon, space or tab character.

 * Failure to parse this variable, load dlls and resolve functions as specified
 * bye this variable will not prevent libc processes from starting. So, do NOT
 * count on it as a security measure.
 *
 * @subsubsection   Types
 *
 * The default type is 'generic' and means that the libc module handle and
 * the address of __libc_Back_ldrSymbol will be passed.
 *
 * 'pathrewrite' is another type. The parameters here are the three
 * __libc_PathRewrite functions. The function should
 *
 *
 * @subsubsection   Examples
 *
 *  set LIBC_HOOK_DLLS=prwhome.dll@_instprws!pathrewrite
 *  set LIBC_HOOK_DLLS=/usr/lib/prwhome.dll@_instprws!pathrewrite
 *  set LIBC_HOOK_DLLS="c:\My Plugins\prwhome.dll"@_instprws!pathrewrite
 *  set LIBC_HOOK_DLLS=prwhome@_insthomeprws!pathrewrite prwhome@_instetcprws!pathrewrite
 */


/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#define _GNU_SOURCE
#include "libc-alias.h"
#define INCL_BASE
#define INCL_EXAPIS
#include <os2emx.h>
#include "backend.h"
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include "syscalls.h"
#include <InnoTekLIBC/backend.h>
#include <InnoTekLIBC/pathrewrite.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_INITTERM
#include <InnoTekLIBC/logstrict.h>



/**
 * Initializes the various LIBC hooks using the given
 * specification string.
 */
void __libc_back_hooksInit(const char *pszSpec, unsigned long hmod)
{
    LIBCLOG_ENTER("pszSpec=%s\n", pszSpec);

    /*
     * Parse the variable.
     */
    const char *psz = pszSpec;
    for (;;)
    {
        char        chQuote;
        const char *pszDll;
        const char *pszDllEnd;
        const char *pszSym;
        const char *pszSymEnd;
        const char *pszType;
        const char *pszTypeEnd;
        char        szSym[CCHMAXPATH];
        char        szDll[CCHMAXPATH];
        HMODULE     hmod;
        APIRET      rc;

        /*
         * Trimming off blanks.
         */
        while ((chQuote = *psz) == ' ' || chQuote == '\t' || chQuote == ';')
            psz++;
        if (!chQuote)
            break;

        /*
         * Check for quotes and find the end of the spec.
         */
        if (chQuote != '\'' && chQuote != '"')
        {
            pszDll = psz;
            chQuote = 0;
            pszDllEnd = strpbrk(psz, " \t;@!");
            if (!pszDllEnd)
            {
                LIBC_ASSERTM_FAILED("missformed dll specification; '%s'\n", psz);
                LIBCLOG_RETURN_MSG_VOID("ret void. bad spec (1)! stopped parsing!\n");
            }
            pszSym = pszDllEnd;
        }
        else
        {   /* quoted */
            pszDll = psz + 1;
            pszDllEnd = strchr(pszDll, chQuote);
            if (!pszDllEnd)
            {
                LIBC_ASSERTM_FAILED("mismatching quotes '%s'\n", psz);
                LIBCLOG_RETURN_MSG_VOID("ret void. bad spec (2)! stopped parsing!\n");
            }
            pszSym = pszDllEnd + 1;
        }

        pszSym++;
        pszSymEnd = strpbrk(pszSym, " \t;!@'\"");
        if (!pszSymEnd)
            pszSymEnd = strchr(pszSym, '\0');
        if (    pszSymEnd == pszSym
            || (*pszSymEnd != '!' && *pszSymEnd != ' ' && *pszSymEnd != '\t' && *pszSymEnd != '\0' && *pszSymEnd != ';'))
        {
            LIBC_ASSERTM_FAILED("missformed export specification; '%s'\n", psz);
            LIBCLOG_RETURN_MSG_VOID("ret void. bad spec (3)! stopped parsing!\n");
        }

        /* type */
        if (*pszSymEnd == '!')
        {
            pszType = pszSymEnd + 1;
            pszTypeEnd = strpbrk(pszType, " \t;!@'\"");
            if (!pszTypeEnd)
                pszTypeEnd = strchr(pszType, '\0');
            if (    pszTypeEnd == pszType
                || (*pszTypeEnd != ' ' && *pszTypeEnd != '\t' && *pszTypeEnd != '\0' && *pszTypeEnd != ';'))
            {
                LIBC_ASSERTM_FAILED("missformed export specification; '%s'\n", psz);
                LIBCLOG_RETURN_MSG_VOID("ret void. bad spec (4)! stopped parsing!\n");
            }
        }
        else
            pszTypeEnd = pszType = NULL;

        /* lengths */
        if (pszDllEnd - pszDll >= sizeof(szDll))
        {
            LIBC_ASSERTM_FAILED("dll name is too long! %d bytes ; '%s'\n", pszDllEnd - pszDll, psz);
            LIBCLOG_RETURN_MSG_VOID("ret void. bad spec (5)! stopped parsing!\n");
        }

        if (pszSymEnd - pszSym >= sizeof(szSym))
        {
            LIBC_ASSERTM_FAILED("symbol name is too long! %d bytes ; '%s'\n", pszSymEnd - pszSym, psz);
            LIBCLOG_RETURN_MSG_VOID("ret void. bad spec (6)! stopped parsing!\n");
        }

        /*
         * Load the DLL.
         */
        memcpy(szDll, pszDll, pszDllEnd - pszDll);
        szDll[pszDllEnd - pszDll] = '\0';
        #if defined(DEBUG_LOGGING) || defined(__LIBC_STRICT)
        rc = DosLoadModuleEx((PSZ)szSym, sizeof(szSym), (PCSZ)szDll, &hmod);
        #else
        rc = DosLoadModuleEx(NULL, 0, (PCSZ)szDll, &hmod);
        #endif
        if (!rc)
        {
            union
            {
                PFN pfn;
                void (*pfnRewriter)(int (*pfnAdd)(), int (*pfnRemove)(), int (*pfnRewrite)(), void *pvHandle);
                void (*pfnGeneric)(void *pvHandle, int (*pfnLdrSymbol)(void *pvHandle,  const char *pszSymbol, void **ppfn));
            } u;

            /*
             * Resolve the symbol.
             */
            memcpy(szSym, pszSym, pszSymEnd - pszSym);
            szSym[pszSymEnd - pszSym] = '\0';
            rc = DosQueryProcAddr(hmod, 0, (PSZ)szSym, &u.pfn);
            if (!rc)
            {
                LIBCLOG_MSG("Successfully loaded and resolved hook '%s' in '%s' to %p\n",
                            szSym, szDll, (void *)u.pfn);
                if (!pszType || !strncmp(pszType, "generic", sizeof("generic") - 1))
                    u.pfnGeneric((void *)hmod, __libc_Back_ldrSymbol);
                else if (!strncmp(pszType, "pathrewrite", sizeof("pathrewrite") - 1))
                    u.pfnRewriter(__libc_PathRewriteAdd, __libc_PathRewriteRemove, __libc_PathRewrite, (void *)hmod);

            }
            else
                LIBC_ASSERTM_FAILED("Failed to resolve rewriter '%s' in '%s', rc=%lu\n", szSym, szDll, rc);
        }
        else
            LIBC_ASSERTM_FAILED("Failed to load rewriter '%s', rc=%lu szObj='%s'\n", szDll, rc, szSym);

        /*
         * Next part.
         */
        psz = pszType ? pszTypeEnd : pszSymEnd;
    }

    LIBCLOG_RETURN_VOID();
}

/** @todo Term */
