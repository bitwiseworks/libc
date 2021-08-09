/* $Id: b_panic.c 3848 2014-03-16 21:15:48Z bird $ */
/** @file
 *
 * LIBC SYS Backend - panic.
 *
 * Copyright (c) 2005-2006 knut st. osmundsen <bird@anduin.net>
 *
 *
 * This file is part of InnoTek LIBC.
 *
 * InnoTek LIBC is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * InnoTek LIBC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with InnoTek LIBC; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */



/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_NOGROUP
#define INCL_BASE
#define INCL_ERRORS
#define INCL_FSMACROS
#define INCL_DOSUNDOCUEMENTED
#define INCL_EXAPIS
#define _GNU_SOURCE
#include <os2emx.h>
#include <string.h>
#include <InnotekLIBC/backend.h>
#include <InnotekLIBC/sharedpm.h>
#include <InnotekLIBC/logstrict.h>


/*******************************************************************************
*   Defined Constants And Macros                                               *
*******************************************************************************/
/** OS/2 default standard error handle. */
#define HFILE_STDERR    ((HFILE)2)

#define CHLOWER(ch)     ((ch) < 'A' || (ch) > 'Z' ? (ch) : (ch) + ('a' - 'A'))


/**
 * Calculates the length of a string.
 * @returns String length in bytes.
 * @param   psz     Pointer to the string.
 */
static size_t panicStrLen(const char *psz)
{
    const char *pszEnd = psz;
    while (*pszEnd)
        pszEnd++;
    return pszEnd - psz;
}

/**
 * Compare and advance if found.
 *
 * @returns 1 if match.
 * @returns 0 if no match.
 */
static int panicStrICmp(const char **ppsz, const char *pszLower, const char *pszUpper)
{
    const char *psz = *ppsz;
    while (*psz == *pszLower || *psz == *pszUpper)
    {
        psz++;
        pszUpper++;
        if (!*pszUpper)
        {
            *ppsz = psz;
            return 1;
        }
        pszLower++;
    }
    return 0;
}


/**
 * Termination time hex formatter.
 *
 * @returns number of bytes formatted.
 * @param   pszBuf  Where to store the formatted number.
 * @param   u       Number to format.
 * @param   cDigits Minimum number of digits in the output. Use 0 for whatever fits.
 * @param   fPutNul 1 to terminate the string with '\0'.
 */
static int panicHex2(char *pszBuf, unsigned u, unsigned cDigits, int fPutNul)
{
    static const char szHex[17] = "0123456789abcdef";
    if (u <= 0xf && cDigits <= 1)
    {
        pszBuf[0] = szHex[u & 0xf];
        if (fPutNul)
            pszBuf[1] = '\0';
        return 1;
    }
    if (u <= 0xff && cDigits <= 2)
    {
        pszBuf[0] = szHex[u >> 4];
        pszBuf[1] = szHex[u & 0xf];
        if (fPutNul)
            pszBuf[2] = '\0';
        return 2;
    }
    else if (u <= 0xffff && cDigits <= 4)
    {
        pszBuf[0] = szHex[u >> 12];
        pszBuf[1] = szHex[(u >> 8) & 0xf];
        pszBuf[2] = szHex[(u >> 4) & 0xf];
        pszBuf[3] = szHex[u & 0xf];
        if (fPutNul)
            pszBuf[4] = '\0';
        return 4;
    }
    else
    {
        pszBuf[0] = szHex[u >> 28];
        pszBuf[1] = szHex[(u >> 24) & 0xf];
        pszBuf[2] = szHex[(u >> 20) & 0xf];
        pszBuf[3] = szHex[(u >> 16) & 0xf];
        pszBuf[4] = szHex[(u >> 12) & 0xf];
        pszBuf[5] = szHex[(u >>  8) & 0xf];
        pszBuf[6] = szHex[(u >>  4) & 0xf];
        pszBuf[7] = szHex[u & 0xf];
        if (fPutNul)
            pszBuf[8] = '\0';
        return 8;
    }
}


/**
 * Termination time hex formatter.
 *
 * Equivalent to panicHex2(pszBuf, u, cDigits, 1).
 */
