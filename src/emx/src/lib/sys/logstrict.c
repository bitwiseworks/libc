/* $Id: logstrict.c 3371 2007-05-27 06:37:38Z bird $ */
/** @file
 *
 * LIBC SYS Backend - Debug Logging and Strict Checking Facilities.
 *
 * Copyright (c) 2004 InnoTek Systemberatung GmbH
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

/*******************************************************************************
*   Defined Constants And Macros                                               *
*******************************************************************************/
#define __LIBC_LOG_GROUP    __LIBC_LOG_GRP_NOGROUP

#define CCHTMPMSGBUFFER     512

#define ISDIGIT(c) ((c) >= '0' && (c) <= '9')
//#define MAX(a, b)  ((a) >= (b) ? (a) : (b))
//#define MIN(a, b)  ((a) < (b) ? (a) : (b))

#define NTSF_CAPITAL    0x0001
#define NTSF_LEFT       0x0002
#define NTSF_ZEROPAD    0x0004
#define NTSF_SPECIAL    0x0008
#define NTSF_VALSIGNED  0x0010
#define NTSF_PLUS       0x0020
#define NTSF_BLANK      0x0040


/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#define _GNU_SOURCE                     /* strnlen() */
#include <InnoTekLIBC/logstrict.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <process.h>
#include <errno.h>
#include <sys/builtin.h>
#include <emx/umalloc.h>
#include <setjmp.h>
#include <machine/param.h>
#include <InnoTekLIBC/thread.h>
#include <InnoTekLIBC/fork.h>

#define INCL_BASE
#define INCL_FSMACROS
#include <os2.h>
#ifndef HFILE_STDERR
#define HFILE_STDERR 2
#endif


/*******************************************************************************
*   Structures and Typedefs                                                    *
*******************************************************************************/
/** Internal representation of a logger instance. */
typedef struct __libc_logInstance
{
    /** Write Semaphore. */
    HMTX                    hmtx;
    /** Filehandle. */
    HFILE                   hFile;
    /** Api groups. */
    __LIBC_PLOGGROUPS       pGroups;
} __LIBC_LOGINST, *__LIBC_PLOGINST;


/** Extended exception registration record for usewith __libc_logXcptHandler(). */
typedef struct __libc_XCPTRegistrationRec
{
    EXCEPTIONREGISTRATIONRECORD RegRec;
    jmp_buf                     jmp;
} __LIBC_XCPTREG, *__LIBC_PXCPTREG;


/*******************************************************************************
*   Global Variables                                                           *
*******************************************************************************/
/** Hex digits. */
static const char       gszHexDigits[17] = "0123456789abcdef";
/** Uppercase hex digits. */
static const char       gszHexDigitsUpper[17] = "0123456789ABCDEF";
/** Pointer to default logger instance. */
static __LIBC_PLOGINST  gpDefault;


/*******************************************************************************
*   Internal Functions                                                         *
*******************************************************************************/
static void *   __libc_logInit(__LIBC_PLOGINST pInst, unsigned fFlags, __LIBC_PLOGGROUPS pGroups, const char *pszFilename);
static void *   __libc_logDefault(void);
static int      __libc_logBuildMsg(char *pszMsg, const char *pszFormatMsg, va_list args, const char *pszFormatPrefix, ...) __printflike(4, 5);
static void     __libc_logWrite(__LIBC_PLOGINST pInst, unsigned fGroupAndFlags, const char *pszMsg, size_t cch, int fStdErr);
static inline unsigned getTimestamp(void);
static inline unsigned getTid(void);
static inline unsigned getPid(void);
static ULONG _System __libc_logXcptHandler(PEXCEPTIONREPORTRECORD pRepRec, struct _EXCEPTIONREGISTRATIONRECORD * pRegRec, PCONTEXTRECORD pCtxRec, PVOID pv);
static int      __libc_logVSNPrintf(char *pszBuffer, size_t cchBuffer, const char *pszFormat, va_list args);
static int      __libc_logSNPrintf(char *pszBuffer, size_t cchBuffer, const char *pszFormat, ...) __printflike(3, 4);
int __libc_logForkParent(__LIBC_PFORKHANDLE pForkHandle, __LIBC_FORKOP enmOperation);



/**
 * Create a logger.
 *
 * @returns Pointer to a logger instance on success.
 * @returns NULL on failure, errno set.
 * @param   fFlags              Flags reserved for future use. Set to zero.
 * @param   pGroups             Pointer to a table of logging groups used for this
 *                              logger instance.
 * @param   pszFilenameFormat   Format string for making up the log filename.
 * @param   ...                 Arguments to the format string.
 */
void *__libc_LogInit(unsigned fFlags, __LIBC_PLOGGROUPS pGroups, const char *pszFilenameFormat, ...)
{
    __LIBC_PLOGINST pInst;
    char           *pszFilename;
    va_list         args;
    void           *pvRet;

    /*
     * Validate input.
     */
    if (    fFlags != 0
        ||  pszFilenameFormat == NULL
        ||  (   pGroups
             && (   !pGroups->paGroups
                 || !pGroups->cGroups
                 || pGroups->uBase > 0xffff
                 )
             )
        )
    {
        errno = EINVAL;
        return NULL;
    }

    /*
     * Allocate a logger instance.
     */
    pInst = _hmalloc(sizeof(__LIBC_LOGINST) + CCHMAXPATH);
    if (!pInst)
        return NULL;
    pszFilename = (char*)(pInst + 1);

    /*
     * Format the filename.
     */
    va_start(args, pszFilenameFormat);
    __libc_logVSNPrintf(pszFilename, CCHMAXPATH, pszFilenameFormat, args);
    va_end(args);

    /*
     * Call internal inititation worker.
     * (shared with the default logger init code)
     */
    pvRet = __libc_logInit(pInst, fFlags, pGroups, pszFilename);
    if (!pvRet)
    {
        free(pInst);                /* failure. */
        errno = EACCES;                 /* good try, DosOpen probably failed. */
    }

    return pvRet;
}


/**
 * Initates the logger instance, opening the specified file and such.
 *
 * @returns Pointer to pInst on success.
 * @returns NULL on failure.
 * @param   pInst       Pointer to logger instance (to be initiated).
 * @param   fFlags      Flags. (reserved for future use, thus 0)
 * @param   pGroups     Pointer to message groups.
 * @param   pszFilename Name of the log file.
 */
