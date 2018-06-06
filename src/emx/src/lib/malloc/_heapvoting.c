/* $Id: _heapvoting.c 2254 2005-07-17 12:25:44Z bird $ */
/** @file
 *
 * Voting on default heap in High or Low memory.
 *
 * Copyright (c) 2003 knut st. osmundsen <bird-srcspam@anduin.net>
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
#include <emx/umalloc.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_HEAP
#include <InnoTekLIBC/logstrict.h>


/*******************************************************************************
*   Global Variables                                                           *
*******************************************************************************/
/** The voting result. */
static enum { enmHigh = 0, enmLow = 1, enmResultLow = 2, enmResultHigh = 4 }
    enmResult = enmHigh;


/**
 * Place a vote for which heap to use as default.
 *
 * @param   fDefaultHeapInHighMem
 */
void    __libc_HeapVote(int fDefaultHeapInHighMem)
{
    LIBCLOG_ENTER("fDefaultHeapInHighMem=%d\n", fDefaultHeapInHighMem);
    if (enmResult < enmResultLow)
    {
        if (!fDefaultHeapInHighMem)
            enmResult = enmLow;
    }
    LIBCLOG_LEAVE("ret void - enmResult=%d\n", enmResult);
}


/**
 * End the voting.
 */
void    __libc_HeapEndVoting(void)
{
    LIBCLOG_ENTER("\n");
    if (enmResult < enmResultLow)
        enmResult = (enmResult == enmHigh) ? enmResultHigh : enmResultLow;
    else
        LIBC_ASSERT_FAILED();
    LIBCLOG_LEAVE("ret void - enmResult=%d\n", enmResult);
}


/**
 * Asks what the result of the voting were.
 *
 * @returns 0 if to use the low memory heap.
 * @returns 1 if to use the high memory heap.
 * @returns -1 if the voting isn't finished yet.
 */
int     __libc_HeapGetResult(void)
{
    LIBCLOG_ENTER("\n");
    if (enmResult < enmResultLow)
        LIBCLOG_ERROR_RETURN_INT(-1);
    LIBCLOG_RETURN_INT(enmResult == enmResultHigh);
}

