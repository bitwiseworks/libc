/* Copyright (C) 1991, 92, 93, 96, 98, 2003 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

/* should've been an -include but anyway, this'll do for now. */
#include <libc-symbols.h>

/* config.h candidates? */
#undef NO_UNDERSCORES
#define HAVE_CPP_ASM_DEBUGINFO
#define HAVE_ASM_SET_DIRECTIVE
#define HAVE_ASM_WEAKEXT_DIRECTIVE
#ifndef __BOUNDED_POINTERS__
#define __BOUNDED_POINTERS__ 0
#endif

/* unix/i386/sysdep.h */
#ifdef	__ASSEMBLER__

#define	r0		%eax	/* Normal return-value register.  */
#define	r1		%edx	/* Secondary return-value register.  */
#define scratch 	%ecx	/* Call-clobbered register for random use.  */
#define MOVE(x,y)	movl x, y

#endif	/* __ASSEMBLER__ */

/* unix/sysdep.h */
#undef HAVE_SYSCALLS

/* include i386 sysdep.h, which in turn includes the generic one. */
#include <sysdeps/i386/sysdep.h>
