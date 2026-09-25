/* LIBC Next runtime version information. */

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
