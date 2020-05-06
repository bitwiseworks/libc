/* $Id: b_fs.h 3840 2014-03-16 19:45:45Z bird $ */
/** @file
 *
 * LIBC SYS Backend - file system stuff.
 *
 * Copyright (c) 2004 knut st. osmundsen <bird@innotek.de>
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

#ifndef __fs_h__
#define __fs_h__

#include <sys/cdefs.h>
#include <sys/syslimits.h>
#include <sys/types.h>
#include <emx/io.h>
#include <InnoTekLIBC/sharedpm.h>
#include "backend.h"

struct stat;

__BEGIN_DECLS

/** Indicator whether or not we're in the unix tree. */
extern int  __libc_gfInUnixTree;
/** Length of the unix root if the unix root is official. */
extern int  __libc_gcchUnixRoot;
/** The current unix root. */
extern char __libc_gszUnixRoot[PATH_MAX];
/** The current umask of the process. */
extern mode_t __libc_gfsUMask;


/** @name   Unix Attribute EA Names
 * @{ */
/** Symlink EA name. */
#define EA_SYMLINK          "SYMLINK"
/** File EA owner. */
#define EA_UID              "UID"
/** File EA group. */
#define EA_GID              "GID"
/** File EA mode. */
#define EA_MODE             "MODE"
/** File EA i-node number. */
#define EA_INO              "INO"
/** File EA rdev number. */
#define EA_RDEV             "RDEV"
/** File EA gen number. */
#define EA_GEN              "GEN"
/** File EA user flags. */
#define EA_FLAGS            "FLAGS"
/** @} */

/** The minimum EA size of a file for it to possibly contain any LIBC Unix EAs. */
#define LIBC_UNIX_EA_MIN    (1 + 1 + 2 + sizeof("???") + 2 + 2 + sizeof(uint32_t))

#ifdef _OS2EMX_H

/**
 * The prefilled FEA2LIST construct for creating all unix attributes except symlink.
 */
#pragma pack(1)
extern const struct __LIBC_FSUNIXATTRIBSCREATEFEA2LIST
{
    ULONG   cbList;

    ULONG   offUID;
    BYTE    fUIDEA;
    BYTE    cbUIDName;
    USHORT  cbUIDValue;
    CHAR    szUIDName[sizeof(EA_UID)];
    USHORT  usUIDType;
    USHORT  cbUIDData;
    uint32_t u32UID;
    CHAR    achUIDAlign[((sizeof(EA_UID) + 4) & ~3) - sizeof(EA_UID)];

    ULONG   offGID;
    BYTE    fGIDEA;
    BYTE    cbGIDName;
    USHORT  usGIDValue;
    CHAR    szGIDName[sizeof(EA_GID)];
    USHORT  usGIDType;
    USHORT  cbGIDData;
    uint32_t u32GID;
    CHAR    achGIDAlign[((sizeof(EA_GID) + 4) & ~3) - sizeof(EA_GID)];

    ULONG   offMode;
    BYTE    fModeEA;
    BYTE    cbModeName;
    USHORT  usModeValue;
    CHAR    szModeName[sizeof(EA_MODE)];
    USHORT  usModeType;
    USHORT  cbModeData;
    uint32_t u32Mode;
    CHAR    achModeAlign[((sizeof(EA_MODE) + 4) & ~3) - sizeof(EA_MODE)];

    ULONG   offINO;
    BYTE    fINOEA;
    BYTE    cbINOName;
    USHORT  usINOValue;
    CHAR    szINOName[sizeof(EA_INO)];
    USHORT  usINOType;
    USHORT  cbINOData;
    uint64_t u64INO;
    CHAR    achINOAlign[((sizeof(EA_INO) + 4) & ~3) - sizeof(EA_INO)];

    ULONG   offRDev;
    BYTE    fRDevEA;
    BYTE    cbRDevName;
    USHORT  usRDevValue;
    CHAR    szRDevName[sizeof(EA_RDEV)];
    USHORT  usRDevType;
    USHORT  cbRDevData;
    uint32_t u32RDev;
    CHAR    achGDAlign[((sizeof(EA_RDEV) + 4) & ~3) - sizeof(EA_RDEV)];

    ULONG   offGen;
    BYTE    fGenEA;
    BYTE    cbGenName;
    USHORT  usGenValue;
    CHAR    szGenName[sizeof(EA_GEN)];
    USHORT  usGenType;
    USHORT  cbGenData;
    uint32_t u32Gen;
    CHAR    achGenAlign[((sizeof(EA_GEN) + 4) & ~3) - sizeof(EA_GEN)];

    ULONG   offFlags;
    BYTE    fFlagsEA;
    BYTE    cbFlagsName;
    USHORT  usFlagsValue;
    CHAR    szFlagsName[sizeof(EA_FLAGS)];
    USHORT  usFlagsType;
    USHORT  cbFlagsData;
    uint32_t u32Flags;
    CHAR    achFlagsAlign[((sizeof(EA_FLAGS) + 4) & ~3) - sizeof(EA_FLAGS)];

} __libc_gFsUnixAttribsCreateFEA2List;
#pragma pack()


