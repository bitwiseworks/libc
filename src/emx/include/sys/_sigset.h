/* $Id: _sigset.h 1617 2004-11-07 09:33:03Z bird $ */
/** @file
 *
 * Signal Set Internal Declaration.
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


#ifndef _SYS__SIGSET_H_
#define _SYS__SIGSET_H_

/*******************************************************************************
*   Defined Constants And Macros                                               *
*******************************************************************************/
/** @group sys__sigset_h_sigset_macros  Signal Set Macros
 * @{
 */
#define __SIGSET_CLONGS     2           /** Space for 64 signals. */
#if __BSD_VISIBLE
#define __SIGSET_MAXSIGNALS 64          /** Maximum signals. */
/** Check if the signal is valid. */
#define __SIGSET_SIG_VALID(sig)         ( (sig) > 0 && (sig) < __SIGSET_MAXSIGNALS)
/** Sets a bit in the bitmap. */
#define __SIGSET_SET(sigset, sig)       (   (sigset)->__bitmap[((sig) - 1) >> 5] |=  (1 << (((sig) - 1) & 31)) )
/** Clears a bit in the bitmap. */
#define __SIGSET_CLEAR(sigset, sig)     (   (sigset)->__bitmap[((sig) - 1) >> 5] &= ~(1 << (((sig) - 1) & 31)) )
/** Tests a bit in the bitmap. */
#define __SIGSET_ISSET(sigset, sig)     ( ( (sigset)->__bitmap[((sig) - 1) >> 5]  &  (1 << (((sig) - 1) & 31)) ) != 0 )
/** Clears all bits in the bitmap. */
#define __SIGSET_EMPTY(sigset)          ( (sigset)->__bitmap[0] = 0, (sigset)->__bitmap[1] = 0, (void)0 )
/** Sets all bits in the bitmap. */
#define __SIGSET_FILL(sigset)           ( (sigset)->__bitmap[0] = ~0, (sigset)->__bitmap[1] = ~0, (void)0 )
/** Checks if the set is empty. */
#define __SIGSET_ISEMPTY(sigset)        ( !(sigset)->__bitmap[0] && !(sigset)->__bitmap[1] )
/** Invert a set (bitwise not) */
#define __SIGSET_NOT(sigset) \
    do { \
        (sigset)->__bitmap[0] = ~(sigset)->__bitmap[0]; \
        (sigset)->__bitmap[1] = ~(sigset)->__bitmap[1]; \
    } while (0)
/** And together two signal sets writing the result to a third one. */
#define __SIGSET_AND(result, set1, set2) \
    do { \
        (result)->__bitmap[0] = (set1)->__bitmap[0] & (set2)->__bitmap[0]; \
        (result)->__bitmap[1] = (set1)->__bitmap[1] & (set2)->__bitmap[1]; \
    } while (0)
/** Or together two signal sets writing the result to a third one. */
#define __SIGSET_OR(result, set1, set2) \
    do { \
        (result)->__bitmap[0] = (set1)->__bitmap[0] | (set2)->__bitmap[0]; \
        (result)->__bitmap[1] = (set1)->__bitmap[1] | (set2)->__bitmap[1]; \
    } while (0)
/** @} */
#endif /* BSD_VISIBLE */

/**
 * Signal set structure.
 */
typedef struct __sigset
{
    /** Bitmap map, each represent a signal. */
    unsigned long __bitmap[__SIGSET_CLONGS];
} __sigset_t;

#endif

