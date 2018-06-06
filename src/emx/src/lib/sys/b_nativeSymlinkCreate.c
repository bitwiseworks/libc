/* $Id: b_nativeSymlinkCreate.c 3792 2012-03-23 00:48:44Z bird $ */
/** @file
 *
 * LIBC SYS Backend - symlink.
 *
 * Copyright (c) 2004-2005 knut st. osmundsen <bird@innotek.de>
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


/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include "libc-alias.h"
#define INCL_FSMACROS
#define INCL_ERRORS
#include <os2emx.h>
#include "b_fs.h"
#include "syscalls.h"
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <InnoTekLIBC/libc.h>
#include <InnoTekLIBC/backend.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_BACK_FS
#include <InnoTekLIBC/logstrict.h>


/*******************************************************************************
*   Global Variables                                                           *
*******************************************************************************/
/**
 * The prefilled FEA2LIST construct for creating all unix attributes including symlink.
 */
#pragma pack(1)
static const struct __LIBC_FSUNIXATTRIBSCREATESYMLINKFEA2LIST
{
    struct __LIBC_FSUNIXATTRIBSCREATEFEA2LIST Core;

    ULONG   offSymlink;
    BYTE    fSymlinkEA;
    BYTE    cbSymlinkName;
    USHORT  cbSymlinkValue;
    CHAR    szSymlinkName[sizeof(EA_SYMLINK)];
    USHORT  usSymlinkType;
    USHORT  cbSymlinkData;
    char    szSymlink[1];

} __libc_gFsUnixAttribsCreateSymlinkFEA2List =
{
#define OFF(a,b)  offsetof(struct __LIBC_FSUNIXATTRIBSCREATESYMLINKFEA2LIST, a)  - offsetof(struct __LIBC_FSUNIXATTRIBSCREATESYMLINKFEA2LIST, b)
    {
        sizeof(__libc_gFsUnixAttribsCreateSymlinkFEA2List),
        OFF(Core.offGID,    Core.offUID),   FEA_NEEDEA,  sizeof(EA_UID) - 1,   sizeof(uint32_t) + 4, EA_UID,   EAT_BINARY, sizeof(uint32_t), 0, "",
        OFF(Core.offMode,   Core.offGID),   FEA_NEEDEA,  sizeof(EA_GID) - 1,   sizeof(uint32_t) + 4, EA_GID,   EAT_BINARY, sizeof(uint32_t), 0, "",
        OFF(Core.offINO,    Core.offMode),  FEA_NEEDEA,  sizeof(EA_MODE) - 1,  sizeof(uint32_t) + 4, EA_MODE,  EAT_BINARY, sizeof(uint32_t), 0, "",
        OFF(Core.offRDev,   Core.offINO),   FEA_NEEDEA,  sizeof(EA_INO) - 1,   sizeof(uint64_t) + 4, EA_INO,   EAT_BINARY, sizeof(uint64_t), 0, "",
        OFF(Core.offGen,    Core.offRDev),  FEA_NEEDEA,  sizeof(EA_RDEV) - 1,  sizeof(uint32_t) + 4, EA_RDEV,  EAT_BINARY, sizeof(uint32_t), 0, "",
        OFF(Core.offFlags,  Core.offGen),   FEA_NEEDEA,  sizeof(EA_GEN) - 1,   sizeof(uint32_t) + 4, EA_GEN,   EAT_BINARY, sizeof(uint32_t), 0, "",
        OFF(offSymlink,     Core.offFlags), FEA_NEEDEA,  sizeof(EA_FLAGS) - 1, sizeof(uint32_t) + 4, EA_FLAGS, EAT_BINARY, sizeof(uint32_t), 0, "",
    },
    0,                                      FEA_NEEDEA,  sizeof(EA_SYMLINK) - 1, 0              + 4, EA_SYMLINK,EAT_ASCII, 0,                ""
#undef OFF
};
#pragma pack()