static void *   __libc_logInit(__LIBC_PLOGINST pInst, unsigned fFlags, __LIBC_PLOGGROUPS pGroups, const char *pszFilename)
{
    ULONG       ulAction;
    int         rc;
    char       *pszMsg;
    FS_VAR();
    FS_SAVE_LOAD();

    pInst->hmtx     = NULLHANDLE;
    pInst->hFile    = NULLHANDLE;
    pInst->pGroups  = pGroups;

    /*
     * Create the mutex.
     */
    rc = DosCreateMutexSem(NULL, &pInst->hmtx, 0, 0);
    if (rc)
    {
        FS_RESTORE();
        return NULL;
    }

    /*
     * Open the file.
     * Make sure the filehandle is above the frequently used range (esp. std handles).
     */
    rc = DosOpen((PCSZ)pszFilename, &pInst->hFile, &ulAction, 0, FILE_NORMAL,
                 OPEN_ACTION_CREATE_IF_NEW | OPEN_ACTION_REPLACE_IF_EXISTS,
                 OPEN_FLAGS_SEQUENTIAL | OPEN_FLAGS_NOINHERIT | OPEN_SHARE_DENYWRITE | OPEN_ACCESS_WRITEONLY,
                 NULL);
    if (rc)
    {
        DosCloseMutexSem(pInst->hmtx);
        FS_RESTORE();
        return NULL;
    }
    if (pInst->hFile < 10)
    {
        int     i;
        HFILE   ah[10];
        for (i = 0; i < 10; i++)
        {
            ah[i] = -1;
            rc = DosDupHandle(pInst->hFile, &ah[i]);
            if (rc)
                break;
        }
        if (i-- > 0)
        {
            DosClose(pInst->hFile);
            pInst->hFile = ah[i];
            while (i-- > 0)
                DosClose(ah[i]);

            ULONG fulFlags = ~0U;
            if (DosQueryFHState(pInst->hFile, &fulFlags) == NO_ERROR)
            {
                fulFlags |= OPEN_FLAGS_NOINHERIT;
                fulFlags &= OPEN_FLAGS_WRITE_THROUGH | OPEN_FLAGS_FAIL_ON_ERROR | OPEN_FLAGS_NO_CACHE | OPEN_FLAGS_NOINHERIT; /* Turn off non-participating bits. */
                DosSetFHState(pInst->hFile, fulFlags);
            }
        }
    }

    /*
     * Write log header.
     */
    pszMsg = alloca(CCHTMPMSGBUFFER);
    if (pszMsg)
    {
        PTIB        pTib;
        PPIB        pPib;
        DATETIME    dt;
        const char *psz;
        ULONG       cb;
        int         cch;

        /* The current time+date and basic process attributes. */
        DosGetInfoBlocks(&pTib, &pPib);
        DosGetDateTime(&dt);
        cch = __libc_logSNPrintf(pszMsg, CCHTMPMSGBUFFER,
                                 "Opened log at %04d-%02d-%02d %02d:%02d:%02d.%02d\n"
                                 "Process ID: %#x (%d) Parent PID: %#x (%d) Type: %d\n",
                                 dt.year, dt.month, dt.day, dt.hours, dt.minutes, dt.seconds, dt.hundredths,
                                 (int)pPib->pib_ulpid, (unsigned)pPib->pib_ulpid, (int)pPib->pib_ulppid, (unsigned)pPib->pib_ulppid,
                                 (int)pPib->pib_ultype);
        DosWrite(pInst->hFile, pszMsg, cch, &cb);

        /* The executable module. */
        cch = __libc_logSNPrintf(pszMsg, CCHTMPMSGBUFFER,
                                 "Exe hmte  : %#x (",
                                 (unsigned)pPib->pib_hmte);
        DosWrite(pInst->hFile, pszMsg, cch, &cb);
        if (DosQueryModuleName(pPib->pib_hmte, CCHTMPMSGBUFFER - 4, pszMsg))
            pszMsg[0] = '\0';
        cch = strlen(pszMsg);
        pszMsg[cch++] = ')';
        pszMsg[cch++] = '\n';
        DosWrite(pInst->hFile, pszMsg, cch, &cb);

        /* The raw arguments. */
        cch = __libc_logSNPrintf(pszMsg, CCHTMPMSGBUFFER,
                                 "First arg : %s\n"
                                 "Second arg: ",
                                 pPib->pib_pchcmd);
        DosWrite(pInst->hFile, pszMsg, cch, &cb);
        psz = pPib->pib_pchcmd + strlen(pPib->pib_pchcmd) + 1;
        cch = strlen(psz);
        DosWrite(pInst->hFile, psz, cch, &cb);
        DosWrite(pInst->hFile, "\n", 1, &cb);

        /* The current drive and directory. */
        ULONG ulDisk = 0;
        ULONG fLogical = 0;
        if (!DosQueryCurrentDisk(&ulDisk, &fLogical))
        {
            cch = __libc_logSNPrintf(pszMsg, CCHTMPMSGBUFFER,
                                     "Cur dir   : %c:\\",
                                     (char)ulDisk + ('A' - 1));
            DosWrite(pInst->hFile, pszMsg, cch, &cb);

            ULONG cchDir = CCHTMPMSGBUFFER - 4;
            if (DosQueryCurrentDir(ulDisk, (PSZ)pszMsg, &cchDir))
                pszMsg[0] = '\0';
            cch = strlen(pszMsg);
            pszMsg[cch++] = '\n';
            DosWrite(pInst->hFile, pszMsg, cch, &cb);
        }

        /* The CRT module. */
        char    szMod[128];
        ULONG   iObj = ~0;
        ULONG   offObj = ~0;
        HMODULE hmod = ~0;
        if (DosQueryModFromEIP(&hmod, &iObj, sizeof(szMod), &szMod[0], &offObj, (uintptr_t)__libc_logInit))
        {
            szMod[0] = '\0';
            hmod = NULLHANDLE;
        }
        cch = __libc_logSNPrintf(pszMsg, CCHTMPMSGBUFFER,
                                 "CRT Module: %s hmod=%#lx (",
                                 szMod, hmod);
        DosWrite(pInst->hFile, pszMsg, cch, &cb);

        if (    hmod == NULLHANDLE
            ||  DosQueryModuleName(hmod, CCHTMPMSGBUFFER - 4, pszMsg))
            pszMsg[0] = '\0';
        cch = strlen(pszMsg);
        pszMsg[cch++] = ')';
        pszMsg[cch++] = '\n';
        DosWrite(pInst->hFile, pszMsg, cch, &cb);

        cch = __libc_logSNPrintf(pszMsg, CCHTMPMSGBUFFER,
                                 "__libc_logInit: addr %p iObj=%ld offObj=%#lx\n",
                                 (void *)__libc_logInit, iObj, offObj);
        DosWrite(pInst->hFile, pszMsg, cch, &cb);

        /* column headers */
        cch = __libc_logSNPrintf(pszMsg, CCHTMPMSGBUFFER,
                                 "   Millsecond Timestamp.\n"
                                 "   |     Thread ID.\n"
                                 "   |     |  Call Nesting Level.\n"
                                 "   |     |  |   Log Group.\n"
                                 "   |     |  |   |    Message Type.\n"
                                 "   |     |  |   |    |    errno in hex (0xface if not available).\n"
                                 "   |     |  |   |    |    |      Function Name.\n"
                                 "   |     |  |   |    |    |      |       Millisconds In function (Optional).\n"
                                 "   v     v  v   v    v    v      v       v\n"
                                 "xxxxxxxx tt nn gggg dddd eeee function [(ms)]: message\n");
        DosWrite(pInst->hFile, pszMsg, cch, &cb);
    }

    FS_RESTORE();
    return pInst;
}


/**
 * Parses the given environment variable and sets the group
 * flags accordingly.
 *
 * The environment variable is a sequence of group idendifiers with
 * a prefix which determins whether or not that group is enabled.
 * A special group 'all' can be used to address all groups.
 *
 * If the environment variable is not present no changes will be
 * performed.
 *
 * @param   pGroups     Pointer to groups to init.
 * @param   pszEnvVar   Name of the environment variable.
 *                      This is taken from the initial environment of the process
 *                      and not from the current!!
 */
void __libc_LogGroupInit(__LIBC_PLOGGROUPS pGroups, const char *pszEnvVar)
{
    PSZ pszEnv = NULL;

    /*
     * Get and process the env.var. containing the logging settings.
     */
    if (DosScanEnv((PCSZ)pszEnvVar, &pszEnv) || !pszEnv)
        return;

    /*
     * Process it.
     */
    while (*pszEnv)
    {
        char    ch;
        int     fEnabled = 1;
        PCSZ    pszStart;
        int     i;
        int     cch;

        /* Prefixes (blanks, + and -). */
        while ((ch = *pszEnv) == '+' || ch == '-' || ch == ' ' || ch == '\t' || ch == ';')
        {
            if (ch == '+' || ch == '-' || ch == ';')
                fEnabled = ch != '-';
            pszEnv++;
        }
        if (!*pszEnv)
            break;

        /*
         * Find end.
         */
        pszStart = pszEnv;
        while ((ch = *pszEnv) != '\0' && ch != '+' && ch != '-' && ch != ' ' && ch != '\t' && ch != ';')
            pszEnv++;

        /*
         * Find the group (ascii case insensitive search).
         * Special group 'all'.
         */
        cch = pszEnv - pszStart;
        if (    cch == 3
            &&  (pszStart[0] == 'a' || pszStart[0] == 'A')
            &&  (pszStart[1] == 'l' || pszStart[1] == 'L')
            &&  (pszStart[2] == 'l' || pszStart[2] == 'L')
                )
        {   /* all */
            for (i = 0; i < pGroups->cGroups; i++)
                pGroups->paGroups[i].fEnabled = fEnabled;
        }
        else
        {   /* specific group */
            int fFound;
            for (i = 0, fFound = 0; i < pGroups->cGroups && !fFound; i++)
            {
                const char *psz1 = pGroups->paGroups[i].pszGroupName;
                if (psz1 && *psz1)
                {
                    const char *psz2    = (const char*)pszStart;
                    int         cchLeft = cch;
                    #define CHLOWER(ch)     ((ch) < 'A' && (ch) > 'Z' ? (ch) : (ch) + ('a' - 'A'))
                    while (CHLOWER(*psz1) == CHLOWER(*psz2))
                    {
                        psz1++;
                        if (!--cchLeft)
                        {   /* done */
                            if (!*psz1)
                            {
                                fFound = 1;
                                pGroups->paGroups[i].fEnabled = fEnabled;
                            }
                            break;
                        }
                        psz2++;
                    }
                }
            }
        }

    } /* parse value */
}


/**
 * Get the default logger instance.
 * This call will open the default logger instance if required.
 *
 * @returns Pointer to default logger.
 * @returns NULL on failure.
 */
static void *__libc_logDefault(void)
{
    /** Set if we've already tried to init the log, but failed. */
    static int              fAlreadyTried;

    if (gpDefault)
        return gpDefault;
    else if (!fAlreadyTried)
    {
        static __LIBC_LOGINST   DefInst;
        static __LIBC_LOGGROUP  aDefGrps[__LIBC_LOG_GRP_MAX + 1] =
        {
            { 1, "NOGROUP" },           /*  0 */
            { 1, "PROCESS" },           /*  1 */
            { 1, "HEAP" },              /*  2 */
            { 1, "STREAM" },            /*  3 */
            { 1, "IO" },                /*  4 */
            { 1, "STRING" },            /*  5 */
            { 1, "LOCALE" },            /*  6 */
            { 1, "REGEX" },             /*  7 */
            { 1, "MATH" },              /*  8 */
            { 1, "TIME" },              /*  9 */
            { 1, "BSD_DB" },            /* 10 */
            { 1, "GLIBC_POSIX" },       /* 11 */
            { 1, "THREAD" },            /* 12 */
            { 1, "MUTEX" },             /* 13 */
            { 1, "SIGNAL" },            /* 14 */
            { 1, "ENV" },               /* 15 */
            { 1, "MMAN" },              /* 16 */
            { 1, "future" },            /* 17 */
            { 1, "BACK_THREAD" },       /* 18 */
            { 1, "BACK_PROCESS" },      /* 19 */
            { 1, "BACK_SIGNAL" },       /* 20 */
            { 1, "BACK_MMAN" },         /* 21 */
            { 1, "BACK_LDR" },          /* 22 */
            { 1, "BACK_FS" },           /* 23 */
            { 1, "BACK_SPM" },          /* 24 */
            { 1, "FORK" },              /* 25 */
            { 1, "BACK_IO" },           /* 26 */
            { 1, "INITTERM" },          /* 27 */
            { 1, "BACKEND" },           /* 28 */
            { 1, "MISC" },              /* 29 */
            { 1, "BSD_GEN" },           /* 30 */
            { 1, "GLIBC_MISC" },        /* 31 */
            { 1, "SOCKET" },            /* 32 */
            { 1, "TCPIP" },             /* 33 */
            { 1, "ICONV" },             /* 34 */
            { 1, "DLFCN" },             /* 35 */
            { 1, "PTHREAD" },           /* 36 */
            { 1, "DOSEX" }              /* 37 */
        };
        static __LIBC_LOGGROUPS DefGrps =
        {
            0,                          /* uBase */
            __LIBC_LOG_GRP_MAX + 1,     /* cGroups */
            &aDefGrps[0]                /* paGroups */
        };
        char                    szFilename[20];
        fAlreadyTried = 1;

        /*
         * Create the log instance.
         */
        __libc_logSNPrintf(szFilename, sizeof(szFilename), "libc_%04x.log", getPid());
        if (__libc_logInit(&DefInst, 0, &DefGrps, szFilename))
            gpDefault = &DefInst;

        /*
         * Init the groups.
         */
        __libc_LogGroupInit(&DefGrps, "LIBC_LOGGING");
    }

    return gpDefault;
}


