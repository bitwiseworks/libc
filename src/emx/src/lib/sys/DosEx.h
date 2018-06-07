/* $Id: DosEx.h 1831 2005-03-13 10:50:50Z bird $ */
/** @file
 *
 * Dos API Extension Fundament.
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

#ifndef __DosEx_h__
#define __DosEx_h__

#include <sys/cdefs.h>

/**
 * Entry type.
 */
typedef enum
{
    /** This block is free. */
    DOSEX_TYPE_FREE = 0,
    /** Allocation. */
    DOSEX_TYPE_MEM_ALLOC,
    /** Shared allocation. */
    DOSEX_TYPE_MEM_OPEN,
    /** Mutex create. */
    DOSEX_TYPE_MUTEX_CREATE,
    /** Mutex open. */
    DOSEX_TYPE_MUTEX_OPEN,
    /** Event create. */
    DOSEX_TYPE_EVENT_CREATE,
    /** Event open. */
    DOSEX_TYPE_EVENT_OPEN,
    /** Load module. */
    DOSEX_TYPE_LOAD_MODULE,
    /** Max type (exclusive). */
    DOSEX_TYPE_MAX
} DOSEXTYPE;


/**
 * Fixed size record for recording an extended Dos operation.
 *
 * @remark  Make sure this structure have a nice size.
 */
#pragma pack(1)
typedef struct _DOSEXENTRY
{
    /** Next pointer. */
    struct _DOSEXENTRY *pNext;

    /** Type specific data. */
    union
    {
        /** Search key.
         * We assume that sizeof(unsigned) == sizeof(PVOID) == sizeof(HMTX) == sizeof(HEV) == sizeof(HMODULE).
         */
        unsigned        uKey;

        /**
         * DosAllocMemEx with OBJ_FORK.
         */
        struct
        {
            /** Object address. */
            PVOID       pv;
            /** Object size. */
            ULONG       cb;
            /** Object flags. */
            ULONG       flFlags;
        }   MemAlloc;

        /**
         * Shared memory.
         * (Must be givable!)
         */
        struct
        {
            /** Object address. */
            PVOID       pv;
            /** Open flags. */
            ULONG       flFlags;
#ifdef PER_PROCESS_OPEN_COUNTS
            /** Open count. */
            unsigned    cOpens;
#endif
        }   MemOpen;

        /**
         * Create mutex.
         */
        struct
        {
            /** Mutex handle. */
            HMTX        hmtx;
            /** Flags. */
            ULONG       flFlags;
            /** Initial state. */
            unsigned short  fInitialState;
            /** Current state. */
            unsigned short  cCurState;
        }   MutexCreate;

        /**
         * Open mutex.
         */
        struct
        {
            /** Mutex handle. */
            HMTX        hmtx;
#ifdef PER_PROCESS_OPEN_COUNTS
            /** Open count. */
            unsigned    cOpens;
#endif
        }   MutexOpen;

        /**
         * Create event.
         */
        struct
        {
            /** Event handle. */
            HEV         hev;
            /** Flags. */
            ULONG       flFlags;
            /** Initial state. */
            unsigned short  fInitialState;
            /** Current state. */
            unsigned short  cCurState;
        }   EventCreate;

        /**
         * Open mutex.
         */
        struct
        {
            /** Mutex handle. */
            HEV         hev;
#ifdef PER_PROCESS_OPEN_COUNTS
            /** Open count. */
            unsigned    cOpens;
#endif
        }   EventOpen;

        /**
         * Loaded module.
         */
        struct
        {
            /** Module handle. */
            HMODULE     hmte;
            /** Load count. */
            unsigned    cLoads;
        }   LoadModule;
    }       u;
} DOSEX;
#pragma pack()
/** Pointer to DoxEx record. */
typedef DOSEX *PDOSEX;


__BEGIN_DECLS

/** The current size of allocated private memory.
 * Updated atomically. */
extern size_t           __libc_gcbDosExMemAlloc;


/**
 * Allocate an entry.
 * @returns Pointer to allocated entry.
 * @returns NULL on memory shortage.
 * @param   enmType     Entry type.
 */
PDOSEX  __libc_dosexAlloc(DOSEXTYPE enmType);

/**
 * Free entry.
 * @returns 0 on success.
 * @returns -1 if not found.
 * @returns OS/2 error code on failure.
 * @param   enmType     Enter type.
 * @param   uKey        They entry key.
 * @remark  Caller is responsible for saving/loading/restoring FS.
 */
int     __libc_dosexFree(DOSEXTYPE enmType, unsigned uKey);

/**
 * Finds a entry given by type and key.
 *
 * @returns Pointer to the entry on success.
 *          The caller _must_ call __libc_dosexRelease() with this pointer!
 * @returns NULL on failure.
 *
 * @param   enmType     Type of the entry to find.
 * @param   uKey        Entery key.
 */
PDOSEX  __libc_dosexFind(DOSEXTYPE enmType, unsigned uKey);

/**
 * Releases an entry obtained by __libc_dosexFind().
 * @param   pEntry      Pointer to the entry to release.
 */
void    __libc_dosexRelease(PDOSEX pEntry);

/**
 * Get the memory stats.
 *
 * This api is intended for fork when it's figuring out the minimum and maximum
 * sizes of the fork buffer.
 *
 * @param   pcbPools        Where to store the size of the pools.
 * @param   pcbMemAlloc     Where to store the size of the allocated private memory.
 *                          I.e. memory allocated by DosAllocMemEx(,,..|OBJ_FORK).
 */
void    __libc_dosexGetMemStats(size_t *pcbPools, size_t *pcbMemAlloc);

#ifdef __InnoTekLIBC_fork_h__
/**
 * This function is called very early in the fork, before any
 * datasegments are copied or anything. The purpose is to
 * allocate system resources as early to get the best chance to
 * get hold of them.
 *
 * @returns 0 on success.
 * @returns -errno on failure.
 * @param   pForkHandle     Pointer to the fork handle.
 */
int     __libc_dosexFork(__LIBC_PFORKHANDLE pForkHandle);
#endif

__END_DECLS
#endif
