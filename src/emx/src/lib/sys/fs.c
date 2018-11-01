/* $Id: fs.c 3917 2014-10-24 20:02:16Z bird $ */
/** @file
 *
 * LIBC SYS Backend - file system stuff.
 *
 * Copyright (c) 2004-2014 knut st. osmundsen <bird@innotek.de>
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
#define _GNU_SOURCE
#include "libc-alias.h"
#define INCL_FSMACROS
#define INCL_BASE
#define INCL_ERRORS
#include <os2emx.h>
#include "b_fs.h"
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <386/builtin.h>
#include <sys/stat.h>
#include <sys/fmutex.h>
#include <sys/smutex.h>
#include <emx/startup.h>
#include "syscalls.h"
#include <InnoTekLIBC/sharedpm.h>
#include <InnoTekLIBC/pathrewrite.h>
#include <InnoTekLIBC/libc.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_BACK_FS
#include <InnoTekLIBC/logstrict.h>


/*******************************************************************************
*   Defined Constants And Macros                                               *
*******************************************************************************/
#define ORD_DOS32OPENL              981
#define ORD_DOS32SETFILEPTRL        988
#define ORD_DOS32SETFILESIZEL       989
#define ORD_DOS32SETFILELOCKSL      986

#define SIZEOF_ACHBUFFER    (sizeof(FILEFINDBUF4) + 32)


/*******************************************************************************
*   Global Variables                                                           *
*******************************************************************************/
/** Mutex protecting all the FS globals.
 * __libc_gfsUMask and the API pointers are not protected by this semaphore. */
static _fmutex __libc_gmtxFS;

/** Indicator whether or not we're in the unix tree. */
int     __libc_gfInUnixTree = 0;
/** Length of the unix root if the unix root is official. */
int     __libc_gcchUnixRoot = 0;
/** The current unix root. */
char    __libc_gszUnixRoot[PATH_MAX] = "";
/** The umask of the process. */
mode_t  __libc_gfsUMask = 022;


ULONG (* _System __libc_gpfnDosOpenL)(PCSZ pszFileName, PHFILE phFile, PULONG pulAction, LONGLONG llFileSize, ULONG ulAttribute, ULONG ulOpenFlags, ULONG ulOpenMode, PEAOP2 pEABuf) = NULL;
ULONG (* _System __libc_gpfnDosSetFilePtrL)(HFILE hFile, LONGLONG llOffset, ULONG ulOrigin, PLONGLONG pllPos) = NULL;
ULONG (* _System __libc_gpfnDosSetFileSizeL)(HFILE hFile, LONGLONG cbSize) = NULL;
ULONG (* _System __libc_gpfnDosSetFileLocksL)(HFILE hFile, __const__ FILELOCKL *pflUnlock, __const__ FILELOCKL *pflLock, ULONG ulTimeout, ULONG flFlags) = NULL;

/** Symlink EA name. */
static const char __libc_gszSymlinkEA[] = EA_SYMLINK;
/** UID EA name. */
static const char __libc_gszUidEA[]     = EA_UID;
/** GID EA name. */
static const char __libc_gszGidEA[]     = EA_GID;
/** Mode EA name. */
static const char __libc_gszModeEA[]    = EA_MODE;
/** Ino EA name. */
static const char __libc_gszInoEA[]     = EA_INO;
/** RDev EA name. */
static const char __libc_gszRDevEA[]    = EA_RDEV;
/** Gen(eration) EA name. */
static const char __libc_gszGenEA[]     = EA_GEN;
/** User flags EA name. */
static const char __libc_gszFlagsEA[]   = EA_FLAGS;

/**
 * The prefilled GEA2LIST construct for querying a symlink.
 */
#pragma pack(1)
static const struct
{
    ULONG   cbList;
    ULONG   oNextEntryOffset;
    BYTE    cbName;
    CHAR    szName[sizeof(EA_SYMLINK)];
} gGEA2ListSymlink =
{
    sizeof(gGEA2ListSymlink),
    0,
    sizeof(EA_SYMLINK) - 1,
    EA_SYMLINK
};
#pragma pack()

/**
 * The prefilled GEA2LIST construct for querying all unix attributes.
 */
const struct __LIBC_BACK_FSUNIXATTRIBSGEA2LIST __libc_gFsUnixAttribsGEA2List =
{
#define OFF(a,b)  offsetof(struct __LIBC_BACK_FSUNIXATTRIBSGEA2LIST, a) - offsetof(struct __LIBC_BACK_FSUNIXATTRIBSGEA2LIST, b)
    sizeof(__libc_gFsUnixAttribsGEA2List),
    OFF(offUID,   offSymlink), sizeof(EA_SYMLINK) - 1, EA_SYMLINK,
    OFF(offGID,   offUID),     sizeof(EA_UID) - 1,     EA_UID,
    OFF(offMode,  offGID),     sizeof(EA_GID) - 1,     EA_GID,
    OFF(offINO,   offMode),    sizeof(EA_MODE) - 1,    EA_MODE,
    OFF(offRDev,  offINO),     sizeof(EA_INO) - 1,     EA_INO,
    OFF(offGen,   offRDev),    sizeof(EA_RDEV) - 1,    EA_RDEV,
    OFF(offFlags, offGen),     sizeof(EA_GEN) - 1,     EA_GEN,
    0,                         sizeof(EA_FLAGS) - 1,   EA_FLAGS
#undef OFF
};
#pragma pack()

/**
 * The prefilled GEA2LIST construct for querying all the mode attribute.
 */
#pragma pack(1)
static const struct
{
    ULONG   cbList;
    ULONG   oNextEntryOffset;
    BYTE    cbName;
    CHAR    szName[sizeof(EA_MODE)];
} gGEA2ListMode =
{
    sizeof(gGEA2ListMode),
    0,
    sizeof(EA_MODE) - 1,
    EA_MODE
};
#pragma pack()

/**
 * The prefilled FEA2LIST construct for setting all attributes during a creation operation.
 */
#pragma pack(1)
const struct __LIBC_FSUNIXATTRIBSCREATEFEA2LIST __libc_gFsUnixAttribsCreateFEA2List =
{
#define OFF(a,b)  offsetof(struct __LIBC_FSUNIXATTRIBSCREATEFEA2LIST, a)  - offsetof(struct __LIBC_FSUNIXATTRIBSCREATEFEA2LIST, b)
    sizeof(__libc_gFsUnixAttribsCreateFEA2List),
    OFF(offGID,    offUID),   0,  sizeof(EA_UID) - 1,   sizeof(uint32_t) + 4, EA_UID,   EAT_BINARY, sizeof(uint32_t), 0, "",
    OFF(offMode,   offGID),   0,  sizeof(EA_GID) - 1,   sizeof(uint32_t) + 4, EA_GID,   EAT_BINARY, sizeof(uint32_t), 0, "",
    OFF(offINO,    offMode),  0,  sizeof(EA_MODE) - 1,  sizeof(uint32_t) + 4, EA_MODE,  EAT_BINARY, sizeof(uint32_t), 0, "",
    OFF(offRDev,   offINO),   0,  sizeof(EA_INO) - 1,   sizeof(uint64_t) + 4, EA_INO,   EAT_BINARY, sizeof(uint64_t), 0, "",
    OFF(offGen,    offRDev),  0,  sizeof(EA_RDEV) - 1,  sizeof(uint32_t) + 4, EA_RDEV,  EAT_BINARY, sizeof(uint32_t), 0, "",
    OFF(offFlags,  offGen),   0,  sizeof(EA_GEN) - 1,   sizeof(uint32_t) + 4, EA_GEN,   EAT_BINARY, sizeof(uint32_t), 0, "",
    0,                        0,  sizeof(EA_FLAGS) - 1, sizeof(uint32_t) + 4, EA_FLAGS, EAT_BINARY, sizeof(uint32_t), 0, ""
#undef OFF
};
#pragma pack()

/**
 * The prefilled GEA2LIST construct for querying unix attributes for directory listing.
 */
const struct __LIBC_BACK_FSUNIXATTRIBSDIRGEA2LIST __libc_gFsUnixAttribsDirGEA2List =
{
#define OFF(a,b)  offsetof(struct __LIBC_BACK_FSUNIXATTRIBSDIRGEA2LIST, a) - offsetof(struct __LIBC_BACK_FSUNIXATTRIBSDIRGEA2LIST, b)
    sizeof(__libc_gFsUnixAttribsDirGEA2List),
    OFF(offINO,   offMode),    sizeof(EA_MODE) - 1,    EA_MODE,
    0,                         sizeof(EA_INO) - 1,     EA_INO,
#undef OFF
};
#pragma pack()

/**
 * The special /@unixroot rewrite rule.
 */
static __LIBC_PATHREWRITE   gUnixRootRewriteRule =
{
    __LIBC_PRWF_CASE_SENSITIVE | __LIBC_PRWF_TYPE_DIR,  "/@unixroot",  10, __libc_gszUnixRoot, 0
};

/** The full path to the executable directory. */
char __libc_gszExecPath[CCHMAXPATH];
/** The system drive. */
char __libc_gszSystemDrive[4] = "C:";
/** The full path to the host system root directory. */
char __libc_gszSystemRoot[8] = "C:/OS2";
/** The path to the default tmp directory (might be a winding path). */
char __libc_gszTmpDir[CCHMAXPATH] = "C:/TEMP";

/**
 * The misc rewrite rules.
 */
static __LIBC_PATHREWRITE   gaMiscRewriteRules[4] =
{
    { __LIBC_PRWF_CASE_SENSITIVE | __LIBC_PRWF_TYPE_DIR,  "/@executable_path",  17, __libc_gszExecPath, 0 },
    { __LIBC_PRWF_CASE_SENSITIVE | __LIBC_PRWF_TYPE_DIR,  "/@system_drive",     14, __libc_gszSystemDrive, 2 },
    { __LIBC_PRWF_CASE_SENSITIVE | __LIBC_PRWF_TYPE_DIR,  "/@system_root",      13, __libc_gszSystemRoot, 6 },
    { __LIBC_PRWF_CASE_SENSITIVE | __LIBC_PRWF_TYPE_DIR,  "/@tmpdir",            8, __libc_gszTmpDir, 7 }
};

/** Array of pointers to fs info objects for all
 * possible OS/2 volumes.
 */
static __LIBC_FSINFO  g_aFSInfoVolumes['Z' - 'A' + 1];
/** Mutex semaphore protecting the g_aFSInfoVolumes array. */
static _fmutex        g_mtxFSInfoVolumes = _FMUTEX_INITIALIZER_EX(_FMC_MUST_COMPLETE, "mtxFSInfoVolumes");

/** Array of Unix EA settings overrides for all possible OS/2 volumes.
 * Tristate: -1 force off, 0 default, 1 force on.
 */
static char           g_achUnixEAsOverrides['Z' - 'A' + 1];


/*******************************************************************************
*   Internal Functions                                                         *
*******************************************************************************/
static int fsResolveUnix(const char *pszUserPath, unsigned fFlags, char *pszNativePath, int *pfInUnixTree);
static int fsResolveOS2(const char *pszUserPath, unsigned fFlags, char *pszNativePath, int *pfInUnixTree);
static uint32_t crc32str(const char *psz);
#if 0
static uint32_t djb2(const char *str);
#endif
static uint32_t sdbm(const char *str);


#ifndef STANDALONE_TEST
/**
 * Init the file system stuff.
 *
 * @returns 0 on success.
 * @returns -1 on failure.
 */