/**
 * Builds a one line message from two format strings and arguments.
 *
 * @returns Message length in bytes.
 *
 * @param   pszMsg              Buffer of size CCHTMPMSGBUFFER.
 * @param   pszFormatMsg        Message format string.
 * @param   args                Message format arguments.
 * @param   pszFormatPrefix     Message prefix format string.
 * @param   ...                 Message prefix format string.
 */
static int __libc_logBuildMsg(char *pszMsg, const char *pszFormatMsg, va_list args, const char *pszFormatPrefix, ...)
{
    int     cch;
    va_list args2;

    /*
     * Message prefix
     */
    va_start(args2, pszFormatPrefix);
    cch = __libc_logVSNPrintf(pszMsg, CCHTMPMSGBUFFER, pszFormatPrefix, args2);
    va_end(args2);

    /*
     * The message.
     */
    cch += __libc_logVSNPrintf(pszMsg + cch, CCHTMPMSGBUFFER - cch, pszFormatMsg, args);

    /*
     * ensure '\n'.
     */
    if (cch >= CCHTMPMSGBUFFER)
        memcpy(pszMsg + CCHTMPMSGBUFFER - 5, "...\n", 5);
    else if (pszMsg[cch - 1] != '\n')
        memcpy(&pszMsg[cch++], "\n", 2);

    return cch;
}


/**
 * Writes a string to the log.
 *
 * @param   pInst           Logger instance to write to.
 * @param   fGroupAndFlags  Flags and group for the write (group part is ignored)
 * @param   pszMsg          String to write.
 * @param   cch             Length of string.
 * @param   fStdErr         If set write to standard err too.
 */
static void __libc_logWrite(__LIBC_PLOGINST pInst, unsigned fGroupAndFlags, const char *pszMsg, size_t cch, int fStdErr)
{
    APIRET  rcSem;
    APIRET  rcMC;
    ULONG   ulIgnore = 0;
    ULONG   cb = 0;
    HMTX    hmtx = pInst->hmtx;
    FS_VAR();
    FS_SAVE_LOAD();

    /*
     * Aquire mutex ownership.
     * We enter a must-complete section here to avoid getting interrupted
     * while writing and creating deadlocks.
     */
    rcMC = DosEnterMustComplete(&ulIgnore);
    rcSem = DosRequestMutexSem(hmtx, 5000);
    switch (rcSem)
    {
        case NO_ERROR:
            break;

        /* recreate the semaphore for some odd. */
        case ERROR_INVALID_HANDLE:
        case ERROR_SEM_OWNER_DIED:
        {
            HMTX    hmtxNew;
            rcSem = DosCreateMutexSem(NULL, &hmtxNew, 0, TRUE);
            if (!rcSem)
            {
                hmtx = __lxchg((int*)&pInst->hmtx, hmtxNew);
                DosCloseMutexSem(hmtx);
            }
            break;
        }
        /* ignore other errors. */
    }

    /*
     * Write message and release the semaphore (if owned).
     */
    DosWrite(pInst->hFile, pszMsg, cch, &cb);
    if (fGroupAndFlags & __LIBC_LOG_MSGF_FLUSH)
        DosResetBuffer(pInst->hFile);
    if (!rcSem)
        DosReleaseMutexSem(pInst->hmtx);
    if (!rcMC)
        DosExitMustComplete(&ulIgnore);
    FS_RESTORE();

    /*
     * Write to stderr too?
     */
    if (fStdErr)
    {
        /*
         * Need to convert '\n' to '\r\n'.
         */
        const char *pszNewLine = NULL;
        do
        {
            int cchWrite;

            /* look for next new line */
            pszNewLine = memchr(pszMsg, '\n', cch);
            while (pszNewLine > pszMsg && pszNewLine[-1] == '\r')
            {
                cchWrite = cch - (pszNewLine - pszMsg + 1);
                if (cchWrite <= 0)
                {
                    pszNewLine = NULL;
                    break;
                }
                pszNewLine = memchr(pszNewLine + 1, '\n', cchWrite);
            }
            cchWrite = pszNewLine ? pszNewLine - pszMsg : strnlen(pszMsg, cch);
            DosWrite(HFILE_STDERR, pszMsg, cchWrite, &cb);

            /* Write newline. */
            if (!pszNewLine)
                break; /* done */
            DosWrite(HFILE_STDERR, "\r\n", 2, &cb);
            pszMsg = pszNewLine + 1;
            cch -= cchWrite + 1;
        } while (cch);

    }
}



/**
 * Get ts. (milliseconds preferably)
 */
inline static unsigned getTimestamp(void)
{
    unsigned long ulTs = 0;
    FS_VAR();
    FS_SAVE_LOAD();
    DosQuerySysInfo(QSV_MS_COUNT, QSV_MS_COUNT, &ulTs, sizeof(ulTs));
    FS_RESTORE();
    return (unsigned)ulTs;
}


/**
 * Gets the current thread id.
 */
inline static unsigned getTid(void)
{
    PTIB    pTib;
    PPIB    pPib;
    FS_VAR();
    FS_SAVE_LOAD();
    DosGetInfoBlocks(&pTib, &pPib);
    FS_RESTORE();
    return pTib->tib_ptib2->tib2_ultid;
}


/**
 * Gets the current process id.
 */
inline static unsigned getPid(void)
{
    PTIB    pTib;
    PPIB    pPib;
    FS_VAR();
    FS_SAVE_LOAD();
    DosGetInfoBlocks(&pTib, &pPib);
    FS_RESTORE();
    return pPib->pib_ulpid;
}


/**
 * Output an enter function log message.
 * An enter message is considered to be one line and is appended a newline if
 * none was given.
 *
 * @returns Current timestamp.
 * @param   pvInstance      Logger instance. If NULL the message goes to the
 *                          default log instance.
 * @param   fGroupAndFlags  Logging group and logging flags.
 * @param   pszFunction     Name of the function which was entered.
 * @param   pszFormat       Format string to display arguments.
 * @param   ...             Arguments to the format string.
 */
unsigned __libc_LogEnter(void *pvInstance, unsigned fGroupAndFlags, const char *pszFunction, const char *pszFormat, ...)
{
    unsigned        uTS = getTimestamp();
    __LIBC_PTHREAD  pThread = __libc_threadCurrentNoAuto();
    char           *pszMsg;
    int             cch;
    va_list         args;
    __LIBC_PLOGINST pInst;
    unsigned       *pcDepth = NULL;
    unsigned        cDepth;

    /*
     * Check instance, get default.
     */
    if (!pvInstance)
    {
        pvInstance = __libc_logDefault();
        if (!pvInstance)
            return uTS;
        if (pThread)
            pcDepth = &pThread->cDefLoggerDepth;
    }
    pInst = (__LIBC_PLOGINST)pvInstance;

    /*
     * Check if this group is enabled.
     */
    if (pInst->pGroups)
    {
        int iGroup = __LIBC_LOG_GETGROUP(fGroupAndFlags);
        if (    iGroup >= 0
            &&  iGroup < pInst->pGroups->cGroups
            &&  !pInst->pGroups->paGroups[iGroup].fEnabled)
            return uTS;
    }

    /*
     * Nesting depth.
     */
    if (pcDepth)
        cDepth = ++*pcDepth;
    else
        cDepth = 0xff;

    /*
     * Allocate logging buffer and format the message.
     */
    pszMsg = alloca(CCHTMPMSGBUFFER);
    if (!pszMsg)
        return uTS;

    va_start(args, pszFormat);
    cch = __libc_logBuildMsg(pszMsg, pszFormat, args, "%08x %02x %02x %04x Entr %04x %s: ",
                             uTS, getTid(), cDepth, __LIBC_LOG_GETGROUP(fGroupAndFlags),
                             pThread ? pThread->iErrNo : 0xface, pszFunction);
    va_end(args);

    /*
     * Write the message.
     */
    __libc_logWrite(pInst, fGroupAndFlags, pszMsg, cch, 0);
    return uTS;
}


/**
 * Output a leave function log message.
 * A leave message is considered to be one line and is appended a newline if
 * none was given.
 *
 * @param   uEnterTS        The timestamp returned by LogEnter.
 * @param   pvInstance      Logger instance. If NULL the message goes to the
 *                          default log instance.
 * @param   fGroupAndFlags  Logging group and logging flags.
 * @param   pszFunction     Name of the function which was entered.
 * @param   pszFormat       Format string to display the result.
 * @param   ...             Arguments to the format string.
 */
void     __libc_LogLeave(unsigned uEnterTS, void *pvInstance, unsigned fGroupAndFlags, const char *pszFunction, const char *pszFormat, ...)
{
    unsigned        uTS = getTimestamp();
    __LIBC_PTHREAD  pThread = __libc_threadCurrentNoAuto();
    char           *pszMsg;
    int             cch;
    va_list         args;
    __LIBC_PLOGINST pInst;
    unsigned       *pcDepth = NULL;
    unsigned        cDepth;

    /*
     * Check instance, get default.
     */
    if (!pvInstance)
    {
        pvInstance = __libc_logDefault();
        if (!pvInstance)
            return;
        if (pThread)
            pcDepth = &pThread->cDefLoggerDepth;
    }
    pInst = (__LIBC_PLOGINST)pvInstance;

    /*
     * Check if this group is enabled.
     */
    if (pInst->pGroups)
    {
        int iGroup = __LIBC_LOG_GETGROUP(fGroupAndFlags);
        if (    iGroup >= 0
            &&  iGroup < pInst->pGroups->cGroups
            &&  !pInst->pGroups->paGroups[iGroup].fEnabled)
            return;
    }

    /*
     * Nesting depth.
     */
    if (pcDepth)
    {
        if (*pcDepth)
        {
            cDepth = *pcDepth;
            *pcDepth -= 1;
        }
        else
            cDepth = 0xfe;
    }
    else
        cDepth = 0xff;

    /*
     * Allocate logging buffer and format the message.
     */
    pszMsg = alloca(CCHTMPMSGBUFFER);
    if (!pszMsg)
        return;

    va_start(args, pszFormat);
    cch = __libc_logBuildMsg(pszMsg, pszFormat, args, "%08x %02x %02x %04x Leav %04x %s (%d ms): ",
                             uTS, getTid(), cDepth, __LIBC_LOG_GETGROUP(fGroupAndFlags),
                             pThread ? pThread->iErrNo : 0xface, pszFunction, uTS - uEnterTS);
    va_end(args);

    /*
     * Write the message.
     */
    __libc_logWrite(pInst, fGroupAndFlags, pszMsg, cch, 0);
}


