/* $Id: FastInfoBlocks.c 1666 2004-11-22 22:32:54Z bird $ */
/** @file
 *
 * Fast InfoBlock Access.
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


/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include "libc-alias.h"
#define INCL_DOSINFOSEG
#define INCL_DOSPROCESS
#define INCL_FSMACROS
#include <os2emx.h>
#include <InnoTekLIBC/FastInfoBlocks.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_MISC
#include <InnoTekLIBC/logstrict.h>


/*******************************************************************************
*   Global Variables                                                           *
*******************************************************************************/
struct __libc_GpFIBLIS_s *__libc_GpFIBLIS = NULL;
struct __libc_GpFIBGIS_s *__libc_GpFIBGIS = NULL;
struct __libc_GpFIBPIB_s *__libc_GpFIBPIB = NULL;


/**
 * Initialize the global infoblock/segment pointers.
 * @param   fForced     Used during fork().
 */
int __libc_back_fibInit(int fForced)
{
    LIBCLOG_ENTER("\n");
    if (!fForced && __libc_GpFIBPIB)
        LIBCLOG_RETURN_INT(0);

    __libc_GpFIBGIS = (struct __libc_GpFIBGIS_s *)GETGINFOSEG();
    __libc_GpFIBLIS = (struct __libc_GpFIBLIS_s *)GETLINFOSEG();
    PPIB pPib = NULL;
    PTIB pTib;
    FS_VAR();
    FS_SAVE_LOAD();
    DosGetInfoBlocks(&pTib, &pPib);
    __libc_GpFIBPIB = (struct __libc_GpFIBPIB_s *)pPib;
    __libc_Back_fibDumpAll();
    FS_RESTORE();
    LIBCLOG_RETURN_INT(0);
}


/**
 * Logs the content of all the structures.
 */
