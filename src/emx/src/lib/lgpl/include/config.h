/* $Id: config.h 3058 2007-04-08 18:51:40Z bird $ */
/** @file
 *
 * config.h file for GLIBC porting.
 *
 * Copyright (c) 2004 knut st. osmundsen <bird-srcspam@anduin.net>
 *
 *
 * This file is part of Innotek LIBC.
 *
 * Innotek LIBC is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Innotek LIBC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Innotek LIBC; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef __config_h_
#define __config_h_


#define HAVE_VPRINTF 1
#define STDC_HEADERS 1
#define HAVE_DECL_STRERROR_R 1
#define HAVE_STRERROR_R 1
#define HAVE_STRERROR 1
#define HAVE_DECL_STRERROR 1
#define HAVE_STRDUP 1
#define HAVE_MEMPCPY 1
#define HAVE_ALLOCA 1
#define HAVE_ALLOCA_H 1
#define HAVE_LIMITS_H 1
#define HAVE_GETUID 1
#define HAVE_GETGID 1
#define HAVE_GETEUID 1
#define HAVE_GETEGID 1
#define HAVE_ICONV 1
#define HAVE_STPCPY 1
#define HAVE_UNISTD_H 1
#define HAVE_BUILTIN_EXPECT 1
#define HAVE_ARGZ_H 1
#define HAVE___ARGZ_COUNT 1
#define HAVE___ARGZ_STRINGIFY 1
#define HAVE___ARGZ_NEXT 1
#define HAVE_STDINT_H 1
#define HAVE_STDINT_H_WITH_UINTMAX 1
#define HAVE_STRCASECMP 1
#define HAVE_STRTOUL 1
#define HAVE_SYS_PARAM_H 1
#define HAVE_GETCWD 1
#define HAVE_TSEARCH 1
#define HAVE_STDDEF_H 1
#define HAVE_STDLIB_H 1
#define HAVE_STRING_H 1
#define HAVE_LANGINFO_H 1
#define HAVE_LANGINFO_CODESET 1
#define HAVE_SETLOCALE 1
#define HAVE_LOCALE_H 1
#define HAVE_LC_MESSAGES 1
#define HAVE_LOCALE_NULL 1
#define HAVE_WCHAR_H 1
#define HAVE_WCTYPE_H 1
#define HAVE_ISBLANK 1
#define HAVE_WCRTOMB 1
#define HAVE_MBRTOWC 1
#define HAVE_WCSCOLL 0
#define HAVE_DECL_GETC_UNLOCKED 1
#define HAVE_STDBOOL_H 1

#define STRERROR_R_CHAR_P 0
#define DISALLOW_MMAP 1

/* shut up stupid yy */
#define YYSTACK_USE_ALLOCA 1
#define YYMAXDEPTH 10000
#define YYDEBUG 0
#define YYLSP_NEEDED 0

#define LOCALEDIR           "/@unixroot/usr/locale"
#define LIBDIR              "/@unixroot/usr/lib"
#define LOCALE_ALIAS_PATH   LOCALEDIR

#if 0
# define INTUSE(name)                   __libc_internal_##name
# define INTDEF(name)                   strong_alias (name, __libc_internal_##name)
# define INTVARDEF(name)                _INTVARDEF (name, __libc_internal_##name)
# if defined HAVE_VISIBILITY_ATTRIBUTE
#  define _INTVARDEF(name, aliasname)   extern __typeof (name) aliasname __attribute__ ((alias (#name), visibility ("hidden")));
# else
#  define _INTVARDEF(name, aliasname)   extern __typeof (name) aliasname __attribute__ ((alias (#name)));
# endif
# define INTDEF2(name, newname)         strong_alias (name, __libc_internal_##newname)
# define INTVARDEF2(name, newname)      _INTVARDEF (name,  __libc_internal_##newname)
#else
# define INTUSE(name)                   name
# define INTDEF(name)
# define INTVARDEF(name)
# define INTDEF2(name, newname)
# define INTVARDEF2(name, newname)
#endif

#undef HAVE_ASM_SET_DIRECTIVE
#define ASM_GLOBAL_DIRECTIVE        .globl
#undef ASM_TYPE_DIRECTIVE_PREFIX
#undef HAVE_GNU_LD
#undef HAVE_ELF
#undef HAVE_ASM_WEAK_DIRECTIVE
#undef HAVE_ASM_WEAKEXT_DIRECTIVE

#define DO_VERSIONING 0
#ifndef __BOUNDED_POINTERS__
#define __BOUNDED_POINTERS__ 0
#endif

#ifndef __ASSEMBLER__
#include "libc-alias-glibc.h"
#endif
#endif