int __libc_back_fsInit(void)
{
    LIBCLOG_ENTER("\n");

    /*
     * Only once.
     */
    static int  fInited;
    if (fInited)
        LIBCLOG_RETURN_INT(0);
    fInited = 1;

    /*
     * Create the mutex.
     */
    int rc = _fmutex_create2(&__libc_gmtxFS, 0, "LIBC SYS FS Mutex");
    if (rc)
        LIBCLOG_RETURN_INT(-1);

    /*
     * Inherit File System Data from parent.
     */
    __LIBC_PSPMINHERIT  pInherit = __libc_spmInheritRequest();

    /* part 2, the umask. */
    __LIBC_PSPMINHFS2   pFS2;
    if (    pInherit
        &&  pInherit->cb > offsetof(__LIBC_SPMINHERIT, pFS2)
        &&  (pFS2 = pInherit->pFS2) != NULL
        &&  !(pFS2->fUMask & ~0777))
    {
        LIBCLOG_MSG("Inherited fUMask=%04o\n", pFS2->fUMask);
        __libc_gfsUMask = pFS2->fUMask & 0777;
    }

    /* part 1, the unixroot. */
    __LIBC_PSPMINHFS    pFS;
    if (    pInherit
        &&  pInherit->cb > offsetof(__LIBC_SPMINHERIT, pFS)
        &&  (pFS = pInherit->pFS) != NULL
        &&  pFS->cchUnixRoot)
    {
        LIBCLOG_MSG("Inherited unixroot=%s fInUnixTree=%d cchUnixRoot=%d\n", pFS->szUnixRoot, pFS->fInUnixTree, pFS->cchUnixRoot);
        __libc_gfNoUnix     = 0;
        __libc_gfInUnixTree = pFS->fInUnixTree;
        __libc_gcchUnixRoot = pFS->cchUnixRoot;
        memcpy(&__libc_gszUnixRoot[0], &pFS->szUnixRoot[0], pFS->cchUnixRoot + 1);

        __libc_spmInheritRelease();

        /* Register rewrite rule. */
        gUnixRootRewriteRule.pszTo = "/";
        gUnixRootRewriteRule.cchTo = 1;
        if (__libc_PathRewriteAdd(&gUnixRootRewriteRule, 1))
            LIBCLOG_RETURN_INT(-1);
    }
    else
    {
        /*
         * Nothing to inherit.
         */
        if (pInherit)
            __libc_spmInheritRelease();
        if (!__libc_gfNoUnix)
        {
            /*
             * Setup unofficial unixroot.
             */
            const char *psz = getenv("UNIXROOT");
            if (   psz
                && ((psz[0] >= 'A' && psz[0] <= 'Z') || (psz[0] >= 'a' && psz[0] <= 'z'))
                && psz[1] == ':'
                && (psz[2] == '\\' || psz[2] == '/' || psz[2] == '\0')
                )
            {
                LIBCLOG_MSG("Unofficial unixroot=%s\n", psz);
                size_t cch = strlen(psz);
                if (cch >= PATH_MAX - 32)
                {
                    LIBC_ASSERTM_FAILED("The UNIXROOT environment variable is too long! cch=%d maxlength=%d\n", cch, PATH_MAX - 32);
                    LIBCLOG_RETURN_INT(-1);
                }
                memcpy(&__libc_gszUnixRoot[0], psz, cch + 1);

                /* Clean it up a tiny little bit. */
                if (__libc_gszUnixRoot[0] >= 'a')
                    __libc_gszUnixRoot[0] -= 'a' - 'A';

                if (cch == 2)
                {
                    __libc_gszUnixRoot[cch++] = '/';
                    __libc_gszUnixRoot[cch] = '\0';
                }
                else
                {
                    size_t off = cch;
                    while (off-- > 0)
                        if (__libc_gszUnixRoot[off] == '\\')
                            __libc_gszUnixRoot[off] = '/';

                    while (cch > 3 && __libc_gszUnixRoot[cch - 1] == '/')
                        __libc_gszUnixRoot[--cch] = '\0';
                }

                /* Register the rewrite rule. */
                gUnixRootRewriteRule.cchTo = cch;
                if (__libc_PathRewriteAdd(&gUnixRootRewriteRule, 1))
                    LIBCLOG_RETURN_INT(-1);

                /* Should we pretend chroot($UNIXROOT) + chdir()? */
                psz = getenv("UNIXROOT_CHROOTED");
                if (psz && *psz != '\0')
                {
                    __libc_gcchUnixRoot = cch;

                    if (!getenv("UNIXROOT_OUTSIDE"))
                    {
                        ULONG ulDisk, ulIgnored;
                        rc = DosQueryCurrentDisk(&ulDisk, &ulIgnored);
                        if (   rc == NO_ERROR
                            && ulDisk + 'A' - 1 == __libc_gszUnixRoot[0])
                        {
                            if (cch == 3)
                                __libc_gfInUnixTree = 1;
                            else
                            {
                                char szCurDir[PATH_MAX];
                                ULONG cbCurDir = sizeof(szCurDir);
                                rc = DosQueryCurrentDir(0, (PSZ)&szCurDir[0], &cbCurDir);
                                __libc_gfInUnixTree = rc == NO_ERROR
                                                   && cch <= cbCurDir + 3
                                                   && memicmp(&__libc_gszUnixRoot[3], szCurDir, cch - 3) == 0
                                                   && (   cch == cbCurDir + 3
                                                       || szCurDir[cch - 3] == '\\');
                            }

                        }
                    }
                    LIBCLOG_MSG(__libc_gfInUnixTree ? "Inside unixroot chroot.\n" : "Outside unixroot chroot.\n");
                }
            }
            else if (psz)
                LIBCLOG_MSG("Invalid UNIXROOT=\"%s\" - must start with drive letter and root slash.\n", psz);
        }
    }

    /*
     * Setup the the executable path rewrite rule.
     */
    PTIB pTib;
    PPIB pPib;
    DosGetInfoBlocks(&pTib, &pPib);
    rc = DosQueryModuleName(pPib->pib_hmte, sizeof(__libc_gszExecPath), &__libc_gszExecPath[0]);
    if (!rc)
    {
        char *psz = strchr(&__libc_gszExecPath[0], '\0');
        while (     psz > &__libc_gszExecPath[0]
               &&   *psz != '\\'
               &&   *psz != '/'
               &&   *psz != ':')
            psz--;
        *psz = '\0';
        gaMiscRewriteRules[0].cchTo = psz - &__libc_gszExecPath[0];
    }
    else
    {
        LIBC_ASSERTM_FAILED("DosQueryModuleName(exe) failed: hmte=%lx rc=%d\n", pPib->pib_hmte, rc);
        gaMiscRewriteRules[0].cchTo = 1;
        gaMiscRewriteRules[0].pszTo = "/";
    }

    /*
     * Setup the the (host) system root path and system root drive rewrite rules.
     */
    ULONG ulBootDrive;
    if (DosQuerySysInfo(QSV_BOOT_DRIVE, QSV_BOOT_DRIVE, &ulBootDrive, sizeof(ulBootDrive)))
        ulBootDrive = 'C' - 'A' - 1; /* A = 1 */
    __libc_gszTmpDir[0] = __libc_gszSystemRoot[0] = __libc_gszSystemDrive[0]
        = (char)ulBootDrive + 'A' - 1;

    /*
     * Setup the the temp path rewrite rule.
     */
    size_t cchTmp = 0;
    char *pszTmp = getenv("TMP");
    if (pszTmp)
        cchTmp = strlen(pszTmp);
    if (!pszTmp || cchTmp >= sizeof(__libc_gszTmpDir) - 1)
    {
        pszTmp = getenv("TEMP");
        if (pszTmp)
            cchTmp = strlen(pszTmp);
        if (!pszTmp || cchTmp >= sizeof(__libc_gszTmpDir) - 1)
        {
            pszTmp = getenv("TMPDIR");
            if (pszTmp)
                cchTmp = strlen(pszTmp);
        }
    }
    if (pszTmp && cchTmp <= sizeof(__libc_gszTmpDir) - 1)
    {
        /** @todo cleanup the thing so it won't get rejected! */
        memcpy(__libc_gszTmpDir, pszTmp, cchTmp + 1);
        gaMiscRewriteRules[3].cchTo = cchTmp;
    }

    /*
     * Finally register the rules.
     */
/** @todo Make static rules from these and only update the replacement value and length. */
    if (__libc_PathRewriteAdd(&gaMiscRewriteRules[0], 4))
    {
        /* tmp rewrite rule is busted by the env.var., use the default. */
        memcpy(__libc_gszTmpDir, "C:/TEMP", sizeof("C:/TEMP"));
        gaMiscRewriteRules[3].cchTo = sizeof("C:/TEMP") - 1;
        if (__libc_PathRewriteAdd(&gaMiscRewriteRules[0], 4))
            LIBCLOG_RETURN_INT(-1);
    }


    /*
     * Try resolve large file APIs.
     *
     * First, get a handle to DOSCALLS/DOSCALL1 which  is suitable
     * for DosQueryProcAddr.
     * Second, query the procedure addresses.
     * Third, free DOSCALLS since there's no point messing up any
     * reference counters for that module.
     */
    HMODULE     hmod = NULLHANDLE;
    if (DosLoadModule(NULL, 0, (PCSZ)"DOSCALLS", &hmod))
    {
        LIBC_ASSERTM_FAILED("DosLoadModule failed on doscalls!\n");
        abort();
        LIBCLOG_RETURN_INT(-1);
    }
    if (    DosQueryProcAddr(hmod, ORD_DOS32OPENL,          NULL, (PFN*)(void *)&__libc_gpfnDosOpenL)
        ||  DosQueryProcAddr(hmod, ORD_DOS32SETFILEPTRL,    NULL, (PFN*)(void *)&__libc_gpfnDosSetFilePtrL)
        ||  DosQueryProcAddr(hmod, ORD_DOS32SETFILESIZEL,   NULL, (PFN*)(void *)&__libc_gpfnDosSetFileSizeL)
        ||  DosQueryProcAddr(hmod, ORD_DOS32SETFILELOCKSL,  NULL, (PFN*)(void *)&__libc_gpfnDosSetFileLocksL)
          )
    {
        __libc_gpfnDosOpenL         = NULL;
        __libc_gpfnDosSetFilePtrL   = NULL;
        __libc_gpfnDosSetFileSizeL  = NULL;
        __libc_gpfnDosSetFileLocksL = NULL;
    }
    DosFreeModule(hmod);

    /*
     * Look for the UNIX EAs control environment variable.
     * The value form: !a, c, !d-e
     */
    const char *pszUnixEAs = getenv("LIBC_UNIX_EAS");
    if (pszUnixEAs)
    {
        while (*pszUnixEAs)
        {
            char chDrv;
            while ((chDrv = *pszUnixEAs) == ',' || chDrv == ';' || chDrv == ' ' || chDrv == '\t')
                pszUnixEAs++;
            if (!chDrv)
                break;

            /* check for the operator. */
            int fOverride = 1;
            if (chDrv == '!')
            {
                fOverride = -1;
                chDrv = *++pszUnixEAs;
            }

            /* check for the first drive letter, upper case it. */
            if (chDrv >= 'a' && chDrv <= 'z')
                chDrv -= 'a' - 'A';
            if (chDrv < 'A' || chDrv > 'Z')
                LIBCLOG_ERROR_RETURN_MSG(-1, "Bad LIBC_UNIX_EAS value; ch=%c\n", chDrv);
            pszUnixEAs++;
            if (*pszUnixEAs == ':')
                pszUnixEAs++;

            /* check if it is a range spec. */
            char chDrv2;
            while ((chDrv2 = *pszUnixEAs) == ' ' || chDrv2 == '\t')
                pszUnixEAs++;
            if (chDrv2 == '-')
            {
                pszUnixEAs++;
                while ((chDrv2 = *pszUnixEAs) == ' ' || chDrv2 == '\t')
                    pszUnixEAs++;

                if (chDrv >= 'a' && chDrv <= 'z')
                    chDrv -= 'a' - 'A';
                if (chDrv < 'A' || chDrv > 'Z')
                    LIBCLOG_ERROR_RETURN_MSG(-1, "Bad LIBC_UNIX_EAS value; ch=%c\n", chDrv2);
                pszUnixEAs++;
                if (*pszUnixEAs == ':')
                    pszUnixEAs++;
            }
            else
                chDrv2 = chDrv;

            /* Be nice and swap the values if they are not in ascending order. */
            if (chDrv2 < chDrv)
            {
                char chDrvTmp = chDrv2;
                chDrv2 = chDrv;
                chDrv = chDrvTmp;
            }

            /* apply them. */
            do
                g_achUnixEAsOverrides[chDrv - 'A'] = fOverride;
            while (chDrv++ < chDrv2);
        }
    }

    LIBCLOG_RETURN_INT(0);
}

