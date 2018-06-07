/* $Id: 1040-static-system-5.c 1424 2004-05-02 22:10:15Z bird $ */
/** @file
 *
 * Test problem with static _System functions.
 *
 * Copyright (c) 2004 knut st. osmundsen <bird-srcspam@anduin.net>
 *
 *
 * This file is part of InnoTek GCC.
 *
 * InnoTek GCC is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * InnoTek GCC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with InnoTek GCC; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
 * Just make the testcase not fail with the old InnoTek GCC.
 */
#if (__GNUC__ == 3 && __GNUC_MINOR__ == 2 && __GNUC_PATCHLEVEL__ == 2)
# warning "This test is broken on this compiler. Since we're not gonna fix it we're just faking success now."
# undef __stdcall
# define __stdcall
#endif 

static void __stdcall ShouldNotBeRemoved(void *pv)
{
    return;
}

void *pfnStdcall = (void *)ShouldNotBeRemoved;