/**
 * Output a leave function log message upon a user error.
 *
 * The function may breakpoint if configured to do so. A leave message is
 * considered to be one line and is appended a newline if none was given.
 *
 * @param   uEnterTS        The timestamp returned by LogEnter.
 * @param   pvInstance      Logger instance. If NULL the message goes to the
 *                          default log instance.
 * @param   fGroupAndFlags  Logging group and logging flags.
 * @param   pszFunction     Name of the function which was entered.
 * @param   pszFile         Source filename.
 * @param   uLine           Line number.
 * @param   pszFormat       Format string to display the result.
 * @param   ...             Arguments to the format string.
 */
void     __libc_LogErrorLeave(unsigned uEnterTS, void *pvInstance, unsigned fGroupAndFlags, const char *pszFunction, const char *pszFile, unsigned uLine, const char *pszFormat, ...)
{
    unsigned        uTS = getTimestamp();
    __LIBC_PTHREAD  pThread = __libc_threadCurrentNoAuto();
    char           *pszMsg;
    int             cch;
    va_list         args;
    __LIBC_PLOGINST pInst;
    unsigned       *pcDepth = NULL;
    unsigned        cDepth;

    /*
     * Check instance, get default.
     */
    if (!pvInstance)
    {
        pvInstance = __libc_logDefault();
        if (!pvInstance)
            return;
        if (pThread)
            pcDepth = &pThread->cDefLoggerDepth;
    }
    pInst = (__LIBC_PLOGINST)pvInstance;

    /*
     * Check if this group is enabled.
     */
    if (pInst->pGroups)
    {
        int iGroup = __LIBC_LOG_GETGROUP(fGroupAndFlags);
        if (    iGroup >= 0
            &&  iGroup < pInst->pGroups->cGroups
            &&  !pInst->pGroups->paGroups[iGroup].fEnabled)
            return;
    }

    /*
     * Nesting depth.
     */
    if (pcDepth)
    {
        if (*pcDepth)
        {
            cDepth = *pcDepth;
            *pcDepth -= 1;
        }
        else
            cDepth = 0xfe;
    }
    else
        cDepth = 0xff;

    /*
     * Allocate logging buffer.
     */
    pszMsg = alloca(CCHTMPMSGBUFFER);
    if (!pszMsg)
        return;

    /*
     * First message is about where this error occured.
     */
    cch = __libc_logSNPrintf(pszMsg, CCHTMPMSGBUFFER, "%08x %02x %02x %04x ErrL %04x %s (%d ms): %s(%d):\n",
                             uTS, getTid(), cDepth, __LIBC_LOG_GETGROUP(fGroupAndFlags),
                             pThread ? pThread->iErrNo : 0xface, pszFunction, uTS - uEnterTS,
                             pszFile, uLine);
    __libc_logWrite(pInst, fGroupAndFlags, pszMsg, cch, 0);

    /*
     * Second message is the one from the caller.
     */
    va_start(args, pszFormat);
    cch = __libc_logBuildMsg(pszMsg, pszFormat, args, "%08x %02x %02x %04x ErrL %04x: ",
                             uTS, getTid(), cDepth, __LIBC_LOG_GETGROUP(fGroupAndFlags),
                             pThread ? pThread->iErrNo : 0xface);
    va_end(args);
    __libc_logWrite(pInst, fGroupAndFlags, pszMsg, cch, 0);
}


/**
 * Output a log message.
 * A log message is considered to be one line and is appended a newline if
 * none was given.
 *
 * @param   uEnterTS        The timestamp returned by LogEnter.
 * @param   pvInstance      Logger instance. If NULL the message goes to the
 *                          default log instance.
 * @param   fGroupAndFlags  Logging group and logging flags.
 * @param   pszFunction     Name of the function which was entered.
 * @param   pszFormat       Format string for the message to log.
 * @param   ...             Arguments to the format string.
 */
void     __libc_LogMsg(unsigned uEnterTS, void *pvInstance, unsigned fGroupAndFlags, const char *pszFunction, const char *pszFormat, ...)
{
    unsigned        uTS = getTimestamp();
    __LIBC_PTHREAD  pThread = __libc_threadCurrentNoAuto();
    char           *pszMsg;
    int             cch;
    va_list         args;
    __LIBC_PLOGINST pInst;
    unsigned        cDepth = 0xff;

    /*
     * Check instance, get default.
     */
    if (!pvInstance)
    {
        pvInstance = __libc_logDefault();
        if (!pvInstance)
            return;
        if (pThread)
            cDepth = pThread->cDefLoggerDepth;
    }
    pInst = (__LIBC_PLOGINST)pvInstance;

    /*
     * Check if this group is enabled.
     */
    if (pInst->pGroups)
    {
        int iGroup = __LIBC_LOG_GETGROUP(fGroupAndFlags);
        if (    iGroup >= 0
            &&  iGroup < pInst->pGroups->cGroups
            &&  !pInst->pGroups->paGroups[iGroup].fEnabled)
            return;
    }

    /*
     * Allocate logging buffer and format the message.
     */
    pszMsg = alloca(CCHTMPMSGBUFFER);
    if (!pszMsg)
        return;

    va_start(args, pszFormat);
    if (uEnterTS != ~0)
        cch = __libc_logBuildMsg(pszMsg, pszFormat, args, "%08x %02x %02x %04x Mesg %04x %s (%d ms): ",
                                 uTS, getTid(), cDepth, __LIBC_LOG_GETGROUP(fGroupAndFlags),
                                 pThread ? pThread->iErrNo : 0xface, pszFunction, uTS - uEnterTS);
    else
        cch = __libc_logBuildMsg(pszMsg, pszFormat, args, "%08x %02x %02x %04x Mesg %04x %s: ",
                                 uTS, getTid(), cDepth, __LIBC_LOG_GETGROUP(fGroupAndFlags),
                                 pThread ? pThread->iErrNo : 0xface, pszFunction);
    va_end(args);

    /*
     * Write the message.
     */
    __libc_logWrite(pInst, fGroupAndFlags, pszMsg, cch, 0);
}


/**
 * Output a log user error message.
 *
 * This may raise a breakpoint exception if configured so. A log message is
 * considered to be one line and is appended a newline if none was given.
 *
 * @param   uEnterTS        The timestamp returned by LogEnter.
 * @param   pvInstance      Logger instance. If NULL the message goes to the
 *                          default log instance.
 * @param   fGroupAndFlags  Logging group and logging flags.
 * @param   pszFunction     Name of the function which was entered.
 * @param   pszFile         Source filename.
 * @param   uLine           Line number.
 * @param   pszFormat       Format string for the message to log.
 * @param   ...             Arguments to the format string.
 */
void     __libc_LogError(unsigned uEnterTS, void *pvInstance, unsigned fGroupAndFlags, const char *pszFunction, const char *pszFile, unsigned uLine, const char *pszFormat, ...)
{
    unsigned        uTS = getTimestamp();
    __LIBC_PTHREAD  pThread = __libc_threadCurrentNoAuto();
    char           *pszMsg;
    int             cch;
    va_list         args;
    __LIBC_PLOGINST pInst;
    unsigned        cDepth = 0xff;

    /*
     * Check instance, get default.
     */
    if (!pvInstance)
    {
        pvInstance = __libc_logDefault();
        if (!pvInstance)
            return;
        if (pThread)
            cDepth = pThread->cDefLoggerDepth;
    }
    pInst = (__LIBC_PLOGINST)pvInstance;

    /*
     * Check if this group is enabled.
     */
    if (pInst->pGroups)
    {
        int iGroup = __LIBC_LOG_GETGROUP(fGroupAndFlags);
        if (    iGroup >= 0
            &&  iGroup < pInst->pGroups->cGroups
            &&  !pInst->pGroups->paGroups[iGroup].fEnabled)
            return;
    }

    /*
     * Allocate logging buffer and format the message.
     */
    pszMsg = alloca(CCHTMPMSGBUFFER);
    if (!pszMsg)
        return;

    /*
     * First message is about where this error occured.
     */
    if (uEnterTS != ~0)
        cch = __libc_logSNPrintf(pszMsg, CCHTMPMSGBUFFER, "%08x %02x %02x %04x ErrM %04x %s (%d ms): %s(%d):\n",
                                 uTS, getTid(), cDepth, __LIBC_LOG_GETGROUP(fGroupAndFlags),
                                 pThread ? pThread->iErrNo : 0xface, pszFunction, uTS - uEnterTS,
                                 pszFile, uLine);
    else
        cch = __libc_logSNPrintf(pszMsg, CCHTMPMSGBUFFER, "%08x %02x %02x %04x ErrM %04x %s: %s(%d):\n",
                                 uTS, getTid(), cDepth, __LIBC_LOG_GETGROUP(fGroupAndFlags),
                                 pThread ? pThread->iErrNo : 0xface, pszFunction,
                                 pszFile, uLine);
    __libc_logWrite(pInst, fGroupAndFlags, pszMsg, cch, 0);

    /*
     * Second message is the one from the caller.
     */
    va_start(args, pszFormat);
    cch = __libc_logBuildMsg(pszMsg, pszFormat, args, "%08x %02x %02x %04x ErrM %04x: ",
                             uTS, getTid(), cDepth, __LIBC_LOG_GETGROUP(fGroupAndFlags),
                             pThread ? pThread->iErrNo : 0xface);
    va_end(args);
    __libc_logWrite(pInst, fGroupAndFlags, pszMsg, cch, 0);
}