static inline int panicHex(char *pszBuf, unsigned u, unsigned cDigits)
{
    return panicHex2(pszBuf, u, cDigits, 1);
}


/**
 * Print a panic message and dump/kill the process.
 *
 * @param   fFlags      A combination of the __LIBC_PANIC_* defines.
 * @param   pvCtx       Pointer to a context record if available. This is a PCONTEXTRECORD.
 * @param   pszFormat   User message which may contain %s and %x.
 * @param   ...         String pointers and unsigned intergers as specified by the %s and %x in pszFormat.
 */
void __libc_Back_panic(unsigned fFlags, void *pvCtx, const char *pszFormat, ...)
{
    va_list args;
    va_start(args, pszFormat);
    __libc_Back_panicV(fFlags, pvCtx, pszFormat, args);
    va_end(args);
}


/**
 * Print a panic message and dump/kill the process.
 *
 * @param   fFlags      A combination of the __LIBC_PANIC_* defines.
 * @param   pvCtx       Pointer to a context record (or exception parameter list) if available.
 *                      This is a PCONTEXTRECORD (or PXCPTPARAMS if __LIBC_PANIC_XCPTPARAMS is set in fFlags).
 * @param   pszFormat   User message which may contain %s and %x.
 * @param   args        String pointers and unsigned intergers as specified by the %s and %x in pszFormat.
 */