/**
 * Pack inherit data.
 * @returns 0 on success.
 * @returns -1 on failure.
 * @param   ppFS    Where to store the pointer to the inherit data, part 1.
 * @param   pcbFS   Where to store the size of the inherit data, part 1.
 * @param   ppFS2   Where to store the pointer to the inherit data, part 2.
 * @param   pcbFS2  Where to store the size of the inherit data, part 2.
 */
int __libc_back_fsInheritPack(__LIBC_PSPMINHFS *ppFS, size_t *pcbFS, __LIBC_PSPMINHFS2 *ppFS2, size_t *pcbFS2)
{
    LIBCLOG_ENTER("ppFS=%p pcbFS=%p ppFS2=%p pcbFS2=%p\n", (void *)ppFS, (void *)pcbFS, (void *)ppFS2, (void *)pcbFS2);

    *ppFS = NULL;
    *pcbFS = 0;
    *ppFS2 = NULL;
    *pcbFS2 = 0;

    if (__libc_back_fsMutexRequest())
        return -1;

    int rc = 0;
    if (__libc_gcchUnixRoot)
    {
        size_t cb = sizeof(__LIBC_SPMINHFS) + __libc_gcchUnixRoot;
        __LIBC_PSPMINHFS pFS = (__LIBC_PSPMINHFS)malloc(cb);
        if (pFS)
        {
            pFS->fInUnixTree = __libc_gfInUnixTree;
            pFS->cchUnixRoot = __libc_gcchUnixRoot;
            memcpy(pFS->szUnixRoot, __libc_gszUnixRoot, pFS->cchUnixRoot + 1);
            LIBCLOG_MSG("unixroot=%s fInUnixTree=%d cchUnixRoot=%d\n", pFS->szUnixRoot, pFS->fInUnixTree, pFS->cchUnixRoot);

            *pcbFS = cb;
            *ppFS = pFS;
        }
        else
            rc = -1;
    }

    if (!rc)
    {
        __LIBC_PSPMINHFS2 pFS2 = (__LIBC_PSPMINHFS2)malloc(sizeof(*pFS2));
        if (pFS2)
        {
            pFS2->cb = sizeof(*pFS2);
            pFS2->fUMask = __libc_gfsUMask;
            LIBCLOG_MSG("fUMask=%04o\n", pFS2->fUMask);

            *pcbFS2 = sizeof(*pFS2);
            *ppFS2 = pFS2;
        }
        else
            rc = -1;
    }

    __libc_back_fsMutexRelease();
    LIBCLOG_RETURN_INT(rc);
}


int __libc_back_fsMutexRequest(void)
{
    int rc = _fmutex_request(&__libc_gmtxFS, 0);
    if (!rc)
        return 0;
    return -__libc_native2errno(rc);
}


void __libc_back_fsMutexRelease(void)
{
    int rc = _fmutex_release(&__libc_gmtxFS);
    if (!rc)
        return;
    LIBC_ASSERTM_FAILED("_fmutex_release(gmtxFS) -> rc=%d\n", rc);
}

#endif /*!STANDALONE_TEST*/

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
int __libc_back_fsNativeSymlinkRead(const char *pszNativePath, char *pachBuf, size_t cchBuf)
{
    LIBCLOG_ENTER("pszNativePath=%p:{%s} pachBuf=%p cchBuf=%d\n", (void *)pszNativePath, pszNativePath, (void *)pachBuf, cchBuf);

    /*
     * Query the symlink EA value.
     */
    char    _achBuffer[SIZEOF_ACHBUFFER + 4];
    char   *pachBuffer = (char *)((uintptr_t)&_achBuffer[3] & ~3);
    EAOP2   EaOp;
    EaOp.fpGEA2List = (PGEA2LIST)&gGEA2ListSymlink;
    EaOp.fpFEA2List = (PFEA2LIST)pachBuffer;
    EaOp.oError     = 0;
    EaOp.fpFEA2List->cbList = SIZEOF_ACHBUFFER;
    int rc = DosQueryPathInfo((PCSZ)pszNativePath, FIL_QUERYEASFROMLIST, &EaOp, sizeof(EaOp));
    if (!rc)
    {
        if (    EaOp.fpFEA2List->cbList > sizeof(EaOp.fpFEA2List->list[0])
            &&  EaOp.fpFEA2List->list[0].cbValue)
        {
            /*
             * Validate the EA.
             */
            PUSHORT pusType = (PUSHORT)((char *)&EaOp.fpFEA2List->list[1] + EaOp.fpFEA2List->list[0].cbName);
            int     cchSymlink = pusType[1];
            char   *pszSymlink = (char *)&pusType[2];
            if (    pusType[0] == EAT_ASCII
                &&  cchSymlink < EaOp.fpFEA2List->list[0].cbValue
                &&  cchSymlink > 0
                &&  *pszSymlink)
            {
                /*
                 * Copy it to the buffer and return.
                 */
                if (cchSymlink > cchBuf)
                {
                    LIBCLOG_ERROR("Buffer to small. %d bytes required, %d bytes available. pszSymlink='%.*s' pszNativePath='%s'\n",
                                  cchSymlink, cchBuf, cchSymlink, pszSymlink, pszNativePath);
                    cchSymlink = cchBuf;
                }
                memcpy(pachBuf, pszSymlink, cchSymlink);
                LIBCLOG_RETURN_INT(cchSymlink);
            }
            else
                LIBCLOG_ERROR_RETURN(-EFTYPE, "ret -EFTYPE - Invalid symlink EA! type=%x len=%d cbValue=%d *pszSymlink=%c pszNativePath='%s'\n",
                                     pusType[0], pusType[1], EaOp.fpFEA2List->list[0].cbValue, *pszSymlink, pszNativePath);
        }
        else
            LIBCLOG_ERROR_RETURN(-EINVAL, "ret -EINVAL - %s is not a symlink!\n", pszNativePath);
    }
    else
        LIBCLOG_ERROR("DosQueryPathInfo(%s) -> %d!\n", pszNativePath, rc);

    /* failure */
    rc = -__libc_native2errno(rc);
    LIBCLOG_ERROR_RETURN_INT(rc);
}


/**
 * Checks if the specified path is a symlink or not.
 * @returns true / false.
 * @param   pszNativePath       Path to the potential symlink.
 */
static int fsIsSymlink(const char *pszNativePath)
{
    char sz[16];
    return __libc_back_fsNativeSymlinkRead(pszNativePath, sz, sizeof(sz)) >= 0;
}


#if 0 //just testing, not useful.
/**
 * Updates a symlink.
 *
 * @returns 0 on success.
 * @returns -1 and errno on failure.
 * @param   pszTarget       The symlink target.
 * @param   pszSymlink      The symlink, the one to be updated.
 */
int __libc_back_fsSymlinkWrite(const char *pszTarget, const char *pszSymlink)
{
    LIBCLOG_ENTER("pszTarget=%s pszSymlink=%s\n", pszTarget, pszSymlink);

    /*
     * Validate input.
     */
    int cchTarget = strlen(pszTarget);
    if (cchTarget >= PATH_MAX)
        LIBCLOG_ERROR_RETURN(-ENAMETOOLONG, "ret -ENAMETOOLONG - Target is too long, %d bytes. (%s)\n", cchTarget, pszTarget);

    /*
     * Allocate FEA buffer.
     */
    EAOP2   EaOp = {0,0,0};
    int     cb = cchTarget + sizeof(USHORT) * 2 + sizeof(EA_SYMLINK) + sizeof(FEALIST);
    EaOp.fpFEA2List = alloca(cb);
    if (!EaOp.fpFEA2List)
        LIBCLOG_ERROR_RETURN(-ENOMEM, "ret -ENOMEM - Out of stack! alloca(%d) failed\n", cb);

    /*
     * Setup the EAOP structure.
     */
    EaOp.fpFEA2List->cbList = cb;
    PFEA2   pFEA = &EaOp.fpFEA2List->list[0];
    pFEA->oNextEntryOffset  = 0;
    pFEA->cbName            = sizeof(EA_SYMLINK) - 1;
    pFEA->cbValue           = cchTarget + sizeof(USHORT) * 2;
    pFEA->fEA               = FEA_NEEDEA;
    memcpy(pFEA->szName, EA_SYMLINK, sizeof(EA_SYMLINK));
    PUSHORT pus = (PUSHORT)&pFEA->szName[sizeof(EA_SYMLINK)];
    pus[0]                  = EAT_ASCII;
    pus[1]                  = cchTarget;
    memcpy(&pus[2], pszTarget, cchTarget);
    int rc = DosSetPathInfo((PCSZ)pszTarget, FIL_QUERYEASIZE, &EaOp, sizeof(EAOP2), 0);
    if (!rc)
        LIBCLOG_RETURN_INT(0);

    rc = -__libc_native2errno(rc);
    LIBCLOG_ERROR_RETURN_INT(rc);
}
#endif


/**
 * Updates the global unix root stuff.
 * Assumes caller have locked the fs stuff.
 *
 * @param   pszUnixRoot     The new unix root. Fully resolved and existing.
 */
void __libc_back_fsUpdateUnixRoot(const char *pszUnixRoot)
{
    gUnixRootRewriteRule.pszTo = "/";
    gUnixRootRewriteRule.cchTo = 1;

    int cch = strlen(pszUnixRoot);
    memcpy(__libc_gszUnixRoot, pszUnixRoot, cch + 1);
    __libc_gcchUnixRoot = cch;
}


/**
 * Cleans up a path specifier a little bit.
 * This includes removing duplicate slashes, uncessary single dots, and
 * trailing slashes.
 *
 * @returns Number of bytes in the clean path.
 * @param   pszPath     The path to cleanup.
 * @param   fFlags      Flags controlling the operation of the function.
 *                      See the BACKFS_FLAGS_* defines.
 * @param   pfFlags     Where to clear the BACKFS_FLAGS_RESOLVE_DIR_MAYBE_ flag if a
 *                      trailing slash was found.
 */