void __libc_Back_fibDumpAll(void)
{
    LIBCLOG_ENTER("\n");

    LIBCLOG_MSG("_gpfibPIB=%p\n", (void *)__libc_GpFIBPIB);
    if (__libc_GpFIBPIB)
    {
        LIBCLOG_MSG("pib_ulpid    = %08lx\n", __libc_GpFIBPIB->pib_ulpid    );
        LIBCLOG_MSG("pib_ulppid   = %08lx\n", __libc_GpFIBPIB->pib_ulppid   );
        LIBCLOG_MSG("pib_hmte     = %08lx\n", __libc_GpFIBPIB->pib_hmte     );
        LIBCLOG_MSG("pib_pchcmd   = %p\n",    __libc_GpFIBPIB->pib_pchcmd   );
        LIBCLOG_MSG("pib_pchenv   = %p\n",    __libc_GpFIBPIB->pib_pchenv   );
        LIBCLOG_MSG("pib_flstatus = %08lx\n", __libc_GpFIBPIB->pib_flstatus );
        LIBCLOG_MSG("pib_ultype   = %08lx\n", __libc_GpFIBPIB->pib_ultype   );
    }
    LIBCLOG_MSG("__libc_GpFIBLIS = %p\n", (void *)__libc_GpFIBLIS);
    if (__libc_GpFIBLIS)
    {
        LIBCLOG_MSG("pidCurrent     = %08x\n", (unsigned)__libc_GpFIBLIS->pidCurrent    );
        LIBCLOG_MSG("pidParent      = %08x\n", (unsigned)__libc_GpFIBLIS->pidParent     );
        LIBCLOG_MSG("prtyCurrent    = %08x\n", (unsigned)__libc_GpFIBLIS->prtyCurrent   );
        LIBCLOG_MSG("tidCurrent     = %08x\n", (unsigned)__libc_GpFIBLIS->tidCurrent    );
        LIBCLOG_MSG("sgCurrent      = %08x\n", (unsigned)__libc_GpFIBLIS->sgCurrent     );
        LIBCLOG_MSG("rfProcStatus   = %08x\n", (unsigned)__libc_GpFIBLIS->rfProcStatus  );
        LIBCLOG_MSG("LIS_fillbyte1  = %08x\n", (unsigned)__libc_GpFIBLIS->LIS_fillbyte1 );
        LIBCLOG_MSG("fFoureground   = %08x\n", (unsigned)__libc_GpFIBLIS->fFoureground  );
        LIBCLOG_MSG("typeProcess    = %08x\n", (unsigned)__libc_GpFIBLIS->typeProcess   );
        LIBCLOG_MSG("LIS_fillbyte2  = %08x\n", (unsigned)__libc_GpFIBLIS->LIS_fillbyte2 );
        LIBCLOG_MSG("selEnv         = %08x\n", (unsigned)__libc_GpFIBLIS->selEnv        );
        LIBCLOG_MSG("offCmdLine     = %08x\n", (unsigned)__libc_GpFIBLIS->offCmdLine    );
        LIBCLOG_MSG("cbDataSegment  = %08x\n", (unsigned)__libc_GpFIBLIS->cbDataSegment );
        LIBCLOG_MSG("cbStack        = %08x\n", (unsigned)__libc_GpFIBLIS->cbStack       );
        LIBCLOG_MSG("cbHeap         = %08x\n", (unsigned)__libc_GpFIBLIS->cbHeap        );
        LIBCLOG_MSG("hmod           = %08x\n", (unsigned)__libc_GpFIBLIS->hmod          );
        LIBCLOG_MSG("selDS          = %08x\n", (unsigned)__libc_GpFIBLIS->selDS         );
        LIBCLOG_MSG("LIS_PackSel    = %08x\n", (unsigned)__libc_GpFIBLIS->LIS_PackSel   );
        LIBCLOG_MSG("LIS_PackShrSel = %08x\n", (unsigned)__libc_GpFIBLIS->LIS_PackShrSel);
        LIBCLOG_MSG("LIS_PackPckSel = %08x\n", (unsigned)__libc_GpFIBLIS->LIS_PackPckSel);
    }
    LIBCLOG_MSG("__libc_GpFIBGIS=%p\n", (void *)__libc_GpFIBGIS);
    if (__libc_GpFIBGIS)
    {
        LIBCLOG_MSG("SIS_BigTime        = %08x\n", (unsigned)__libc_GpFIBGIS->SIS_BigTime       );
        LIBCLOG_MSG("SIS_MsCount        = %08x\n", (unsigned)__libc_GpFIBGIS->SIS_MsCount       );
        LIBCLOG_MSG("SIS_HrsTime        = %08x\n", (unsigned)__libc_GpFIBGIS->SIS_HrsTime       );
        LIBCLOG_MSG("SIS_MinTime        = %08x\n", (unsigned)__libc_GpFIBGIS->SIS_MinTime       );
        LIBCLOG_MSG("SIS_SecTime        = %08x\n", (unsigned)__libc_GpFIBGIS->SIS_SecTime       );
        LIBCLOG_MSG("SIS_HunTime        = %08x\n", (unsigned)__libc_GpFIBGIS->SIS_HunTime       );
        LIBCLOG_MSG("SIS_TimeZone       = %08x\n", (unsigned)__libc_GpFIBGIS->SIS_TimeZone      );
        LIBCLOG_MSG("SIS_ClkIntrvl      = %08x\n", (unsigned)__libc_GpFIBGIS->SIS_ClkIntrvl     );
        LIBCLOG_MSG("SIS_DayDate        = %08x\n", (unsigned)__libc_GpFIBGIS->SIS_DayDate       );
        LIBCLOG_MSG("SIS_MonDate        = %08x\n", (unsigned)__libc_GpFIBGIS->SIS_MonDate       );
        LIBCLOG_MSG("SIS_YrsDate        = %08x\n", (unsigned)__libc_GpFIBGIS->SIS_YrsDate       );
        LIBCLOG_MSG("SIS_DOWDate        = %08x\n", (unsigned)__libc_GpFIBGIS->SIS_DOWDate       );
        LIBCLOG_MSG("SIS_VerMajor       = %08x\n", (unsigned)__libc_GpFIBGIS->SIS_VerMajor      );
        LIBCLOG_MSG("SIS_VerMinor       = %08x\n", (unsigned)__libc_GpFIBGIS->SIS_VerMinor      );
        LIBCLOG_MSG("SIS_RevLettr       = %08x\n", (unsigned)__libc_GpFIBGIS->SIS_RevLettr      );
        LIBCLOG_MSG("SIS_CurScrnGrp     = %08x\n", (unsigned)__libc_GpFIBGIS->SIS_CurScrnGrp    );
        LIBCLOG_MSG("SIS_MaxScrnGrp     = %08x\n", (unsigned)__libc_GpFIBGIS->SIS_MaxScrnGrp    );
        LIBCLOG_MSG("SIS_HugeShfCnt     = %08x\n", (unsigned)__libc_GpFIBGIS->SIS_HugeShfCnt    );
        LIBCLOG_MSG("SIS_ProtMdOnly     = %08x\n", (unsigned)__libc_GpFIBGIS->SIS_ProtMdOnly    );
        LIBCLOG_MSG("SIS_FgndPID        = %08x\n", (unsigned)__libc_GpFIBGIS->SIS_FgndPID       );
        LIBCLOG_MSG("SIS_Dynamic        = %08x\n", (unsigned)__libc_GpFIBGIS->SIS_Dynamic       );
        LIBCLOG_MSG("SIS_MaxWait        = %08x\n", (unsigned)__libc_GpFIBGIS->SIS_MaxWait       );
        LIBCLOG_MSG("SIS_MinSlice       = %08x\n", (unsigned)__libc_GpFIBGIS->SIS_MinSlice      );
        LIBCLOG_MSG("SIS_MaxSlice       = %08x\n", (unsigned)__libc_GpFIBGIS->SIS_MaxSlice      );
        LIBCLOG_MSG("SIS_BootDrv        = %08x\n", (unsigned)__libc_GpFIBGIS->SIS_BootDrv       );
        LIBCLOG_MSG("SIS_MaxVioWinSG    = %08x\n", (unsigned)__libc_GpFIBGIS->SIS_MaxVioWinSG   );
        LIBCLOG_MSG("SIS_MaxPresMgrSG   = %08x\n", (unsigned)__libc_GpFIBGIS->SIS_MaxPresMgrSG  );
        LIBCLOG_MSG("SIS_SysLog         = %08x\n", (unsigned)__libc_GpFIBGIS->SIS_SysLog        );
        LIBCLOG_MSG("SIS_MMIOBase       = %08x\n", (unsigned)__libc_GpFIBGIS->SIS_MMIOBase      );
        LIBCLOG_MSG("SIS_MMIOAddr       = %08x\n", (unsigned)__libc_GpFIBGIS->SIS_MMIOAddr      );
        LIBCLOG_MSG("SIS_MaxVDMs        = %08x\n", (unsigned)__libc_GpFIBGIS->SIS_MaxVDMs       );
        LIBCLOG_MSG("SIS_Reserved       = %08x\n", (unsigned)__libc_GpFIBGIS->SIS_Reserved      );
    }
    LIBCLOG_RETURN_VOID();
}