void __libc_Back_panicV(unsigned fFlags, void *pvCtx, const char *pszFormat, va_list args)
{
    LIBCLOG_ENTER("fFlags=%#x pvCtx=%p pszFormat=%p:{%s}\n", fFlags, pvCtx, (void *)pszFormat, pszFormat);

    FS_VAR();
    FS_SAVE_LOAD();
    PTIB pTib;
    PPIB pPib;
    DosGetInfoBlocks(&pTib, &pPib);

    /*
     * First, check if we have an EXCEPTQ DLL available.
     */
    BOOL fHaveExceptq = FALSE;
    void APIENTRY (*SetExceptqOptions)(const char* pszOptions,
                                       const char* pszReportInfo) = NULL;
    ULONG APIENTRY (*ExceptqHandler)(EXCEPTIONREPORTRECORD* pExRepRec,
                                     EXCEPTIONREGISTRATIONRECORD* pExRegRec,
                                     CONTEXTRECORD* pCtxRec,
                                     void* p) = NULL;
    {
        HMODULE hmod = NULLHANDLE;
        if (!DosLoadModuleEx(NULL, 0, (PSZ)"EXCEPTQ", &hmod))
        {
            DosQueryProcAddr(hmod, 0, (PSZ)"SetExceptqOptions", (PPFN)&SetExceptqOptions);
            DosQueryProcAddr(hmod, 0, (PSZ)"MYHANDLER", (PPFN)&ExceptqHandler);
            fHaveExceptq = !!SetExceptqOptions && !!ExceptqHandler;
        }
    }

    /*
     * Set the exit reason in SPM.
     */
    if (!(fFlags & __LIBC_PANIC_NO_SPM_TERM))
        __libc_spmTerm(__LIBC_EXIT_REASON_KILL, 127);

    /*
     * Get the panic config.
     */
    BOOL fQuiet = FALSE;
    BOOL fVerbose = TRUE;
    BOOL fBreakpoint = DosSysCtl(DOSSYSCTL_AM_I_DEBUGGED, NULL) == TRUE;
    BOOL fDumpProcess = FALSE;

    const char *pszPanicCfg = NULL;
    int rc = DosScanEnv((PCSZ)"LIBC_PANIC", (PSZ *)(void *)&pszPanicCfg);
    if (!rc && pszPanicCfg && strnlen(pszPanicCfg, 512) < 512)
    {
        int c = 512;
        while (c-- > 0 && *pszPanicCfg)
        {
            if (panicStrICmp(&pszPanicCfg, "quiet", "QUIET"))
                fQuiet = TRUE;
            else if (panicStrICmp(&pszPanicCfg, "noquiet", "NOQUIET"))
                fQuiet = FALSE;
            else if (panicStrICmp(&pszPanicCfg, "terse", "TERSE"))
                fVerbose = FALSE;
            else if (panicStrICmp(&pszPanicCfg, "verbose", "VERBOSE"))
                fVerbose = TRUE;
            else if (panicStrICmp(&pszPanicCfg, "breakpoint", "BREAKPOINT"))
                fBreakpoint = TRUE;
            else if (panicStrICmp(&pszPanicCfg, "nobreakpoint", "NOBREAKPOINT"))
                fBreakpoint = FALSE;
            else if (panicStrICmp(&pszPanicCfg, "dump", "DUMP"))
                fDumpProcess = TRUE;
            else if (panicStrICmp(&pszPanicCfg, "nodump", "NODUMP"))
                fDumpProcess = FALSE;
            else if (panicStrICmp(&pszPanicCfg, "noexceptq", "NOEXCEPTQ"))
                fHaveExceptq = FALSE;
            else
                pszPanicCfg++;
        }
    }

    ULONG   cb;
    char    szHexNum[12];
    char    szMsg[80]; /* 80 chars - current EXCEPTQ limitation... */
    size_t  cbMsg = 0;
    char    szPathBuf[CCHMAXPATH];

    if (!fQuiet || fHaveExceptq)
    {
        /*
         * Write user message to stderr.
         */
#define TO_CON 0x1
#define TO_BUF 0x2
#define TO_BOTH (TO_CON | TO_BUF)
#define PRINT_BUF_(to, buf, size) do { \
    if ((to) & TO_CON) DosWrite(HFILE_STDERR, (buf), (size), &cb); \
    if (((to) & TO_BUF) && cbMsg < sizeof(szMsg) - 1) { memcpy(szMsg + cbMsg, (buf), (size)); cbMsg += (size); szMsg[cbMsg] = 0; } \
} while (0)
/* Compatibility */
#define PRINT_CHAR(ch)  PRINT_BUF_(TO_CON, &ch,      1)
#define PRINT_C(msg)    PRINT_BUF_(TO_CON, msg,      sizeof(msg) - 1)
#define PRINT_P(msg)    PRINT_BUF_(TO_CON, msg,      panicStrLen(msg))
#define PRINT_H(hex)    PRINT_BUF_(TO_CON, szHexNum, panicHex(szHexNum, hex, 0))
#define PRINT_H16(hex)  PRINT_BUF_(TO_CON, szHexNum, panicHex(szHexNum, hex, 4))
#define PRINT_H32(hex)  PRINT_BUF_(TO_CON, szHexNum, panicHex(szHexNum, hex, 8))
/* Flexible */
#define PRINT_CHAR_(to, ch) PRINT_BUF_(to, &ch,      1)
#define PRINT_C_(to, msg)   PRINT_BUF_(to, msg,      sizeof(msg) - 1)
#define PRINT_P_(to, msg)   PRINT_BUF_(to, msg,      panicStrLen(msg))
#define PRINT_H_(to, hex)   PRINT_BUF_(to, szHexNum, panicHex(szHexNum, hex, 0))
#define PRINT_H16_(to, hex) PRINT_BUF_(to, szHexNum, panicHex(szHexNum, hex, 4))
#define PRINT_H32_(to, hex) PRINT_BUF_(to, szHexNum, panicHex(szHexNum, hex, 8))
        if (!(fFlags & __LIBC_PANIC_SIGNAL) && fVerbose)
            PRINT_C("\r\nLIBC PANIC!!\r\n");
        else
            PRINT_C("\r\n");
        if (!(fFlags & __LIBC_PANIC_SIGNAL))
            PRINT_C_(TO_BUF, "LIBC PANIC!! ");
        else
            PRINT_C_(TO_BUF, "LIBC: ");
        char ch;
        while ((ch = *pszFormat++) != '\0')
        {
            switch (ch)
            {
                case '\n':
                    PRINT_C_(TO_BOTH, "\r\n");
                    break;
                case '\r':
                    break;
                case '%':
                    ch = *pszFormat++;
                    switch (ch)
                    {
                        /* string */
                        case 's':
                        {
                            const char *psz = va_arg(args, const char *);
                            if ((uintptr_t)psz >= 0x10000 && (uintptr_t)psz < 0xe0000000)
                                PRINT_P_(TO_BOTH, psz);
                            else if (!psz)
                                PRINT_C_(TO_BOTH, "<null>");
                            else
                            {
                                PRINT_C_(TO_BOTH, "<bogus string ptr 0x");
                                PRINT_H_(TO_BOTH, (uintptr_t)psz);
                                PRINT_C_(TO_BOTH, ">");
                            }
                            break;
                        }

                        /* it's all hex to me */
                        case 'd':
                        case 'x':
                        case 'u':
                        {
                            unsigned u = va_arg(args, unsigned);
                            PRINT_C_(TO_BOTH, "0x");
                            PRINT_H_(TO_BOTH, u);
                            break;
                        }

                        case 'p':
                        {
                            uintptr_t u = va_arg(args, uintptr_t);
                            PRINT_H_(TO_BOTH, u);
                            break;
                        }

                        case '%':
                            PRINT_C_(TO_BOTH, "%");
                            break;

                        default:
                        {
                            PRINT_C_(TO_BOTH, "<Unknown format %");
                            PRINT_CHAR_(TO_BOTH, ch);
                            PRINT_C_(TO_BOTH, " arg 0x");
                            unsigned u = va_arg(args, unsigned);
                            PRINT_H_(TO_BOTH, u);
                            PRINT_C_(TO_BOTH, ">");
                            break;
                        }
                    }
                    break;

                default:
                    PRINT_CHAR_(TO_BOTH, ch);
                    break;
            }
        } /* print loop */

        if (fVerbose)
        {
            /*
             * Write generic message to stderr.
             */

            /* pid, tid and stuff */
            PRINT_C("pid=0x");   PRINT_H16(pPib->pib_ulpid);
            PRINT_C(" ppid=0x"); PRINT_H16(pPib->pib_ulppid);
            PRINT_C(" tid=0x");  PRINT_H16(pTib->tib_ptib2->tib2_ultid);
            PRINT_C(" slot=0x"); PRINT_H16(pTib->tib_ordinal);
            PRINT_C(" pri=0x");  PRINT_H16(pTib->tib_ptib2->tib2_ulpri);
            PRINT_C(" mc=0x");   PRINT_H16(pTib->tib_ptib2->tib2_usMCCount);
            PRINT_C(" ps=0x");   PRINT_H16(pPib->pib_flstatus);

            /* executable name. */
            if (!DosQueryModuleName(pPib->pib_hmte, sizeof(szPathBuf), &szPathBuf[0]))
            {
                szPathBuf[CCHMAXPATH - 1] = '\0';
                PRINT_C("\r\n");
                PRINT_P(szPathBuf);
            }

            /*
             * Context dump
             */
            if (pvCtx)
            {
                PCONTEXTRECORD pCtx = (PCONTEXTRECORD)pvCtx;

                /* the module name */
                if (pCtx->ctx_RegEip >= 0x10000)
                {
                    HMODULE hmod;
                    ULONG iObj = 0;
                    ULONG offObj = 0;
                    if (!DosQueryModFromEIP(&hmod, &iObj, sizeof(szPathBuf), szPathBuf, &offObj, pCtx->ctx_RegEip))
                    {
                        PRINT_C("\r\n");
                        PRINT_P(szPathBuf);
                        PRINT_C(" ");
                        PRINT_H(iObj);
                        PRINT_C(":");
                        PRINT_H32(offObj);
                    }
                }

                /* registers */
                PRINT_C("\r\ncs:eip=");
                PRINT_H16(pCtx->ctx_SegCs);
                PRINT_C(":");
                PRINT_H32(pCtx->ctx_RegEip);

                PRINT_C("      ss:esp=");
                PRINT_H16(pCtx->ctx_SegSs);
                PRINT_C(":");
                PRINT_H32(pCtx->ctx_RegEsp);

                PRINT_C("      ebp=");
                PRINT_H32(pCtx->ctx_RegEbp);

                PRINT_C("\r\n ds=");
                PRINT_H16(pCtx->ctx_SegDs);
                PRINT_C("      es=");
                PRINT_H16(pCtx->ctx_SegEs);
                PRINT_C("      fs=");
                PRINT_H16(pCtx->ctx_SegFs);
                PRINT_C("      gs=");
                PRINT_H16(pCtx->ctx_SegGs);
                PRINT_C("     efl=");
                PRINT_H32(pCtx->ctx_EFlags);

                PRINT_C("\r\neax=");
                PRINT_H32(pCtx->ctx_RegEax);
                PRINT_C(" ebx=");
                PRINT_H32(pCtx->ctx_RegEbx);
                PRINT_C(" ecx=");
                PRINT_H32(pCtx->ctx_RegEcx);
                PRINT_C(" edx=");
                PRINT_H32(pCtx->ctx_RegEdx);
                PRINT_C(" edi=");
                PRINT_H32(pCtx->ctx_RegEdi);
                PRINT_C(" esi=");
                PRINT_H32(pCtx->ctx_RegEsi);
                /** @todo fpu */
            }

            /* final newline */
            PRINT_C("\r\n");
        }
    } /* !QUIET */

    /*
     * Attempt dumping the process.
     */
    if (fDumpProcess)
    {
        LIBCLOG_MSG("Calling DosDumpProcess()\n");
        rc = DosDumpProcess(DDP_PERFORMPROCDUMP, 0, 0);
        if (!fQuiet)
        {
            if (!rc)
                PRINT_C("Process has been dumped\r\n");
            else if (rc == 0x00011176 || rc == ERROR_INVALID_PARAMETER) /* first is actual, 2nd is docs */
                PRINT_C("Process dumping was disabled, use DUMPPROC / PROCDUMP to enable it.\r\n");
            else
            {
                PRINT_C("DosDumpProcess failed with rc=0x");
                PRINT_H(rc);
                PRINT_C("\r\n");
            }
        }
    }

    /*
     * EXCEPTQ report
     */
    if (fHaveExceptq && SetExceptqOptions && ExceptqHandler)
    {
        LIBCLOG_MSG("EXCEPTQ report\n");

        /* Make sure EXCEPTQ_DEBUG_EXCEPTION is enabled (and set extended report info) */
        SetExceptqOptions("D", szMsg);

        EXCEPTIONREPORTRECORD XcptRepRec, *pXcptRepRec = NULL;
        EXCEPTIONREGISTRATIONRECORD *pXcptRegRec = NULL;
        CONTEXTRECORD Ctx, *pCtx = NULL;
        PVOID pvWhatEver = NULL;

        if (pvCtx && (fFlags & __LIBC_PANIC_XCPTPARAMS))
        {
            /* Use the existing exception parameters */
            pXcptRepRec = ((PXCPTPARAMS)pvCtx)->pXcptRepRec;
            pXcptRegRec = ((PXCPTPARAMS)pvCtx)->pXcptRegRec;
            pCtx = ((PXCPTPARAMS)pvCtx)->pCtx;
            pvWhatEver = ((PXCPTPARAMS)pvCtx)->pvWhatEver;
        }
        else
        {
            /* Compose exception parameters ourselves */
            XcptRepRec.ExceptionNum                = 0x71785158; /* EXCEPTQ_DEBUG_EXCEPTION */
            XcptRepRec.fHandlerFlags               = 0;
            XcptRepRec.NestedExceptionReportRecord = NULL;
            XcptRepRec.ExceptionAddress            = NULL;
            XcptRepRec.cParameters                 = 0;

            pXcptRepRec = &XcptRepRec;

            if (pvCtx)
            {
                pCtx = (PCONTEXTRECORD)pvCtx;

                /* Set the exception address to EIP (otherwise EXCEPTQ won't generate disassembly */
                if (pCtx->ContextFlags & CONTEXT_CONTROL)
                    XcptRepRec.ExceptionAddress = (PVOID)pCtx->ctx_RegEip;
            }
            else
            {
                memset(&Ctx, 0, sizeof(Ctx));

                /*
                 * Grab the current thread's context. Note that this is a "syntethic" exception
                 * where we are only interested in the stack trace for .TRP reports so we grab
                 * control registers (and segments - for information) and not integer registers
                 * which are useless.  We also leave ExceptionAddress NULL to suppress
                 * disassembly generation as it's useless too (it will always show our own assembly
                 * below.
                 */
                __asm__(
                    "mov %%gs,   %0\n\t"
                    "mov %%fs,   %1\n\t"
                    "mov %%es,   %2\n\t"
                    "mov %%ds,   %3\n\t"
                    "movl %%ebp, %4\n\t"
                    "movl $.,    %5\n\t"
                    "mov %%cs,   %6\n\t"
                    "pushf; pop  %7\n\t"
                    "movl %%esp, %8\n\t"
                    "mov %%ss,   %9\n\t"
                    :
                    "=m" (Ctx.ctx_SegGs),
                    "=m" (Ctx.ctx_SegFs),
                    "=m" (Ctx.ctx_SegEs),
                    "=m" (Ctx.ctx_SegDs),
                    "=m" (Ctx.ctx_RegEbp),
                    "=m" (Ctx.ctx_RegEip),
                    "=m" (Ctx.ctx_SegCs),
                    "=m" (Ctx.ctx_EFlags),
                    "=m" (Ctx.ctx_RegEsp),
                    "=m" (Ctx.ctx_SegSs)
                );

                Ctx.ContextFlags = CONTEXT_CONTROL | CONTEXT_SEGMENTS;
                pCtx = &Ctx;
            }
        }

        ExceptqHandler(pXcptRepRec, pXcptRegRec, pCtx, pvWhatEver);

        /*
         * Attempt to move the .TRP file to the system log directory unless it
         * LIBC logging is redirected to the current directory or stdout/stderr
         * in which case just rename it to to follow LIBC log filename scheme
         * (note: the env. var name should be the same as in __libc_logDefault,
         * also check __libc_logInit where this var is handled!).
         *
         * TODO: This is a temporary solution, a proper way is to teach EXCEPTQ
         * to take the filename argument, see https://github.com/bitwiseworks/libc/issues/99.
         */
        PSZ pszEnv = NULL;
        int fCurDir = 0;
        if (!DosScanEnv((PCSZ)"LIBC_LOGGING_OUTPUT", &pszEnv) && pszEnv && *pszEnv)
            fCurDir = 1;

        /*
         * The original .TRP name is PPPP_TT.TRP where PPPP is PID in hex
         * and TT is TID in hex, compose it from the trap data.
         */
        char szTrpFile[] = "PPPP_TT.TRP";
        panicHex2(szTrpFile, pPib->pib_ulpid, 4, 0);
        panicHex2(szTrpFile + 5, pTib->tib_ptib2->tib2_ultid, 2, 0);

        /*
         * The new .TRP name should be in sync with what __libc_logInit does
         * for default log filenames!
         */
        char szNewTrpFileBegin[] = "TTTTTTTT-PPPP_TT";
        char szNewTrpFileEnd[] = "-exceptq.txt";

        /* Query the EXE name and strip off the path and .EXE extension. */
        char *pszExeName = NULL;
        if (!DosQueryModuleName(pPib->pib_hmte, sizeof(szPathBuf), &szPathBuf[0]))
        {
            szPathBuf[CCHMAXPATH - 1] = '\0';
            pszExeName = szPathBuf;
            /* Copied from __libc_logInit (except __stricmp_ascii) */
            char *pszEnd = pszExeName;
            while (*pszEnd)
                ++pszEnd;
            if (   pszEnd - pszExeName >= 4
                && pszEnd[-4] == '.'
                && CHLOWER(pszEnd[-3]) == 'e'
                && CHLOWER(pszEnd[-2]) == 'x'
                && CHLOWER(pszEnd[-1]) == 'e')
            {
                pszEnd -= 4;
                *pszEnd = '\0';
            }
            while (   pszEnd > pszExeName
                   && (*pszEnd != '/' && *pszEnd != '\\')
                   && *pszEnd != ':')
                --pszEnd;
            if (pszEnd > pszExeName)
                pszExeName = pszEnd + 1;
        }

        /* Account for a leading dash if we have the EXE name. */
        size_t cchExeName = pszExeName ? panicStrLen(pszExeName) + 1 : 0;

        const char *pszLogDir = NULL;
        size_t cchLogDir = 0;
        if (!fCurDir)
        {
            pszLogDir = __libc_LogGetDefaultLogDir();
            cchLogDir = panicStrLen(pszLogDir);
            if (pszLogDir[cchLogDir - 1] != '\\')
                cchLogDir++; /* account for a future backslash */
        }

        /* Now check that we fit into the path buffer. */
        if (cchLogDir + sizeof(szNewTrpFileBegin) - 1 + cchExeName + sizeof(szNewTrpFileEnd) <= CCHMAXPATH)
        {
            /*
             * We don't query QSV_TIME_HIGH as it will remain 0 until 19-Jan-2038 and for
             * our purposes (generate a unique log name sorted by date) it's fine.
             */
            ULONG ulTime;
            DosQuerySysInfo(QSV_TIME_LOW, QSV_TIME_LOW, &ulTime, sizeof(ulTime));

            panicHex2(szNewTrpFileBegin + 0, ulTime, 8, 0);
            panicHex2(szNewTrpFileBegin + 9, pPib->pib_ulpid, 4, 0);
            panicHex2(szNewTrpFileBegin + 14, pTib->tib_ptib2->tib2_ultid, 2, 0);

            if (cchExeName)
            {
                /* pszExeName is in szPathBuf now, move it to the right position first! */
                memmove(szPathBuf + cchLogDir + sizeof(szNewTrpFileBegin), pszExeName, cchExeName - 1);
                szPathBuf[cchLogDir + sizeof(szNewTrpFileBegin) - 1] = '-';
            }

            if (pszLogDir)
            {
                memcpy(szPathBuf, pszLogDir, cchLogDir);
                if (!szPathBuf[cchLogDir - 1])
                    szPathBuf[cchLogDir - 1] = '\\'; /* add the missing backslash */
            }
            memcpy(szPathBuf + cchLogDir, szNewTrpFileBegin, sizeof(szNewTrpFileBegin) - 1);
            memcpy(szPathBuf + cchLogDir + sizeof(szNewTrpFileBegin) - 1 + cchExeName, szNewTrpFileEnd, sizeof(szNewTrpFileEnd));

            int fOp = 0;

            /* First, try to move. */
            if (!DosMove((PCSZ)szTrpFile, (PCSZ)szPathBuf))
                fOp = 1 /* move ok */;
            else
            {
                /* Then, try to copy (no overwrite!) + delete. */
                if (!DosCopy((PCSZ)szTrpFile, (PCSZ)szPathBuf, 0))
                {
                    fOp = 2; /* copy ok */
                    if (!DosDelete((PCSZ)szTrpFile))
                        fOp = 1; /* move ok */
                }
            }
            if (!fOp)
                PRINT_C("Failed to move ");
            else if (fOp == 1)
                PRINT_C("Moved ");
            else
                PRINT_C("Copied ");
            PRINT_C(szTrpFile);
            PRINT_C(" to ");
            PRINT_P(szPathBuf);
            PRINT_C("\r\n");
        }
    }

    /*
     * Terminate the exception handler chain to avoid recursive trouble.
     */
    pTib->tib_pexchain = END_OF_CHAIN;

    /*
     * Breakpoint
     */
    if (fBreakpoint)
    {
        LIBCLOG_MSG("Breakpoint\n");
        __asm__ __volatile__ ("int3\n\t"
                              "nop\n\t");
    }

    /*
     * Terminate the process.
     */
#if 0
    if (pTib->tib_ptib2->tib2_usMCCount)
    {
        ULONG       ulIgnore;
        unsigned    cMCExits = pTib->tib_ptib2->tib2_usMCCount;
        LIBCLOG_MSG("Calling DosExitMustComplete() %u times\n", cMCExits);
        while (cMCExits-- > 0)
            DosExitMustComplete(&ulIgnore);
    }
#endif
    LIBCLOG_MSG("Calling DosKillProcess() \n");
    for (;;)
    {
        DosKillProcess(DKP_PROCESS, pPib->pib_ulpid);
        DosExit(EXIT_PROCESS, 127);
    }
}