static int fsCleanPath(char *pszPath, unsigned fFlags, unsigned *pfFlags)
{
    /*
     * Change to '/' and remove duplicates.
     */
    int     fUnc = 0;
    char   *pszSrc = pszPath;
    char   *pszTrg = pszPath;
    if (    (pszPath[0] == '\\' || pszPath[0] == '/')
        &&  (pszPath[1] == '\\' || pszPath[1] == '/')
        &&   pszPath[2] != '\\' && pszPath[2] != '/')
    {   /* Skip first slash in a unc path. */
        pszSrc++;
        *pszTrg++ = '/';
        fUnc = 1;
    }

    for (;;)
    {
        char ch = *pszSrc++;
        if (ch == '/' || ch == '\\')
        {
            *pszTrg++ = '/';
            for (;;)
            {
                do  ch = *pszSrc++;
                while (ch == '/' || ch == '\\');

                /* Remove '/./' and '/.'. */
                if (ch != '.' || (*pszSrc && *pszSrc != '/' && *pszSrc != '\\'))
                    break;
            }
        }
        *pszTrg = ch;
        if (!ch)
            break;
        pszTrg++;
    }

    /*
     * Remove trailing slash if the path may be pointing to a directory.
     * A symlink search is converted to a directory search if this is encountered.
     */
    int cch = pszTrg - pszPath;
    if (    (fFlags & (BACKFS_FLAGS_RESOLVE_DIR | BACKFS_FLAGS_RESOLVE_FULL_SYMLINK))
        &&  cch > 1
        &&  pszTrg[-1] == '/'
        &&  pszTrg[-2] != ':'
        &&  pszTrg[-2] != '/')
    {
        pszPath[--cch] = '\0';
        if (pfFlags)
        {
            *pfFlags &= ~(BACKFS_FLAGS_RESOLVE_DIR_MAYBE_ | BACKFS_FLAGS_RESOLVE_FULL_SYMLINK);
            if (fFlags & BACKFS_FLAGS_RESOLVE_FULL_SYMLINK)
                *pfFlags |= BACKFS_FLAGS_RESOLVE_DIR | BACKFS_FLAGS_RESOLVE_FULL;
        }
    }

    return cch;
}


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
int __libc_back_fsResolve(const char *pszUserPath, unsigned fFlags, char *pszNativePath, int *pfInUnixTree)
{
    if (pszUserPath && *pszUserPath)
    {
        if (!__libc_gfNoUnix)
            return fsResolveUnix(pszUserPath, fFlags, pszNativePath, pfInUnixTree);
        else
            return fsResolveOS2(pszUserPath, fFlags, pszNativePath, pfInUnixTree);
    }

    /* failure */
    *pszNativePath = '\0';
    if (pfInUnixTree)
        *pfInUnixTree = 0;

    LIBC_ASSERT(pszUserPath);
    if (!pszUserPath)
        return -EINVAL;
    return -ENOENT;
}


/**
 * Resolves and verifies the user path to a native path.
 *
 * @returns 0 on success.
 * @returns Negative error code (errno.h) on failure.
 * @param   pszUserPath     The user path.
 * @parm    fFlags          Flags controlling the operation of the function.
 *                          See the BACKFS_FLAGS_* defines.
 * @param   pszNativePath   Where to store the native path. This buffer is at
 *                          least PATH_MAX bytes big.
 * @param   pfInUnixTree    Where to store the result-in-unix-tree indicator. Optional.
 */
