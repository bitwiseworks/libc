/* $Id: $ */
/** @file
 *
 * Testcase sprintf-1.
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
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>


/*******************************************************************************
*   Global Variables                                                           *
*******************************************************************************/
/** Global error count. */
static unsigned g_cErrors = 0;


#define CHECK(expect, ...) \
    do { \
        static const char s_szExpect[] = expect; \
        char szBuf[4096]; \
        int rc = sprintf(szBuf, __VA_ARGS__); \
        if (rc != sizeof(s_szExpect) - 1) \
        { \
            printf("sprintf-1(%d): rc=%d expected %d.\n" \
                   "    args: %s\n"  \
                   "  actual: '%s'\n" \
                   "expected: '%s'\n", \
                   __LINE__, rc, sizeof(s_szExpect) - 1, #__VA_ARGS__, szBuf, s_szExpect); \
            g_cErrors++; \
        } \
        else if (memcmp(szBuf, s_szExpect, sizeof(s_szExpect))) \
        { \
            printf("sprintf-1(%d): Output mismatch!\n" \
                   "    args: %s\n" \
                   "  actual: '%s'\n" \
                   "expected: '%s'\n", \
                   __LINE__, rc, sizeof(s_szExpect) - 1, #__VA_ARGS__, szBuf, s_szExpect); \
            g_cErrors++; \
        } \
    } while (0)


void simple_tests(void)
{
    CHECK("4095 42 4095 42 4095 42 4095 42 4095 42 4095 42 4095",
          "%d %lld %d %zd %d %jd %d %td %d %qd %d %Ld %d",
          4095, (long long)42, 4095, (size_t)42, 4095, (intmax_t)42, 4095, (ptrdiff_t)42, 4095, (long long)42, 4095, (long long)42, 4095);
}


int main()
{
    /* the tests */
    simple_tests();

    /* report */
    if (!g_cErrors)
        printf("sprintf-1: SUCCESS\n");
    else
        printf("sprintf-1: FAILURE - %d errors\n", g_cErrors);
    return !!g_cErrors;
}