/**
 * Output a raw log message.
 * Nothing is prepended. No newline is appended.
 *
 * @param   uEnterTS        The timestamp returned by LogEnter.
 * @param   pvInstance      Logger instance. If NULL the message goes to the
 *                          default log instance.
 * @param   fGroupAndFlags  Logging group and logging flags.
 * @param   pszFunction     Name of the function which was entered.
 * @param   pszString       Pointer to raw log message.
 * @param   cchMax          Maximum number of bytes to write.
 */
void     __libc_LogRaw(void *pvInstance, unsigned fGroupAndFlags, const char *pszString, unsigned cchMax)
{
    int             cch;
    __LIBC_PLOGINST pInst;

    /*
     * Check instance, get default.
     */
    if (!pvInstance)
    {
        pvInstance = __libc_logDefault();
        if (!pvInstance)
            return;
    }
    pInst = (__LIBC_PLOGINST)pvInstance;

    /*
     * Check if this group is enabled.
     */
    if (pInst->pGroups)
    {
        int iGroup = __LIBC_LOG_GETGROUP(fGroupAndFlags);
        if (    iGroup >= 0
            &&  iGroup < pInst->pGroups->cGroups
            &&  !pInst->pGroups->paGroups[iGroup].fEnabled)
            return;
    }

    /*
     * Figure out how much to write.
     */
    cch = strnlen(pszString, cchMax);

    /*
     * Write the message.
     */
    __libc_logWrite(pInst, fGroupAndFlags, pszString, cch, 0);
}


/**
 * Dumps a byte block.
 *
 * @param   uEnterTS        The timestamp returned by LogEnter.
 * @param   pvInstance      Logger instance. If NULL the message goes to the
 *                          default log instance.
 * @param   fGroupAndFlags  Logging group and logging flags.
 * @param   pszFunction     Name of the function which was entered.
 * @param   pvData          Pointer to the bytes to dump.
 * @param   cbData          Number of bytes to dump.
 * @param   pszFormat       Format string for the message to log.
 * @param   ...             Arguments to the format string.
 */
void     __libc_LogDumpHex(unsigned uEnterTS, void *pvInstance, unsigned fGroupAndFlags, const char *pszFunction, void *pvData, unsigned cbData, const char *pszFormat, ...)
{
    unsigned        uTS = getTimestamp();
    __LIBC_PTHREAD  pThread = __libc_threadCurrentNoAuto();
    char           *pszMsg;
    int             cch;
    va_list         args;
    __LIBC_PLOGINST pInst;
    unsigned        cDepth = 0xff;
    char           *pszOffset;
    char           *pbData;
    unsigned        off;

    /*
     * Check instance, get default.
     */
    if (!pvInstance)
    {
        pvInstance = __libc_logDefault();
        if (!pvInstance)
            return;
        if (pThread)
            cDepth = pThread->cDefLoggerDepth;
    }
    pInst = (__LIBC_PLOGINST)pvInstance;

    /*
     * Check if this group is enabled.
     */
    if (pInst->pGroups)
    {
        int iGroup = __LIBC_LOG_GETGROUP(fGroupAndFlags);
        if (    iGroup >= 0
            &&  iGroup < pInst->pGroups->cGroups
            &&  !pInst->pGroups->paGroups[iGroup].fEnabled)
            return;
    }

    /*
     * Allocate logging buffer and format the message.
     */
    pszMsg = alloca(CCHTMPMSGBUFFER);
    if (!pszMsg)
        return;

    va_start(args, pszFormat);
    if (uEnterTS != ~0)
        cch = __libc_logBuildMsg(pszMsg, pszFormat, args, "%08x %02x %02x %04x Dump %04x %s (%d ms): ",
                                 uTS, getTid(), cDepth, __LIBC_LOG_GETGROUP(fGroupAndFlags),
                                 pThread ? pThread->iErrNo : 0xface, pszFunction, uTS - uEnterTS);
    else
        cch = __libc_logBuildMsg(pszMsg, pszFormat, args, "%08x %02x %02x %04x Dump %04x %s: ",
                                 uTS, getTid(), cDepth, __LIBC_LOG_GETGROUP(fGroupAndFlags),
                                 pThread ? pThread->iErrNo : 0xface, pszFunction);
    va_end(args);

    /*
     * Write the message.
     */
    __libc_logWrite(pInst, fGroupAndFlags, pszMsg, cch, 0);


    /*
     * Init the dumping.
     *  (Reusing the first part of the message, skipping past the 'Dump' here.)
     */
    pszOffset = pszMsg;
    while (pszOffset[0] != 'D' && pszOffset[1] != 'u')
        pszOffset++;
    pszOffset += 4;
    *pszOffset++ = ':';
    *pszOffset++ = ' ';

    /*
     * Do the dumping.
     */
    off = 0;
    pbData = (char *)pvData;
    while (cbData > 0)
    {
        char *pszHex, *pszChar, *pszHexEnd;
        /* print offsets. */
        pszHex = pszOffset + __libc_logSNPrintf(pszOffset, 64, "%08x %08x  ", (unsigned)pvData, off); /* !portability! */
        pszHexEnd = pszChar = pszHex + 16 * 3 + 2;  /* 16 chars with on space, two space before chars column. */

        /* output chars. */
        while (cbData > 0)
        {
            char ch = *pbData++;
            cbData--;
            *pszHex++ = gszHexDigits[(unsigned)ch >> 4];
            *pszHex++ = gszHexDigits[(unsigned)ch & 4];
            *pszHex++ = ' ';
            if (ch < 0x30 /*|| more unprintables */)
                *pszChar++ = '.';
            else
                *pszChar++ = ch;
        }

        /* finish it */
        while (pszHex < pszHexEnd)
            *pszHex++ = ' ';
        pszHexEnd[-(8 * 3)] = '-';
        *pszChar++ = '\n';
        *pszChar = '\0';

        /* write line */
        __libc_logWrite(pInst, fGroupAndFlags, pszMsg, pszChar - pszMsg, 0);

        /* next */
        off += 16;
    }
}


/**
 * Assertion helper.
 * Logs and displays (stderr) an assertion failed message.
 *
 * @param   pvInstance      Logger instance. If NULL the message goes to the
 *                          default log instance.
 * @param   pszFunction     Name of the function which was entered.
 * @param   pszFile         Source filename.
 * @param   uLine           Line number.
 * @param   pszExpression   Expression.
 * @param   pszFormat       Format string for the message to log.
 * @param   ...             Arguments to the format string.
 */
void     __libc_LogAssert(void *pvInstance, unsigned fGroupAndFlags,
                          const char *pszFunction, const char *pszFile, unsigned uLine, const char *pszExpression,
                          const char *pszFormat, ...)
{
    static int      fEnabled = -1;
    unsigned        uTS = getTimestamp();
    __LIBC_PTHREAD  pThread = __libc_threadCurrentNoAuto();
    char           *pszMsg;
    int             cch;
    va_list         args;
    __LIBC_PLOGINST pInst;
    unsigned        cDepth = 0xff;
    FS_VAR();

    /*
     * Check if strict is enabled or not.
     * Disabled when LIBC_STRICT_DISABLED is present.
     */
    if (fEnabled == -1)
    {
        PSZ pszValue;
        fEnabled = DosScanEnv((PCSZ)"LIBC_STRICT_DISABLED", &pszValue) != NO_ERROR;
    }

    /*
     * Check instance, get default.
     */
    if (!pvInstance)
    {
        pvInstance = __libc_logDefault();
        if (!pvInstance)
            return;
        if (pThread)
            cDepth = pThread->cDefLoggerDepth;
    }
    pInst = (__LIBC_PLOGINST)pvInstance;

#if 0
    /*
     * Check if this group is enabled.
     */
    if (pInst->pGroups)
    {
        int iGroup = __LIBC_LOG_GETGROUP(fGroupAndFlags);
        if (    iGroup >= 0
            &&  iGroup < pInst->pGroups->cGroups
            &&  !pInst->pGroups->paGroups[iGroup].fEnabled)
            return;
    }
#endif

    /*
     * Allocate logging buffer and format the message.
     * NOTE: This buffer *must* be in low memory!!!
     */
    pszMsg = alloca(CCHTMPMSGBUFFER + 1);
    if (!pszMsg)
        return;

    /*
     * Assertion message.
     */
    FS_SAVE_LOAD();
    va_start(args, pszFormat);          /* make compiler happy we do it here. */
    cch = __libc_logBuildMsg(pszMsg, "", args, "%08x %02x %02x %04x Asrt: Assertion Failed!!!\n", uTS, getTid(), cDepth, __LIBC_LOG_GETGROUP(fGroupAndFlags));
    __libc_logWrite(pInst, fGroupAndFlags, pszMsg, cch, fEnabled);

    cch = __libc_logBuildMsg(pszMsg, "", args, "%08x %02x %02x %04x Asrt: Function: %s\n", uTS, getTid(), cDepth, __LIBC_LOG_GETGROUP(fGroupAndFlags), pszFunction);
    __libc_logWrite(pInst, fGroupAndFlags, pszMsg, cch, fEnabled);

    cch = __libc_logBuildMsg(pszMsg, "", args, "%08x %02x %02x %04x Asrt: File:     %s\n", uTS, getTid(), cDepth, __LIBC_LOG_GETGROUP(fGroupAndFlags), pszFile);
    __libc_logWrite(pInst, fGroupAndFlags, pszMsg, cch, fEnabled);

    cch = __libc_logBuildMsg(pszMsg, "", args, "%08x %02x %02x %04x Asrt: Line:     %d\n", uTS, getTid(), cDepth, __LIBC_LOG_GETGROUP(fGroupAndFlags), uLine);
    __libc_logWrite(pInst, fGroupAndFlags, pszMsg, cch, fEnabled);

    cch = __libc_logBuildMsg(pszMsg, "", args, "%08x %02x %02x %04x Asrt: Expr:     %s\n", uTS, getTid(), cDepth, __LIBC_LOG_GETGROUP(fGroupAndFlags), pszExpression);
    __libc_logWrite(pInst, fGroupAndFlags, pszMsg, cch, fEnabled);

    cch = __libc_logBuildMsg(pszMsg, pszFormat, args, "%08x %02x %02x %04x Asrt: ",        uTS, getTid(), cDepth, __LIBC_LOG_GETGROUP(fGroupAndFlags));
    __libc_logWrite(pInst, fGroupAndFlags, pszMsg, cch, fEnabled);
    va_end(args);

    /*
     * Breakpoint. (IBM debugger: T or Ctrl-T to get to caller)
     */
    if (fEnabled)
    {
        /*
         * Raise a breakpoint here, that's more convenient in the debugger.
         * Just press 'T' and you'll be at the LIBC_ASSERT*.
         */
        EXCEPTIONREPORTRECORD  XRepRec = {0};
        XRepRec.ExceptionNum        = XCPT_BREAKPOINT;
        XRepRec.ExceptionAddress    = (void*)&__libc_LogAssert;
        if (DosRaiseException(&XRepRec))
            __asm__ __volatile__("int $3");
    }
}