static int fsResolveUnix(const char *pszUserPath, unsigned fFlags, char *pszNativePath, int *pfInUnixTree)
{
    LIBCLOG_ENTER("pszUserPath=%p:{%s} fFlags=%#x pszNativePath=%p *pfInUnixTree=%p\n",
                  (void *)pszUserPath, pszUserPath, fFlags, (void *)pszNativePath, (void *)pfInUnixTree);
    const char     *pszUserPathIn = pszUserPath;
    char            _achBuffer[SIZEOF_ACHBUFFER + 4];
    char           *pachBuffer = (char *)((uintptr_t)&_achBuffer[3] & ~3);
    unsigned        cLoopsLeft = 8;
    int             fInUnixTree = __libc_gfInUnixTree;
    int             rcRet = 0;
    HDIR            hDir = HDIR_CREATE;
    __LIBC_PFSINFO  pFsInfo = NULL;
    int             fUnixEAs;
    FS_VAR()
    FS_SAVE_LOAD();
    for (;;)
    {
        /*
         * Determin root slash position.
         */
        int iRoot;
        if (pszUserPath[0] == '/' || pszUserPath[0] == '\\')
        {
            iRoot = 0;
            fInUnixTree = __libc_gfInUnixTree;
        }
        else if (   pszUserPath[0] && pszUserPath[1] == ':'
                 && (pszUserPath[2] == '/' || pszUserPath[2] == '\\'))
        {
            iRoot = 2;
            fInUnixTree = 0;
        }
        /*
         * No root slash? Make one!
         * This can only happen in the first pass (unless something BAD happens of course).
         */
        else
        {
            ULONG   ul;
            int     rc;
            if (pszUserPath[1] != ':')
            {
                /*
                 * Current drive.
                 */
                ULONG ulDisk;
                rc = DosQueryCurrentDisk(&ulDisk, &ul);
                pszNativePath[0] = ulDisk + 'A' - 1;
                ul = PATH_MAX - 2;
                if (!rc)
                    rc = DosQueryCurrentDir(0, (PSZ)&pszNativePath[3], &ul);
                iRoot = __libc_gfInUnixTree ? __libc_gcchUnixRoot : 2;
                /* fInUnixTree remains whatever it is */
            }
            else
            {
                /*
                 * Drive letter but no root slash.
                 */
                pszNativePath[0] = pszUserPath[0];
                ul = PATH_MAX - 2;
                rc = DosQueryCurrentDir(pszUserPath[0] - (pszUserPath[0] >= 'A' && pszUserPath[0] <= 'Z' ? 'A' : 'a') + 1, (PSZ)&pszNativePath[3], &ul);
                pszUserPath += 2;
                iRoot = 2;
                fInUnixTree = 0;
            }
            /* failed? */
            if (rc)
            {
                rcRet = -__libc_native2errno(rc);
                break;
            }

            /*
             * Add the path stuff from the user.
             */
            pszNativePath[1] = ':';
            pszNativePath[2] = '/';
            int cch = strlen(pszNativePath);
            pszNativePath[cch++] = '/';
            int cchUserPath = strlen(pszUserPath) + 1;
            if (cch + cchUserPath > PATH_MAX)
            {
                rcRet = -ENAMETOOLONG;
                break;
            }
            memcpy(&pszNativePath[cch], pszUserPath, cchUserPath);
            pszUserPath = memcpy(pachBuffer, pszNativePath, cch + cchUserPath);
        }

        /*
         * Verify any drive specifier.
         */
        if (pszUserPath[1] == ':')
        {
            if (*pszUserPath >= 'a' && *pszUserPath <= 'z')
            {
                if ((uintptr_t)(pszUserPath - pachBuffer) > SIZEOF_ACHBUFFER)
                {
                    size_t cbUserPath = strlen(pszUserPath) + 1;
                    if (cbUserPath > PATH_MAX)
                    {
                        rcRet = -ENAMETOOLONG;
                        break;
                    }
                    pszUserPath = memcpy(pachBuffer, pszUserPath, cbUserPath);
                }
                *(char *)(void *)pszUserPath += 'A' - 'a';
            }
            else if (!(*pszUserPath >= 'A' && *pszUserPath <= 'Z'))
            {
                rcRet = -ENOENT;
                break;
            }
        }

        /*
         * Apply rewrite rules.
         * Path now goes to pszNativePath buffer.
         */
        int cchNativePath = __libc_PathRewrite(pszUserPath, pszNativePath, PATH_MAX);
        if (cchNativePath > 0)
        {
            /*
             * Redetermin root because of rewrite.
             */
            iRoot = pszNativePath[1] == ':' ? 2 : 0;
        }
        else if (!cchNativePath)
        {
            cchNativePath = strlen(pszUserPath);
            if (cchNativePath + 2 > PATH_MAX)
            {
                rcRet = -ENAMETOOLONG;
                break;
            }
            memcpy(pszNativePath, pszUserPath, cchNativePath + 1);
        }
        else
        {
            rcRet = -EINVAL;
            break;
        }

        /*
         * Check if special OS/2 name.
         */
        if (pszNativePath[0] == '/' || pszNativePath[0] == '\\')
        {
            int fDone = 0;
            if (    (pszNativePath[1] == 'p' || pszNativePath[1] == 'P')
                &&  (pszNativePath[2] == 'i' || pszNativePath[2] == 'I')
                &&  (pszNativePath[3] == 'p' || pszNativePath[3] == 'P')
                &&  (pszNativePath[4] == 'e' || pszNativePath[4] == 'E')
                &&  (pszNativePath[5] == '/' || pszNativePath[5] == '\\'))
                fDone = 1;
            else if ((pszNativePath[1]== 'd' || pszNativePath[1] == 'D')
                &&  (pszNativePath[2] == 'e' || pszNativePath[2] == 'E')
                &&  (pszNativePath[3] == 'v' || pszNativePath[3] == 'V')
                &&  (pszNativePath[4] == '/' || pszNativePath[4] == '\\')
                )
            {
                PFSQBUFFER2 pfsqb = (PFSQBUFFER2)pachBuffer;
                ULONG       cb = SIZEOF_ACHBUFFER;
                fDone = !DosQueryFSAttach((PCSZ)pszNativePath, 0, FSAIL_QUERYNAME, pfsqb, &cb);
            }

            /* If it was, copy path to pszNativePath and go to exit code. */
            if (fDone)
            {
                int cch = strlen(pszNativePath) + 1;
                if (cch <= PATH_MAX)
                {
                    fsCleanPath(pszNativePath, fFlags, NULL);
                    rcRet = 0;
                }
                else
                    rcRet = -ENAMETOOLONG;
                break;
            }
        }

        /*
         * Remove excessive slashing and convert all slashes to '/'.
         */
        cchNativePath = fsCleanPath(pszNativePath, fFlags, &fFlags);

        /*
         * Expand unix root or add missing drive letter.
         */
        if (pszNativePath[0] == '/' && pszNativePath[1] != '/')
        {
            memcpy(pachBuffer, pszNativePath, cchNativePath + 1);
            if (__libc_gcchUnixRoot)
            {
                iRoot = __libc_gcchUnixRoot;
                if (cchNativePath + iRoot >= PATH_MAX)
                {
                    rcRet = -ENAMETOOLONG;
                    break;
                }
                memcpy(pszNativePath, __libc_gszUnixRoot, iRoot);
                fInUnixTree = 1;
            }
            else
            {
                iRoot = 2;
                ULONG   ulDisk = 0;
                ULONG   ul;
                DosQueryCurrentDisk(&ulDisk, &ul);
                if (cchNativePath + iRoot >= PATH_MAX)
                {
                    rcRet = -ENAMETOOLONG;
                    break;
                }
                pszNativePath[0] = ulDisk + 'A' - 1;
                pszNativePath[1] = ':';
            }
            if (cchNativePath != 1 || iRoot <= 2)
                memcpy(&pszNativePath[iRoot], pachBuffer, cchNativePath + 1);
            else
                pszNativePath[iRoot] = pszNativePath[iRoot + 1] = '\0'; /* The +1 fixes '/' access in an compartement (pszPrev below). */
            cchNativePath += iRoot;
        }


        /*
         * Check all directory components.
         *
         * We actually don't bother checking if they're directories, only that
         * they exists. This shouldn't matter much since any operation on the
         * next/final component will assume the previous one being a directory.
         *
         * While walking we will clean up all use of '.' and '..' so we'll end
         * up with an optimal path in the end.
         */
        /** @todo If we've retreived the current directory, we can safely save of the effort of validating it! */

        /*
         * Find the end of the first component (start and end + 1).
         */
        char *pszPrev;
        char *psz;
        if (pszNativePath[0] == '/' && pszNativePath[1] == '/' && pszNativePath[2] != '/')
        {
            /* UNC - skip past the share name. */
            psz = strchr(&pszNativePath[2], '/');
            if (!psz)
            {
                rcRet = -ENOENT;
                break;
            }
            psz = strchr(psz + 1, '/');
            if (!psz)
            {
                strupr(pszNativePath);  /* The server + share is uppercased for consistant ino_t calculations. */
                rcRet = 0;
                break;
            }
            *psz = '\0';                /* The server + share is uppercased for consistant ino_t calculations. */
            strupr(pszNativePath);
            *psz = '/';
            pszPrev = ++psz;
            fInUnixTree = 0;            /* Unix root cannot be UNC. */
            iRoot = psz - pszNativePath;
            psz = strchr(psz, '/');

            /* We don't do UNIX EAs on UNCs */
            fUnixEAs = 0;
        }
        else
        {
            /* drive letters are always uppercase! (inode dev number req) */
            LIBC_ASSERT(pszNativePath[1] == ':');
            if (pszNativePath[0] >= 'a' && pszNativePath[0] <= 'z')
                pszNativePath[0] -= 'a' - 'A';
            pszPrev = &pszNativePath[iRoot + 1];
            psz = strchr(pszPrev, '/');

            /* Unix EAs? */
            pFsInfo = __libc_back_fsInfoObjByPathCached(pszNativePath, pFsInfo);
            LIBC_ASSERTM(pFsInfo, "%s\n", pszNativePath);
            fUnixEAs = pFsInfo ? pFsInfo->fUnixEAs : 0;
        }
        LIBC_ASSERTM(pszPrev - pszNativePath >= iRoot, "iRoot=%d  pszPrev offset %d  pszNativePath=%s\n", iRoot, pszPrev - pszNativePath, pszNativePath);
        /* If only one component, we'll check if the fVerifyLast was requested. */
        if (    !psz
            &&  (fFlags & (BACKFS_FLAGS_RESOLVE_FULL | BACKFS_FLAGS_RESOLVE_FULL_SYMLINK))
            &&  *pszPrev)
            psz = strchr(pszNativePath, '\0');

        /*
         * Walking loop.
         */
        int fDone = 1;
        while (psz)
        {
            char chSlash = *psz;
            *psz = '\0';

            /*
             * Kill . and .. specs.
             */
            if (    pszPrev[0] == '.'
                &&  (   pszPrev[1] == '\0'
                     || (pszPrev[1] == '.' && pszPrev[2] == '\0') ) )
            {
                if (    pszPrev[1] != '\0'
                    &&  (uintptr_t)(pszPrev - pszNativePath) != iRoot + 1)
                {
                    for (pszPrev -= 2; *pszPrev != '/'; pszPrev--)
                        /* noop */;
                    pszPrev++;
                }
                *psz = chSlash;
                memmove(pszPrev - 1, psz, cchNativePath - (psz - pszNativePath) + 1);
                cchNativePath -= psz - (pszPrev - 1);

                /*
                 * Next path component and restart loop.
                 */
                if (!chSlash)
                {
                    rcRet = 0;
                    break;
                }
                psz = pszPrev;
                while (*psz != '/')
                {
                    if (*psz)
                        psz++;
                    else
                    {
                        if (!(fFlags & (BACKFS_FLAGS_RESOLVE_FULL | BACKFS_FLAGS_RESOLVE_FULL_SYMLINK)))
                        {
                            rcRet = 0;
                            psz = NULL;
                        }
                        break;
                    }
                }
                continue;
            }

            /*
             * Check for obviously illegal characters in this path component
             * saving us from having DosFindFirst getting '*' and '?'.
             */
            if (strpbrk(pszPrev, "*?"))
            {
                rcRet = -ENOENT;        /* hmm. will this be correct for all situation? */
                break;
            }

            /*
             * Find the correct name and to check if it is a directory or not.
             * This'll of course also provide proper verification of the path too. :-)
             *
             * Use DosFindFirst to get the casing resolved.
             *
             * The two find buffers are assumed to be equal down thru attrFile.
             */
            //LIBC_ASSERT(psz - pszNativePath == cchNativePath); - figure this one.
            PFILEFINDBUF4 pFindBuf4 = (PFILEFINDBUF4)pachBuffer;
            PFILEFINDBUF3 pFindBuf3 = (PFILEFINDBUF3)pachBuffer;
            ULONG cFiles = 1;
            int rc = DosFindFirst((PCSZ)pszNativePath, &hDir, FILE_READONLY | FILE_HIDDEN | FILE_SYSTEM | FILE_DIRECTORY | FILE_ARCHIVED,
                                  pachBuffer, SIZEOF_ACHBUFFER, &cFiles, fUnixEAs ? FIL_QUERYEASIZE : FIL_STANDARD);
            if (!rc)
                DosFindClose(hDir);
            hDir = HDIR_CREATE;
            if (rc || cFiles == 0)
            {
                LIBCLOG_MSG("DosFindFirst('%s',,,,,) -> %d resolving '%s'\n", pszNativePath, rc, pszUserPathIn);
                if ((fFlags & BACKFS_FLAGS_RESOLVE_FULL_MAYBE_) && !chSlash)
                {
                    *psz = chSlash;
                    rcRet = 0;
                }
                else
                    rcRet = rc == ERROR_FILE_NOT_FOUND && chSlash ? -ENOENT : -__libc_native2errno(rc);
                break;
            }
            memcpy(pszPrev, fUnixEAs ? pFindBuf4->achName : pFindBuf3->achName, psz - pszPrev);
            int     fIsDirectory = (pFindBuf4->attrFile & FILE_DIRECTORY) != 0;

            /*
             * Try querying the symlink EA value.
             * (This operation will reuse the achBuffer overwriting the pFindBuf[3/4] data!)
             *
             * Yeah, we could do this in the same operation as the DosFindFirst() but
             * a little bird told me that level 3 DosFindFirst request had not been
             * returning the right things at some point, and besides it's return data
             * is rather clumsily laid out. So, I decided not to try my luck on it.
             *
             * On second thought, we seems to end up having to use DosFindFirst in some
             * cases anyway... very nice.
             */
            if (    fUnixEAs
                &&   pFindBuf4->cbList > sizeof(USHORT) * 2 + 1
                &&  (   (fFlags & BACKFS_FLAGS_RESOLVE_FULL)
                     || chSlash)
                &&  !fIsDirectory)
            {
                PEAOP2  pEaOp2 = (PEAOP2)pachBuffer;
                pEaOp2->fpGEA2List = (PGEA2LIST)&gGEA2ListSymlink;
                pEaOp2->fpFEA2List = (PFEA2LIST)(pEaOp2 + 1);
                pEaOp2->oError     = 0;
                pEaOp2->fpFEA2List->cbList = SIZEOF_ACHBUFFER - sizeof(*pEaOp2);
                rc = DosQueryPathInfo((PCSZ)pszNativePath, FIL_QUERYEASFROMLIST, pEaOp2, sizeof(*pEaOp2));
                if (rc)
                {
                    cFiles = 1;
                    pEaOp2->fpGEA2List = (PGEA2LIST)&gGEA2ListSymlink;
                    pEaOp2->fpFEA2List = (PFEA2LIST)(pEaOp2 + 1);
                    pEaOp2->oError     = 0;
                    pEaOp2->fpFEA2List->cbList = SIZEOF_ACHBUFFER - sizeof(*pEaOp2);
                    rc = DosFindFirst((PCSZ)pszNativePath, &hDir, 0, pEaOp2, pEaOp2->fpFEA2List->cbList + sizeof(*pEaOp2), &cFiles, FIL_QUERYEASFROMLIST);
                    if (rc || cFiles == 0)
                    {
                        hDir = HDIR_CREATE;
                        LIBCLOG_MSG("DosFindFirst('%s',,,pEaOp2,) -> %d resolving '%s'\n", pszNativePath, rc, pszUserPathIn);
                        if ((fFlags & BACKFS_FLAGS_RESOLVE_FULL_MAYBE_) && !chSlash)
                            rcRet = 0;
                        else
                            rcRet = rc == ERROR_FILE_NOT_FOUND && chSlash ? -ENOENT : -__libc_native2errno(rc);
                        break;
                    }
                    pEaOp2->fpFEA2List = (PFEA2LIST)((char *)pEaOp2 + sizeof(EAOP2) + offsetof(FILEFINDBUF3, cchName));
                }

                /*
                 * Did we find any symlink EA?
                 */
                if (    pEaOp2->fpFEA2List->cbList > sizeof(pEaOp2->fpFEA2List->list[0])
                    &&  pEaOp2->fpFEA2List->list[0].cbValue)
                {
                    /* Validate the EA. */
                    PUSHORT pusType = (PUSHORT)((char *)&pEaOp2->fpFEA2List->list[1] + pEaOp2->fpFEA2List->list[0].cbName);
                    char   *pszSymlink = (char *)&pusType[2];
                    if (    pusType[0] != EAT_ASCII
                        ||  pusType[1] > pEaOp2->fpFEA2List->list[0].cbValue
                        ||  !pusType[1]
                        ||  !*pszSymlink)
                    {
                        LIBCLOG_ERROR("Invalid symlink EA! type=%x len=%d cbValue=%d *pszSymlink=%c\n",
                                      pusType[0], pusType[1], pEaOp2->fpFEA2List->list[0].cbValue, *pszSymlink);
                        rcRet = -EFTYPE;
                        break;
                    }

                    /* Check if we've reached the max number of symlink loops before we continue. */
                    if (cLoopsLeft-- <= 0)
                    {
                        rcRet = -ELOOP;
                        break;
                    }

                    /* Cleanup the symlink and find it's length. */
                    pszSymlink[pusType[1]] = '\0';
                    int cchSymlink = fsCleanPath(pszSymlink, fFlags, NULL);

                    /* Merge the symlink with the path. */
                    int cchLeft = cchNativePath - (psz - pszNativePath);
                    if (*pszSymlink == '/' || *pszSymlink == '\\' || pszSymlink[1] == ':')
                    {
                        /*
                         * Replace the path up to the current point with the symlink,
                         */
                        if (cchSymlink + cchLeft + 2 >= PATH_MAX)
                        {
                            rcRet = -ENAMETOOLONG;
                            break;
                        }
                        if (cchLeft)
                        {
                            if (pszSymlink[cchSymlink - 1] != '/')
                                pszSymlink[cchSymlink++] = '/';
                            memcpy(&pszSymlink[cchSymlink], psz + 1, cchLeft + 1);
                        }

                        /* restart the whole shebang. */
                        pszUserPath = pszSymlink;
                        fDone = 0;
                        break;
                    }
                    else
                    {
                        /*
                         * Squeeze the symlink in instead of the current path component.
                         */
                        if (cchSymlink + cchNativePath + 2 >= PATH_MAX)
                        {
                            rcRet = -ENAMETOOLONG;
                            break;
                        }
                        if (cchLeft)
                        {
                            pszSymlink[cchSymlink++] = '/';
                            memcpy(&pszSymlink[cchSymlink], psz + 1, cchLeft + 1);
                            cchSymlink += cchLeft;
                        }
                        memcpy(pszPrev, pszSymlink, cchSymlink + 1);
                        cchNativePath += cchSymlink - (psz - pszPrev);

                        /* restart this component. */
                        psz = pszPrev;
                        while (*psz != '/')
                        {
                            if (*psz)
                                psz++;
                            else
                            {
                                if (!(fFlags & (BACKFS_FLAGS_RESOLVE_FULL | BACKFS_FLAGS_RESOLVE_FULL_SYMLINK)))
                                {
                                    rcRet = 0;
                                    psz = NULL;
                                }
                                break;
                            }
                        }
                        continue;
                    }
                }
            }

            /*
             * If we get here it was not a symlink and we should check how the directoryness
             * of it fit's into the big picture...
             */
            if (!fIsDirectory && chSlash)
            {
                LIBCLOG_ERROR("'%s' is not a directory (resolving '%s')\n", pszNativePath, pszUserPathIn);
                rcRet = -ENOTDIR;
                break;
            }

            /*
             * Next path component.
             */
            *psz++ = chSlash;
            if (!chSlash)
            {
                rcRet = 0;
                if (    (fFlags & (BACKFS_FLAGS_RESOLVE_DIR | BACKFS_FLAGS_RESOLVE_DIR_MAYBE_)) == BACKFS_FLAGS_RESOLVE_DIR
                    &&  !fIsDirectory
                    &&  (     !fUnixEAs
                         ||    pFindBuf4->cbList <= sizeof(USHORT) * 2 + 1
                         ||   !fsIsSymlink(pszNativePath)))
                    rcRet = -ENOTDIR;
                break;
            }
            pszPrev = psz;
            while (*psz != '/')
            {
                if (!*psz)
                {
                    if (!(fFlags & (BACKFS_FLAGS_RESOLVE_FULL | BACKFS_FLAGS_RESOLVE_FULL_SYMLINK)))
                    {
                        rcRet = 0;
                        psz = NULL;
                    }
                    break;
                }
                psz++;
            }
        } /* inner loop */

        if (fDone)
            break;
    } /* outer loop */

    /*
     * Cleanup find handle and fs object.
     */
    if (hDir != HDIR_CREATE)
        DosFindClose(hDir);
    if (pFsInfo)
        __libc_back_fsInfoObjRelease(pFsInfo);
    FS_RESTORE();

    /*
     * Copy the resolved path the the caller buffer.
     */
    if (pfInUnixTree)
        *pfInUnixTree = fInUnixTree;
    if (!rcRet)
    {
        int cch = strlen(pszNativePath) + 1;
        if (cch == 1 || (cch == 3 && pszNativePath[1] == ':'))
        {
            pszNativePath[cch - 1] = '/';
            pszNativePath[cch] = '\0';
        }
        LIBCLOG_RETURN_MSG(0, "ret 0 pszNativePath=%p:{%s}\n", (void *)pszNativePath, pszNativePath);
    }

    /* failure */
    LIBCLOG_ERROR_RETURN_MSG(rcRet, "ret %d pszNativePath=%p:{%s}\n", rcRet, (void *)pszNativePath, pszNativePath);
    pszUserPathIn = pszUserPathIn;
}


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
static int fsResolveOS2(const char *pszUserPath, unsigned fFlags, char *pszNativePath, int *pfInUnixTree)
{
    LIBCLOG_ENTER("pszUserPath=%p:{%s} fFlags=%x pszNativePath=%p pfInUnixTree=%p\n",
                  (void *)pszUserPath, pszUserPath, fFlags, (void *)pszNativePath, (void *)pfInUnixTree);

    /*
     * Apply rewrite rule.
     */
    int cch = __libc_PathRewrite(pszUserPath, pszNativePath, PATH_MAX);
    if (cch < 0)
        LIBCLOG_ERROR_RETURN_INT(-EINVAL);
    if (cch == 0)
    {
        cch = strlen(pszUserPath);
        if (cch >= PATH_MAX)
            LIBCLOG_ERROR_RETURN_INT(-ENAMETOOLONG);
        memcpy(pszNativePath, pszUserPath, cch + 1);
    }

    /*
     * Convert slashes.
     */
    char *psz = strchr(pszNativePath, '/');
    while (psz)
    {
        *psz++ = '\\';
        psz = strchr(psz, '/');
    }

    /** @todo Validate the path? hopefully not necessary. */

    if (pfInUnixTree)
        *pfInUnixTree = 0;

    LIBCLOG_RETURN_INT(0);
}


