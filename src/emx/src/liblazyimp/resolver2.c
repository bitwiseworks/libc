/* $Id: resolver2.c 1545 2004-10-07 06:36:42Z bird $ */
/** @file
 *
 * Lazy Import - resolver2.
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
#include <386/builtin.h>
#include <dlfcn.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

void *__lazyimp_resolver2(unsigned uEAX, unsigned uEDX, unsigned uECX,
                          void **ppvModule, const char *pszModule, void **ppvResolved, const char *pszSymbol);

/**
 * This function is called by __lazyimp_resolver, and assembly worker.
 * __lazyimp_resolver was in turn called by a stub in the lazy import record.
 *
 * @returns Resolved address and *ppvModule and *ppvResovled set.
 */
void *__lazyimp_resolver2(unsigned uEAX, unsigned uEDX, unsigned uECX,
                          void **ppvModule, const char *pszModule, void **ppvResolved, const char *pszSymbol)
{
    /*
     * Load the module.
     */
    void *pvModule;
    if (!(pvModule = *ppvModule))
    {
        pvModule = dlopen(pszModule, 0);
        if (!pvModule)
        {
            static char szMsg[] = "lazy import failure!\n\r";
            const char *pszErr = dlerror();
            write(2, szMsg, sizeof(szMsg) - 1);
            write(2, pszErr, strlen(pszErr));
            write(2, "\r\n", 2);
            abort();
            for (;;)
                exit(127);
        }
        __atomic_xchg((volatile unsigned *)ppvModule, (unsigned)pvModule);
    }

    /*
     * Resolve the symbol.
     */
    void *pvSym = dlsym(pvModule, pszSymbol);
    if (pvSym)
    {
        __atomic_xchg((volatile unsigned *)ppvResolved, (unsigned)pvSym);
        return pvSym;
    }
    else
    {
        static char szMsg[] = "lazy import failure!\n\r";
        const char *pszErr = dlerror();
        write(2, szMsg, sizeof(szMsg) - 1);
        write(2, pszErr, strlen(pszErr));
        write(2, "\r\n", 2);
        abort();
        for (;;)
            exit(127);
    }
    return 0;
}

