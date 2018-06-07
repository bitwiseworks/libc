/* clib.h -- Header file for emx.dll's minimal C run-time library
   Copyright (c) 1992-1995 by Eberhard Mattes

This file is part of emx.

emx is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

emx is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with emx; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.

As special exception, emx.dll can be distributed without source code
unless it has been changed.  If you modify emx.dll, this exception
no longer applies and you must remove this paragraph from all source
files for emx.dll.  */


#if !defined (_SIZE_T)
#define _SIZE_T
typedef unsigned long size_t;
#endif

#define	offsetof(type, member)  ((size_t)&((type *)0)->member)

void _defext (char *dst, const char *ext);
char *_getname (const char *path);
void *memcpy (void *dst, const void *src, size_t count);
size_t strlen (const char *s);
int strcmp (const char *string1, const char *string2);
char *strcpy (char *dst, const char *src);
int stricmp (const char *string1, const char *string2);
void *memset (void *dst, int c, size_t n);
int strncmp (const char *string1, const char *string2, size_t n);

static inline int tolower (int c)
{
  return (c >= 'A' && c <= 'Z' ? c - 'A' + 'a' : c);
}
