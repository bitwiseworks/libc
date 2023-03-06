/* $Id: startup.h 3645 2008-05-18 23:42:37Z bird $ */
/** @file
 * kLIBC startup stuff.
 *
 *
 * Copyright (c) 2008 knut st. osmundsen <bird-src-spam@anduin.net>
 *
 *
 * This file is part of kLIBC.
 *
 * kLIBC is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * kLIBC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with kLIBC; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */


/** @name Flags at argv[x][-1].
 * @remarks Only on OS/2 and NT.
 * @remarks Compatible with the _ARG_* stuff in emx/startup.h.
 * @{ */
#define __KLIBC_ARG_SIGNATURE   "\177kLIBC\177"
/** This a filler to make sure the flag char isn't zero. */
#define __KLIBC_ARG_NONZERO     0x80
/** Mask out the actual flags. */
#define __KLIBC_ARG_MASK        0x7f
/** Argument was double quoted. */
#define __KLIBC_ARG_DQUOTE      0x01
/** Argument was read from response file. */
#define __KLIBC_ARG_RESPONSE    0x02
/** Argument was expaneded from wildcard. */
#define __KLIBC_ARG_WILDCARD    0x04
/** Argument was expaneded from environment. */
#define __KLIBC_ARG_ENV         0x08
/** Argument was expanded by the shell. */
#define __KLIBC_ARG_SHELL       0x10
/** Argument was passed via a unix argv. */
#define __KLIBC_ARG_ARGV        0x20
/** Argument was passed via a shared memory block. */
#define __KLIBC_ARG_SHMEM       0x40
/** @} */

/** @name OS/2 Stub info.
 * @{ */
/** The stub signature of kLIBC binaries. */
#define __KLIBC_STUB_SIGNATURE_BASE     "OS/2 executable module built for kLIBC v"
/** The of the signature from the start of the executable. */
#define __KLIBC_STUB_SIGNATURE_OFF      0x40
/** The minimum size of a kLIBC binary. */
#define __KLIBC_STUB_MIN_SIZE           0x80
/** @} */