/**
 * Exception handler for catching read and write errors.
 *
 * Special registration record which is bundled with a longjump structure
 * so we can jump back to the error handling of the failing code.
 *
 * @returns See cpref
 * @param   pRepRec     See cpref.
 * @param   pRegRec     See cpref.
 * @param   pCtxRec     See cpref.
 * @param   pv          See cpref.
 */
static ULONG _System __libc_logXcptHandler(PEXCEPTIONREPORTRECORD pRepRec, struct _EXCEPTIONREGISTRATIONRECORD * pRegRec, PCONTEXTRECORD pCtxRec, PVOID pv)
{
    switch (pRepRec->ExceptionNum)
    {
        case XCPT_ACCESS_VIOLATION:
        {
            __LIBC_PXCPTREG pXctpRegRec = (__LIBC_PXCPTREG)pRegRec;
            longjmp(pXctpRegRec->jmp, 1);
            break;
        }
    }

    return XCPT_CONTINUE_SEARCH;
}


/**
 * Validate a memory area for read access.
 * @returns 1 if readable.
 * @returns 0 if not entirely readable.
 * @param   pv      Pointer to memory area.
 * @param   cb      Size of memory area.
 */
int      __libc_StrictMemoryR(const void *pv, size_t cb)
{
    int             rc;
    __LIBC_XCPTREG  XcptRegRec = { { (void *)-1, __libc_logXcptHandler }, };
    FS_VAR();

    /*
     * Zero bytes.
     */
    if (!cb)
        return 1;

    FS_SAVE_LOAD();
    DosSetExceptionHandler(&XcptRegRec.RegRec);
    rc = setjmp(XcptRegRec.jmp);
    if (!rc)
    {
        /*
         * Read all the pages in the buffer we just got.
         */
        volatile const char *   pch = (volatile const char*)pv;
        for (;;)
        {
            rc |= *pch;
            pch += PAGE_SIZE;
            if (cb < PAGE_SIZE)
                break;
            cb -= PAGE_SIZE;
        }
        rc = 1;
    }
    else
    {
        /*
         * failure - no specific actions.
         */
        rc = 0;
    }
    DosUnsetExceptionHandler(&XcptRegRec.RegRec);
    FS_RESTORE();

    return rc;
}


/**
 * Validate a memory area for read & write access.
 * @returns 1 if readable and writable.
 * @returns 0 if not entirely readable and writable.
 * @param   pv      Pointer to memory area.
 * @param   cb      Size of memory area.
 */
int      __libc_StrictMemoryRW(void *pv, size_t cb)
{
    int             rc;
    __LIBC_XCPTREG  XcptRegRec = { { (void *)-1, __libc_logXcptHandler }, };
    FS_VAR();

    /*
     * Zero bytes.
     */
    if (!cb)
        return 1;

    FS_SAVE_LOAD();
    DosSetExceptionHandler(&XcptRegRec.RegRec);
    rc = setjmp(XcptRegRec.jmp);
    if (!rc)
    {
        /*
         * Read/Write all the pages in the buffer we just got.
         */
        volatile char *   pch = (volatile char*)pv;
        for (;;)
        {
            rc = *pch;
            *pch = (char)rc;
            pch += PAGE_SIZE;
            if (cb < PAGE_SIZE)
                break;
            cb -= PAGE_SIZE;
        }
        rc = 1;
    }
    else
    {
        /*
         * failure - no specific actions.
         */
        rc = 0;
    }
    DosUnsetExceptionHandler(&XcptRegRec.RegRec);
    FS_RESTORE();

    return rc;
}


/**
 * Validate a zero terminated string for read access.
 * @returns 1 if readable.
 * @returns 0 if not entirely readable.
 * @param   psz         Pointer to string.
 * @param   cchMax      Max string length. Use ~0 if to very all the
 *                      way to the terminator.
 */
int      __libc_StrictStringR(const char *psz, size_t cchMax)
{
    int             rc;
    __LIBC_XCPTREG  XcptRegRec = { { (void *)-1, __libc_logXcptHandler }, };
    FS_VAR();

    /*
     * Zero bytes.
     */
    if (!cchMax)
        return 1;

    FS_SAVE_LOAD();
    DosSetExceptionHandler(&XcptRegRec.RegRec);
    rc = setjmp(XcptRegRec.jmp);
    if (!rc)
    {
        /*
         * Read all the pages in the buffer we just got.
         */
        while (cchMax-- && *psz)
            psz++;
        rc = 1;
    }
    else
    {
        /*
         * failure - no specific actions.
         */
        rc = 0;
    }
    DosUnsetExceptionHandler(&XcptRegRec.RegRec);
    FS_RESTORE();

    return rc;
}



/**
 * Formats a number according to the parameters.
 *
 * @returns   Pointer to next char. (after number)
 * @param     psz            Pointer to output string.
 * @param     lValue         Value
 * @param     uiBase         Number representation base.
 * @param     cchWidth       Width
 * @param     cchPrecision   Precision.
 * @param     fFlags         Flags (NTFS_*).
 */
static char * numtostr(char *psz, long lValue, unsigned int uiBase,
                       int cchWidth, int cchPrecision, unsigned int fFlags)
{
    const char     *pszDigits = gszHexDigits;
    int             cchValue;
    unsigned long   ul;
    int             i;
    int             j;

    if (uiBase < 2 || uiBase > 16)
        return NULL;
    if (fFlags & NTSF_CAPITAL)
        pszDigits = gszHexDigitsUpper;
    if (fFlags & NTSF_LEFT)
        fFlags &= ~NTSF_ZEROPAD;

    /* determin value length */
    cchValue = 0;
    ul = (unsigned long)((fFlags & NTSF_VALSIGNED) && lValue < 0L ? -lValue : lValue);
    do
    {
        cchValue++;
        ul /= uiBase;
    } while (ul > 0);

    i = 0;
    if (fFlags & NTSF_VALSIGNED)
    {
        if (lValue < 0)
        {
            lValue = -lValue;
            psz[i++] = '-';
        }
        else if (fFlags & (NTSF_PLUS | NTSF_BLANK))
            psz[i++] = (char)(fFlags & NTSF_PLUS ? '+' : ' ');
    }

    if (fFlags & NTSF_SPECIAL && (uiBase % 8) == 0)
    {
        psz[i++] = '0';
        if (uiBase == 16)
            psz[i++] = (char)(fFlags & NTSF_CAPITAL ? 'X' : 'x');
    }


    /* width - only if ZEROPAD */
    cchWidth -= i + cchValue;
    if (fFlags & NTSF_ZEROPAD)
        while (--cchWidth >= 0)
        {
            psz[i++] = '0';
            cchPrecision--;
        }
    else if (!(fFlags & NTSF_LEFT) && cchWidth > 0)
    {
        for (j = i-1; j >= 0; j--)
            psz[cchWidth + j] = psz[j];
        for (j = 0; j < cchWidth; j++)
            psz[j] = ' ';
        i += cchWidth;
    }

    psz += i;

    /* percision */
    while (--cchPrecision >= cchValue)
        *psz++ = '0';

    /* write number - not good enough but it works */
    i = -1;
    psz += cchValue;
    do
    {
        psz[i--] = pszDigits[lValue % uiBase];
        lValue /= uiBase;
    } while (lValue > 0);

    /* width if NTSF_LEFT */
    if (fFlags & NTSF_LEFT)
        while (--cchWidth >= 0)
            *psz++ = ' ';


    return psz;
}


/**
 * Formats a number according to the parameters.
 *
 * @returns   Pointer to next char. (after number)
 * @param     psz            Pointer to output string.
 * @param     llValue        Value (64 bit).
 * @param     uiBase         Number representation base.
 * @param     cchWidth       Width.
 * @param     cchPrecision   Precision.
 * @param     fFlags         Flags (NTFS_*).
 */
