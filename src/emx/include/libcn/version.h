/** @file
 *
 * LIBC Next runtime version information.
 *
 * Copyright (c) 2026 Dmitrii Kuminov <coding@dmik.org>
 *
 *
 * This file is part of LIBC Next.
 *
 * LIBC Next is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * LIBC Next is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with LIBC Next; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef _LIBCN_VERSION_H
#define _LIBCN_VERSION_H

#include <sys/cdefs.h>

__BEGIN_DECLS

/** LIBC Next runtime version. */
struct __libcn_version
{
    /** Size of this structure, in bytes. */
    unsigned int cb;
    unsigned int major;
    unsigned int minor;
    unsigned int build;
};

/**
 * Queries the version of the LIBC Next DLL used by the caller.
 *
 * Before calling, set version->cb to the size of the caller's structure (at
 * least sizeof(__libcn_version.cb)), or to 0 to imply sizeof(__libcn_version).
 *
 * On success, up to the smaller of the caller and runtime structure sizes is
 * copied from the runtime DLL, and unused part of the caller structure (if any)
 * is zero-filled.
 *
 * For kLIBC DLLs and older LIBCn DLLs that lack version information, -1 is
 * returned and errno is set to ENOSYS.
 *
 * @returns 0 on success, -1 on failure with errno set.
 */
int __libcn_query_version(struct __libcn_version *version);

__END_DECLS

#endif /* _LIBCN_VERSION_H */