/**
 * Initializes a unix attribute structure before creating a new inode.
 * The call must have assigned default values to the the structure before doing this call!
 *
 * @returns Device number.
 * @param   pFEas           The attribute structure to fill with actual values.
 * @param   pszNativePath   The native path, used to calculate the inode number.
 * @param   Mode            The correct mode (the caller have fixed this!).
 */
dev_t __libc_back_fsUnixAttribsInit(struct __LIBC_FSUNIXATTRIBSCREATEFEA2LIST *pFEas, char *pszNativePath, mode_t Mode)
{
    /*
     * Calc Inode number.
     * We replace the lower 32-bit with a random value.
     */
    /* low 32-bit - random */
    static union
    {
        uint64_t        u64TSC;
        unsigned short  ausSeed[4];
    } s_Seed = {0};
    static _smutex s_smtxSeed = 0;
    _smutex_request(&s_smtxSeed);
    /* seed it ? */
    if (!s_Seed.ausSeed[3])
    {
        __asm__ __volatile__ ("rdtsc" : "=A" (s_Seed.u64TSC));
        s_Seed.ausSeed[3] = 1;
    }
    pFEas->u64INO = nrand48(&s_Seed.ausSeed[0]) & 0xffffffff;
    _smutex_release(&s_smtxSeed);

    /* high 32-bit - crc32 */
    const char *psz = pszNativePath;
    if (psz[1] == ':')
        psz += 2;
    pFEas->u64INO |= (uint64_t)crc32str(psz) << 32;

    /*
     * The other stuff.
     */
    pFEas->u32UID = __libc_spmGetId(__LIBC_SPMID_EUID);
    pFEas->u32GID = __libc_spmGetId(__LIBC_SPMID_EGID); /** @todo sticky bit! */
    pFEas->u32Mode = Mode;
    LIBC_ASSERT(pFEas->u32RDev == 0);
    LIBC_ASSERT(pFEas->u32Gen == 0);
    LIBC_ASSERT(pFEas->u32Flags == 0);

    /*
     * Calc device.
     */
    char chDrv = *pszNativePath;
    if (chDrv == '/' || chDrv == '\\')
        return makedev('U', 0);          /* U as in UNC */
    LIBC_ASSERT(chDrv >= 'A' && chDrv <= 'Z');
    return makedev('V', chDrv);      /* V as in Volume */
}


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
int __libc_back_fsUnixAttribsGet(int hFile, const char *pszNativePath, struct stat *pStat)
{
    LIBCLOG_ENTER("hFile=%d pszNativePath=%p:{%s} pStat=%p\n", hFile, (void *)pszNativePath, pszNativePath, (void *)pStat);

    /* Try come up with an accurate max estimate of a maximum result. */
    char    achBuffer[sizeof(EAOP2) + sizeof(FILEFINDBUF3) + sizeof(__libc_gFsUnixAttribsGEA2List) + 7 * (sizeof(USHORT) * 2 + sizeof(BYTE)) + CCHMAXPATH + 6 * sizeof(uint32_t) + 0x30];
    char   *pachBuffer = (char *)((uintptr_t)&achBuffer[3] & ~3);
    PEAOP2  pEaOp2 = (PEAOP2)pachBuffer;
    int     rc = -1000;

    /*
     * Issue the query.
     */
    if (hFile >= 0)
    {
        pEaOp2->fpGEA2List = (PGEA2LIST)&__libc_gFsUnixAttribsGEA2List;
        pEaOp2->fpFEA2List = (PFEA2LIST)(pEaOp2 + 1);
        pEaOp2->oError     = 0;
        pEaOp2->fpFEA2List->cbList = sizeof(achBuffer) - 4 - sizeof(EAOP2);
        rc = DosQueryFileInfo(hFile, FIL_QUERYEASFROMLIST, pEaOp2, sizeof(EAOP2));
    }
    if (rc && pszNativePath)
    {
        pEaOp2->fpGEA2List = (PGEA2LIST)&__libc_gFsUnixAttribsGEA2List;
        pEaOp2->fpFEA2List = (PFEA2LIST)(pEaOp2 + 1);
        pEaOp2->oError     = 0;
        pEaOp2->fpFEA2List->cbList = sizeof(achBuffer) - 4 - sizeof(EAOP2);
        rc = DosQueryPathInfo((PCSZ)pszNativePath, FIL_QUERYEASFROMLIST, pEaOp2, sizeof(EAOP2));
        if (rc == ERROR_SHARING_VIOLATION || rc == ERROR_ACCESS_DENIED || rc == ERROR_INVALID_ACCESS)
        {
            HDIR hDir = HDIR_CREATE;
            ULONG cFiles = 1;
            pEaOp2->fpGEA2List = (PGEA2LIST)&__libc_gFsUnixAttribsGEA2List;
            pEaOp2->fpFEA2List = (PFEA2LIST)(pEaOp2 + 1);
            pEaOp2->oError     = 0;
            pEaOp2->fpFEA2List->cbList = sizeof(achBuffer) - 4 - sizeof(EAOP2);
            rc = DosFindFirst((PCSZ)pszNativePath, &hDir, 0, pEaOp2, pEaOp2->fpFEA2List->cbList + sizeof(*pEaOp2), &cFiles, FIL_QUERYEASFROMLIST);
            if (!rc)
            {
                DosFindClose(hDir);
                pEaOp2->fpFEA2List = (PFEA2LIST)((char *)pEaOp2 + sizeof(EAOP2) + offsetof(FILEFINDBUF3, cchName));
            }
        }
    }
    if (rc)
    {
        LIBC_ASSERTM(rc == ERROR_ACCESS_DENIED || rc == ERROR_SHARING_VIOLATION || rc == ERROR_EAS_NOT_SUPPORTED,
                     "Bogus EAs? rc=%d oError=%ld\n", rc, pEaOp2->oError);
        rc = -__libc_native2errno(rc);
        LIBCLOG_ERROR_RETURN_INT(rc);
    }
    if (pEaOp2->fpFEA2List->cbList < LIBC_UNIX_EA_MIN)
        LIBCLOG_RETURN_INT(0);

    /*
     * Parse the result.
     */
    PFEA2   pFea2 = &pEaOp2->fpFEA2List->list[0];
    for (;;)
    {
        if (pFea2->cbValue > 0)
        {
#define COMPARE_EANAME(name) (pFea2->cbName == sizeof(name) - 1 && !memcmp(name, pFea2->szName, sizeof(name) - 1))
            if (COMPARE_EANAME(__libc_gszSymlinkEA))
                pStat->st_mode = (pStat->st_mode & ~S_IFMT) | S_IFLNK;
            else
            {
                PUSHORT pusType = (PUSHORT)&pFea2->szName[pFea2->cbName + 1];
                if (*pusType == EAT_BINARY)
                {
                    pusType++;
                    if (*pusType == sizeof(uint32_t))
                    {
                        uint32_t u32 = *(uint32_t *)++pusType;
                        if (COMPARE_EANAME(__libc_gszUidEA))
                            pStat->st_uid  = u32;
                        else if (COMPARE_EANAME(__libc_gszGidEA))
                            pStat->st_gid  = u32;
                        else if (COMPARE_EANAME(__libc_gszModeEA))
                        {
                            if (    S_ISDIR(u32)  || S_ISCHR(u32) || S_ISBLK(u32)  || S_ISREG(u32)
                                ||  S_ISFIFO(u32) || S_ISLNK(u32) || S_ISSOCK(u32) || S_ISWHT(u32))
                                pStat->st_mode = u32;
                            else
                                LIBC_ASSERTM_FAILED("Invalid file mode EA: u32=0%o (st_mode=0%o)\n", u32, pStat->st_mode);
                        }
                        else if (COMPARE_EANAME(__libc_gszRDevEA))
                            pStat->st_rdev = u32;
                        else if (COMPARE_EANAME(__libc_gszGenEA))
                            pStat->st_gen  = u32;
                        else if (COMPARE_EANAME(__libc_gszFlagsEA))
                            pStat->st_flags = u32;
                        else
                            LIBC_ASSERTM_FAILED("Huh?!? got an ea named '%s', namelen=%d! u32=%#x (%d)\n", pFea2->szName, pFea2->cbName, u32, u32);
                    }
                    else if (*pusType == sizeof(uint64_t))
                    {
                        uint64_t u64 = *(uint64_t *)++pusType;
                        if (COMPARE_EANAME(__libc_gszInoEA))
                            pStat->st_ino = u64;
                        else
                            LIBC_ASSERTM_FAILED("Huh?!? got an ea named '%s', namelen=%d! u64=%#llx (%lld)\n", pFea2->szName, pFea2->cbName, u64, u64);
                    }
                    else
                        LIBC_ASSERTM_FAILED("Invalid LIBC EA! '%s' len=%#x and len=4 or 8.\n", pFea2->szName, *pusType);
                }
                else
                    LIBC_ASSERTM_FAILED("Invalid LIBC EA! '%s' type=%#x len=%#x, expected type=%#x and len=4 or 8.\n",
                                        pFea2->szName, pusType[0], pusType[1], EAT_BINARY);
            }
#undef COMPARE_EANAME
        }

        /* next */
        if (pFea2->oNextEntryOffset <= sizeof(FEA2))
            break;
        pFea2 = (PFEA2)((uintptr_t)pFea2 + pFea2->oNextEntryOffset);
    }

    /*
     * Calc st_ino and st_dev if not found.
     */
    if ((!pStat->st_ino || !pStat->st_dev) && pszNativePath)
    {
        ino_t Inode;
        dev_t Dev = __libc_back_fsPathCalcInodeAndDev(pszNativePath, &Inode);
        if (!pStat->st_ino)
            pStat->st_ino = Inode;
        if (!pStat->st_dev)
            pStat->st_dev = Dev;
    }

    LIBCLOG_RETURN_INT(0);
}


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
int __libc_back_fsUnixAttribsGetMode(int hFile, const char *pszNativePath, mode_t *pMode)
{
     LIBCLOG_ENTER("hFile=%d pszNativePath=%p:{%s} pMode=%p\n", hFile, (void *)pszNativePath, pszNativePath, (void *)pMode);

     /* Try come up with an accurate max estimate of a maximum result. */
     char    achBuffer[sizeof(EAOP2) + sizeof(FILEFINDBUF3) + sizeof(gGEA2ListMode) + 1 * (sizeof(USHORT) * 2 + sizeof(BYTE)) + 1 * sizeof(uint32_t) + 0x30];
     char   *pachBuffer = (char *)((uintptr_t)&achBuffer[3] & ~3);
     PEAOP2  pEaOp2 = (PEAOP2)pachBuffer;
     int     rc = -1000;

     *pMode = 0;

/** @todo the following query is generic! It's repeated 3 times already in this file (only the gea list and buffer size varies). */
     /*
      * Issue the query.
      */
     if (hFile >= 0)
     {
         pEaOp2->fpGEA2List = (PGEA2LIST)&gGEA2ListMode;
         pEaOp2->fpFEA2List = (PFEA2LIST)(pEaOp2 + 1);
         pEaOp2->oError     = 0;
         pEaOp2->fpFEA2List->cbList = sizeof(achBuffer) - 4 - sizeof(EAOP2);
         rc = DosQueryFileInfo(hFile, FIL_QUERYEASFROMLIST, pEaOp2, sizeof(EAOP2));
     }
     if (rc && pszNativePath)
     {
         pEaOp2->fpGEA2List = (PGEA2LIST)&gGEA2ListMode;
         pEaOp2->fpFEA2List = (PFEA2LIST)(pEaOp2 + 1);
         pEaOp2->oError     = 0;
         pEaOp2->fpFEA2List->cbList = sizeof(achBuffer) - 4 - sizeof(EAOP2);
         rc = DosQueryPathInfo((PCSZ)pszNativePath, FIL_QUERYEASFROMLIST, pEaOp2, sizeof(EAOP2));
         if (rc == ERROR_SHARING_VIOLATION || rc == ERROR_ACCESS_DENIED || rc == ERROR_INVALID_ACCESS)
         {
             HDIR hDir = HDIR_CREATE;
             ULONG cFiles = 1;
             pEaOp2->fpGEA2List = (PGEA2LIST)&gGEA2ListMode;
             pEaOp2->fpFEA2List = (PFEA2LIST)(pEaOp2 + 1);
             pEaOp2->oError     = 0;
             pEaOp2->fpFEA2List->cbList = sizeof(achBuffer) - 4 - sizeof(EAOP2);
             rc = DosFindFirst((PCSZ)pszNativePath, &hDir, 0, pEaOp2, pEaOp2->fpFEA2List->cbList + sizeof(*pEaOp2), &cFiles, FIL_QUERYEASFROMLIST);
             if (!rc)
             {
                 DosFindClose(hDir);
                 pEaOp2->fpFEA2List = (PFEA2LIST)((char *)pEaOp2 + sizeof(EAOP2) + offsetof(FILEFINDBUF3, cchName));
             }
         }
     }
     if (rc)
     {
         LIBC_ASSERTM(rc == ERROR_ACCESS_DENIED || rc == ERROR_SHARING_VIOLATION || rc == ERROR_EAS_NOT_SUPPORTED,
                      "Bogus EAs? rc=%d oError=%ld\n", rc, pEaOp2->oError);
         rc = -__libc_native2errno(rc);
         LIBCLOG_ERROR_RETURN_INT(rc);
     }

     /*
      * Parse the result.
      * There is only one EA here, so this is gonna be pretty simple...
      */
     rc = -ENOTSUP;
     PFEA2   pFea2 = &pEaOp2->fpFEA2List->list[0];
     if (   pEaOp2->fpFEA2List->cbList > sizeof(*pFea2)
         && pFea2->cbValue > 0
         && pFea2->cbName == sizeof(EA_MODE) - 1
         && !memcmp(EA_MODE, pFea2->szName, sizeof(EA_MODE) - 1)
        )
     {
         PUSHORT pusType = (PUSHORT)&pFea2->szName[pFea2->cbName + 1];
         if (   pusType[0] == EAT_BINARY
             && pusType[1] == sizeof(uint32_t)
             )
         {
             uint32_t u32 = *(uint32_t *)(pusType + 2);
             if (    S_ISDIR(u32)  || S_ISCHR(u32) || S_ISBLK(u32)  || S_ISREG(u32)
                 ||  S_ISFIFO(u32) || S_ISLNK(u32) || S_ISSOCK(u32) || S_ISWHT(u32))
             {
                 *pMode = u32;
                 rc = 0;
             }
             else
                 LIBC_ASSERTM_FAILED("Invalid file mode EA: u32=0%o\n", u32);
         }
         else
             LIBC_ASSERTM_FAILED("Invalid LIBC EA! '%s' type=%#x len=%#x, expected type=%#x and len=4.\n",
                                 pFea2->szName, pusType[0], pusType[1], EAT_BINARY);
     }

     LIBCLOG_RETURN_MSG(rc, "ret %d *pMode=#%x\n", rc, *pMode);
}


