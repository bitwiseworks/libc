/*
    libc.h -- Internal definitions for OS/2 libc
    Copyright (c) 2003 InnoTek Systemberatung GmbH
*/

#ifndef __LIBC_ALIAS_H__
#define __LIBC_ALIAS_H__

/*
    Since VAC and MSVC (and possibly ANSI) requires all basic libc functions
    to be callable with and without a leading underscore (e.g. _fwrite should
    be an alias for fwrite) we have to mark these functions somehow to apply
    special treatment when building c_alias.a library and when building the
    .def file for libc dll.

    A effective way to do this is to prefix every such symbol with a certain
    unique prefix, which is then detected and replaced with whatever is
    required. For this we define a few macros that should be used just when
    the function is declared (not used) which will mark the function as
    'aliased'.

    The following macro means exactly the following:
    The function is a standard (POSIX) function that should
    have an alias with the same name prefixed by ONE underscore.

    In fact, such functions can be referenced by three different names:
    say, if you use _STD(xxx) you can reference later std_xxx, xxx
    or _xxx - all names will point to the same function (of course,
    if you don't have a function with one of those three names).
*/
#define _STD(x) _std_ ## x

/* The libc-std.h file is auto-generated from all libc .c files
   and contains lines like ``#define malloc _STD(malloc)'' */
#include "libc-std.h"

#ifdef TIMEBOMB
void __libc_Timebomb(void);
asm(".stabs  \"___libc_Timebomb\",1,0,0,0");
#endif


#endif /* __LIBC_ALIAS_H__ */