/**
 * The prefilled GEA2LIST construct for querying all unix attributes.
 */
#pragma pack(1)
extern const struct __LIBC_BACK_FSUNIXATTRIBSGEA2LIST
{
    ULONG   cbList;

    ULONG   offSymlink;
    BYTE    cbSymlinkName;
    CHAR    szSymlinkName[((sizeof(EA_SYMLINK) + 4) & ~3) - 1];

    ULONG   offUID;
    BYTE    cbUIDName;
    CHAR    szUIDName[((sizeof(EA_UID) + 4) & ~3) - 1];

    ULONG   offGID;
    BYTE    cbGIDName;
    CHAR    szGIDName[((sizeof(EA_GID) + 4) & ~3) - 1];

    ULONG   offMode;
    BYTE    cbModeName;
    CHAR    szModeName[((sizeof(EA_MODE) + 4) & ~3) - 1];

    ULONG   offINO;
    BYTE    cbINOName;
    CHAR    szINOName[((sizeof(EA_INO) + 4) & ~3) - 1];

    ULONG   offRDev;
    BYTE    cbRDevName;
    CHAR    szRDevName[((sizeof(EA_RDEV) + 4) & ~3) - 1];

    ULONG   offGen;
    BYTE    cbGenName;
    CHAR    szGenName[((sizeof(EA_GEN) + 4) & ~3) - 1];

    ULONG   offFlags;
    BYTE    cbFlagsName;
    CHAR    szFlagsName[((sizeof(EA_FLAGS) + 4) & ~3) - 1];
} __libc_gFsUnixAttribsGEA2List;
#pragma pack()


/**
 * The prefilled GEA2LIST construct for querying unix attributes for directory listing.
 */
#pragma pack(1)
extern const struct __LIBC_BACK_FSUNIXATTRIBSDIRGEA2LIST
{
    ULONG   cbList;

    ULONG   offMode;
    BYTE    cbModeName;
    CHAR    szModeName[((sizeof(EA_MODE) + 4) & ~3) - 1];

    ULONG   offINO;
    BYTE    cbINOName;
    CHAR    szINOName[((sizeof(EA_INO) + 4) & ~3) - 1];
} __libc_gFsUnixAttribsDirGEA2List;
#pragma pack()


/** Indicator whether or not Large File Support API is available. */
extern int  __libc_gfHaveLFS;

#endif /* _OS2EMX_H */

/**
 * Init the file system stuff.
 *
 * @returns 0 on success.
 * @returns -1 on failure.
 */
int __libc_back_fsInit(void);

/**
 * Pack inherit data.
 * @returns 0 on success.
 * @returns -1 on failure.
 * @param   ppFS    Where to store the pointer to the inherit data, part 1.
 * @param   pcbFS   Where to store the size of the inherit data, part 1.
 * @param   ppFS2   Where to store the pointer to the inherit data, part 2.
 * @param   pcbFS2  Where to store the size of the inherit data, part 2.
 */
int __libc_back_fsInheritPack(__LIBC_PSPMINHFS *ppFS, size_t *pcbFS, __LIBC_PSPMINHFS2 *ppFS2, size_t *pcbFS2);

/**
 * Request the owner ship of the FS mutex.
 *
 * @returns 0 on success.
 * @returns -1 and errno on failure.
 */
int __libc_back_fsMutexRequest(void);

/**
 * Releases the owner ship of the FS mutex obtained by __libc_back_fsMutexRelease().
 */
void __libc_back_fsMutexRelease(void);

/**
 * Updates the global unix root stuff.
 * Assumes caller have locked the fs stuff.
 *
 * @param   pszUnixRoot     The new unix root. Fully resolved and existing.
 */
void __libc_back_fsUpdateUnixRoot(const char *pszUnixRoot);

/**
 * Resolves and verifies the user path to a native path.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failiure.
 * @param   pszUserPath     The user path.
 * @parm    fFlags          Flags controlling the operation of the function.
 *                          See the BACKFS_FLAGS_* defines.
 * @param   pszNativePath   Where to store the native path. This buffer is at
 *                          least PATH_MAX bytes big.
 * @param   pfInUnixTree    Where to store the result-in-unix-tree indicator. Optional.
 */