/**
 * Helper path and handle based native functions for writing EAs.
 *
 * This tries to work around sharing issues as well as trying to use
 * fchmod/fchown on handles opened without write access.
 *
 * @returns Native status code.
 * @param   hNative         The native handle, -1 if not available.
 * @param   pszNativePath   The native path.
 * @param   pEAs            The list of EAs to set.
 * @param   pEaOp2          The EA operation 2 buffer to use.  This is passed in
 *                          as a parameter so that the calling code can access
 *                          error information.
 */
int __libc_back_fsNativeSetEAs(intptr_t hNative, const char *pszNativePath, PFEA2LIST pEAs, PEAOP2 pEaOp2)
{
    /*
     * First copy the EAs so we can do a retry with a different access path.
     */
    PFEA2LIST pCopyEAs = (PFEA2LIST)alloca(pEAs->cbList);
    memcpy(pCopyEAs, pEAs, pEAs->cbList);

    /*
     * First try.
     */
    APIRET rc;
    pEaOp2->fpGEA2List = NULL;
    pEaOp2->fpFEA2List = pEAs;
    pEaOp2->oError     = 0;
    if (hNative != -1)
        rc = DosSetFileInfo(hNative, FIL_QUERYEASIZE, pEaOp2, sizeof(*pEaOp2));
    else
        rc = DosSetPathInfo((PCSZ)pszNativePath, FIL_QUERYEASIZE, pEaOp2, sizeof(*pEaOp2), 0);

    if (   (   rc == ERROR_SHARING_VIOLATION
            || rc == ERROR_ACCESS_DENIED)
        && pszNativePath
        && pszNativePath[0])
    {
        /*
         * To write EAs using DosSetPathInfo, the system requires deny-all
         * access, but when using DosSetFileInfo we're seemingly free to choose
         * this ourselves. So, try open the file with write access and retry.
         */
        APIRET rc2;
        HFILE  hFile2      = (HFILE)-1;
        ULONG  ulAction    = 0;
        ULONG  flOpenFlags = OPEN_ACTION_OPEN_IF_EXISTS | OPEN_ACTION_FAIL_IF_NEW;
        ULONG  flOpenMode  = OPEN_ACCESS_WRITEONLY | OPEN_FLAGS_NOINHERIT | OPEN_SHARE_DENYNONE;

#if OFF_MAX > LONG_MAX
        if (__libc_gpfnDosOpenL)
            rc2 = __libc_gpfnDosOpenL((PCSZ)pszNativePath, &hFile2, &ulAction, 0, FILE_NORMAL, flOpenFlags, flOpenMode, NULL);
        else
#endif
            rc2 = DosOpen((PCSZ)pszNativePath, &hFile2, &ulAction, 0, FILE_NORMAL, flOpenFlags, flOpenMode, NULL);
    if (rc2 == NO_ERROR)
    {
            memcpy(pEAs, pCopyEAs, pCopyEAs->cbList);
            pEaOp2->fpGEA2List = NULL;
            pEaOp2->fpFEA2List = pEAs;
            pEaOp2->oError     = 0;
            rc = DosSetFileInfo(hFile2, FIL_QUERYEASIZE, pEaOp2, sizeof(*pEaOp2));

            DosClose(hFile2);
        }
    }

    return rc;
}


/**
 * Calc the Inode and Dev based on native path.
 *
 * @returns device number and *pInode.
 *
 * @param   pszNativePath       Pointer to native path.
 * @param   pInode              Where to store the inode number.
 * @remark  This doesn't work right when in non-unix mode!
 */
dev_t __libc_back_fsPathCalcInodeAndDev(const char *pszNativePath, ino_t *pInode)
{
    /*
     * Calc device.
     */
    dev_t   Dev;
    char    chDrv = *pszNativePath;
    if (chDrv == '/' || chDrv == '\\')
        Dev = makedev('U', 0);          /* U as in UNC */
    else
    {
        LIBC_ASSERT(chDrv >= 'A' && chDrv <= 'Z');
        if (chDrv >= 'a' && chDrv <= 'z')
            chDrv -= 'a' - 'A';
        Dev = makedev('V', chDrv);      /* V as in Volume */
    }

    /*
     * Calc Inode number.
     */
    const char *psz = pszNativePath;
    if (psz[1] == ':')
        psz += 2;
    *pInode = ((uint64_t)crc32str(psz) << 32) | (uint64_t)sdbm(psz);
    return Dev;
}


/**
 * Updates the FS info object.
 */