/**
 * Creates a symbolic link.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   pszTarget       The symlink target.
 * @param   pszNativePath   The path to the symbolic link to create.
 *                          This is not 'const' because the unix attribute init routine may have to
 *                          temporarily modify it to read the sticky bit from the parent directory.
 */
int __libc_back_fsNativeSymlinkCreate(const char *pszTarget, char *pszNativePath)
{
    LIBCLOG_ENTER("pszTarget=%s pszNativePath=%s\n", pszTarget, pszNativePath);

    /*
     * Validate input.
     */
    int cchTarget = strlen(pszTarget);
    if (__predict_false(cchTarget >= PATH_MAX))
        LIBCLOG_ERROR_RETURN(-ENAMETOOLONG, "ret -ENAMETOOLONG - Target is too long, %d bytes. (%s)\n", cchTarget, pszTarget);

    /*
     * This call isn't available in Non-Unix mode - we don't handle symlinks there.
     */
    if (__predict_false(__libc_gfNoUnix != 0))
        LIBCLOG_ERROR_RETURN(-ENOSYS, "ret -ENOSYS - __libc_gfNoUnix=%d\n", __libc_gfNoUnix);

    /*
     * Do we have UnixEAs on this path?
     */
    if (__predict_false(!__libc_back_fsInfoSupportUnixEAs(pszNativePath)))
        LIBCLOG_ERROR_RETURN(-ENOTSUP, "ret -ENOTSUP - no Unix EAs on '%s'\n", pszNativePath);

    /*
     * Allocate FEA buffer.
     */
    EAOP2   EaOp = {0,0,0};
    int     cb = cchTarget + 1 + sizeof(__libc_gFsUnixAttribsCreateSymlinkFEA2List);
    EaOp.fpFEA2List = alloca(cb);
    if (__predict_false(!EaOp.fpFEA2List))
        LIBCLOG_ERROR_RETURN(-ENOMEM, "ret -ENOMEM -Out of stack! alloca(%d) failed\n", cb);

    /*
     * Setup the EAOP2 structure.
     */
    struct __LIBC_FSUNIXATTRIBSCREATESYMLINKFEA2LIST *pFEas = (struct __LIBC_FSUNIXATTRIBSCREATESYMLINKFEA2LIST *)EaOp.fpFEA2List;
    *pFEas = __libc_gFsUnixAttribsCreateSymlinkFEA2List;
    __libc_back_fsUnixAttribsInit(&pFEas->Core, pszNativePath, S_IFLNK | S_IRWXO | S_IRWXG | S_IRWXU);
    pFEas->Core.cbList = cchTarget + sizeof(__libc_gFsUnixAttribsCreateSymlinkFEA2List) - 1;
    pFEas->cbSymlinkValue += cchTarget;
    pFEas->cbSymlinkData  = cchTarget;
    memcpy(pFEas->szSymlink, pszTarget, cchTarget);

    /*
     * Attempt creating the symlink file.
     */
    HFILE   hf = -1;
    ULONG   ul = 0;
    int rc = DosOpen((PCSZ)pszNativePath, &hf, &ul, cchTarget, FILE_NORMAL,
                     OPEN_ACTION_CREATE_IF_NEW | OPEN_ACTION_FAIL_IF_EXISTS,
                     OPEN_FLAGS_NOINHERIT | OPEN_SHARE_DENYNONE | OPEN_ACCESS_READWRITE,
                     &EaOp);
    if (__predict_true(rc == NO_ERROR))
    {
        DosWrite(hf, pszTarget, cchTarget, &ul);
        DosClose(hf);
        LIBCLOG_RETURN_INT(0);
    }

    struct stat s;
    if (rc == ERROR_EAS_NOT_SUPPORTED)
        rc = -EOPNOTSUPP;
    else if (   rc == ERROR_OPEN_FAILED
             && !__libc_back_fsNativeFileStat(pszNativePath, &s))
        rc = -EEXIST;
    else
        rc = -__libc_native2errno(rc);
    LIBCLOG_ERROR_RETURN_INT(rc);
}

