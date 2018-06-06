/* $Id: atexit.h 2786 2006-08-27 14:26:13Z bird $ */
/** @file
 *
 * LIBC atexit() and on_exit() support.
 *
 * Copyright (c) 2005 knut st. osmundsen <bird@anduin.net>
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

#ifndef __InnoTekLIBC_atexit_h__
#define __InnoTekLIBC_atexit_h__

#include <sys/types.h>

/** Entry type. */
typedef enum __LIBC_ATEXITTYPE
{
    __LIBC_ATEXITTYPE_FREE = 0,
    __LIBC_ATEXITTYPE_ATEXIT,
    __LIBC_ATEXITTYPE_ONEXIT,
    /** State transition state. */
    __LIBC_ATEXITTYPE_TRANS,
    /** The module containing the callback was unloaded. */
    __LIBC_ATEXITTYPE_UNLOADED
} __LIBC_ATEXITTYPE;

/**
 * At exit entry.
 */
typedef struct __LIBC_ATEXIT
{
    /** Entry type. */
    __LIBC_ATEXITTYPE volatile enmType;
    /** The (native) module handle this callback is connected to.
     * This will be 0 if no such association. */
    uintptr_t                  hmod;
    union
    {
        /** Data for a atexit() registration. */
        struct
        {
            /** Pointer to the callback. */
            void (*pfnCallback)();
        } AtExit;

        /** Data for a on_exit() registration. */
        struct
        {
            /** Pointer to the callback .*/
            void (*pfnCallback)(int iExit, void *pvUser);
            /** User argument. */
            void *pvUser;
        } OnExit;
    } u;
} __LIBC_ATEXIT, *__LIBC_PATEXIT;

/**
 * Chunk of at exit registrations.
 */
typedef struct __LIBC_ATEXITCHUNK
{
    /** Pointer to the next chunk. */
    struct __LIBC_ATEXITCHUNK * volatile pNext;
    /** Number of entries which may contain a valid callback.
     * (May include freed entries.) */
    uint32_t volatile   c;
    /** Array of at exit entries. */
    __LIBC_ATEXIT       a[64];
} __LIBC_ATEXITCHUNK, *__LIBC_PATEXITCHUNK;


__BEGIN_DECLS

/** Pointer to the head of the exit list chain - LIFO. */
extern __LIBC_PATEXITCHUNK __libc_gAtExitHead;

/**
 * Allocate a new atexit entry.
 *
 * @returns Pointer to new entry.
 * @returns NULL on failure.
 *
 * @param   pvCallback      The callback address.
 *                          This used to initialize the __LIBC_AT_EXIT::hmod field.
 */
__LIBC_PATEXIT __libc_atexit_new(void *pvCallback);

/**
 * Invalidate all atexit and on_exit callback for a
 * module which is being unloaded.
 *
 * @param   hmod        The module handle.
 */
void __libc_atexit_unload(uintptr_t hmod);

__END_DECLS

#endif
