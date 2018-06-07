/* $Id: pathrewrite-1.c 2055 2005-06-19 08:04:00Z bird $ */
/** @file
 *
 * InnoTek LIBC Testcase - Path Rewrite Feature.
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
#include <InnoTekLIBC/pathrewrite.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>


/*******************************************************************************
*   Global Variables                                                           *
*******************************************************************************/
/** buffer for dynamically getting the right ETC. */
char                gszEtcTo[256];
/** rewrite the /etc directory. */
__LIBC_PATHREWRITE  gEtcRule =
{
    __LIBC_PRWF_TYPE_DIR | __LIBC_PRWF_CASE_SENSITIVE,
    "/etc",  4,
    &gszEtcTo[0], 0
};

__LIBC_PATHREWRITE  gEtcRuleBad =
{
    __LIBC_PRWF_TYPE_DIR | __LIBC_PRWF_CASE_SENSITIVE,
    "/etc/",  5,
    "c:/mptn/etc", -1
};





int main(int argc, char **argv)
{
    int     rcRet = 0;
    FILE    *pFile;

    /*
     * Test default rewriters.
     */
    pFile = fopen("/dev/null", "r");
    if (pFile)
        fclose(pFile);
    else
    {
        printf("pathrewrite: /dev/null is not rewrittend\n");
        rcRet++;
    }

    pFile = fopen("/dev/tty", "r");
    if (pFile)
        fclose(pFile);
    else
    {
        printf("pathrewrite: /dev/tty is not rewrittend\n");
        rcRet++;
    }


    /*
     * Add rules.
     */
    if (    __libc_PathRewriteAdd(&gEtcRuleBad, 1) != -1
        &&  errno == EINVAL)
    {
        printf("pathrewrite: negative add test failed\n");
        rcRet++;
    }

    strcpy(&gszEtcTo[0], getenv("ETC"));
    gEtcRule.cchTo = strlen(gEtcRule.pszTo);
    if (__libc_PathRewriteAdd(&gEtcRule, 1))
    {
        printf("pathrewrite: add test failed\n");
        rcRet++;
    }

    pFile = fopen("/etc/hosts", "r");
    if (pFile)
        fclose(pFile);
    else
    {
        printf("pathrewrite: /etc/hosts is not rewrittend\n");
        rcRet++;
    }


    /*
     * Remove rules.
     */
    if (__libc_PathRewriteRemove(&gEtcRule, 1))
    {
        printf("pathrewrite: remove test failed\n");
        rcRet++;
    }

    /*
     * Status and exit.
     */
    if (!rcRet)
        printf("pathrewrite: Successfully executed all tests\n");
    else
        printf("pathrewrite: %d tests failed\n", rcRet);
    return rcRet;
}
