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
#define _GNU_SOURCE
#include <string.h>
#include <InnotekLIBC/backend.h>
#include <InnotekLIBC/sharedpm.h>
#include <InnotekLIBC/logstrict.h>
#include <os2emx.h>


/*******************************************************************************
*   Defined Constants And Macros                                               *
*******************************************************************************/
/** OS/2 default standard error handle. */
#define HFILE_STDERR    ((HFILE)2)


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
 */
static int panicHex(char *pszBuf, unsigned u, unsigned cDigits)
{
    static const char szHex[17] = "0123456789abcdef";
    if (u <= 0xf && cDigits <= 1)
    {
        pszBuf[0] = szHex[u & 0xf];
        pszBuf[1] = '\0';
        return 1;
    }
    if (u <= 0xff && cDigits <= 2)
    {
        pszBuf[0] = szHex[u >> 4];
        pszBuf[1] = szHex[u & 0xf];
        pszBuf[2] = '\0';
        return 2;
    }
    else if (u <= 0xffff && cDigits <= 4)
    {
        pszBuf[0] = szHex[u >> 12];
        pszBuf[1] = szHex[(u >> 8) & 0xf];
        pszBuf[2] = szHex[(u >> 4) & 0xf];
        pszBuf[3] = szHex[u & 0xf];
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
        pszBuf[8] = '\0';
        return 8;
    }
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
 * @param   pvCtx       Pointer to a context record if available. This is a PCONTEXTRECORD.
 * @param   pszFormat   User message which may contain %s and %x.
 * @param   args        String pointers and unsigned intergers as specified by the %s and %x in pszFormat.
 */
void __libc_Back_panicV(unsigned fFlags, void *pvCtx, const char *pszFormat, va_list args)
{
    LIBCLOG_ENTER("fFlags=%#x pvCtx=%p pszFormat=%p:{%s}\n", fFlags, pvCtx, (void *)pszFormat, args);

    /*
     * First, terminate the exception handler chain to avoid recursive trouble.
     */
    FS_VAR();
    FS_SAVE_LOAD();
    PTIB pTib;
    PPIB pPib;
    DosGetInfoBlocks(&pTib, &pPib);
    pTib->tib_pexchain = END_OF_CHAIN;

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
    BOOL fDumpProcess = TRUE;
    const char *pszPanicCfg = NULL;
    int rc = DosScanEnv((PCSZ)"LIBC_PANIC", (PSZ *)(void *)&pszPanicCfg);
    if (!rc && pszPanicCfg && strnlen(pszPanicCfg, 512) < 512)
    {
        int c = 512;
        while (c-- > 0 && *pszPanicCfg)
        {
            if (panicStrICmp(&pszPanicCfg, "quiet", "QUIET"))
                fQuiet = TRUE;
            //else if (panicStrICmp(&pszPanicCfg, "noquiet", "NOQUIET"))
            //    fQuiet = FALSE;
            else if (panicStrICmp(&pszPanicCfg, "terse", "TERSE"))
                fVerbose = FALSE;
            //else if (panicStrICmp(&pszPanicCfg, "verbose", "VERBOSE"))
            //    fVerbose = TRUE;
            else if (panicStrICmp(&pszPanicCfg, "breakpoint", "BREAKPOINT"))
                fBreakpoint = TRUE;
            else if (panicStrICmp(&pszPanicCfg, "nobreakpoint", "NOBREAKPOINT"))
                fBreakpoint = FALSE;
            //else if (panicStrICmp(&pszPanicCfg, "dump", "DUMP"))
            //    fDumpProcess = TRUE;
            else if (panicStrICmp(&pszPanicCfg, "nodump", "NODUMP"))
                fDumpProcess = FALSE;
            else
                pszPanicCfg++;
        }
    }

    ULONG   cb;
    char    szHexNum[12];
    if (!fQuiet)
    {
        /*
         * Write user message to stderr.
         */
#define PRINT_CHAR(ch)  DosWrite(HFILE_STDERR, &ch,      1,                                    &cb)
#define PRINT_C(msg)    DosWrite(HFILE_STDERR, msg,      sizeof(msg) - 1,                      &cb)
#define PRINT_P(msg)    DosWrite(HFILE_STDERR, msg,      panicStrLen(msg),           &cb)
#define PRINT_H(hex)    DosWrite(HFILE_STDERR, szHexNum, panicHex(szHexNum, hex, 0), &cb)
#define PRINT_H16(hex)  DosWrite(HFILE_STDERR, szHexNum, panicHex(szHexNum, hex, 4), &cb)
#define PRINT_H32(hex)  DosWrite(HFILE_STDERR, szHexNum, panicHex(szHexNum, hex, 8), &cb)
        if (!(fFlags & __LIBC_PANIC_SIGNAL) && fVerbose)
            PRINT_C("\r\nLIBC PANIC!!\r\n");
        else
            PRINT_C("\r\n");
        char ch;
        while ((ch = *pszFormat++) != '\0')
        {
            switch (ch)
            {
                case '\n':
                    PRINT_C("\r\n");
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
                                PRINT_P(psz);
                            else if (!psz)
                                PRINT_C("<null>");
                            else
                            {
                                PRINT_C("<bogus string ptr 0x");
                                PRINT_H((uintptr_t)psz);
                                PRINT_C(">");
                            }
                            break;
                        }

                        /* it's all hex to me */
                        case 'd':
                        case 'x':
                        case 'u':
                        {
                            unsigned u = va_arg(args, unsigned);
                            PRINT_C("0x");
                            PRINT_H(u);
                            break;
                        }

                        case 'p':
                        {
                            uintptr_t u = va_arg(args, uintptr_t);
                            PRINT_H(u);
                            break;
                        }

                        case '%':
                            PRINT_C("%");
                            break;

                        default:
                        {
                            PRINT_C("<Unknown format %");
                            PRINT_CHAR(ch);
                            PRINT_C(" arg 0x");
                            unsigned u = va_arg(args, unsigned);
                            PRINT_H(u);
                            PRINT_C(">");
                            break;
                        }
                    }
                    break;

                default:
                    PRINT_CHAR(ch);
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
            static char szExeName[CCHMAXPATH];
            if (!DosQueryModuleName(pPib->pib_hmte, sizeof(szExeName), &szExeName[0]))
            {
                szExeName[CCHMAXPATH - 1] = '\0';
                PRINT_C("\r\n");
                PRINT_P(szExeName);
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
                    if (!DosQueryModFromEIP(&hmod, &iObj, sizeof(szExeName), szExeName, &offObj, pCtx->ctx_RegEip))
                    {
                        PRINT_C("\r\n");
                        PRINT_P(szExeName);
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
     * Breakpoint
     */
    if (fBreakpoint)
    {
        LIBCLOG_MSG("Breakpoint\n");
        __asm__ __volatile__ ("int3\n\t"
                              "nop\n\t");
    }

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