int __libc_back_fsResolve(const char *pszUserPath, unsigned fFlags, char *pszNativePath, int *pfInUnixTree);

/** __libc_back_fsResolve() flags.
 * @{ */
/** Resolves the path up to but not including the last component. */
#define BACKFS_FLAGS_RESOLVE_PARENT             0x00
/** Resolves and verfies the entire path. */
#define BACKFS_FLAGS_RESOLVE_FULL               0x01
/** Resolves and verfies the entire path, but don't resolve any symlink in the last component. */
#define BACKFS_FLAGS_RESOLVE_FULL_SYMLINK       0x02
/** Internal, use BACKFS_FLAGS_RESOLVE_FULL_MAYBE. */
#define BACKFS_FLAGS_RESOLVE_FULL_MAYBE_        0x08
/** Resolves and verfies the entire path but it's ok if the last component doesn't exist. */
#define BACKFS_FLAGS_RESOLVE_FULL_MAYBE         (BACKFS_FLAGS_RESOLVE_FULL_MAYBE_ | BACKFS_FLAGS_RESOLVE_FULL)
/** Resolves and verfies the entire path but it's ok if the last component doesn't exist. */
#define BACKFS_FLAGS_RESOLVE_FULL_SYMLINK_MAYBE (BACKFS_FLAGS_RESOLVE_FULL_MAYBE_ | BACKFS_FLAGS_RESOLVE_FULL_SYMLINK)
/** The specified path is a directory. */
#define BACKFS_FLAGS_RESOLVE_DIR                0x10
/** Internal, use BACKFS_FLAGS_RESOLVE_DIR_MAYBE. */
#define BACKFS_FLAGS_RESOLVE_DIR_MAYBE_         0x80
/** The specified path maybe a directory. */
#define BACKFS_FLAGS_RESOLVE_DIR_MAYBE          (BACKFS_FLAGS_RESOLVE_DIR_MAYBE_ | BACKFS_FLAGS_RESOLVE_DIR)
/** @} */


#ifdef _OS2EMX_H
/**
 * Initializes a unix attribute structure before creating a new inode.
 * The call must have assigned default values to the the structure before doing this call!
 *
 * @returns Device number.
 * @param   pFEas           The attribute structure to fill with actual values.
 * @param   pszNativePath   The native path, used to calculate the inode number.
 * @param   Mode            The correct mode (the caller have fixed this!).
 */
dev_t __libc_back_fsUnixAttribsInit(struct __LIBC_FSUNIXATTRIBSCREATEFEA2LIST *pFEas, char *pszNativePath, mode_t Mode);
#endif /* _OS2EMX_H */

/**
 * Reads the unix EAs for a file which is being stat'ed.
 *
 * @returns 0 on success.
 * @returns Negative errno on failure.
 * @param   hFile           File handle to the fs object. If no handle handy, set to -1.
 * @param   pszNativePath   Native path to the fs object. If handle is give this will be ignored.
 * @param   pStat           Pointer to the stat buffer.
 *                          The buffer is only updated if and with the EAs we find,
 *                          so the caller must fill the fields with defaults before
 *                          calling this function.
 */
int __libc_back_fsUnixAttribsGet(int hFile, const char *pszNativePath, struct stat *pStat);

/**
 * Reads the unix file mode EA.
 *
 * @returns 0 on success.
 * @returns -ENOTSUP if the file mode EA is not present or if Unix EAs isn't supported on the volume.
 * @returns Negative errno on failure.
 * @param   hFile           File handle to the fs object. If no handle handy, set to -1.
 * @param   pszNativePath   Native path to the fs object. If handle is give this will be ignored.
 * @param   pMode           Where to store the mode mask.
 */
int __libc_back_fsUnixAttribsGetMode(int hFile, const char *pszNativePath, mode_t *pMode);

/**
 * Creates a symbolic link.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   pszTarget       The target of the symlink link.
 * @param   pszNativePath   The path to the symbolic link to create.
 *                          This is not 'const' because the unix attribute init routine may have to
 *                          temporarily modify it to read the sticky bit from the parent directory.
 */
int __libc_back_fsNativeSymlinkCreate(const char *pszTarget, char *pszNativePath);

/**
 * Reads the content of a symbolic link.
 *
 * This is weird interface as it will return a truncated result if not
 * enough buffer space. It is also weird in that there is no string
 * terminator.
 *
 * @returns Number of bytes returned in pachBuf.
 * @returns -1 and errno on failure.
 * @param   pszNativePath   The path to the symlink to read.
 * @param   pachBuf         Where to store the symlink value.
 * @param   cchBuf          Size of buffer.
 */
