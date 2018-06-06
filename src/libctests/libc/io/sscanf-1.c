/* $Id: $ */
/** @file
 *
 * Testcase sscanf-1, ticket #113.
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
#include <string.h>
#include <stdint.h>
#include <limits.h>


/*******************************************************************************
*   Global Variables                                                           *
*******************************************************************************/
/** Global error count. */
static unsigned g_cErrors = 0;


#define CHECK(expr, type, fmttype, expect) \
    do { \
        type rc = (expr); \
        if (rc != (expect)) \
        { \
            printf("sscanf-1(%d): " #expr " -> " fmttype " expected " fmttype "(" #expect ")\n", __LINE__, rc, (expect)); \
            g_cErrors++; \
        } \
    } while (0)


void simple_tests(void)
{
    long long ll = -1;
    CHECK(sscanf("42", "%lld", &ll), int, "%d", 1);
    CHECK(ll, long long, "%lld", 42);

    if (sizeof(long long) >= sizeof(uint64_t))
    {
        CHECK(sscanf("0x1234567812345678", "%llx", &ll), int, "%d", 1);
        CHECK(ll, long long, "%llx", 0x1234567812345678LL);

        CHECK(sscanf("0x9234567812345678", "%llx", &ll), int, "%d", 1);
        CHECK(ll, long long, "%llx", 0x9234567812345678LL);
    }
}


int main()
{
    /* the tests */
    simple_tests();

    /* report */
    if (!g_cErrors)
        printf("sscanf-1: SUCCESS\n");
    else
        printf("sscanf-1: FAILURE - %d errors\n", g_cErrors);
    return !!g_cErrors;
}
