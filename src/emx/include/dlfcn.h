/* dlfcn.h,v 1.1 2003/05/26 09:28:55 bird Exp */
/** @file
 * Dynamic Library Loading.
 *
 * Copyright (c) 2001-2003 knut st. osmundsen <bird@anduin.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with This program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef _DLFCN_H_
#define _DLFCN_H_

#include <sys/cdefs.h>


/** @name dlopen flags
 * @{ */
/** Relocations are performed at an implementation-defined time.
 * OS2: Happens when the pages are touched. Option is ignored. */
#define RTLD_LAZY   1
/** Relocations are performed when the object is loaded.
 * OS2: Happens when the pages are touched. Option is ignored. */
#define RTLD_NOW    2
/** All symbols are available for relocation processing of other modules.
 * OS2: Ignored as symbols are resolved using explicit link time reference. */
#define RTLD_GLOBAL 0x100
/** All symbols are not made available for relocation processing by other modules.
 * OS2: Ignored as symbols are resolved using explicit link time reference. */
#define RTLD_LOCAL  0
/** @} */

__BEGIN_DECLS

void *          dlopen(const char * pszLibrary, int flFlags);
const char *    dlerror(void);
void *          dlsym(void *pvHandle, const char * pszSymbol);
int             dlclose(void *pvHandle);

#if __BSD_VISIBLE
struct __dlfunc_arg
{
    int         __dlfunc_dummy;
};
typedef	void (*dlfunc_t)(struct __dlfunc_arg);
dlfunc_t        dlfunc(void * __restrict, const char * __restrict);
#endif

__END_DECLS

#endif