static char * llnumtostr(char *psz, long long llValue, unsigned int uiBase,
                         int cchWidth, int cchPrecision, unsigned int fFlags)
{
    const char         *pszDigits = gszHexDigits;
    int                 cchValue;
    unsigned long long  ull;
    int                 i;
    /*int                 j;*/

    if (uiBase < 2 || uiBase > 16)
        return NULL;
    if (fFlags & NTSF_CAPITAL)
        pszDigits = gszHexDigitsUpper;
    if (fFlags & NTSF_LEFT)
        fFlags &= ~NTSF_ZEROPAD;

    /* determin value length */
    cchValue = 0;
    ull = (unsigned long long)((fFlags & NTSF_VALSIGNED) && llValue < 0L ? -llValue : llValue);
    do
    {
        cchValue++;
        ull /= uiBase;
    } while (ull > 0);

    i = 0;
    if (fFlags & NTSF_VALSIGNED)
    {
        if (llValue < 0)
        {
            llValue = -llValue;
            psz[i++] = '-';
        }
        else if (fFlags & (NTSF_PLUS | NTSF_BLANK))
            psz[i++] = (char)(fFlags & NTSF_PLUS ? '+' : ' ');
    }

    if (fFlags & NTSF_SPECIAL && (uiBase % 8) == 0)
    {
        psz[i++] = '0';
        if (uiBase == 16)
            psz[i++] = (char)(fFlags & NTSF_CAPITAL ? 'X' : 'x');
    }


    /* width - only if ZEROPAD */
    cchWidth -= i + cchValue;
    if (fFlags & NTSF_ZEROPAD)
        while (--cchWidth >= 0)
        {
            psz[i++] = '0';
            cchPrecision--;
        }
    #if 0  /** @todo */
    else if (!(fFlags & NTSF_LEFT) && cchWidth > 0)
    {
        for (j = i-1; j >= 0; j--)
            psz[cchWidth + j] = psz[j];
        for (j = 0; j < cchWidth; j++)
            psz[j] = ' ';
        i += cchWidth;
    }
    #endif

    psz += i;

    /* percision */
    while (--cchPrecision >= cchValue)
        *psz++ = '0';

    /* write number - not good enough but it works */
    i = -1;
    psz += cchValue;
    do
    {
        psz[i--] = pszDigits[llValue % uiBase];
        llValue /= uiBase;
    } while (llValue > 0);

    /* width if NTSF_LEFT */
    if (fFlags & NTSF_LEFT)
        while (--cchWidth >= 0)
            *psz++ = ' ';

    return psz;
}



/**
 * __libc_logVSNPrintf worker.
 *
 * @returns number of bytes formatted.
 * @param   pszBuffer   Where to put the the formatted string.
 * @param   cchBuffer   Size of the buffer.
 * @param   pszFormat   Format string.
 * @param   args        Argument list.
 */
static int      __libc_logVSNPrintfInt(char *pszBuffer, size_t cchBuffer, const char *pszFormat, va_list args)
{
    int cch = 0;
    while (*pszFormat != '\0' && cchBuffer)
    {
        if (*pszFormat == '%')
        {
            pszFormat++;  /* skip '%' */
            if (*pszFormat == '%')    /* '%%'-> '%' */
            {
                *pszBuffer++ = '%';
                cchBuffer--;
                pszFormat++;
                cch++;
            }
            else
            {
                unsigned int fFlags = 0;
                int          cchWidth = -1;
                int          cchPrecision = -1;
                unsigned int uiBase = 10;
                char         chArgSize;

                /* flags */
                for (;;)
                {
                    switch (*pszFormat++)
                    {
                        case '#':   fFlags |= NTSF_SPECIAL; continue;
                        case '-':   fFlags |= NTSF_LEFT; continue;
                        case '+':   fFlags |= NTSF_PLUS; continue;
                        case ' ':   fFlags |= NTSF_BLANK; continue;
                        case '0':   fFlags |= NTSF_ZEROPAD; continue;
                    }
                    pszFormat--;
                    break;
                }
                /* width */
                if (ISDIGIT(*pszFormat))
                {
                    for (cchWidth = 0; ISDIGIT(*pszFormat); pszFormat++)
                    {
                        cchWidth *= 10;
                        cchWidth += *pszFormat - '0';
                    }
                }
                else if (*pszFormat == '*')
                {
                    pszFormat++;
                    cchWidth = va_arg(args, int);
                    if (cchWidth < 0)
                    {
                        cchWidth = -cchWidth;
                        fFlags |= NTSF_LEFT;
                    }
                }

                /* precision */
                if (*pszFormat == '.')
                {
                    pszFormat++;
                    if (ISDIGIT(*pszFormat))
                    {
                        for (cchPrecision = 0; ISDIGIT(*pszFormat); pszFormat++)
                        {
                            cchPrecision *= 10;
                            cchPrecision += *pszFormat - '0';
                        }

                    }
                    else if (*pszFormat == '*')
                    {
                        pszFormat++;
                        cchPrecision = va_arg(args, int);
                    }
                    if (cchPrecision < 0)
                        cchPrecision = 0;
                }

                /* argsize */
                chArgSize = *pszFormat;
                if (    chArgSize != 'l' && chArgSize != 'L' && chArgSize != 'h'  && chArgSize != 'H'
                    &&  chArgSize != 'j' && chArgSize != 'z' && chArgSize != 't')
                    chArgSize = 0;
                else
                {
                    pszFormat++;
                    if (*pszFormat == 'l' && chArgSize == 'l')
                    {
                        chArgSize = 'L';
                        pszFormat++;
                    }
                    else if (*pszFormat == 'h' && chArgSize == 'h')
                    {
                        chArgSize = 'H';
                        pszFormat++;
                    }
                }

                /* type */
                switch (*pszFormat++)
                {
                    /* char */
                    case 'c':
                    {
                        char ch;

                        if (!(fFlags & NTSF_LEFT))
                            while (--cchWidth > 0 && cchBuffer)
                            {
                                cchBuffer--;
                                *pszBuffer++ = ' ';
                                cch++;
                            }

                        ch = (char)va_arg(args, int);
                        if (!cchBuffer)
                            break;

                        cchBuffer--;
                        *pszBuffer++ = ch;
                        cch++;

                        while (--cchWidth > 0 && cchBuffer)
                        {
                            cchBuffer--;
                            *pszBuffer++ = ' ';
                            cch++;
                        }
                        continue;
                    }

                    case 's':   /* string */
                    {
                        int   cchStr;
                        char *pszStr = va_arg(args, char*);

                        if (pszStr < (char*)0x10000)
                            pszStr = "<NULL>";
                        cchStr = strnlen(pszStr, (unsigned)cchPrecision);
                        if (!(fFlags & NTSF_LEFT))
                            while (--cchWidth >= cchStr && cchBuffer)
                            {
                                cchBuffer--;
                                *pszBuffer++ = ' ';
                                cch++;
                            }

                        while (cchStr && cchBuffer)
                        {
                            cchStr--;
                            cchBuffer--;
                            *pszBuffer++ = *pszStr++;
                            cch++;
                        }

                        while (--cchWidth >= cchStr && cchBuffer)
                        {
                            cchBuffer--;
                            *pszBuffer++ = ' ';
                            cch++;
                        }
                        continue;
                    }

                    /*-----------------*/
                    /* integer/pointer */
                    /*-----------------*/
                    case 'd':
                    case 'i':
                    case 'o':
                    case 'p':
                    case 'u':
                    case 'x':
                    case 'X':
                    case 'L':
                    {
                        char    achNum[64]; /* FIXME */
                        char   *pszNum;
                        int     cchNum;

                        switch (pszFormat[-1])
                        {
                            case 'd': /* signed decimal integer */
                            case 'i':
                                fFlags |= NTSF_VALSIGNED;
                                break;

                            case 'o':
                                uiBase = 8;
                                break;

                            case 'p':
                                fFlags |= NTSF_SPECIAL | NTSF_ZEROPAD; /* Note not standard behaviour (but I like it this way!) */
                                uiBase = 16;
                                if (cchWidth < 0)
                                    cchWidth = sizeof(char *) * 2 + 2;
                                break;

                            case 'u':
                                uiBase = 10;
                                break;

                            case 'X':
                                fFlags |= NTSF_CAPITAL;
                            case 'x':
                                uiBase = 16;
                                break;

                            case 'L':
                                uiBase = 10;
                                chArgSize = 'L';
                                break;
                        }

                        pszNum = &achNum[0];
                        if (chArgSize == 'L')
                        {
                            unsigned long long ullValue;
                            ullValue = va_arg(args, unsigned long long);
                            cchNum = llnumtostr(pszNum, ullValue, uiBase, cchWidth, cchPrecision, fFlags) - pszNum;
                        }
                        else
                        {
                            unsigned long ulValue;
                            if (pszFormat[-1] == 'p')
                                ulValue = (unsigned long)va_arg(args, char *);
                            else if (chArgSize == 'l')
                                ulValue = va_arg(args, signed long);
                            else if (chArgSize == 'h')
                                ulValue = va_arg(args, signed /*short*/ int); /* the great GCC pedantically saith use int. */
                            else if (chArgSize == 'H')
                                ulValue = va_arg(args, /* int8_t */ int); /* the great GCC pedantically saith use int. */
                            else if (chArgSize == 'j')
                                ulValue = va_arg(args, intmax_t);
                            else if (chArgSize == 'z')
                                ulValue = va_arg(args, size_t);
                            else if (chArgSize == 't')
                                ulValue = va_arg(args, ptrdiff_t);
                            else
                                ulValue = va_arg(args, signed int);
                            cchNum = numtostr(pszNum, ulValue, uiBase, cchWidth, cchPrecision, fFlags) - pszNum;
                        }

                        for (; cchNum && cchBuffer; cchNum--, cchBuffer--, pszBuffer++, pszNum++, cch++)
                            *pszBuffer = *pszNum;
                        continue;
                    }

                    /* extensions. */
                    case 'Z':
                    {
                        pszFormat++;
                        switch (*pszFormat++)
                        {
                            /* hex dump */
                            case 'd':
                            {
                                const char *pb = va_arg(args, const char *);
                                int cb = cchPrecision;
                                if (cb <= 0)
                                    cb = cchWidth;
                                if (cb <= 0)
                                    cb = 4;
                                if (pb < (const char *)0x10000)
                                {
                                    if (cb && cchBuffer >= 2)
                                    {
                                        char ch = *pb++;
                                        *pszBuffer++ = gszHexDigits[(unsigned)ch >> 4];
                                        *pszBuffer++ = gszHexDigits[(unsigned)ch & 0xf];
                                        cb--;
                                        cchBuffer -= 2;
                                        cch += 2;

                                        while (cb && cchBuffer >= 3)
                                        {
                                            ch = *pb++;
                                            cb--;
                                            *pszBuffer++ = ' ';
                                            *pszBuffer++ = gszHexDigits[(unsigned)ch >> 4];
                                            *pszBuffer++ = gszHexDigits[(unsigned)ch & 0xf];
                                            cchBuffer -= 3;
                                            cch += 3;
                                        }
                                    }
                                }
                                else
                                {
                                    pb = "<NULL>";
                                    cb = 6;
                                    while (cb && cchBuffer)
                                    {
                                        *pszBuffer++ = *pb++;
                                        cb--;
                                        cchBuffer--;
                                        cch++;
                                    }
                                }
                                break;
                            }

                            default:
                                continue;
                        }
                    }


                    default:
                        continue;
                }
            }
        }
        else
        {
            *pszBuffer++ = *pszFormat++;
            cchBuffer--;
            cch++;
        }
    }

    /*
     * Terminator.
     */
    if (cchBuffer)
        *pszBuffer = '\0';
    else /* Use last buffer entry */
    {
        pszBuffer[-1] = '\0';
        cch--;
    }

    return cch;
}


