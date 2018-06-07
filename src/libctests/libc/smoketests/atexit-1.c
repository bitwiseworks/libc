/* $Id: $ */
/** @file
 *
 * atexit-1 testcase, ticket #113.
 *
 * Copyright (c) 2006 knut st. osmundsen <bird@innotek.de>
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
#include <unistd.h>
#include <sys/wait.h>
#include <dlfcn.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <limits.h>


int failure(const char *pszFormat, ...)
{
    printf("fork-3: ");
    va_list va;
    va_start(va, pszFormat);
    vprintf(pszFormat, va);
    va_end(va);
    return 1;
}

int forkit(const char *pszTestName)
{
    int pid = fork();
    if (pid < 0)
        return failure("%s: fork failed: %d - %s\n", pszTestName, errno, strerror(errno));
    if (!pid)
        exit(0);
    int rc;
    for (;;)
    {
        rc = 1;
        int pid2 = waitpid(pid, &rc, 0);
        if (pid2 < 0 && errno == EINTR)
            continue;
        if (pid2 < 0)
            return failure("%s: waitpid(%d,,) failed: %d - %s\n", pszTestName, pid, errno, strerror(errno));
        if (pid2 != pid)
            return failure("%s: waitpid(%d,,) -> %d\n", pszTestName, pid, pid2);
        if (rc != 0)
            return failure("%s: child rc=%d, expected 0!\n", pszTestName, rc);
        return 0;
    }
}

int main()
{
    /*
     * Figure DLL name.
     */
    char szDll[PATH_MAX];
    if (_execname(szDll, sizeof(szDll) - 4))
        return failure("_execname: %d - %s\n", errno, strerror(errno));
    char *pszExt = _getext(szDll);
    if (pszExt)
        strcpy(pszExt, ".dll");
    else
        strcat(szDll, ".dll");

    /*
     * Open and close the DLL. It will register the callbacks.
     *
     * We could fork, but for simplicity we will just exit and let
     * the makefile catch exceptions.
     */
    void *pvDll = dlopen(szDll, 0);
    if (!pvDll)
        return failure("Can't open '%s': %s\n", szDll, dlerror());
    if (dlclose(pvDll))
        return failure("Can't close '%s': %s\n", szDll, dlerror());
    return 0;
}