int __libc_back_fsNativeSymlinkRead(const char *pszNativePath, char *pachBuf, size_t cchBuf);

/**
 * Stats a native file.
 *
 * @returns 0 on success.
 * @returns -1 and errno on failure.
 * @param   pszNativePath   Path to the file to stat. This path is resolved, no
 *                          processing required.
 * @param   pStat           Where to store the file stats.
 */
int __libc_back_fsNativeFileStat(const char *pszNativePath, struct stat *pStat);

/**
 * Sets the file access mode of a native file.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   fh      Handle to file.
 * @param   Mode    The filemode.
 */
int __libc_back_fsNativeFileModeSet(const char *pszNativePath, mode_t Mode);

/**
 * Sets the file times of a native file.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   pszNativePath   Path to the file to set the times of.
 * @param   paTimes         Two timevalue structures. If NULL the current time is used.
 */
int __libc_back_fsNativeFileTimesSet(const char *pszNativePath, const struct timeval *paTimes);

int __libc_back_fsNativeFileOwnerSet(const char *pszNativePath, uid_t uid, gid_t gid);
int __libc_back_fsNativeFileOwnerSetCommon(intptr_t hNative, const char *pszNativePath, int fUnixEAs, uid_t uid, gid_t gid);
struct _FEA2LIST;
struct _EAOP2;
int __libc_back_fsNativeSetEAs(intptr_t hNative, const char *pszNativePath, struct _FEA2LIST *pEAs, struct _EAOP2 *pEaOp2);

/**
 * Calc the Inode and Dev based on native path.
 *
 * @returns device number and *pInode.
 *
 * @param   pszNativePath       Pointer to native path.
 * @param   pInode              Where to store the inode number.
 * @remark  This doesn't work right when in non-unix mode!
 */
dev_t __libc_back_fsPathCalcInodeAndDev(const char *pszNativePath, ino_t *pInode);

/**
 * Gets the fs info object for the specfied path.
 *
 * @returns Pointer to info object for the path, if it got one.
 * @param   Dev     The device we want the file system object for.
 */
__LIBC_PFSINFO __libc_back_fsInfoObjByDev(dev_t Dev);

/**
 * Gets the fs info object for the specfied path.
 *
 * @returns Pointer to info object for the path, if it got one.
 * @param   pszNativePath   The native path as returned by the resolver.
 */
__LIBC_PFSINFO __libc_back_fsInfoObjByPath(const char *pszNativePath);

/**
 * Gets the fs info object for the specfied path, with cache.
 *
 * @returns Pointer to info object for the path, if it got one.
 * @param   pszNativePath   The native path as returned by the resolver.
 * @param   pCached         An cached fs info object reference. Can be NULL.
 */
__LIBC_PFSINFO __libc_back_fsInfoObjByPathCached(const char *pszNativePath, __LIBC_PFSINFO pCached);

/**
 * Adds a reference to an existing FS info object.
 *
 * The caller is responsible for making sure that the object cannot
 * reach 0 references while inside this function.
 *
 * @returns pFsInfo.
 * @param   pFsInfo     Pointer to the fs info object to reference.
 */
__LIBC_PFSINFO __libc_back_fsInfoObjAddRef(__LIBC_PFSINFO pFsInfo);

/**
 * Releases the fs info object for the specfied path.
 *
 * @param   pFsInfo     Pointer to the fs info object to release a reference to.
 */
void __libc_back_fsInfoObjRelease(__LIBC_PFSINFO pFsInfo);

/**
 * Checks if the path supports Unix EAs or not.
 *
 * @returns true / false.
 * @param   pszNativePath       The native path to check.
 */
int __libc_back_fsInfoSupportUnixEAs(const char *pszNativePath);

int __libc_back_fsInfoPathConf(__LIBC_PFSINFO pFsInfo, int iName, long *plValue);

/**
 * Gets the current directory of the process on a
 * specific drive or on the current one.
 *
 * @note Differs from __libc_Back_fsDirCurrentGet in that it doesn't lock the
 * FS mutex assuming that the caller has locked it.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   pszPath     Where to store the path to the current directory.
 *                      This will be prefixed with a drive letter if we're
 *                      not in the unix tree.
 * @param   cchPath     The size of the path buffer.
 * @param   chDrive     The drive letter of the drive to get it for.
 *                      If '\0' the current dir for the current drive is returned.
 * @param   fFlags      Flags for skipping drive letter and slash.
 */
int __libc_back_fsDirCurrentGet(char *pszPath, size_t cchPath, char chDrive, int fFlags);

__END_DECLS

#endif