/**
 * Partial vsprintf implementation.
 *
 * @returns number of bytes formatted.
 * @param   pszBuffer   Where to put the the formatted string.
 * @param   cchBuffer   Size of the buffer.
 * @param   pszFormat   Format string.
 * @param   args        Argument list.
 */
static int      __libc_logVSNPrintf(char *pszBuffer, size_t cchBuffer, const char *pszFormat, va_list args)
{
    int rc;
    int cch;
    __LIBC_XCPTREG  XcptRegRec = { { (void *)-1, __libc_logXcptHandler }, };
    FS_VAR();

    /* Ignore NULL format string. */
    if (!pszFormat)
    {
        if (cchBuffer)
            *pszBuffer = '\0';
        return 0;
    }

    /* Get along with the formatting. */
    FS_SAVE_LOAD();
    DosSetExceptionHandler(&XcptRegRec.RegRec);
    rc = setjmp(XcptRegRec.jmp);
    if (!rc)
    {
        cch = __libc_logVSNPrintfInt(pszBuffer, cchBuffer, pszFormat, args);
        DosUnsetExceptionHandler(&XcptRegRec.RegRec);
    }
    else
    {
        /*
         * Exception during formatting.
         *
         * We unset the exception handler to make sure we will not
         * mess up in loop...
         */
        const char *pszMsg = "!!!Format Exception!!! ";
        DosUnsetExceptionHandler(&XcptRegRec.RegRec);
        cch = 0;
        while (*pszMsg && cchBuffer)
        {
            *pszBuffer++ = *pszMsg++;
            cch++;
            cchBuffer--;
        }
        if (__libc_StrictStringR(pszFormat, cchBuffer))
        {
            while (*pszFormat && cchBuffer)
            {
                *pszBuffer++ = *pszFormat++;
                cch++;
                cchBuffer--;
            }
        }
        if (!cchBuffer)
            pszBuffer[-1] = '\0';
    }

    FS_RESTORE();
    return cch;
}


/**
 * Partial vsprintf implementation.
 *
 * @returns number of bytes formatted.
 * @param   pszBuffer   Where to put the the formatted string.
 * @param   cchBuffer   Size of the buffer.
 * @param   pszFormat   Format string.
 * @param   ...         Format arguments.
 */
static int      __libc_logSNPrintf(char *pszBuffer, size_t cchBuffer, const char *pszFormat, ...)
{
    va_list args;
    int     cch;
    va_start(args, pszFormat);
    cch = __libc_logVSNPrintf(pszBuffer, cchBuffer, pszFormat, args);
    va_end(args);
    return cch;
}


/**
 * Procedure invoked by the parent for aligning the log file handle
 * and semaphore handle with the parent ones.
 *
 * @returns 0 on success.
 * @returns -errno on failure.
 * @param pForkHandle   The fork handle
 * @param pvArg         Pointer to a __LIBC_LOGINST containing the parent handles.
 * @param cbArg         sizeof(__LIBC_LOGINST).
 */
static int __libc_logForkChildRelocate(__LIBC_PFORKHANDLE pForkHandle, void *pvArg, size_t cbArg)
{
    const __LIBC_LOGINST *pParent = (const __LIBC_LOGINST *)pvArg;

    /*
     * Make sure there is a log handle.
     */
    __LIBC_PLOGINST pChild = gpDefault;
    if (!pChild)
    {
        pChild = __libc_logDefault();
        if (!pChild)
            return 0;
    }

    /*
     * The file handle.
     */
    if (    pParent->hFile != pChild->hFile
        &&  pParent->hFile >= 0
        &&  pChild->hFile  >= 0)
    {
        HFILE hNew = pParent->hFile;
        APIRET rc = DosDupHandle(pChild->hFile, &hNew);
        if (rc == ERROR_TOO_MANY_OPEN_FILES)
        {
            DosSetMaxFH(pParent->hFile + 1);
            hNew = pParent->hFile;
            rc = DosDupHandle(pChild->hFile, &hNew);
        }
        if (rc == NO_ERROR)
        {
            /* mark it non-inherit */
            ULONG fulFlags = ~0U;
            rc = DosQueryFHState(hNew, &fulFlags);
            if (rc == NO_ERROR)
            {
                fulFlags |= OPEN_FLAGS_NOINHERIT;
                fulFlags &= OPEN_FLAGS_WRITE_THROUGH | OPEN_FLAGS_FAIL_ON_ERROR | OPEN_FLAGS_NO_CACHE | OPEN_FLAGS_NOINHERIT; /* Turn off non-participating bits. */
                rc = DosSetFHState(hNew, fulFlags);
            }
            if (rc == NO_ERROR)
                __libc_LogMsg(0, pChild, 0xffff, __FUNCTION__,
                              "Successfully relocated the log handle from %ld to %ld\n", pChild->hFile, hNew);
            else
                __libc_LogError(0, pChild, 0xffff, __FUNCTION__, __FILE__, __LINE__,
                                "Failed to modify the inherit flag of %ld. rc=%ld. fulFlags=%lx\n", hNew, rc, fulFlags);

            /* replace. */
            DosClose(pChild->hFile);
            pChild->hFile = hNew;

        }
        else
            __libc_LogError(0, pChild, 0xffff, __FUNCTION__, __FILE__, __LINE__,
                            "DosDupHandle(%ld, %ld) -> %ld\n", pChild->hFile, pParent->hFile, rc);
    }

    /*
     * The mutex handle.
     */
    if (    pParent->hmtx != pChild->hmtx
        &&  pParent->hmtx != NULLHANDLE
        &&  pChild->hmtx  != NULLHANDLE)
    {
        APIRET rc = NO_ERROR;
        if ((pParent->hmtx & 0xffff) < 0x3000) /* I'm so sorry, but if it's higher than 0x3000 we can't run the risk of exhausting the stack. */
        {
            /*
             * Mutex handles are allocated in ascending order, so we'll have to allocate
             * mutexes until we get or pass the desired handle.
             */
            struct __LIBC_LOGFORKMTXRELOC
            {
                struct __LIBC_LOGFORKMTXRELOC *pNext;
                int         iCur;
                HMTX        ahmtx[128];
            }  *pHead = NULL,
               *pNew;
            for (;;)
            {
                HMTX hmtx;
                rc = DosCreateMutexSem(NULL, &hmtx, 0, FALSE);
                if (rc != NO_ERROR)
                    break;
                if (hmtx == pParent->hmtx)
                {
                    __libc_LogMsg(0, pChild, 0xffff, __FUNCTION__,
                                  "Successfully relocated the log mutex from %ld to %ld\n", pChild->hmtx, hmtx);
                    DosCloseMutexSem(pChild->hmtx);
                    pChild->hmtx = hmtx;
                    break;
                }
                if (hmtx > pParent->hmtx)
                    break;

                /* record it */
                if (    pHead == NULL
                    ||  pHead->iCur >= sizeof(pNew->ahmtx) / sizeof(pNew->ahmtx[0]))
                {
                    pNew = alloca(sizeof(*pNew));
                    if (!pNew)
                    {
                        DosCloseMutexSem(hmtx);
                        break;
                    }
                    pNew->iCur = 0;
                    pNew->pNext = pHead;
                    pHead = pNew;
                }
                pHead->ahmtx[pHead->iCur++] = hmtx;
            }

            /* Cleanup */
            while (pHead)
            {
                while (pHead->iCur-- > 0)
                    DosCloseMutexSem(pHead->ahmtx[pHead->iCur]);
                pHead = pHead->pNext;
            }
        }
        if (pChild->hmtx != pParent->hmtx)
            __libc_LogError(0, pChild, 0xffff, __FUNCTION__, __FILE__, __LINE__,
                            "Failed to relocate the mutex.  parent=%#lx child=%#lx rc=%ld\n", pParent->hmtx, pChild->hmtx, rc);
    }

    /* We don't fail the fork() even if the log relocation fails. */
    return 0;
}


/**
 * Fork callback which will make sure the child log handles are reallocated
 * so they are identical to the ones used in the parent.
 *
 * @returns 0 on success.
 * @returns -errno on failure.
 * @param   pForkHandle     Pointer to fork handle.
 * @param   enmOperation    Fork operation.
 */
int __libc_logForkParent(__LIBC_PFORKHANDLE pForkHandle, __LIBC_FORKOP enmOperation)
{
    if (    enmOperation == __LIBC_FORK_OP_EXEC_PARENT
        &&  gpDefault)
        return pForkHandle->pfnInvoke(pForkHandle, __libc_logForkChildRelocate, gpDefault, sizeof(*gpDefault));
    return 0;
}

_FORK_PARENT1(0xfffffff0, __libc_logForkParent)

