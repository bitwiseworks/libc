/** @file
 * LIBC Next runtime version query helper.
 */

#include <errno.h>
#include <string.h>
#define INCL_DOSERRORS
#define INCL_DOSMODULEMGR
#include <os2emx.h>
#include <emx/startup.h>
#include <libcn/version.h>
#include "backend.h"

#define ORD___libcn_version 2034 /* Must match libc.def */

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
int __libcn_query_version(struct __libcn_version *version)
{
    APIRET rc;
    HMODULE hmod;
    char szModName[260];
    int fLoaded = 0;
    union
    {
        PFN pfn;
        const struct __libcn_version *version;
    } u;

    if (!version || (version->cb && version->cb < sizeof(version->cb)))
    {
        errno = EINVAL;
        return -1;
    }

    if (!version->cb)
        version->cb = sizeof(struct __libcn_version);

    rc = DosQueryModFromEIP(&hmod, NULL, sizeof(szModName), szModName, NULL, (ULONG)_CRT_init);
    if (rc)
    {
        errno = __libc_native2errno(rc);
        return -1;
    }

    rc = DosQueryProcAddr(hmod, ORD___libcn_version, NULL, &u.pfn);
    if (rc)
    {
        if (rc == ERROR_INVALID_HANDLE)
        {
            /* Load the module explicitly and repeat the query */
            rc = DosLoadModule(NULL, 0, (PSZ)szModName, &hmod);
            if (rc)
            {
                errno = __libc_native2errno(rc);
            }
            else
            {
                fLoaded = 1;
                rc = DosQueryProcAddr(hmod, ORD___libcn_version, NULL, &u.pfn);
            }
        }
        if (rc)
        {
            /* kLIBC/old LIBCn returns a misleading EINVAL for ERROR_INVALID_ORDINAL, fix it */
            if (rc == ERROR_INVALID_ORDINAL)
                errno = ENOSYS;
            else
                errno = __libc_native2errno(rc);
        }
    }

    if (!rc)
    {
        if (!u.version || u.version->cb < sizeof(u.version->cb))
        {
            errno = EINVAL;
            rc = -1;
        }
        else
        {
            if (version->cb > u.version->cb)
                memset(((char*)version) + u.version->cb, 0, version->cb - u.version->cb);
            memcpy(version, u.version, version->cb < u.version->cb ? version->cb : u.version->cb);
        }
    }

    if (fLoaded)
        DosFreeModule(hmod);

    return rc ? -1 : 0;
}