static void fsInfoObjUpdate(__LIBC_PFSINFO pFsInfo, dev_t Dev)
{
    static char achBuffer[384]; /* we're protected by the mutex, don't assume too much stack! */
    ULONG       cb = sizeof(achBuffer);
    PFSQBUFFER2 pfsqb = (PFSQBUFFER2)achBuffer;

    /* init the structure with defaults. */
    pFsInfo->fZeroNewBytes      = 0;
    pFsInfo->fUnixEAs           = 0;
    pFsInfo->fChOwnRestricted   = 1;
    pFsInfo->fNoNameTrunc       = 1;
    pFsInfo->cFileSizeBits      = 64;
    pFsInfo->cchMaxPath         = CCHMAXPATH;
    pFsInfo->cchMaxName         = CCHMAXPATHCOMP;
    pFsInfo->cchMaxSymlink      = CCHMAXPATH;
    pFsInfo->cMaxLinks          = 1;
    pFsInfo->cchMaxTermCanon    = MAX_CANON;
    pFsInfo->cchMaxTermInput    = MAX_INPUT;
    pFsInfo->cbPipeBuf          = _POSIX_PIPE_BUF;
    pFsInfo->cbBlock            = 512;
    pFsInfo->cbXferIncr         = 0x1000;
    pFsInfo->cbXferMax          = 0xf000;
    pFsInfo->cbXferMin          = 0x1000;
    pFsInfo->uXferAlign         = 0x1000;
    pFsInfo->Dev                = Dev;
    pFsInfo->szName[0]          = '\0';
    pFsInfo->szMountpoint[0]    = minor(Dev);
    pFsInfo->szMountpoint[1]    = ':';
    pFsInfo->szMountpoint[2]    = '\0';
    pFsInfo->szMountpoint[3]    = '\0';

    /* query fs info */
    FS_VAR_SAVE_LOAD();
    int rc = DosQueryFSAttach((PCSZ)pFsInfo->szMountpoint, 0, FSAIL_QUERYNAME, pfsqb, &cb);
    LIBC_ASSERTM(!rc, "rc=%d\n", rc);
    if (!rc)
        strncat(pFsInfo->szName, (const char *)&pfsqb->szName[pfsqb->cbName + 1], sizeof(pFsInfo->szName) - 1);
    if (!strcmp(pFsInfo->szName, "HPFS"))
    {
        pFsInfo->fZeroNewBytes  = 1;
        pFsInfo->fUnixEAs       = 1;
        /** @todo detect HPFS386? */
    }
    else if (!strcmp(pFsInfo->szName, "JFS"))
    {
        pFsInfo->fZeroNewBytes  = 1;
        pFsInfo->fUnixEAs       = 1;
    }
    else if (!strcmp(pFsInfo->szName, "FAT"))
    {
        pFsInfo->fZeroNewBytes  = 1;
        pFsInfo->fUnixEAs       = 1;
        pFsInfo->cchMaxName     = 8+1+3;
    }
    else if (!strcmp(pFsInfo->szName, "LAN"))
    {
        /* should find a way of getting the remote fs... */
        pFsInfo->fZeroNewBytes  = 1;    /* for performance reasons, assume it zeros. */
        pFsInfo->cbXferMin      = 0x200;
        pFsInfo->cbXferIncr     = 0x200;
    }
    else if (!strcmp(pFsInfo->szName, "RAMFS"))
        pFsInfo->fZeroNewBytes  = 0;
    /*else if (!strcmp(pFsInfo->szName, "FAT32"))
    { } */

    /* check the Unix EAs overrides. */
    if (g_achUnixEAsOverrides[minor(Dev) - 'A'] != 0)
        pFsInfo->fUnixEAs = g_achUnixEAsOverrides[minor(Dev) - 'A'] > 0;

    LIBCLOG_MSG2("fsInfoObjUpdate: dev:%#x mp:%s fsd:%s fZeroNewBytes=%d fUnixEAs=%d\n",
                 pFsInfo->Dev, pFsInfo->szMountpoint, pFsInfo->szName, pFsInfo->fZeroNewBytes,
                 pFsInfo->fUnixEAs);
    FS_RESTORE();
}


/**
 * Gets the fs info object for the specfied path.
 *
 * @returns Pointer to info object for the path, if it got one.
 * @param   Dev     The device we want the file system object for.
 */
__LIBC_PFSINFO __libc_back_fsInfoObjByDev(dev_t Dev)
{
    __LIBC_PFSINFO pFsInfo = NULL;
    char    chDrv = minor(Dev);
    if (    major(Dev) == 'V'
        &&  chDrv >= 'A' && chDrv <= 'Z')
    {
        pFsInfo = &g_aFSInfoVolumes[(int)chDrv];
        _fmutex_request(&g_mtxFSInfoVolumes, 0);
        int cRefs = __atomic_increment_s32(&pFsInfo->cRefs);
        LIBC_ASSERT(cRefs > 0);
        if (cRefs <= 0)
            pFsInfo->cRefs = cRefs = 1;
        if (cRefs == 1)
            fsInfoObjUpdate(pFsInfo, Dev);
        _fmutex_release(&g_mtxFSInfoVolumes);
    }
    else
    {
        LIBC_ASSERTM(major(Dev) != 'V', "Dev=%#x major=%#x (%c) chDrv=%c\n", Dev, major(Dev), major(Dev), chDrv);
        LIBC_ASSERTM(major(Dev) != 'U' || minor(chDrv) == 0, "Dev=%#x major=%#x (%c)\n", Dev, major(Dev), major(Dev));
    }

    return pFsInfo;
}


/**
 * Gets the fs info object for the specfied path.
 *
 * @returns Pointer to info object for the path, if it got one.
 * @param   pszNativePath   The native path as returned by the resolver.
 */
__LIBC_PFSINFO __libc_back_fsInfoObjByPath(const char *pszNativePath)
{
    /*
     * Calc device.
     */
    dev_t   Dev;
    char    chDrv = *pszNativePath;
    if (chDrv == '/' || chDrv == '\\')
        Dev = makedev('U', 0);          /* U as in UNC */
    else
    {
        LIBC_ASSERT(chDrv >= 'A' && chDrv <= 'Z');
        Dev = makedev('V', chDrv);      /* V as in Volume */
    }
    return __libc_back_fsInfoObjByDev(Dev);
}


/**
 * Gets the fs info object for the specfied path, with cache.
 *
 * @returns Pointer to info object for the path, if it got one.
 * @param   pszNativePath   The native path as returned by the resolver.
 * @param   pCached         An cached fs info object reference. Can be NULL.
 */
__LIBC_PFSINFO __libc_back_fsInfoObjByPathCached(const char *pszNativePath, __LIBC_PFSINFO pCached)
{
    /*
     * Calc device.
     */
    dev_t   Dev;
    char    chDrv = *pszNativePath;
    if (chDrv == '/' || chDrv == '\\')
        Dev = makedev('U', 0);          /* U as in UNC */
    else
    {
        LIBC_ASSERT(chDrv >= 'A' && chDrv <= 'Z');
        Dev = makedev('V', chDrv);      /* V as in Volume */
    }
    if (!pCached)
        return __libc_back_fsInfoObjByDev(Dev);
    if (pCached->Dev == Dev)
        return pCached;
    __libc_back_fsInfoObjRelease(pCached);
    return __libc_back_fsInfoObjByDev(Dev);
}


/**
 * Adds a reference to an existing FS info object.
 *
 * The caller is responsible for making sure that the object cannot
 * reach 0 references while inside this function.
 *
 * @returns pFsInfo.
 * @param   pFsInfo     Pointer to the fs info object to reference.
 */
__LIBC_PFSINFO __libc_back_fsInfoObjAddRef(__LIBC_PFSINFO pFsInfo)
{
    if (pFsInfo)
    {
        int cRefs = __atomic_increment_s32(&pFsInfo->cRefs);
        LIBC_ASSERT(cRefs > 1); (void)cRefs;
    }
    return pFsInfo;
}


/**
 * Releases the fs info object for the specfied path.
 *
 * @param   pFsInfo     Pointer to the fs info object to release a reference to.
 */
void __libc_back_fsInfoObjRelease(__LIBC_PFSINFO pFsInfo)
{
    if (pFsInfo)
    {
        int cRefs = __atomic_decrement_s32(&pFsInfo->cRefs);
        LIBC_ASSERT(cRefs >= 0); (void)cRefs;
    }
}


/**
 * Checks if the path supports Unix EAs or not.
 *
 * @returns true / false.
 * @param   pszNativePath       The native path to check.
 */
int __libc_back_fsInfoSupportUnixEAs(const char *pszNativePath)
{
    if (__libc_gfNoUnix)
        return 0;

    __LIBC_PFSINFO pFsInfo = __libc_back_fsInfoObjByPath(pszNativePath);
    const int fUnixEAs = pFsInfo && pFsInfo->fUnixEAs;
    __libc_back_fsInfoObjRelease(pFsInfo);
    return fUnixEAs;
}


#if 0
/* djb2:
  This algorithm (k=33) was first reported by dan bernstein many years ago in
  comp.lang.c. Another version of this algorithm (now favored by bernstein) uses
  xor: hash(i) = hash(i - 1) * 33 ^ str[i]; the magic of number 33 (why it
  works better than many other constants, prime or not) has never been
  adequately explained. */

static uint32_t djb2(const char *str)
{
    uint32_t hash = 5381;
    int c;

    while ((c = *(unsigned const char *)str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}
#endif

/* sdbm:
   This algorithm was created for sdbm (a public-domain reimplementation of
   ndbm) database library. it was found to do well in scrambling bits,
   causing better distribution of the keys and fewer splits. it also happens
   to be a good general hashing function with good distribution. the actual
   function is hash(i) = hash(i - 1) * 65599 + str[i]; what is included below
   is the faster version used in gawk. [there is even a faster, duff-device
   version] the magic constant 65599 was picked out of thin air while
   experimenting with different constants, and turns out to be a prime.
   this is one of the algorithms used in berkeley db (see sleepycat) and
   elsewhere. */
static uint32_t sdbm(const char *str)
{
    uint32_t hash = 0;
    int c;

    while ((c = *(unsigned const char *)str++))
        hash = c + (hash << 6) + (hash << 16) - hash;

    return hash;
}

/*-
 *  COPYRIGHT (C) 1986 Gary S. Brown.  You may use this program, or
 *  code or tables extracted from it, as desired without restriction.
 *
 *  First, the polynomial itself and its table of feedback terms.  The
 *  polynomial is
 *  X^32+X^26+X^23+X^22+X^16+X^12+X^11+X^10+X^8+X^7+X^5+X^4+X^2+X^1+X^0
 *
 *  Note that we take it "backwards" and put the highest-order term in
 *  the lowest-order bit.  The X^32 term is "implied"; the LSB is the
 *  X^31 term, etc.  The X^0 term (usually shown as "+1") results in
 *  the MSB being 1
 *
 *  Note that the usual hardware shift register implementation, which
 *  is what we're using (we're merely optimizing it by doing eight-bit
 *  chunks at a time) shifts bits into the lowest-order term.  In our
 *  implementation, that means shifting towards the right.  Why do we
 *  do it this way?  Because the calculated CRC must be transmitted in
 *  order from highest-order term to lowest-order term.  UARTs transmit
 *  characters in order from LSB to MSB.  By storing the CRC this way
 *  we hand it to the UART in the order low-byte to high-byte; the UART
 *  sends each low-bit to hight-bit; and the result is transmission bit
 *  by bit from highest- to lowest-order term without requiring any bit
 *  shuffling on our part.  Reception works similarly
 *
 *  The feedback terms table consists of 256, 32-bit entries.  Notes
 *
 *      The table can be generated at runtime if desired; code to do so
 *      is shown later.  It might not be obvious, but the feedback
 *      terms simply represent the results of eight shift/xor opera
 *      tions for all combinations of data and CRC register values
 *
 *      The values must be right-shifted by eight bits by the "updcrc
 *      logic; the shift must be unsigned (bring in zeroes).  On some
 *      hardware you could probably optimize the shift in assembler by
 *      using byte-swap instructions
 *      polynomial $edb88320
 *
 *
 * CRC32 code derived from work by Gary S. Brown.
 */

#include <sys/cdefs.h>
__FBSDID("$FreeBSD: src/sys/libkern/crc32.c,v 1.2 2003/06/11 05:23:04 obrien Exp $");

#include <sys/param.h>
/* #include <sys/systm.h> */

static const uint32_t crc32_tab[] = {
	0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
	0xe963a535, 0x9e6495a3,	0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
	0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
	0xf3b97148, 0x84be41de,	0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
	0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec,	0x14015c4f, 0x63066cd9,
	0xfa0f3d63, 0x8d080df5,	0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
	0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,	0x35b5a8fa, 0x42b2986c,
	0xdbbbc9d6, 0xacbcf940,	0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
	0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
	0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
	0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,	0x76dc4190, 0x01db7106,
	0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
	0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
	0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
	0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
	0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
	0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
	0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
	0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
	0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
	0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
	0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
	0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
	0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
	0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
	0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
	0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
	0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
	0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
	0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
	0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
	0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
	0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
	0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
	0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
	0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
	0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
	0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
	0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
	0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
	0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
	0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
	0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

static uint32_t
crc32str(const char *psz)
{
	const uint8_t *p;
	uint32_t crc;

	p = (const uint8_t *)psz;
	crc = ~0U;

	while (*p)
		crc = crc32_tab[(crc ^ *p++) & 0xFF] ^ (crc >> 8);

	return crc ^ ~0U;
}

