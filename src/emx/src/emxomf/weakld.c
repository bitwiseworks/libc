/* $Id: weakld.c 3753 2012-03-04 21:23:36Z bird $ */
/** @file
 * Weak Symbol Pre-Linker.
 *
 * Copyright (c) 2003 InnoTek Systemberatung GmbH
 * Author: knut st. osmundsen <bird-srcspam@anduin.net>
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with This program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */


/** @page weakld        Weak Pre-Linker
 *
 * In order to get the weak symbols somewhat right it looks like we have to do
 * the pass1 of the linking process in order to resolve the weak symbols.
 *
 *
 *
 * @subsection          Symbols
 *
 * There is a couple of symbol types, but we can skip most of them for this
 * pre-linking operation. We use one symbol type which is public or global
 * symbols if you like. Perhaps it would be wise to devide them into separat
 * type groups, but the choice was made to differenciate this using flags.
 * So, symbol enumeration is done using flag masks.
 * @todo: Finish this stuff.
 *
 */

/*******************************************************************************
*   Defined Constants And Macros                                               *
*******************************************************************************/
#define WLDSYM_HASH_SIZE    211
#define OMF_MAX_REC         1024

/** helper to make verbose only output. */
#define WLDINFO(pWld, a)    do { if (pWld->fFlags & WLDC_VERBOSE) wldInfo a; } while (0)

/** Internal error */
#define WLDINTERR(pWld, pMod)   wldIntErr(pWld, pMod, __FILE__, __LINE__, __FUNCTION__);

//#define WLD_ENABLED_DBG
#ifdef WLD_ENABLED_DBG
#define SYMDBG(pSym, pszMsg)    symDbg(pSym, pszMsg);
#define WLDDBG(a)               wldDbg a
#define WLDDBG2(a)              wldDbg a
#else
#define SYMDBG(pSym, pszMsg)    do {} while (0)
#define WLDDBG(a)               do {} while (0)
#define WLDDBG2(a)              do {} while (0)
#endif


/** Helpers for checking if a symbol is defined strongly. */
#define SYM_IS_DEFINED(fFlags)   (   (fFlags & (WLDSF_TYPEMASK | WLDSF_WEAK)) == WLDSF_PUBLIC \
                                  || (fFlags & (WLDSF_TYPEMASK | WLDSF_WEAK)) == WLDSF_COMM \
                                  || (fFlags & (WLDSF_TYPEMASK | WLDSF_WEAK)) == WLDSF_IMPORT \
                                  )

/** Compares a existing symbol with a new symbol. */
#define SYM_EQUAL(pWld, pSym, _pszName, _fFlags, _uHash, _cchName)                  \
    (   (pSym)->uHash == (_uHash)                                                   \
     && (pSym)->pszName == (_pszName)                                               \
    )

/** Compares a existing symbol with a potential symbol. */
#define SYM_EQUAL2(pWld, pSym, _pszName, _fFlags, _uHash, _cchName, _pfn)           \
    (   (pSym)->uHash == (_uHash)                                                   \
     && !_pfn((pSym)->pszName, (_pszName), (_cchName))                              \
     && !(pSym)->pszName[(_cchName)]                                                \
    )



/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <process.h>
#include <sys/omflib.h>
#include <sys/moddef.h>
#include "defs.h"
#include "grow.h"
#include "weakld.h"


/*******************************************************************************
*   Structures and Typedefs                                                    *
*******************************************************************************/
/** @group OMF stuff
 * @{ */

/** OMF record header. */
#pragma pack(1)
typedef struct OMFREC
{
  unsigned char     chType;
  unsigned short    cb;
} OMFREC, *POMFREC;
#pragma pack()


/** OMF library header. */
#pragma pack(1)
typedef struct OMFLIBHDR
{
  unsigned char     chType;
  unsigned short    cb;
  unsigned long     offDict;
  unsigned short    cDictBlocks;
  unsigned char     fchFlags;
} OMFLIBHDR, *POMFLIBHDR;
#pragma pack()

/** OMF library header - for combining with OMFREC. */
#pragma pack(1)
typedef struct OMFLIBHDRX
{
  unsigned long     offDict;
  unsigned short    cDictBlocks;
  unsigned char     fchFlags;
} OMFLIBHDRX, *POMFLIBHDRX;
#pragma pack()

/** @} */


/**
 * Library structure
 */
typedef struct wldlib
{
    /** Library name. */
    const char *pszLibName;
    /** Filehandle if open */
    FILE *      phFile;
    /** Pointer to extended dictiorary. */
    void *      pDict;
    /** Library header. */
    OMFLIBHDR   LibHdr;
    /** Linked list next pointer. */
    struct wldlib * pNext;
} WLDLIB, *PWLDLIB;


/**
 * Module structure
 */
typedef struct wldmod
{
    /** Module name. */
    const char *pszModName;
    /** Filehandle if open. */
    FILE *      phFile;
    /** Module offset into the file. */
    off_t       off;
    /** Library relation - not used for libraries added thru wld_add_object(). */
    PWLDLIB     pLib;
    /* Linked list next pointer */
    struct wldmod *pNext;
} WLDMOD, *PWLDMOD;


/**
 * Symbol structure.
 */
typedef struct wldsym
{
    /** Symbol name. */
    const char *        pszName;
    /** Weak name - for weak symbols only. */
    const char *        pszWeakName;
    /** The full hash value. */
    unsigned            uHash;

    /** Symbol flags. */
    enum {
        /** @group Symbol Type
         * @{ */
        /* Mask of the symbol type. */
        WLDSF_TYPEMASK  = 0x000f,
        /** Strong symbol.
         * A strong definition exists for this symbol (PUBDEF). */
        WLDSF_PUBLIC    = 0x0001,
        /** Communal data/code.
         * Communal definition exists for this symbol.
         * If a PUBDEF is found for this symbol it will become strong. */
        WLDSF_COMM      = 0x0002,
        /** Imported symbol.
         * This symbol is imported.
         * Note that when combined with WLDSF_LIBSEARCH a symbol in this type
         * will yield to stronger definitions with a little warning. */
        WLDSF_IMPORT    = 0x0003,
        /** Undefined symbol.
         * This symbol is not yet defined anywhere. */
        WLDSF_UNDEF     = 0x0004,
        /** Weak external.
         * This symbol doesn't need to be resolved as it have a default
         * resolution. pWeakDefault is pointing to the default resolution.
         * If an non weak EXTDEF is found in another module, it will become
         * WLDSF_UNDEF.
         */
        WLDSF_WKEXT     = 0x0005,
        /** Exported name.
         * This is used when the exported name differs from the internal one.
         * This name is not considered in the linking, only the internal one. */
        WLDSF_EXP_NM    = 0x0006,
        /** @} */

        /** Uncertain undefined symbol.
         * We're still processing the module containing this and the uncertainty
         * we're facing is that a WLDSF_UNDEF may changed to an WLDSF_WKEXT
         * upon encountering a WKEXT COMENT record. */
        WLDSF_UNCERTAIN = 0x0100,

        /** Symbol found during library search.
         * If this is an attribute to a symbol of the WLDSF_IMPORT type, the
         * symbol is actually kind of weak.
         */
        WLDSF_LIBSEARCH = 0x0200,

        /** Weak symbol.
         * This symbol doesn't need to be resolved (if WLDSF_UNDEF), or
         * it may be overridden by a EXTDEF with no WKEXT.
         * If this is an weak undefined symbol (extern weak, local default)
         * pWeakDefault Will point at it.
         */
        WLDSF_WEAK      = 0x0400,

        /** Alias symbol.
         * This symbol is an alias for another symbol. pAliasFor will be set. */
        WLDSF_ALIAS     = 0x0800,

        /** Internal flags which indicates that the symbol was exported in
         * the definition file. This to select symbols which only appears in
         * __declspec(dllexport) and put them into the definition file for
         * proper weak handling. */
        WLDSF_EXPORT_DEF = 0x2000,

        /** Internal flag to say that we've already aliased this weak
         * in the definition file. */
        WLDSF_WEAKALIASDONE = 0x8000
    }                   fFlags;

    /** The module this symbol is defined in. */
    PWLDMOD             pMod;

    /** Array of modules refering this symbol. */
    PWLDMOD *           paReferers;

    /** Number of modules in the array. */
    unsigned            cReferers;

    /** Per type data union */
    union type_data
    {
        struct import_data
        {
            /** @group Import Attributes
             * Valid when type is WLDSF_IMPORT.
             * @{ */
            /** Import name. */
            const char *        pszImpName;
            /** Import module. */
            const char *        pszImpMod;
            /** Import Ordinal (WLDSF_IMPORT).
             * 0 means no ordinal. */
            unsigned            uImpOrd;
            /** @} */
        } import;

        struct comm_data
        {
            /** Size of comm object */
            signed long         cb;
            /** Number of elements. */
            signed long         cElements;
        } comm;
    } u;

    /** Weak default resolution.
     * Valid if the symbol is of type WLDSF_WKEXT or have the WLDSF_WEAK flag set.
     * Indicates a default resolution for the symbol.
     * For WLDSF_WEAK this only make sense if the type is WLDSF_UNDEF.
     */
    struct wldsym *     pWeakDefault;

    /** Symbol this is an alias for.
     * Valid when WLDSF_ALIAS or WLDSF_EXP_NM are set. */
    struct wldsym *     pAliasFor;

    /** @group Export Attributes
     * Valid when WLDSF_EXP_NM is set.
     * @{ */
    /** Export flags. */
    enum
    {
        /** @group Name */
        /** Name type mask. */
        WLDSEF_NAMEMASK         = 0x03,
        /** Default action depending on if it have an ordinal or not.
         * If it have an ordinal it shall be non-resident, if it hasn't
         * it shall be resident.
         */
        WLDSEF_DEFAULT          = 0x00,
        /** The name shall be in the resident nametable. */
        WLDSEF_RESIDENT         = 0x01,
        /** The name shall be in the resident nametable. */
        WLDSEF_NONRESIDENT      = 0x02,
        /** The export shall only have ordinal. */
        WLDSEF_NONAME           = 0x03,
        /** @} */
        /** no idea what this implies */
        WLDSEF_NODATA           = 0x04,
        /** From an EXPORT section of a .DEF-file. */
        WLDSEF_DEF_FILE         = 0x80
    }                   fExport;
    /** Export word count. */
    unsigned            cExpWords;
    /** Export Ordinal.
     * 0 means no ordinal. */
    unsigned            uExpOrd;
    /** @} */

    /** Next node in the hash bucket. */
    struct wldsym *     pHashNext;
} WLDSYM, *PWLDSYM;


/**
 * Symbol table.
 */
typedef struct wldsymtab
{
    PWLDSYM     ap[WLDSYM_HASH_SIZE];
} WLDSYMTAB, *PWLDSYMTAB;


/**
 * Weak Pre-Linker Instance.
 */
struct wld
{
    /** Linker flags. */
    unsigned            fFlags;

    /** Global symbols. */
    WLDSYMTAB           Global;
    /** Exported symbols. */
    WLDSYMTAB           Exported;

    /** Module definition file. */
    PWLDMOD             pDef;

    /** Linked list (FIFO) of objects included in the link. */
    PWLDMOD             pObjs;
    PWLDMOD *           ppObjsAdd;

    /** Linked list (FIFO) of libraries to be searched in the link. */
    PWLDLIB             pLibs;
    PWLDLIB *           ppLibsAdd;

    /** string pool for miscellaneous string. */
    struct strpool *    pStrMisc;

    /** @group ILINK Crash Workaround
     * @{ */
    /** Maximum number of externals in a object module. */
    unsigned            cMaxObjExts;

    /** Maximum number of externals in a library module. */
    unsigned            cMaxLibExts;
    /** @} */
};
typedef struct wld WLD;

/** symAdd Action. */
typedef enum { WLDSA_NEW, WLDSA_UP, WLDSA_OLD, WLDSA_ERR }  WLDSYMACTION, *PWLDSYMACTION;




extern void *xrealloc (void *ptr, size_t n);
extern void *xmalloc (size_t n);

/** @group Weak LD - Linker Methods (Private)
 * @{ */
#ifdef WLD_ENABLED_DBG
static void         wldDbg(const char *pszFormat, ...);
#endif
static void         wldInfo(const char *pszFormat, ...);
static int          wldWarn(PWLD pWld, const char *pszFormat, ...);
static int          wldErr(PWLD pWld, const char *pszFormat, ...);
static void         wldIntErr(PWLD pWld, PWLDMOD pMod, const char *pszFile, unsigned iLine, const char *pszFunction);
static unsigned     pass1ReadOMFMod(PWLD pWld, PWLDMOD pMod, int fLibSearch);
/** Parameter structure for wldDefCallback(). */
typedef struct wldDefCallback_param
{
    /** Linker instance. */
    PWLD        pWld;
    /** Name of .def file. */
    PWLDMOD     pMod;
    /** Callback return code. Zero ok; non-zero failure; */
    int         rc;
} WLDDEFCBPARAM, *PWLDDEFCBPARAM;
static int          wldDefCallback(struct _md *pMD, const _md_stmt *pStmt, _md_token eToken, void *pvArg);
/** @} */

/** @group Weak LD - Library Methods (Private)
 * @{ */
static FILE *       libOpen(PWLDLIB pLib);
static void         libClose(PWLDLIB pLib);
static int          libLoadDict(PWLDLIB pLib);
static void         libCloseDict(PWLDLIB pLib);
static int          libTryLoadSymbolThruDictionary(PWLD pWld, PWLDLIB pLib, PWLDSYM pSym, unsigned *pcLoaded);
static int          libLoadUndefSymbols(PWLD pWld, PWLDLIB pLib, PWLDSYM pSym, unsigned *pcLoaded);
static int          libErr(PWLDLIB pLib, const char *pszFormat, ...);
#if 0
static void         libWarn(PWLDLIB pLib, const char *pszFormat, ...);
#endif
/** @} */

/** @group Weak LD - Module Methods (Private)
 * @{ */
static FILE *       modOpen(PWLDMOD pMod);
static void         modClose(PWLDMOD pMod);
static int          modErr(PWLDMOD pMod, const char *pszFormat, ...);
static void         modWarn(PWLDMOD pMod, const char *pszFormat, ...);
/** @} */

/** @group Weak LD - Symbole Methods (Private)
 * @{ */
typedef int (*PFNSYMENUM)(PWLD pWld, PWLDSYM pSym, void *pvUser);
static int          symHaveUndefined(PWLD pWld);
static int          symEnum(PWLD pWld, PWLDSYMTAB pSymTab, unsigned fFlags, unsigned fMask, PFNSYMENUM pfnEnum, void *pvUser);
static int          symPrintUnDefEnum(PWLD pWld, PWLDSYM pSym, void *pvUser);
static int          symMatchUnDef(PWLD pWld, const unsigned char *pachPascalString, PWLDSYM pSym);
/** Pointed to by the pvUser parameter of symSearchLibEnum(). */
typedef struct symSearchLibEnum_param
{
    /** Library to search for symbols in. */
    PWLDLIB     pLib;
    /** Number modules which was loaded. */
    unsigned    cLoaded;
} WLDSLEPARAM, *PWLDSLEPARAM;
static int          symSearchLibEnum(PWLD pWld, PWLDSYM pSym, void *pvUser);
static inline unsigned symHash(const char* pszSym, unsigned cch, unsigned fWldCaseFlag);
static const char * symGetDescr(PWLDSYM pSym);
static void         symDumpReferers(PWLDSYM pSym);
static void         symDbg(PWLDSYM pSym, const char *pszMsg);
static PWLDSYM      symAdd(PWLD pWld, PWLDMOD pMod, unsigned fFlags, const char *pachName, int cchName, PWLDSYMTAB pSymTab, PWLDSYMACTION peAction);
static PWLDSYM      symAddImport(PWLD pWld, PWLDMOD pMod, int fLibSearch,
                                 const char *pachName, int cchName,
                                 const char *pachImpName, int cchImpName,
                                 const char *pachModName, int cchModName,
                                 unsigned uOrdinal);
static PWLDSYM symAddExport(PWLD pWld, PWLDMOD pMod, int fLibSearch,
                            unsigned    fExport,
                            unsigned    cExpWords,
                            const char *pachExpName, int cchExpName,
                            const char *pachIntName, int cchIntName,
                            unsigned uOrdinal);
static PWLDSYM symAddPublic(PWLD pWld, PWLDMOD pMod, int fLibSearch,
                            const char *pachName, int cchName,
                            unsigned long ulValue, int iSegment, int iGroup);
static PWLDSYM symAddUnDef(PWLD pWld, PWLDMOD pMod, int fLibSearch,
                           const char *pachName, int cchName);
static PWLDSYM symAddAlias(PWLD pWld, PWLDMOD pMod, int fLibSearch,
                           const char *pachAliasName, int cchAliasName,
                           const char *pachName, int cchName);
static PWLDSYM symAddComdef(PWLD pWld, PWLDMOD pMod, int fLibSearch,
                            const char *pachName, int cchName,
                            signed long cElements, signed long cbElement);
/** @} */









/*=============================================================================
 *                                                                            *
 *                                                                            *
 *  W E A K   L I N K E R   M E T H O D S                                     *
 *  W E A K   L I N K E R   M E T H O D S                                     *
 *  W E A K   L I N K E R   M E T H O D S                                     *
 *  W E A K   L I N K E R   M E T H O D S                                     *
 *  W E A K   L I N K E R   M E T H O D S                                     *
 *                                                                            *
 *                                                                            *
 *============================================================================*/


#ifdef WLD_ENABLED_DBG
/**
 * Put out a debug message.
 * @param   pszFormat   Format string.
 * @param   ...         Format arguments.
 */
static void         wldDbg(const char *pszFormat, ...)
{
    va_list     args;
    fprintf(stderr, "weakld: dbg: ");

    va_start(args, pszFormat);
    vfprintf(stderr, pszFormat, args);
    va_end(args);
    if (pszFormat[strlen(pszFormat) - 1] != '\n')
        fputc('\n', stderr);
}
#endif

/**
 * Put out a info message.
 * @param   pszFormat   Format string.
 * @param   ...         Format arguments.
 */
static void         wldInfo(const char *pszFormat, ...)
{
    va_list     args;
    fprintf(stderr, "weakld: info: ");

    va_start(args, pszFormat);
    vfprintf(stderr, pszFormat, args);
    va_end(args);
    if (pszFormat[strlen(pszFormat) - 1] != '\n')
        fputc('\n', stderr);
}

/**
 * Put out a warning message.
 * @param   pszFormat   Format string.
 * @param   ...         Format arguments.
 */
static int          wldWarn(PWLD pWld, const char *pszFormat, ...)
{
    va_list     args;
    fprintf(stderr, "weakld: warning: ");

    va_start(args, pszFormat);
    vfprintf(stderr, pszFormat, args);
    va_end(args);
    if (pszFormat[strlen(pszFormat) - 1] != '\n')
        fputc('\n', stderr);
    return 4;
}

/**
 * Put out a error message.
 * @param   pszFormat   Format string.
 * @param   ...         Format arguments.
 */
static int          wldErr(PWLD pWld, const char *pszFormat, ...)
{
    va_list     args;
    fprintf(stderr, "weakld: error: ");

    va_start(args, pszFormat);
    vfprintf(stderr, pszFormat, args);
    va_end(args);
    if (pszFormat[strlen(pszFormat) - 1] != '\n')
        fputc('\n', stderr);
    return -1;
}


/**
 * Internal error.
 *
 * @returns don't return, it aborts the process.
 * @param   pWld        Pointer to linker instance (optional).
 * @param   pMod        Pointe to module (optional).
 * @param   pszFile     File name of the error.
 * @param   iLine       Line number of the error.
 * @param   pszFunction The function in which the error occured.
 */
static void         wldIntErr(PWLD pWld, PWLDMOD pMod, const char *pszFile, unsigned iLine, const char *pszFunction)
{
    fprintf(stderr, "\nweakld: ");
    if (pMod)
    {
        if (pMod->pLib)
            fprintf(stderr, "%s(%s) ", pMod->pLib->pszLibName, pMod->pszModName);
        else
            fprintf(stderr, "%s ", pMod->pszModName);
    }
    fprintf(stderr, "internal error!");
    fprintf(stderr, "file: %s  line: %d  function: %s\n", pszFile, iLine, pszFunction);
    abort();
}







/*=============================================================================
 *                                                                            *
 *                                                                            *
 *  L I B R A R Y   M E T H O D S                                             *
 *  L I B R A R Y   M E T H O D S                                             *
 *  L I B R A R Y   M E T H O D S                                             *
 *  L I B R A R Y   M E T H O D S                                             *
 *  L I B R A R Y   M E T H O D S                                             *
 *                                                                            *
 *                                                                            *
 *============================================================================*/



/**
 * Open this library file.
 *
 * @returns Pointer to open file stream.
 * @returns NULL on failure.
 * @param   pLib    Library to open.
 */
static FILE *       libOpen(PWLDLIB pLib)
{
    if (!pLib->phFile)
    {
        pLib->phFile = fopen(pLib->pszLibName, "rb");
        if (!pLib->phFile)
            libErr(pLib, "Failed to open library.");
    }
    return pLib->phFile;
}

/**
 * Close this library file.
 * @param   pLib    Library to close.
 */
static void         libClose(PWLDLIB pLib)
{
    if (pLib->phFile)
    {
        fclose(pLib->phFile);
        pLib->phFile = NULL;
    }
}

/**
 * Load the dictionar for this library into memory.
 *
 * @returns 0 if we successfully loaded the dictionary.
 * @returns -1 if we fail to read the dictionary into memory.
 * @returns 1 if there is no dictionary.
 * @param   pLib    Library which dictionary is to be loaded.
 * @remark  This method will open the library. libClose() must be
 *          called after this function is used.
 */
static int          libLoadDict(PWLDLIB pLib)
{
#if 0
    FILE *phFile;
    /* been here, done that? */
    if (pLib->pDict)
        return 0;

    /* check if it acutally is a library and have an ext dict */
    if (    pLib->LibHdr.chType != LIBHDR
        ||  pLib->LibHdr.offDict != 0
        ||  pLib->LibHdr.cDictBlocks != 0)
        return 1;

    /* ensure it's open. */
    phFile = libOpen(pLib);
    if (!phFile)
        return -1;

    /* position us */
    if (fseek(phFile, pLib->LibHdr.offDict, SEEK_SET))
        return libErr(pLib, "Failed to seek to extended dictionary (offset %d).", (int)pLib->LibHdr.offDict);

    /* read it */
    pLib->pDict = xmalloc(pLib->LibHdr.cDictBlocks * 512);
    if (fread(pLib->pDict, 512, pLib->LibHdr.cDictBlocks, phFile) == pLib->LibHdr.cDictBlocks)
        return 0;
    libErr(pLib, "Failed to read extended dictionary.");
    free(pLib->pDict);
    pLib->pDict = NULL;
    return -1;
#else
    /* till we support processing the dictionary, we pretend there is none. */
    pLib->pDict = NULL;
    return -1;
#endif
}

/**
 * This closes the extended dictionary.
 *
 * @param   pLib    Library which extended dictionary should be closed.
 * @remark  Will not close the library file, libClose() must be used for that.
 */
static void         libCloseDict(PWLDLIB pLib)
{
    if (pLib->pDict)
    {
        free(pLib->pDict);
        pLib->pDict = NULL;
    }
}


/**
 * Does a dictionary lookup on an undefined name.
 *
 * @returns 0 on non failure.
 * @returns 42 if not found.
 * @returns -1 on link abort error.
 * @param   pWld    Linker instance.
 * @param   pLib    Library to search.
 * @param   pSym    Undefined symbol to search for.
 * @param   pcLoaded    Number of modules which was loaded from this library.
 */
static int          libTryLoadSymbolThruDictionary(PWLD pWld, PWLDLIB pLib, PWLDSYM pSym, unsigned *pcLoaded)
{
    return libLoadUndefSymbols(pWld, pLib, pSym, pcLoaded); /* @todo implement this function! */
}


/**
 * Read thru an module looking for definitions for undef symbols.
 * If a definition is found we'll load the module containing it.
 *
 * @returns 0 on non failure.
 * @returns 42 if none found.
 * @returns -1 on link abort error.
 * @param   pWld    Linker instance.
 * @param   pLib    Library to search.
 * @param   pSym    Undefined symbol to search for.
 *                  If NULL we'll try and see if any defined global symbol we
 *                  encounter is undefined.
 * @param   pcLoaded    Number of modules which was loaded from this library.
 */
static int          libLoadUndefSymbols(PWLD pWld, PWLDLIB pLib, PWLDSYM pSym, unsigned *pcLoaded)
{
    FILE *          phFile = pLib->phFile;
    unsigned char   uchEnd1, uchEnd2;
    OMFREC          OmfRec;
    off_t           offCurMod = 0;
    int             fSkipRestOfModule = 0;
    /* generic stuff */
    unsigned long   ul;
    signed long     l2, l3;
    unsigned short  us, us2, us3;
    unsigned char   uch, uch2;


    /* Position the library at the first module record. */
    if (fseek(phFile, pLib->LibHdr.chType == LIBHDR ? pLib->LibHdr.cb + 3 : 0, SEEK_SET))
        return libErr(pLib, "Error when seeking to first module.");

    if (pLib->LibHdr.chType != LIBHDR)
    {
        uchEnd1 = MODEND;
        uchEnd2 = MODEND | REC32;
    }
    else
        uchEnd1 = uchEnd2 = LIBEND;

    OmfRec.chType = uchEnd1;
    fread(&OmfRec, sizeof(OmfRec), 1, phFile);
    while (OmfRec.chType != uchEnd1 &&  OmfRec.chType != uchEnd2)
    {
        int fRead = 0;
        int fLoad = 0;
        switch (OmfRec.chType)
        {
            case THEADR:
                fSkipRestOfModule = 0;
                offCurMod = ftell(phFile) - sizeof(OmfRec);
                break;

            /* read */
            case PUBDEF: case PUBDEF | REC32:
            case ALIAS:  case ALIAS  | REC32:
            case COMDEF: case COMDEF | REC32:
            case COMDAT: case COMDAT | REC32:
            case COMENT: case COMENT | REC32:
                fRead = !fSkipRestOfModule;
                break;
        }

        if (fRead)
        {
            unsigned char   achBuffer[OMF_MAX_REC + 8];
            union
            {
                unsigned char *     puch;
                signed char *       pch;
                unsigned short *    pus;
                signed short *      ps;
                unsigned long *     pul;
                signed long *       pl;
                void *              pv;
            } u, u1, u2;

            /** macro for getting a OMF index out of the buffer */
            #define OMF_GETINDEX()  (*u.puch & 0x80 ? ((*u.pch++ & 0x7f) << 8) + *u.pch++ : *u.pch++)
            #define OMF_BYTE()      (*u.puch++)
            #define OMF_WORD()      (*u.pus++)
            #define OMF_24BITWORD() (OMF_BYTE() | (OMF_WORD() << 8))
            #define OMF_DWORD()     (*u.pul++)
            #define OMF_MORE()      (u.puch - &achBuffer[0] < (int)OmfRec.cb - 1 && !fLoad) /* (different from the next) */
            #define OMF_IS32BIT()   ((OmfRec.chType & REC32) != 0)
            #define OMF_GETTYPELEN(l) \
                do                                                       \
                {                                                        \
                    l = OMF_BYTE();                                      \
                    if (l > 128)                                         \
                        switch (l)                                       \
                        {                                                \
                            case 0x81: l = OMF_WORD(); break;            \
                            case 0x84: l = OMF_24BITWORD(); break;       \
                            case 0x88: l = OMF_DWORD(); break;           \
                            default:                                     \
                                libErr(pLib, "Invalid type length!");/* (different from the next) */ \
                                return -1;                               \
                        }                                                \
                } while (0)

            u.pv = &achBuffer[0];

            /* read it */
            if (fread(achBuffer, OmfRec.cb, 1, phFile) != 1)
            {
                libErr(pLib, "Read error. (2)");
                break;
            }

            /* extract public symbols. */
            switch (OmfRec.chType)
            {
                case COMENT: case COMENT | REC32:
                    uch = OMF_BYTE(); /* comment type */
                    uch = OMF_BYTE(); /* comment class */
                    switch (uch)
                    {
                        case CLASS_PASS:
                            fSkipRestOfModule = 1;
                            break;
                        case CLASS_OMFEXT:
                        {
                            switch (OMF_BYTE())
                            {   /*
                                 * Import definition.
                                 */
                                case OMFEXT_IMPDEF:
                                {
                                    uch = OMF_BYTE();               /* flags */
                                    u1 = u; u.pch += 1 + *u.puch;   /* internal name */
                                    u2 = u; u.pch += 1 + *u.puch;   /* module name */
                                    ul = 0;                         /* ordinal */
                                    if (uch & 1)
                                        ul = OMF_WORD();
                                    if (symMatchUnDef(pWld, u1.pch, pSym))
                                        fLoad = 1;
                                    break;
                                }
                            }
                        }
                    } /* comment class */
                    break;

                case PUBDEF: case PUBDEF | REC32:
                {
                    us2 = OMF_GETINDEX();           /* group index */
                    us3 = OMF_GETINDEX();           /* segment index */
                    if (!us3)
                        us = OMF_WORD();            /* base frame - ignored */
                    while (OMF_MORE())
                    {
                        u1 = u; u.pch += 1 + *u.puch;   /* public name */
                        ul = OMF_IS32BIT() ? OMF_DWORD() : OMF_WORD();
                        us = OMF_GETINDEX();            /* typeindex */
                        if (symMatchUnDef(pWld, u1.pch, pSym))
                            fLoad = 1;
                    }
                    break;
                }

                case ALIAS: case ALIAS | REC32:
                {
                    while (OMF_MORE())
                    {
                        u1 = u; u.pch += 1 + *u.puch;   /* alias name */
                        u2 = u; u.pch += 1 + *u.puch;   /* substitutt name. */
                        if (symMatchUnDef(pWld, u1.pch, pSym))
                            fLoad = 1;
                    }
                    break;
                }

                case COMDEF: case COMDEF | REC32:
                {
                    while (OMF_MORE())
                    {
                        u1 = u; u.pch += 1 + *u.puch;   /* communal name (specs say 1-2 length...) */
                        us2 = OMF_GETINDEX();           /* typeindex */
                        uch2 = OMF_BYTE();              /* date type */
                        switch (uch2)
                        {
                            case COMDEF_TYPEFAR:
                                OMF_GETTYPELEN(l2);     /* number of elements */
                                OMF_GETTYPELEN(l3);     /* element size */
                                break;
                            case COMDEF_TYPENEAR:
                                l2 = 1;                 /* number of elements */
                                OMF_GETTYPELEN(l3);     /* element size */
                                break;
                            default:
                                libErr(pLib, "Invalid COMDEF type %x.", (int)uch2);
                                return -1;
                        }
                        if (symMatchUnDef(pWld, u1.pch, pSym))
                            fLoad = 1;
                    }
                    break;
                }

                case COMDAT: case COMDAT | REC32:
                {
                    /* @todo */
                    break;
                }
            } /* switch */

            #undef OMF_GETINDEX
            #undef OMF_BYTE
            #undef OMF_WORD
            #undef OMF_24BITWORD
            #undef OMF_DWORD
            #undef OMF_MORE
            #undef OMF_IS32BIT
            #undef OMF_GETTYPELEN

            /*
             * Shall we load this module?
             */
            if (fLoad)
            {
                off_t   offSave = ftell(phFile);
                PWLDMOD pMod;
                int     rc;

                pMod = xmalloc(sizeof(*pMod));
                memset(pMod, 0, sizeof(*pMod));
                pMod->off = offCurMod;
                pMod->pLib = pLib;
                pMod->phFile = phFile;
                *pWld->ppObjsAdd = pMod;
                pWld->ppObjsAdd = &pMod->pNext;

                rc = pass1ReadOMFMod(pWld, pMod, 1);
                if (rc)
                {
                    libErr(pLib, "Failed when reading module at offset %x.", (int)offCurMod);
                    return rc;
                }

                /* update statistics */
                if (pcLoaded)
                    (*pcLoaded)++;

                /* if one symbol, we're done now */
                if (pSym)
                    return 0;

                /* Resume searching, but skip the rest of this one */
                fSkipRestOfModule = 1;
                fseek(phFile, offSave, SEEK_SET);
            }
        }
        else
        {
            off_t   offSkip = OmfRec.cb;
            /* Skip to next record. */
            if (OmfRec.chType == MODEND || OmfRec.chType == (MODEND | REC32))
            {
                unsigned    cbPage = pLib->LibHdr.cb + 3;
                off_t       off = ftell(phFile) + offSkip;
                off -= cbPage * (off / cbPage); /* don't trust this to be 2**n. */
                if (off)
                    offSkip += cbPage - off;
            }
            if (fseek(phFile, offSkip, SEEK_CUR))
            {
                libErr(pLib, "Seek error.");
                break;
            }
        }

        /* next header */
        if (fread(&OmfRec, sizeof(OmfRec), 1, phFile) != 1)
        {
            libErr(pLib, "Read error.");
            break;
        }
    }


    return 42;
}


/**
 * Put out an error for this library.
 * @param   pLib        Library which the warning occured in.
 * @param   pszFormat   Message format.
 * @param   ...         Format args.
 */
static int          libErr(PWLDLIB pLib, const char *pszFormat, ...)
{
    va_list     args;
    fprintf(stderr, "weakld: %s: error: ", pLib->pszLibName);

    va_start(args, pszFormat);
    vfprintf(stderr, pszFormat, args);
    va_end(args);
    if (pszFormat[strlen(pszFormat) - 1] != '\n')
        fputc('\n', stderr);
    return -1;
}

#if 0
/**
 * Put out a warning for this library.
 * @param   pLib        Library which the warning occured in.
 * @param   pszFormat   Message format.
 * @param   ...         Format args.
 */
static void         libWarn(PWLDLIB pLib, const char *pszFormat, ...)
{
    va_list     args;
    fprintf(stderr, "weakld: %s: warning: ", pLib->pszLibName);

    va_start(args, pszFormat);
    vfprintf(stderr, pszFormat, args);
    va_end(args);
    if (pszFormat[strlen(pszFormat) - 1] != '\n')
        fputc('\n', stderr);
}
#endif







/*=============================================================================
 *                                                                            *
 *                                                                            *
 *  M O D U L E   M E T H O D S                                               *
 *  M O D U L E   M E T H O D S                                               *
 *  M O D U L E   M E T H O D S                                               *
 *  M O D U L E   M E T H O D S                                               *
 *  M O D U L E   M E T H O D S                                               *
 *                                                                            *
 *                                                                            *
 *============================================================================*/


/**
 * Opens the module (if required) file for reading.
 *
 * @returns Pointer to file stream.
 * @param   pMod    Module to open.
 */
static FILE *   modOpen(PWLDMOD pMod)
{
    const char *pszFilename;

    /* open the file */
    if (!pMod->phFile)
    {
        if (pMod->pLib && pMod->pLib->phFile)
            pMod->phFile = pMod->pLib->phFile;
        else
        {   /* fopen it */
            if (pMod->pLib)
            {
                pszFilename = pMod->pLib->pszLibName;
                pMod->phFile = pMod->pLib->phFile = fopen(pszFilename, "rb");
            }
            else
            {
                pszFilename = pMod->pszModName;
                pMod->phFile = fopen(pszFilename, "rb");
            }
        }
    }

    /* Position the stream at the start of the module. */
    if (!pMod->phFile)
        modErr(pMod, "failed to reopen.");
    else
    {
        if (fseek(pMod->phFile, pMod->off, SEEK_SET))
        {
            modErr(pMod, "failed to seek to module start (%#x).", (int)pMod->off);
            modClose(pMod);
            return NULL;
        }
    }

    return pMod->phFile;
}

/**
 * Closes the module.
 *
 * @param   pMod    Module to close.
 */
static void     modClose(PWLDMOD pMod)
{
    if (!pMod->phFile)
        return;
    if (!pMod->pLib || pMod->pLib->phFile != pMod->phFile)
        fclose(pMod->phFile);
    pMod->phFile = NULL;
}

/**
 * Report error in module.
 * @param   pMod        Pointer to module to report error on.
 * @param   pszFormat   Format string.
 * @param   ...         Format arguments.
 */
static int      modErr(PWLDMOD pMod, const char *pszFormat, ...)
{
    va_list     args;
    if (pMod->pLib)
        fprintf(stderr, "weakld: %s(%s) - error: ", pMod->pLib->pszLibName, pMod->pszModName);
    else
        fprintf(stderr, "weakld: %s - error: ", pMod->pszModName);

    va_start(args, pszFormat);
    vfprintf(stderr, pszFormat, args);
    va_end(args);
    if (pszFormat[strlen(pszFormat) - 1] != '\n')
        fputc('\n', stderr);
    return -1;
}

/**
 * Report warning in module.
 * @param   pMod        Pointer to module to report warning on.
 * @param   pszFormat   Format string.
 * @param   ...         Format arguments.
 */
static void     modWarn(PWLDMOD pMod, const char *pszFormat, ...)
{
    va_list     args;
    if (pMod->pLib)
        fprintf(stderr, "weakld: %s(%s) - warning: ", pMod->pLib->pszLibName, pMod->pszModName);
    else
        fprintf(stderr, "weakld: %s - warning: ", pMod->pszModName);

    va_start(args, pszFormat);
    vfprintf(stderr, pszFormat, args);
    va_end(args);
    if (pszFormat[strlen(pszFormat) - 1] != '\n')
        fputc('\n', stderr);
}










/*=============================================================================
 *                                                                            *
 *                                                                            *
 *  S Y M B O L   M E T H O D S                                               *
 *  S Y M B O L   M E T H O D S                                               *
 *  S Y M B O L   M E T H O D S                                               *
 *  S Y M B O L   M E T H O D S                                               *
 *                                                                            *
 *                                                                            *
 *============================================================================*/


/**
 * Calculate the hash value of a symbol.
 * @returns hash value.
 * @param   pszSym          Symbol to calculate it for.
 * @param   cch             Symbol length.
 * @param   fWldCaseFlag    Case flag from the linker instance.
 *                          Not implemented yet.
 * @todo    This ain't respecting case sensitivity.
 */
static inline unsigned symHash(const char* pszSym, unsigned cch, unsigned fWldCaseFlag)
{
    /* hash alg: original djb2. */
    unsigned uHash = 5381;
    while (     cch
           &&   (pszSym[0] != '$' || pszSym[1] != 'w' || pszSym[2] != '$')
             )
    {
        if (    pszSym[0] == '!'
            &&  pszSym[1] == '_'
            &&  cch > 200)
        {
            uHash = strtol(&pszSym[2], NULL, 16);
            break;
        }
        uHash += (uHash << 5) + *pszSym;
        pszSym++;
        cch--;
    }
    return uHash;
}



/**
 * Find the symbol by the name pszName.
 * @returns Pointer to matching symbol.
 * @returns NULL if not found.
 * @param   pWld    Linker Instance.
 * @param   pszName Name of the symbol to find.
 * @param   pSymTab The symbol table to look it up in.
 */
static PWLDSYM      symLookupCommon(PWLD pWld, const char *pszName, PWLDSYMTAB pSymTab)
{
    PWLDSYM     pSym;
    const char *psz;
    unsigned    cchName;
    unsigned    fFlags = 0;
    unsigned    uHash = 0;

    /*
     * It's easier just to add it to the string table than starting to
     * check the correct case function and such. As there is a good
     * likelyhood that the symbol exists there is little expense in doing
     * this compared to the convenience. If it's slow we'll optimize grow.c
     * (if possible) and gain everywhere.
     */

    /* look for weak suffix and trucation */
    cchName = strlen(pszName);
    for (psz = pszName + cchName - 1; psz > pszName; psz--)
        if (    psz[0] == '$'
            &&  psz[1] == 'w'
            &&  psz[2] == '$')
        {
            cchName = psz - pszName;
            if (cchName > 200)
                break;
        }

    pszName = (pWld->fFlags & WLDC_CASE_INSENSITIVE ? strpool_addnu : strpool_addn)(pWld->pStrMisc, pszName, cchName);
    if (!fFlags)
        uHash = symHash(pszName, cchName, pWld->fFlags & WLDC_CASE_INSENSITIVE);

    /* look it up */
    for (pSym = pSymTab->ap[uHash % WLDSYM_HASH_SIZE]; pSym; pSym = pSym->pHashNext)
       if (SYM_EQUAL(pWld, pSym, pszName, fFlags, uHash, cchName))
           return pSym;

    return NULL;
}


/**
 * Find the symbol by the name pszName.
 *
 * @returns Pointer to matching symbol.
 * @returns NULL if not found.
 * @param   pWld    Linker Instance.
 * @param   pszName Name of the symbol to find.
 */
static PWLDSYM      symLookup(PWLD pWld, const char *pszName)
{
    return symLookupCommon(pWld, pszName, &pWld->Global);
}


/**
 * Find the export symbol by the name pszName.
 *
 * @returns Pointer to matching symbol.
 * @returns NULL if not found.
 * @param   pWld    Linker Instance.
 * @param   pszName Name of the symbol to find.
 */
static PWLDSYM      symLookupExport(PWLD pWld, const char *pszName)
{
    return symLookupCommon(pWld, pszName, &pWld->Exported);
}


/**
 * Symbol enumerator.
 *
 * @returns return code from last pfnEnum call.
 * @param   pWld    Weak Linker Instance.
 * @param   pSymTab The symbol table to enumerate.
 * @param   fFlags  The flags which (pSym->fFlags & fMask) much equal.
 * @param   fMask   The flag mask.
 * @param   pfnEnum Enumeration callback function.
 * @param   pvUser  User arguments to pfnEnum.
 */
static int          symEnum(PWLD pWld, PWLDSYMTAB pSymTab, unsigned fFlags, unsigned fMask, PFNSYMENUM pfnEnum, void *pvUser)
{
    int         i;
    PWLDSYM     pSym;
    int         rc;

    for (i = 0; i < sizeof(pSymTab->ap) / sizeof(pSymTab->ap[0]); i++)
    {
        for (pSym = pSymTab->ap[i]; pSym; pSym = pSym->pHashNext)
        {
            if ((pSym->fFlags & fMask) == fFlags)
            {
                rc = pfnEnum(pWld, pSym, pvUser);
                if (rc)
                    return rc;
            }
        }
    }

    return 0;
}


/**
 * Worker for wldHaveUndefined().
 * @returns 42 and halts the search.
 * @param   pWld    Linker instance.
 * @param   pSym    Symbol.
 * @param   pvUser  Pointer to a FILE stream.
 */
static int symHaveUndefinedEnum(PWLD pWld, PWLDSYM pSym, void *pvUser)
{
    return 42;
}

/**
 * Checks if there is unresovled symbols in the link.
 *
 * @returns 1 if there is undefined symbols
 * @returns 0 if all symbols are defined.
 * @param   pWld    Linker instance.
 */
static int symHaveUndefined(PWLD pWld)
{
    return symEnum(pWld, &pWld->Global, WLDSF_UNDEF, WLDSF_TYPEMASK | WLDSF_WEAK, symHaveUndefinedEnum, NULL) == 42;
}


/**
 * Enumerates the current undefined externals and try to resolve
 * them using the current library passed in the pvUser structure.
 * @returns
 * @param   pWld    Linker instance.
 * @param   pSym    Undefined symbol.
 * @param   pvUser  Pointer to a WLDSLEPARAM structure.
 *                  fMore will be set
 */
static int symSearchLibEnum(PWLD pWld, PWLDSYM pSym, void *pvUser)
{
    int             rc;
    unsigned        cLoaded = 0;
    PWLDSLEPARAM    pParam = (PWLDSLEPARAM)pvUser;

    SYMDBG(pSym, "Searching for");

    /*
     * If we have a dictionary, we'll us it.
     */
    if (pParam->pLib->pDict)
        rc = libTryLoadSymbolThruDictionary(pWld, pParam->pLib, pSym, &cLoaded);
    else
        rc = libLoadUndefSymbols(pWld, pParam->pLib, pSym, &cLoaded);

    /* Housekeeping. */
    pParam->cLoaded += cLoaded;
    if (rc == 42) /* more undef from the load. */
        rc = 0;

    return rc;
}

/**
 * Worker for enumerating unresolved symbols.
 *
 * @returns 0
 * @param   pWld    Linker instance.
 * @param   pSym    Undefined symbol.
 * @param   pvUser  NULL
 */
static int          symPrintUnDefEnum(PWLD pWld, PWLDSYM pSym, void *pvUser)
{
    PWLDMOD pMod = pSym->pMod;

    if (pMod)
        modErr(pMod, "Unresolved symbol (%s) '%s'.", symGetDescr(pSym), pSym->pszName);
    else
        wldErr(pWld, "Unresolved symbol (%s) '%s'.", symGetDescr(pSym), pSym->pszName);
    symDumpReferers(pSym);
    return 0;
}

/**
 * Prints unresolved symbols.
 *
 * @param   pWld    Linker instance.
 */
static void         symPrintUnDefs(PWLD pWld)
{
    symEnum(pWld, &pWld->Global, WLDSF_UNDEF, WLDSF_TYPEMASK | WLDSF_WEAK, symPrintUnDefEnum, NULL);
}

/**
 * Checks the OMF encoded name with the specified undefined
 * symbol, or all undefined symbols.
 *
 * @returns 1 if symbol matches.
 * @returns 0 if symbol mis-matches.
 * @param   pWld                Linker instance.
 * @param   pachPascalString    OMF encoded string.
 * @param   pSym                If NULL match all, if !NULL match this.
 */
static int          symMatchUnDef(PWLD pWld, const unsigned char *pachPascalString, PWLDSYM pSym)
{
    int         cchName = *pachPascalString;
    const char *pszName = pachPascalString + 1;
    const char *psz;
    unsigned    fFlags = 0;
    unsigned    uHash = 0;
    int        (*pfn)(const char *, const char *, size_t) = pWld->fFlags & WLDC_CASE_INSENSITIVE ? strnicmp : strncmp;

    /* look for weak suffix and trucation */
    for (psz = pszName + cchName - 1; psz > pszName; psz--)
        if (    psz[0] == '$'
            &&  psz[1] == 'w'
            &&  psz[2] == '$')
        {
            cchName = psz - pszName;
            if (cchName > 200)
                break;
        }

    /* @todo: this isn't 100% correct when we're talking case in sensitivity. */
    if (!fFlags)
        uHash = symHash(pszName, cchName, pWld->fFlags & WLDC_CASE_INSENSITIVE);

    /* compare */
    if (pSym)
        return SYM_EQUAL2(pWld, pSym, pszName, fFlags, uHash, cchName, pfn);
    else
    {
        for (pSym = pWld->Global.ap[uHash % WLDSYM_HASH_SIZE]; pSym; pSym = pSym->pHashNext)
        {
            if ((pSym->fFlags & (WLDSF_TYPEMASK | WLDSF_WEAK)) == WLDSF_UNDEF)
            {
                if (SYM_EQUAL2(pWld, pSym, pszName, fFlags, uHash, cchName, pfn))
                    return 1;
            }
        }
    }
    return 0;
}


/**
 * Makes a description of the symbol.
 *
 * @returns Pointer to static string buffer.
 * @param   pSym    Symbol to describe.
 */
static const char *symGetDescr(PWLDSYM pSym)
{
    static char szDesc[256];
    char *psz = &szDesc[0];

    switch (pSym->fFlags & WLDSF_TYPEMASK)
    {
        case WLDSF_PUBLIC:  psz += sprintf(psz, "PUBLIC"); break;
        case WLDSF_COMM:    psz += sprintf(psz, "COMM"); break;
        case WLDSF_IMPORT:  psz += sprintf(psz, "IMPORT"); break;
        case WLDSF_UNDEF:   psz += sprintf(psz, "UNDEF"); break;
        case WLDSF_WKEXT:   psz += sprintf(psz, "WKEXT"); break;
        case WLDSF_EXP_NM:  psz += sprintf(psz, "EXPORT"); break;
        default:            psz += sprintf(psz, "!!!internal error!!! "); asm("int $3"); break;
    }
    if (pSym->fFlags & WLDSF_WEAK)
        psz += sprintf(psz, " WEAK");
    if (pSym->fFlags & WLDSF_ALIAS)
        psz += sprintf(psz, " ALIAS");
    if (pSym->fFlags & WLDSF_UNCERTAIN)
        psz += sprintf(psz, " UNCERTAIN");

    return &szDesc[0];
}


/**
 * Dumps the list of modules referencing a symbol.
 *
 * @param   pSym    Symbol in question.
 */
static void symDumpReferers(PWLDSYM pSym)
{
    if (pSym->cReferers)
    {
        int i;
        wldInfo("The symbol is referenced by:\n");
        for (i = 0; i < pSym->cReferers; i++)
        {
            PWLDMOD pMod = pSym->paReferers[i];
            if (pMod->pLib)
                fprintf(stderr, "    %s(%s)\n", pMod->pLib->pszLibName, pMod->pszModName);
            else
                fprintf(stderr, "    %s\n", pMod->pszModName);
        }
    }
}



/**
 * Symbol debug output.
 * @param   pSym    Symbol to dump.
 * @param   pszMsg  Message to put first in the dump.
 */
static void         symDbg(PWLDSYM pSym, const char *pszMsg)
{
    if (pszMsg)
        fprintf(stderr, "weakld: dbg: %s:", pszMsg);
    else
        fprintf(stderr, "weakld: dbg:");
    if (pSym)
        fprintf(stderr, " '%s' %s", pSym->pszName, symGetDescr(pSym));
    else
        fprintf(stderr, " <Symbol is NULL>");
    fprintf(stderr, "\n");
}

/**
 * Adds a symbol.
 *
 * Actually if the symbol exists we'll perform any required symbol 'merger' and
 * either fail due to symbol errors or return the 'merged' one.
 *
 * @returns Pointer to symbold.
 * @returns NULL on failure.
 * @param   pWld        Linker instance.
 * @param   pMod        Module the the symbol is defined in.
 * @param   fFlags      Symbol flags.
 *                      All the flags in WLDSF_TYPEMASK, WLDSF_WEAK and WLDSF_LIBSEARCH.
 *                      WLDSF_ALIAS and WLDSF_UNCERTAIN is ignored as they have no
 *                      sideeffects when resolving symbols.
 * @param   pachName    Pointer to symbol name.
 * @param   cchName     Length to add, use -1 if zeroterminated.
 * @param   pSymTab     The symbol table to add it to.
 * @param   pflAction   What we actually did.
 *                      WLDSA_NEW, WLDSA_UP, WLDSA_OLD, WLDSA_ERR.
 * @sketch
 *
 * We'll simply return existing symbol when:
 *      1. adding a UNDEF where a PUBLIC, COMM, !WEAK UNDEF or IMPORT exists.
 *      2. adding a WKEXT where a PUBLIC or COMM exists.
 *      3. adding a WKEXT where a UNDEF which isn't UNCERTAIN exists.
 *      4. adding a COMM where a !WEAK COMM or PUBLIC exists.
 *      5. adding a WEAK PUBLIC or WEAK COMM where a PUBLIC or COMM exists.
 *      6. adding a WEAK UNDEF where WEAK UNDEF exists.
 *
 * We'll warn and return existing symbol when:
 *      1. adding a IMPORT LIBSEARCH where a PUBLIC or COMM exists.
 *
 * We'll return upgraded existing symbol when:
 *      1. adding a PUBLIC, COMM or IMPORT where a UNDEF or WKEXT exists.
 *      2. adding a !WEAK PUBLIC or !WEAK COMM where a WEAK PUBLIC or WEAK COMM exists.
 *      3. adding a !WEAK UNDEF where a WEAK UNDEF exists.
 *      4. adding a UNDEF where a WKEXT exists.
 *      5. adding a IMPORT where a WEAK exists.
 *      6. adding a PUBLIC where a COMM exists.
 *
 * We'll warn and upgraded existing symbol when:
 *      1. adding a PUBLIC or COMM where a IMPORT LIBSEARCH exists.
 *
 * The rest is failures.
 *
 * There migth be errors in the algorithm. Like adding the same import twice
 * shouldn't harm anybody, but it's unlikely and it requires quite some extra parameters.
 * Also the caller must resolve any conflicting exports (which normally only yields
 * warnings anyway it seems).
 *
 */
static PWLDSYM      symAdd(PWLD pWld, PWLDMOD pMod, unsigned fFlags, const char *pachName, int cchName, 
                           PWLDSYMTAB pSymTab, PWLDSYMACTION peAction)
{
    PWLDSYM     pSym;                   /* The symbol. */
    unsigned    uHash = 0;              /* The symbol name hash. */
    const char *pszName;                /* The symbol name in the string pool */
    int         cchNameWeak = 0;        /* Indicator and length of the weak name. (0 if not weak) */
    /* general stuff */
    const char *    pach;

    if (peAction)
        *peAction = WLDSA_ERR;
    if (cchName < 0)
        cchName = strlen(pachName);

    /* adjust namelength / check for weak name and trucation / hash name */
    pach = pachName + cchName - 2;      /* "$w$" */
    while (pach-- > pachName)
    {
        if (    pach[0] == '$'
            &&  pach[1] == 'w'
            &&  pach[2] == '$')
        {
            cchNameWeak = cchName;
            cchName = pach - pachName;
            fFlags |= WLDSF_WEAK;
            /* If WKEXT we'll upgrade it to weak undefined. (yeah, big deal?) */
            if ((fFlags & WLDSF_TYPEMASK) == WLDSF_WKEXT)
                fFlags = (fFlags & ~WLDSF_TYPEMASK) | WLDSF_UNDEF;

            if (cchName <= 200)
                break;
        }
    }
    pszName = (pWld->fFlags & WLDC_CASE_INSENSITIVE ? strpool_addnu : strpool_addn)(pWld->pStrMisc, pachName, cchName);
    uHash = symHash(pszName, cchName, pWld->fFlags & WLDC_CASE_INSENSITIVE);

    /* search for existing symbol  */
    for (pSym = pSymTab->ap[uHash % WLDSYM_HASH_SIZE]; pSym; pSym = pSym->pHashNext)
        if (SYM_EQUAL(pWld, pSym, pszName, fFlags, uHash, cchName))
            break;

    if (!pSym)
    {
        /*
         * new symbol - this is easy!
         */
        pSym = xmalloc(sizeof(*pSym));
        memset(pSym, 0, sizeof(*pSym));
        pSym->fFlags  = fFlags;
        pSym->pszName = pszName;
        pSym->uHash   = uHash;
        if (cchNameWeak)
        {
            pSym->pszWeakName = strpool_addn(pWld->pStrMisc, pachName, cchNameWeak);
            pSym->fFlags |= WLDSF_WEAK;
            WLDINFO(pWld, ("Weak symbol '%s'.", pSym->pszWeakName));
        }
        pSym->pHashNext = pSymTab->ap[uHash % WLDSYM_HASH_SIZE];
        pSymTab->ap[uHash % WLDSYM_HASH_SIZE] = pSym;
        if (peAction) *peAction = WLDSA_NEW;
        WLDDBG2(("symAdd: New symbol '%s'", pSym->pszName));
    }
    else
    {   /* found existing symbol - more complex... */

        /*
         * We'll simply return existing symbol when:
         *      1. adding a UNDEF where a PUBLIC, COMM, !WEAK UNDEF or IMPORT exists.
         *      2. adding a WKEXT where a PUBLIC or COMM exists.
         *      3. adding a WKEXT where a UNDEF which isn't UNCERTAIN exists.
         *      4. adding a COMM where a !WEAK COMM or PUBLIC exists.
         *      5. adding a WEAK PUBLIC or WEAK COMM where a PUBLIC or COMM exists.
         *      6. adding a WEAK UNDEF where WEAK UNDEF exists.
         *      7. adding a EXP_NM that already exists.
         */
        if ( (     /* 1 */
                (fFlags & WLDSF_TYPEMASK) == WLDSF_UNDEF
             && ((pSym->fFlags & WLDSF_TYPEMASK) == WLDSF_PUBLIC || (pSym->fFlags & WLDSF_TYPEMASK) == WLDSF_COMM
                 || (pSym->fFlags & (WLDSF_TYPEMASK | WLDSF_WEAK)) == WLDSF_UNDEF || (pSym->fFlags & (WLDSF_TYPEMASK)) == WLDSF_IMPORT)
            ) || ( /* 2 */
                (fFlags & WLDSF_TYPEMASK) == WLDSF_WKEXT
             && ((pSym->fFlags & WLDSF_TYPEMASK) == WLDSF_PUBLIC || (pSym->fFlags & WLDSF_TYPEMASK) == WLDSF_COMM)
            ) || ( /* 3 */
                (fFlags & WLDSF_TYPEMASK) == WLDSF_WKEXT
             && (pSym->fFlags & (WLDSF_TYPEMASK | WLDSF_UNCERTAIN)) == WLDSF_UNDEF
            ) || ( /* 4 */
                (fFlags & WLDSF_TYPEMASK) == WLDSF_COMM
             && ((pSym->fFlags & (WLDSF_TYPEMASK | WLDSF_WEAK)) == WLDSF_COMM || (pSym->fFlags & WLDSF_TYPEMASK) == WLDSF_PUBLIC)
            ) || ( /* 5 */
                ((fFlags & (WLDSF_TYPEMASK | WLDSF_WEAK)) == (WLDSF_PUBLIC | WLDSF_WEAK) || (fFlags & (WLDSF_TYPEMASK | WLDSF_WEAK)) == (WLDSF_COMM | WLDSF_WEAK))
             && ((pSym->fFlags & WLDSF_TYPEMASK) == WLDSF_PUBLIC || (pSym->fFlags & WLDSF_TYPEMASK) == WLDSF_COMM)
            ) || ( /* 6 */
                ((fFlags & (WLDSF_TYPEMASK | WLDSF_WEAK)) == (WLDSF_UNDEF | WLDSF_WEAK))
             && ((pSym->fFlags & (WLDSF_TYPEMASK | WLDSF_WEAK)) == (WLDSF_UNDEF | WLDSF_WEAK))
            ) || ( /* 7 */
                ((fFlags & WLDSF_TYPEMASK) == WLDSF_EXP_NM)
             && ((pSym->fFlags & WLDSF_TYPEMASK) == WLDSF_EXP_NM)
            ))
        {
            if (peAction) *peAction = WLDSA_OLD;
            WLDDBG2(("symAdd: Old symbol '%s'", pSym->pszName));
        }
        /*
         * We'll warn and return existing symbol when:
         *      1. adding a IMPORT LIBSEARCH where a PUBLIC or COMM exists.
         */
        else
        if (    (fFlags & (WLDSF_TYPEMASK | WLDSF_LIBSEARCH)) == (WLDSF_IMPORT | WLDSF_LIBSEARCH)
             && ((pSym->fFlags & WLDSF_TYPEMASK) == WLDSF_PUBLIC || (pSym->fFlags & WLDSF_TYPEMASK) == WLDSF_COMM)
            )
        {
            modWarn(pMod, "Ignoring import '%s' as it's defined already.", pszName);
            if (peAction) *peAction = WLDSA_OLD;
            WLDDBG2(("symAdd: Old symbol '%s'", pSym->pszName));
        }
        /*
         * We'll return upgraded existing symbol when:
         *      1. adding a PUBLIC, COMM or IMPORT where a UNDEF or WKEXT exists.
         *      2. adding a !WEAK PUBLIC or !WEAK COMM where a WEAK PUBLIC or WEAK COMM exists.
         *      3. adding a !WEAK UNDEF where a WEAK UNDEF exists.
         *      4. adding a UNDEF where a WKEXT exists.
         *      5. adding a IMPORT where a WEAK exists.
         *      6. adding a PUBLIC where a COMM exists.
         */
        else
        if ( (     /* 1 */
                ((fFlags & WLDSF_TYPEMASK) == WLDSF_PUBLIC || (fFlags & WLDSF_TYPEMASK) == WLDSF_COMM || (fFlags & WLDSF_TYPEMASK) == WLDSF_IMPORT)
            &&  ((pSym->fFlags & WLDSF_TYPEMASK) == WLDSF_WKEXT || (pSym->fFlags & WLDSF_TYPEMASK) == WLDSF_UNDEF)
            ) || ( /* 2 */
                ((fFlags & (WLDSF_TYPEMASK | WLDSF_WEAK)) == WLDSF_PUBLIC || (fFlags & (WLDSF_TYPEMASK | WLDSF_WEAK)) == WLDSF_COMM)
            &&  ((pSym->fFlags & (WLDSF_TYPEMASK | WLDSF_WEAK)) == (WLDSF_PUBLIC | WLDSF_WEAK) || (pSym->fFlags & (WLDSF_TYPEMASK | WLDSF_WEAK)) == (WLDSF_COMM | WLDSF_WEAK))
            ) || ( /* 3 */
                (fFlags & (WLDSF_TYPEMASK | WLDSF_WEAK)) == WLDSF_UNDEF
            &&  (pSym->fFlags & (WLDSF_TYPEMASK | WLDSF_WEAK)) == (WLDSF_UNDEF | WLDSF_WEAK)
            ) || ( /* 4 */
                (fFlags & WLDSF_TYPEMASK) == WLDSF_UNDEF
            &&  (pSym->fFlags & WLDSF_TYPEMASK) == WLDSF_WKEXT
            ) || ( /* 5 */
                (fFlags & WLDSF_TYPEMASK) == WLDSF_IMPORT
            &&  (pSym->fFlags & WLDSF_WEAK) == WLDSF_WEAK
            ) || ( /* 6 */
                (fFlags & WLDSF_TYPEMASK) == WLDSF_PUBLIC
            &&  (pSym->fFlags & WLDSF_TYPEMASK) == WLDSF_COMM
            ))
        {
            if (!(pSym->fFlags & WLDSF_WEAK) && (fFlags & WLDSF_WEAK) && cchNameWeak)
            {   /* the symbol is upgraded to a weak one - there probably won't be a name though. */
                pSym->pszWeakName = strpool_addn(pWld->pStrMisc, pachName, cchNameWeak);
            }
            if ((fFlags & WLDSF_TYPEMASK) == WLDSF_PUBLIC && (pSym->fFlags & WLDSF_TYPEMASK) == WLDSF_COMM)
                memset(&pSym->u, 0, sizeof(pSym->u));
            pSym->fFlags = (pSym->fFlags & ~(WLDSF_TYPEMASK | WLDSF_WEAK | WLDSF_UNCERTAIN | WLDSF_LIBSEARCH)) | fFlags;
            if (peAction) *peAction = WLDSA_UP;
            WLDDBG2(("symAdd: Upgraded symbol '%s'", pSym->pszName));
        }
        /*
         * We'll warn and upgraded existing symbol when:
         *      1. adding a PUBLIC or COMM where a IMPORT LIBSEARCH exists.
         */
        else
        if (   ((fFlags & WLDSF_TYPEMASK) == WLDSF_PUBLIC || (fFlags & WLDSF_TYPEMASK) == WLDSF_COMM)
            && (pSym->fFlags & (WLDSF_TYPEMASK | WLDSF_LIBSEARCH)) == (WLDSF_IMPORT | WLDSF_LIBSEARCH)
            )
        {
            modWarn(pMod, "Ignoring imported symbol '%s' as it's being defined here.", pszName);

            if (!(pSym->fFlags & WLDSF_WEAK) && (fFlags & WLDSF_WEAK) && cchNameWeak)
            {   /* the symbol is upgraded to a weak one - there probably won't be a name though. */
                pSym->pszWeakName = strpool_addn(pWld->pStrMisc, pachName, cchNameWeak);
            }
            pSym->fFlags = (pSym->fFlags & ~(WLDSF_TYPEMASK | WLDSF_WEAK | WLDSF_UNCERTAIN | WLDSF_LIBSEARCH)) | fFlags;
            if (peAction) *peAction = WLDSA_UP;
            memset(&pSym->u, 0, sizeof(pSym->u));
            WLDDBG2(("symAdd: Upgraded symbol '%s'", pSym->pszName));
        }
        /*
         * That's all, now it's just error left.
         *
         * (Afraid we might end up here without wanting to a few times before
         *  squashing all the bugs in the algorithm.)
         */
        else
        {
            modErr(pMod, "Duplicate symbol '%s' ('%s').", pszName, pSym->pszName);
            if (pSym->pMod)
                modErr(pSym->pMod, "Symbol previosly defined in this module.");
            wldInfo("fFlags new 0x%04x  fFlags old 0x%04x (%s).", fFlags, pSym->fFlags, symGetDescr(pSym));
            symDumpReferers(pSym);
            pSym = NULL;
        }
    }

    /*
     * Maintain the module pointer, referers and truncation aliases.
     */
    if (pSym)
    {
        if (SYM_IS_DEFINED(pSym->fFlags))
            pSym->pMod = pMod;
        else
        {
            int i;
            for (i = 0; i < pSym->cReferers; i++)
                if (pSym->paReferers[i] == pMod)
                    break;
            if (i >= pSym->cReferers)
            {
                if (!(pSym->cReferers % 64))
                    pSym->paReferers = xrealloc(pSym->paReferers, sizeof(pSym->paReferers[0]) * (pSym->cReferers + 64));
                pSym->paReferers[pSym->cReferers++] = pMod;
            }
        }
    }

    return pSym;
}



/**
 * Adds an import symbol to the linking.
 *
 * @returns see symAdd()
 * @param   pWld        Pointer to linker instance.
 * @param   pMod        Pointer to module
 * @param   fLibSearch  Set if we're doing library search at this time.
 * @param   pachName    The name which can be referenced in this module.
 * @param   cchName     Length of that name. -1 if zero terminated string.
 * @param   pachImpName Internal name, the name used to import the symbol.
 * @param   cchImpName  Length of that name. -1 if zero terminated string.
 * @param   pachModName Module name where the export should be resolved on load time.
 * @param   cchModName  Length of that name. -1 if zero terminated string.
 * @param   uOrdinal    The ordinal it's exported with from the module.
 *                      0 if exported by the name pachImpName represent.
 */
PWLDSYM symAddImport(PWLD pWld, PWLDMOD pMod, int fLibSearch,
                     const char *pachName, int cchName,
                     const char *pachImpName, int cchImpName,
                     const char *pachModName, int cchModName,
                     unsigned uOrdinal)
{
    WLDSYMACTION    eAction;
    PWLDSYM         pSym;
    const char *    pszImpName;
    const char *    pszImpMod;

    pSym = symAdd(pWld, pMod, WLDSF_IMPORT | (fLibSearch ? WLDSF_LIBSEARCH : 0),
                  pachName, cchName, &pWld->Global, &eAction);
    if (!pSym)
        return NULL;

    /* Add the strings we'll need them very soon */
    if (cchModName > 0)
        pszImpMod = strpool_addnu(pWld->pStrMisc, pachModName, cchModName);
    else
        pszImpMod = strpool_addu(pWld->pStrMisc, pachModName);
    pszImpName = NULL;
    if (!uOrdinal)
    {
        /* If no imp name, we'll use the name we know it as. */
        if (!pachImpName || cchImpName == 0 || !*pachImpName)
        {
            pachImpName = pachName;
            cchImpName = cchName;
        }
        if (cchImpName > 0)
            pszImpName = strpool_addn(pWld->pStrMisc, pachImpName, cchImpName);
        else
            pszImpName = strpool_add(pWld->pStrMisc, pachImpName);
    }

    /* Process the symbol */
    switch (eAction)
    {
        case WLDSA_NEW:
        case WLDSA_UP:
            pSym->u.import.pszImpMod  = pszImpMod;
            pSym->u.import.pszImpName = pszImpName;
            pSym->u.import.uImpOrd    = uOrdinal;
            break;

        case WLDSA_OLD:
        {   /* verify that the name matches */
            if ((pSym->fFlags & WLDSF_TYPEMASK) == WLDSF_IMPORT)
            {
                if (!pSym->u.import.pszImpMod)
                {
                    pSym->u.import.pszImpMod = pszImpMod;
                    pSym->u.import.pszImpName = pszImpName;
                    pSym->u.import.uImpOrd = uOrdinal;
                }
                else
                {
                    if (    pSym->u.import.pszImpMod != pszImpMod
                        &&  pSym->u.import.pszImpName != pszImpName
                        &&  pSym->u.import.uImpOrd != uOrdinal)
                    {
                        modWarn(pMod, "Existing import '%s' have different module name than the new ('%s' != '%s'), different ordinal (%d != %d), and different name (%s != %s).",
                                pSym->pszName, pSym->u.import.pszImpMod, pszImpMod, pSym->u.import.uImpOrd, uOrdinal, pSym->u.import.pszImpName, pszImpName);
                    }
                    else if (pSym->u.import.pszImpMod != pszImpMod)
                    {
                        modWarn(pMod, "Existing import '%s' have different module name than the new ('%s' != '%s').",
                                pSym->pszName, pSym->u.import.pszImpMod, pszImpMod);
                    }
                    else if (pSym->u.import.pszImpName != pszImpName)
                    {
                        modWarn(pMod, "Existing import '%s' have different import name (%s != %s).",
                                pSym->pszName, pSym->u.import.pszImpName, pszImpName);
                    }
                    else if (pSym->u.import.uImpOrd != uOrdinal)
                    {
                        modWarn(pMod, "Existing import '%s' have different ordinal (%d != %d).",
                                pSym->pszName, pSym->u.import.uImpOrd, uOrdinal);
                    }
                }
            }
            /* else: no need to complain, symAdd already did that. */
            break;
        }

        default:
            WLDINTERR(pWld, pMod);
    }

    return pSym;
}


/**
 * Adds(/Marks) an exported symbol.
 *
 * @returns see symAdd()
 * @param   pWld        Pointer to linker instance.
 * @param   pMod        Pointer to module
 * @param   fLibSearch  Set if we're doing library search at this time.
 * @param   fExport     Export flags (WLDSEF_*).
 * @param   cExpWords   Number of words to push on the stack if this
 *                      export is a call gate.
 * @param   pachExpName Exported name.
 * @param   cchExpName  Length of that name. -1 if zero terminated string.
 * @param   pachIntName Internal name. NULL or with cchIntName == 0 if the same
 *                      as the exported one.
 * @param   cchIntName  Length of that name. -1 if zero terminated string.
 *                      0 if the internal name is the same and the exported one.
 * @param   uOrdinal    The ordinal it's exported with
 *                      0 if exported by the name pachIntName represent.
 */
static PWLDSYM symAddExport(PWLD pWld, PWLDMOD pMod, int fLibSearch,
                            unsigned    fExport,
                            unsigned    cExpWords,
                            const char *pachExpName, int cchExpName,
                            const char *pachIntName, int cchIntName,
                            unsigned uOrdinal)
{
    PWLDSYM         pSymExp;
    PWLDSYM         pSymInt;

    /* set default name */
    if (!pachIntName || !cchIntName || !*pachIntName)
    {
        pachIntName = pachExpName;
        cchIntName = cchExpName;
    }

    /*
     * Add the internal name.
     */
    pSymInt = symAdd(pWld, pMod, WLDSF_UNDEF | (fLibSearch ? WLDSF_LIBSEARCH : 0),
                     pachIntName, cchIntName, &pWld->Global, NULL);
    if (!pSymInt)
        return NULL;

    /*
     * Add external name.
     */
    pSymExp = symAdd(pWld, pMod, WLDSF_EXP_NM | (fLibSearch ? WLDSF_LIBSEARCH : 0),
                     pachExpName, cchExpName, &pWld->Exported, NULL);
    if (!pSymExp)
        return NULL;

    /*
     * Is the exported symbol already exported?
     */
    if (pSymExp->pAliasFor)
    {
        if (pSymExp->pAliasFor != pSymInt)
            modWarn(pMod, "Export '%s' (int '%s') is already defined with a different resolution '%s'.", 
                    pSymExp->pszName, pSymInt->pszName, pSymExp->pAliasFor->pszName);
        else if (    (pSymExp->fExport & WLDSEF_DEF_FILE)
                 &&  (fExport & WLDSEF_DEF_FILE))
            modWarn(pMod, "Export '%s' (int '%s') is already defined.", pSymExp->pszName, pSymInt->pszName);
        
        if (    !(pSymExp->fExport & WLDSEF_DEF_FILE)
            &&  (fExport & WLDSEF_DEF_FILE))
            pSymExp->fExport = fExport;
        return pSymExp;
    }
    
    pSymExp->pAliasFor = pSymInt;
    pSymExp->fExport   = fExport;
    pSymExp->cExpWords = cExpWords;
    pSymExp->uExpOrd   = uOrdinal;

    return pSymExp;
}


/**
 * Adds a public symbol.
 *
 * @returns see symAdd()
 * @param   pWld        Pointer to linker instance.
 * @param   pMod        Pointer to module
 * @param   fLibSearch  Set if we're doing library search at this time.
 * @param   pachName    Exported name.
 * @param   cchName     Length of that name. -1 if zero terminated string.
 * @param   ulValue     Value of the symbol.
 * @param   iSegment    Segment of pMod in which this symbol is defined.
 *                      Use -1 if segment is not relevant.
 */
static PWLDSYM symAddPublic(PWLD pWld, PWLDMOD pMod, int fLibSearch,
                            const char *pachName, int cchName,
                            unsigned long ulValue, int iSegment, int iGroup)
{
    PWLDSYM         pSym;

    pSym = symAdd(pWld, pMod, WLDSF_PUBLIC | (fLibSearch ? WLDSF_LIBSEARCH : 0),
                  pachName, cchName, &pWld->Global, NULL);
    if (!pSym)
        return NULL;
    /* @todo: handle values ulValue, iSegment and iGroup? Not really required for this job... */
    return pSym;
}


/**
 * Adds an undefined symbol.
 *
 * @returns see symAdd()
 * @param   pWld        Pointer to linker instance.
 * @param   pMod        Pointer to module
 * @param   fLibSearch  Set if we're doing library search at this time.
 * @param   pachName    Exported name.
 * @param   cchName     Length of that name. -1 if zero terminated string.
 */
static PWLDSYM symAddUnDef(PWLD pWld, PWLDMOD pMod, int fLibSearch,
                           const char *pachName, int cchName)
{
    PWLDSYM         pSym;

    pSym = symAdd(pWld, pMod, WLDSF_UNDEF | WLDSF_UNCERTAIN | (fLibSearch ? WLDSF_LIBSEARCH : 0),
                  pachName, cchName, &pWld->Global, NULL);
    if (!pSym)
        return NULL;
    return pSym;
}


/**
 * Adds an undefined symbol.
 *
 * @returns see symAdd()
 * @param   pWld        Pointer to linker instance.
 * @param   pMod        Pointer to module
 * @param   fLibSearch  Set if we're doing library search at this time.
 * @param   pachName    Exported name.
 * @param   cchName     Length of that name. -1 if zero terminated string.
 */
static PWLDSYM symAddAlias(PWLD pWld, PWLDMOD pMod, int fLibSearch,
                           const char *pachAliasName, int cchAliasName,
                           const char *pachName, int cchName
                           )
{
    WLDSYMACTION    eAction;
    PWLDSYM         pSym;

    /*
     * Start by adding the alias it self.
     */
    pSym = symAdd(pWld, pMod, WLDSF_PUBLIC | WLDSF_ALIAS | (fLibSearch ? WLDSF_LIBSEARCH : 0),
                  pachAliasName, cchAliasName, &pWld->Global, &eAction);
    if (!pSym)
        return NULL;
    switch (eAction)
    {
        case WLDSA_NEW:
        case WLDSA_UP:
        {
            if (!pSym->pAliasFor)
            {
                PWLDSYM pSym2 = symAdd(pWld, pMod, WLDSF_UNDEF | (fLibSearch ? WLDSF_LIBSEARCH : 0),
                                       pachName, cchName, &pWld->Global, NULL);
                if (!pSym2)
                    return NULL;
                pSym->pAliasFor = pSym2;
            }
            else
            {
                modErr(pMod, "Aliased symbol apparently existed already (upgraded - internal error?).");
                symDbg(pSym, "Processing");
                symDbg(pSym->pAliasFor, "Alias");
                pSym = NULL;
            }
            break;
        }

        case WLDSA_OLD:
            modErr(pMod, "Aliased symbol already exists.");
            pSym = NULL;
            break;
        default:
            WLDINTERR(pWld, pMod);
    }

    return pSym;
}


/**
 * Adds(/Marks) an communal symbol.
 *
 * @returns see symAdd()
 * @param   pWld        Pointer to linker instance.
 * @param   pMod        Pointer to module
 * @param   fLibSearch  Set if we're doing library search at this time.
 * @param   fDataType   Datatype.
 * @param   cElements   Number of elements.
 * @param   cbElement   Size of one element.
 * @param   pachName    Symbol name.
 * @param   cchName     Length of the name. -1 if zero terminated string.
 */
static PWLDSYM symAddComdef(PWLD pWld, PWLDMOD pMod, int fLibSearch,
                            const char *pachName, int cchName,
                            signed long cElements, signed long cbElement)
{
    PWLDSYM         pSym;
    WLDSYMACTION    eAction;

    /*
     * Add external name.
     */
    pSym = symAdd(pWld, pMod, WLDSF_COMM | (fLibSearch ? WLDSF_LIBSEARCH : 0),
                  pachName, cchName, &pWld->Global, &eAction);
    if (!pSym)
        return NULL;

    cbElement *= (cElements > 0 ? cElements : 1); /* make it a size */
    switch (eAction)
    {
        case WLDSA_NEW:
            pSym->u.comm.cb        = cbElement * (cElements > 0 ? cElements : 1);
            pSym->u.comm.cElements = cElements;
            break;

        case WLDSA_UP:
            if ((pSym->fFlags & (WLDSF_TYPEMASK)) != WLDSF_COMM)
                break;
            /* fallthru */
        case WLDSA_OLD:
            /* merge size */
            if (pSym->u.comm.cElements < cElements)
                pSym->u.comm.cElements = cElements;
            if (pSym->u.comm.cb < cbElement)
                pSym->u.comm.cb = cbElement;
            break;

        default:
            WLDINTERR(pWld, pMod);
    }

    return pSym;
}


/**
 * Destroys a symbol table, freeing all symbols 
 *
 * @param   pSymTab     The symbol table to destroy.
 */
static void symDestroyTab(PWLDSYMTAB pSymTab)
{
    unsigned        uHash;
    for (uHash = 0; uHash < sizeof(pSymTab->ap) / sizeof(pSymTab->ap[0]); uHash++)
    {
        PWLDSYM     pSym = pSymTab->ap[uHash];
        while (pSym)
        {
            PWLDSYM pNext = pSym->pHashNext;
            if (pSym->paReferers)
                free(pSym->paReferers);
            free(pSym);
            pSym = pNext;
        }
        pSymTab->ap[uHash] = NULL;
    }
}











/*=============================================================================
 *                                                                            *
 *                                                                            *
 *  M I S C   M E T H O D S                                                   *
 *  M I S C   M E T H O D S                                                   *
 *  M I S C   M E T H O D S                                                   *
 *  M I S C   M E T H O D S                                                   *
 *  M I S C   M E T H O D S                                                   *
 *                                                                            *
 *                                                                            *
 *============================================================================*/


/**
 * Reads an OMF module from a file.
 *
 * This may be part of a library file so, we'll only read from THEADR to
 * past the first MODEND or the Pass 1 comment record.
 * The function will return the module stream positioned after the last
 * record it read.
 *
 * @returns 0 on success.
 * @returns non zero on failure.
 * @param   pWld        Pointer to linker instance.
 * @param   pMod        Pointer to module
 * @param   fLibSearch  Set if we're doing library search at this time.
 */
static unsigned     pass1ReadOMFMod(PWLD pWld, PWLDMOD pMod, int fLibSearch)
{
    OMFREC          OmfRec = {0,0};
    FILE *          phFile;             /* Input file. */
    PWLDSYM *       papExts = NULL;     /* Pointer to an array of EXTDEFs (as they appear) */
                                        /* We need them for the WKEXT processing. */
    int             cExts = 0;          /* Number of Entries in papExts. */
    int             fFirst = 1;         /* First record indicator. */
    /* generic stuff we'll use alot with not status associated. */
    PWLDSYM         pSym;
    int             i;
    unsigned long   ul;
    signed long     l2, l3;
    unsigned short  us, us2, us3;
    unsigned char   uch, uch2;

    /* We're counting indexes from 1, so add dummy zero entry. */
    papExts = xmalloc(sizeof(papExts[0])*64);
    papExts[0] = NULL;
    cExts = 1;


    /* open and position the file. */
    phFile = modOpen(pMod);

    /* loop till we get a MODEND */
    for (;;)
    {
        unsigned char   achBuffer[OMF_MAX_REC + 8];
        union
        {
            unsigned char *     puch;
            signed char *       pch;
            unsigned short *    pus;
            signed short *      ps;
            unsigned long *     pul;
            signed long *       pl;
            void *              pv;
        } u, u1, u2, u3;

        /** macro for getting a OMF index out of the buffer */
        #define OMF_GETINDEX()  (*u.puch & 0x80 ? ((*u.pch++ & 0x7f) << 8) + *u.pch++ : *u.pch++)
        #define OMF_BYTE()      (*u.puch++)
        #define OMF_WORD()      (*u.pus++)
        #define OMF_24BITWORD() (OMF_BYTE() | (OMF_WORD() << 8))
        #define OMF_DWORD()     (*u.pul++)
        #define OMF_MORE()      (u.puch - &achBuffer[0] < (int)OmfRec.cb - 1)
        #define OMF_IS32BIT()   ((OmfRec.chType & REC32) != 0)
        #define OMF_GETTYPELEN(l) \
            do                                                       \
            {                                                        \
                l = OMF_BYTE();                                      \
                if (l > 128)                                         \
                    switch (l)                                       \
                    {                                                \
                        case 0x81: l = OMF_WORD(); break;            \
                        case 0x84: l = OMF_24BITWORD(); break;       \
                        case 0x88: l = OMF_DWORD(); break;           \
                        default:                                     \
                            modErr(pMod, "Invalid type length!");    \
                            goto failure;                            \
                    }                                                \
            } while (0)

        u.pv = &achBuffer[0];

        /* read omf record header */
        if (fread(&OmfRec, sizeof(OmfRec), 1, phFile) != 1)
        {
            modErr(pMod, "read error. (offset ~= %#x).", (int)ftell(phFile));
            goto failure;
        }
        if (fFirst)
        {   /* some extra check for the first record. */
            fFirst = 0;
            if (OmfRec.chType != THEADR)
            {
                modErr(pMod, "invalid object module (offset %#x).", (int)pMod->off);
                goto failure;
            }
        }

        /* Read or skip the record. */
        switch (OmfRec.chType)
        {
            /* done */
            case MODEND: case MODEND | REC32:
            case LIBEND:
                fseek(phFile, OmfRec.cb, SEEK_CUR);
                goto done_skip;
            /* read */
            case EXTDEF: case EXTDEF | REC32:
            case PUBDEF: case PUBDEF | REC32:
            case ALIAS:  case ALIAS  | REC32:
            case COMDEF: case COMDEF | REC32:
            case COMDAT: case COMDAT | REC32:
            case COMENT: case COMENT | REC32:
            case THEADR: case THEADR | REC32:
            case LIBHDR:
                break;
            /* skip */
            default:
                fseek(phFile, OmfRec.cb, SEEK_CUR);
                continue;
        }
        if (fread(achBuffer, OmfRec.cb, 1, phFile) != 1)
        {
            modErr(pMod, "read error. (offset ~= %#x)", ftell(phFile));
            goto failure;
        }

        /* Switch on type. */
        switch (OmfRec.chType)
        {
            case THEADR:
            {
                if (!pMod->pszModName)
                    pMod->pszModName = strpool_addn(pWld->pStrMisc, u.pch + 1, *u.puch);
                /* Put out some /INFO stuff similar to ilink. */
                if (pMod->pLib)
                    WLDINFO(pWld, ("Reading @0x%08x %s(%s)", (int)pMod->off, pMod->pLib->pszLibName, pMod->pszModName));
                else
                    WLDINFO(pWld, ("Reading @0x%08x %s", (int)pMod->off, pMod->pszModName));
                break;
            }

            case COMENT: case COMENT | REC32:
                uch = OMF_BYTE(); /* comment type */
                uch = OMF_BYTE(); /* comment class */
                switch (uch)
                {
                    case CLASS_PASS:
                         goto done_noskip;
                    case CLASS_WKEXT:
                    {   /* This is a bit tricky, we need to have an indexable array
                         * of the extdefs for this module. In addition we'll need to
                         * make sure we don't mark an EXTDEF from another module as
                         * weak.
                         */
                        while (OMF_MORE())
                        {
                            int iWeak = OMF_GETINDEX();
                            int iDefault = OMF_GETINDEX();
                            if (   iWeak     >= cExts
                                || iDefault  >= cExts
                                || !papExts[iWeak]
                                || !papExts[iDefault])
                            {
                                modErr(pMod, "Invalid WKEXT record.");
                                goto failure;
                            }
                            if ((papExts[iWeak]->fFlags & (WLDSF_TYPEMASK | WLDSF_UNCERTAIN)) == (WLDSF_UNDEF | WLDSF_UNCERTAIN))
                            {
                                papExts[iWeak]->fFlags = (papExts[iWeak]->fFlags & ~(WLDSF_TYPEMASK | WLDSF_UNCERTAIN)) | WLDSF_WKEXT;
                                papExts[iWeak]->pWeakDefault = papExts[iDefault];
                                SYMDBG(papExts[iWeak], "WKEXT");
                            }
                            else if (   (papExts[iWeak]->fFlags & WLDSF_TYPEMASK) == WLDSF_WKEXT
                                     &&  papExts[iWeak]->pWeakDefault != papExts[iDefault])
                                modWarn(pMod, "WKEXT '%s' already declared with '%s' and not '%s' as default.",
                                        papExts[iWeak]->pszName, papExts[iWeak]->pWeakDefault->pszName, papExts[iDefault]->pszName);
                        }
                        break;
                    }

                    case CLASS_OMFEXT:
                    {
                        switch (OMF_BYTE())
                        {   /*
                             * Import definition.
                             */
                            case OMFEXT_IMPDEF:
                            {
                                uch = OMF_BYTE();               /* flags */
                                u1 = u; u.pch += 1 + *u.puch;   /* internal name */
                                u2 = u; u.pch += 1 + *u.puch;   /* module name */
                                ul = 0;                         /* ordinal */
                                if (uch & 1)
                                    ul = OMF_WORD();

                                pSym = symAddImport(pWld, pMod, fLibSearch,
                                                    u1.pch + 1, *u1.puch,
                                                    u1.pch + 1, *u1.puch,
                                                    u2.pch + 1, *u2.puch,
                                                    ul);
                                if (!pSym) goto failure;
                                SYMDBG(pSym, "IMPDEF");
                                break;
                            }

                            /*
                             * Export definition.
                             * If it have an internal name the exported name will become
                             * an alias record.
                             */
                            case OMFEXT_EXPDEF:
                            {
                                u1 = u; u.pch++;                /* flags */
                                u2 = u; u.pch += 1 + *u.puch;   /* exported name */
                                u3 = u; u.pch += 1 + *u.puch;   /* internal name */
                                ul = 0;                         /* ordinal */
                                if (*u1.pch & 0x80)
                                    ul = OMF_WORD();
                                pSym = symAddExport(pWld, pMod, fLibSearch,
                                                    (*u1.puch & 0x40 ? WLDSEF_RESIDENT : WLDSEF_DEFAULT) | (*u1.puch & 0x20 ? WLDSEF_NODATA : 0),
                                                    ((unsigned)*u1.puch) & 0x1f,
                                                    u2.pch + 1, *u2.puch,
                                                    u3.pch + 1, *u3.puch,
                                                    ul);
                                if (!pSym) goto failure;
                                SYMDBG(pSym, "EXPDEF");
                                break;
                            }
                        }
                    }
                } /* comment class */
                break;

            case EXTDEF:
            {
                while (OMF_MORE())
                {
                    u1 = u; u.pch += 1 + *u.puch;
                    ul = OMF_GETINDEX(); /* typeindex */
                    pSym = symAddUnDef(pWld, pMod, fLibSearch, u1.puch + 1, *u1.puch);
                    if (!pSym) goto failure;
                    SYMDBG(pSym, "EXTDEF");
                    /* put into array of externals */
                    if (!(cExts % 64))
                        papExts = xrealloc(papExts, sizeof(papExts[0]) * (cExts + 64));
                    papExts[cExts++] = pSym;
                }
                break;
            }

            case PUBDEF: case PUBDEF | REC32:
            {
                us2 = OMF_GETINDEX();           /* group index */
                us3 = OMF_GETINDEX();           /* segment index */
                if (!us3)
                    us = OMF_WORD();            /* base frame - ignored */
                while (OMF_MORE())
                {
                    u1 = u; u.pch += 1 + *u.puch;   /* public name */
                    ul = OMF_IS32BIT() ? OMF_DWORD() : OMF_WORD();
                    us = OMF_GETINDEX();            /* typeindex */
                    pSym = symAddPublic(pWld, pMod, fLibSearch, u1.puch + 1, *u1.puch,
                                        ul, us3, us2);
                    if (!pSym) goto failure;
                    SYMDBG(pSym, "PUBDEF");
                }
                break;
            }

            case ALIAS: case ALIAS | REC32:
            {
                while (OMF_MORE())
                {
                    u1 = u; u.pch += 1 + *u.puch;   /* alias name */
                    u2 = u; u.pch += 1 + *u.puch;   /* substitutt name. */
                    pSym = symAddAlias(pWld, pMod, fLibSearch,
                                       u1.puch + 1, *u1.puch,
                                       u2.puch + 1, *u2.puch);
                    if (!pSym) goto failure;
                    SYMDBG(pSym, "ALIAS");
                }
                break;
            }

            case COMDEF: case COMDEF | REC32:
            {
                while (OMF_MORE())
                {
                    u1 = u; u.pch += 1 + *u.puch;   /* communal name (specs say 1-2 length...) */
                    us2 = OMF_GETINDEX();           /* typeindex */
                    uch2 = OMF_BYTE();              /* date type */
                    switch (uch2)
                    {
                        case COMDEF_TYPEFAR:
                            OMF_GETTYPELEN(l2);     /* number of elements */
                            OMF_GETTYPELEN(l3);     /* element size */
                            break;
                        case COMDEF_TYPENEAR:
                            l2 = 1;                 /* number of elements */
                            OMF_GETTYPELEN(l3);     /* element size */
                            break;
                        default:
                            modErr(pMod, "Invalid COMDEF type %x.", (int)uch2);
                            goto failure;
                    }

                    pSym = symAddComdef(pWld, pMod, fLibSearch,
                                        u1.puch + 1, *u1.puch,
                                        l2, l3);
                    if (!pSym) goto failure;
                    SYMDBG(pSym, "ALIAS");
                }
                break;
            }

            case COMDAT: case COMDAT | REC32:
            {
                break;
            }

            case LIBHDR:
                modErr(pMod, "invalid object module (LIBHDR).");
                goto failure;
            default:
                fprintf(stderr, "we shall not be here!!\n");
                asm ("int $3");
                break;
        }

        #undef OMF_GETINDEX
        #undef OMF_BYTE
        #undef OMF_WORD
        #undef OMF_24BITWORD
        #undef OMF_DWORD
        #undef OMF_MORE
        #undef OMF_IS32BIT
        #undef OMF_GETTYPELEN
    }

done_skip:
    /* Skip to end of record */
    fseek(phFile, OmfRec.cb, SEEK_CUR);

done_noskip:
    /* Make all the EXTDEFs uncertain. */
    for (i = 0; i < cExts; i++)
        if (papExts[i])
            papExts[i]->fFlags &= ~WLDSF_UNCERTAIN;

    /* Max extdefs values. */
    if (fLibSearch)
    {
        if (pWld->cMaxLibExts < cExts)
            pWld->cMaxLibExts = cExts;
    }
    else
    {
        if (pWld->cMaxObjExts < cExts)
            pWld->cMaxObjExts = cExts;
    }

    return 0;

failure:
    if (papExts)
        free(papExts);
    return -1;
}










/*=============================================================================
 *                                                                            *
 *                                                                            *
 *  P U B L I C   M E T H O D S                                               *
 *  P U B L I C   M E T H O D S                                               *
 *  P U B L I C   M E T H O D S                                               *
 *  P U B L I C   M E T H O D S                                               *
 *  P U B L I C   M E T H O D S                                               *
 *                                                                            *
 *                                                                            *
 *============================================================================*/

/**
 * Creates an instance of the linker.
 *
 * @returns Pointer to linker instance.
 * @param   fFlags  Linker flags as defined by enum wld_create_flags.
 */
PWLD    WLDCreate(unsigned fFlags)
{
    PWLD    pWld = xmalloc(sizeof(*pWld));

    /* initiate it */
    memset(pWld, 0, sizeof(*pWld));
    pWld->fFlags = fFlags;
    pWld->pStrMisc = strpool_init();
    pWld->ppObjsAdd = &pWld->pObjs;
    pWld->ppLibsAdd = &pWld->pLibs;

    /*
     * Add predefined symbols.
     */
    if (!(fFlags & WLDC_LINKER_LINK386))
    {
        symAddPublic(pWld, NULL, 0, "_end", 4, 0, 0, 0);
        symAddPublic(pWld, NULL, 0, "_edata", 6, 0, 0, 0);
    }

    /* done */
    if (fFlags & WLDC_VERBOSE)
        fprintf(stderr, "weakld: info: linker created\n");
    return pWld;
}

/**
 * Destroys a linker cleaning up open files and memory.
 *
 * @returns 0 on success.
 * @returns some unexplainable randomly picked number on failure.
 * @param   pWld    Linker instance to destroy.
 */
int     WLDDestroy(PWLD pWld)
{
    PWLDMOD     pObj;
    PWLDLIB     pLib;

    if (!pWld)
        return 0;
    if (pWld->fFlags & WLDC_VERBOSE)
        fprintf(stderr, "weakld: info: destroying linker instance\n");

    /* free mods */
    for (pObj = pWld->pObjs; pObj;)
    {
        void *pv = pObj;
        modClose(pObj);
        pObj = pObj->pNext;
        free(pv);
    }
    for (pObj = pWld->pDef; pObj;)
    {
        void *pv = pObj;
        modClose(pObj);
        pObj = pObj->pNext;
        free(pv);
    }

    /* free libs */
    for (pLib = pWld->pLibs; pLib;)
    {
        void *pv = pLib;
        libClose(pLib);
        pLib = pLib->pNext;
        free(pv);
    }

    /* free symbols */
    symDestroyTab(&pWld->Global);
    symDestroyTab(&pWld->Exported);

    /* members and finally the instance structure. */
    strpool_free(pWld->pStrMisc);

    free(pWld);
    return 0;
}


/**
 * Adds a object module to the linking process.
 * The object module will be analysed and the file handle closed.
 *
 * @returns 0 on success.
 * @returns some unexplainable randomly picked number on failure.
 * @param   pWld    Linker instance to destroy.
 * @param   phFile  File handle to pszName.
 * @param   pszName Object file to add. This may be an OMF library too,
 *                  but that only means that all it's content is to be
 *                  linked in as if they have been all specified on the
 *                  commandline.
 * @remark  Don't call wld_add_object after wld_add_library()
 *          or wld_generate_weaklib()!
 */
int     WLDAddObject(PWLD pWld, FILE *phFile, const char *pszName)
{
    OMFREC  OmfRec = {0,0};
    int     rc = 0;
    if (!phFile)
        phFile = fopen(pszName, "rb");
    if (!phFile)
    {
        fprintf(stderr, "weakld: cannot open object file '%s'.\n", pszName);
        return 8;
    }
    WLDINFO(pWld, ("adding object %s.", pszName));

    /*
     * An object module is either a object or a library.
     * In anycase all the modules it contains is to be added to the link.
     */
    fread(&OmfRec, sizeof(OmfRec), 1, phFile);
    if (OmfRec.chType == THEADR)
    {
        /* Single Object */
        PWLDMOD pMod = xmalloc(sizeof(*pMod));
        memset(pMod, 0, sizeof(*pMod));
        pMod->pszModName = strpool_add(pWld->pStrMisc, pszName);
        pMod->phFile = phFile;
        *pWld->ppObjsAdd = pMod;
        pWld->ppObjsAdd = &pMod->pNext;
        rc = pass1ReadOMFMod(pWld, pMod, 0);
        modClose(pMod);
    }
    else if (OmfRec.chType == LIBHDR)
    {
        /* Library of object modules */
        while (OmfRec.chType != LIBEND && OmfRec.chType != (LIBEND | REC32))
        {
            if (OmfRec.chType == THEADR || OmfRec.chType == (THEADR | REC32))
            {
                PWLDMOD     pMod = xmalloc(sizeof(*pMod));
                memset(pMod, 0, sizeof(*pMod));
                pMod->pszModName = strpool_add(pWld->pStrMisc, pszName);
                pMod->phFile = phFile;
                pMod->off = ftell(phFile) - sizeof(OmfRec);
                *pWld->ppObjsAdd = pMod;
                pWld->ppObjsAdd = &pMod->pNext;
                if (pWld->fFlags & WLDC_VERBOSE)
                {
                    char            achModName[256];
                    unsigned char   cchModName;
                    cchModName = fgetc(phFile);
                    achModName[fread(achModName, 1, cchModName, phFile)] = '\0';
                    fprintf(stderr, "weakld: info: adding library member '%s'.\n", achModName);
                }
                rc = pass1ReadOMFMod(pWld, pMod, 0);
                pMod->phFile = NULL;
                if (rc)
                    break;
            }
            else
            {
                /* skip to the net record */
                fseek(phFile, OmfRec.cb, SEEK_CUR);
            }

            /* read next record */
            fread(&OmfRec, sizeof(OmfRec), 1, phFile);
        }
        fclose(phFile);
    }
    else
    {
        fclose(phFile);
        fprintf(stderr, "weakld: invalid object file '%s'.\n", pszName);
        rc = -1;
    }

    return rc;
}


/**
 * Callback function for parsing a module definition file.
 *
 * @returns 0 on success.
 * @returns -1 to stop the parsing.
 * @param   pMD     Pointer to module definition file handle.
 * @param   pStmt   Statement we're processing.
 * @param   eToken  Token we're processing.
 * @param   pvArg   Private arguments - pointer to an WLDDEFCBPARAM structure.
 *                  On failure, we'll update the structures rc member.
 */
static int      wldDefCallback(struct _md *pMD, const _md_stmt *pStmt, _md_token eToken, void *pvArg)
{
    unsigned        fFlags;
    unsigned        uOrdinal;
    unsigned        cWords;
    PWLDSYM         pSym;
    PWLDDEFCBPARAM  pParam = (PWLDDEFCBPARAM)pvArg;

    switch (eToken)
    {
        /*
         * One import.
         */
        case _MD_IMPORTS:
            uOrdinal = 0;
            if (pStmt->import.flags & _MDIP_ORDINAL)
                uOrdinal = pStmt->import.ordinal;
            pSym = symAddImport(pParam->pWld, pParam->pMod, 0,
                                pStmt->import.internalname[0] ? pStmt->import.internalname : pStmt->import.entryname, -1,
                                pStmt->import.entryname[0] ? pStmt->import.entryname : pStmt->import.internalname, -1,
                                pStmt->import.modulename, -1,
                                uOrdinal);
            if (!pSym)
            {
                pParam->rc = -3;
                return -3;
            }
            break;


        /*
         * One export.
         */
        case _MD_EXPORTS:
            fFlags = uOrdinal = cWords = 0;
            if (pStmt->export.flags & _MDEP_ORDINAL)
                uOrdinal = pStmt->export.ordinal;
            if (pStmt->export.flags & _MDEP_RESIDENTNAME)
                fFlags |= WLDSEF_NONRESIDENT;
            else if (pStmt->export.flags & _MDEP_NONAME)
                fFlags |= WLDSEF_NONAME;
            else
                fFlags |= WLDSEF_DEFAULT;
            if (pStmt->export.flags & _MDEP_NODATA)
                fFlags |= WLDSEF_NODATA;
            if (pStmt->export.flags & _MDEP_PWORDS)
                cWords = pStmt->export.pwords;
            pSym = symAddExport(pParam->pWld, pParam->pMod, 0,
                                fFlags | WLDSEF_DEF_FILE, cWords,
                                pStmt->export.entryname, -1,
                                pStmt->export.internalname, -1,
                                uOrdinal);
            if (!pSym)
            {
                pParam->rc = -4;
                return -4;
            }
            break;

        /*
         * Parse error.
         */
        case _MD_parseerror:
            modErr(pParam->pMod, "Parse error %d on line %d. (stmt=%d)",
                   pStmt->error.code, _md_get_linenumber(pMD), pStmt->error.stmt);
            pParam->rc = -2;
            break;

        /*
         * Ignore everything else (it's here to get rid of gcc warnings).
         */
        default:
            break;
    }
    return 0;
}


/**
 * Adds an definition file to the linking process.
 * The definition file will be analysed and the file handle closed.
 *
 * @returns 0 on success.
 * @returns some unexplainable randomly picked number on failure.
 * @param   pWld    Linker instance to destroy.
 * @param   phFile  File handle to pszName. If NULL we'll try open it.
 *                  The handle may be closed on exit, or latest when
 *                  we destroy the linker.
 * @param   pszName Definition file to add.
 * @remark  Don't call wld_add_deffile after wld_add_library()
 *          or wld_generate_weaklib()!
 */
int     WLDAddDefFile(PWLD pWld, FILE *phFile, const char *pszName)
{
    struct _md *    pMD;
    int             rc = -1;
    if (!phFile)
        phFile = fopen(pszName, "r");
    if (!phFile)
    {
        wldErr(pWld, "cannot open deffile file '%s'.\n", pszName);
        return 8;
    }
    WLDINFO(pWld, ("**** PARSE DEFINITIONS FILE ****"));
    WLDINFO(pWld, ("  %s", pszName));

    if (pWld->pDef)
        wldWarn(pWld, "%s: There is already a definition file '%s' loaded.", pszName, pWld->pDef->pszModName);

    /*
     * Process the .def file and be done with it.
     */
    pMD = _md_use_file(phFile);
    if (pMD)
    {
        PWLDMOD         pMod;
        WLDDEFCBPARAM   param;

        pMod = xmalloc(sizeof(*pMod));
        memset(pMod, 0, sizeof(*pMod));
        pMod->off = -1;
        pMod->pszModName = strpool_add(pWld->pStrMisc, pszName);
        pWld->pDef = pMod;

        /* parse it */
        _md_next_token(pMD);
        param.pWld      = pWld;
        param.pMod      = pMod;
        param.rc        = 0;
        rc = _md_parse(pMD, wldDefCallback, &param);
        _md_close(pMD);
    }
    else
        wldErr(pWld, "_md_use_file on '%s'.\n", pszName);

    fclose(phFile);
    return rc;
}


/**
 * Adds a library module to the linking process.
 * The library module will be processed and the file handle eventually closed.
 *
 * @returns 0 on success.
 * @returns some unexplainable randomly picked number on failure.
 * @param   pWld    Linker instance to destroy.
 * @param   phFile  File handle to pszName. If NULL we'll try open it.
 *                  The handle may be closed on exit, or latest when
 *                  we destroy the linker.
 * @param   pszName Library file to add. This may be an OMF object too.
 * @author  Don't call wld_add_library after wld_generate_weaklib()!
 */
int     WLDAddLibrary(PWLD pWld, FILE *phFile, const char *pszName)
{
    PWLDLIB     pLib;
    int         rc = 0;

    if (!phFile)
        phFile = fopen(pszName, "r");
    if (!phFile)
    {
        fprintf(stderr, "weakld: cannot open library file '%s'.\n", pszName);
        return 8;
    }
    WLDINFO(pWld, ("adding library %s\n", pszName));

    /* add it to the link, do nothing till we're asked to do the searching. */
    pLib = xmalloc(sizeof(*pLib));
    memset(pLib, 0, sizeof(*pLib));
    pLib->phFile = phFile;
    pLib->pszLibName = strpool_add(pWld->pStrMisc, pszName);
    pLib->pNext = NULL;

    /* read the library header. */
    if (    !fseek(phFile, 0, SEEK_SET)
        &&  fread(&pLib->LibHdr, sizeof(OMFREC), 1, phFile) == 1
        &&  (   pLib->LibHdr.chType != LIBHDR
             || fread(&pLib->LibHdr.offDict, sizeof(pLib->LibHdr) - sizeof(OMFREC), 1, phFile) == 1
                )
        )
    {
        /* link it in */
        *pWld->ppLibsAdd = pLib;
        pWld->ppLibsAdd = &pLib->pNext;
        libClose(pLib);
    }
    else
    {
        /* We failed. */
        libErr(pLib, "Invalid library format or read error.");
        fclose(phFile);
        free(pLib);
        rc = -1;
    }

    return rc;
}


/**
 * Does the linker pass one - chiefly library search as .def and .obj is
 * already processed as pass 1.
 *
 * @returns 0 on success (all symbols resolved)
 * @returns 42 if there are unresolved symbols.
 * @returns Something else on all other errors.
 * @param   pWld    Linker Instance.
 */
int     WLDPass1(PWLD pWld)
{
    int     fMore;
    int     cLoaded;
    int     fFirstTime = 1;

    WLDINFO(pWld, ("**** PASS ONE ****"));
    WLDINFO(pWld, ("**** LIBRARY SEARCH ****"));
    do
    {
        PWLDLIB     pLib;

        WLDINFO(pWld, ("-- Begin Library Pass --"));
        cLoaded = fMore = 0;
        for (pLib = pWld->pLibs; pLib; pLib = pLib->pNext)
        {
            int         rc;
            WLDSLEPARAM param;
            WLDINFO(pWld, ("  Searching %s", pLib->pszLibName));

            /*
             * Open the library
             */
            if (!libOpen(pLib))
                continue;

            /*
             * Load extended dictionary if we wanna use it.
             */
            if (fFirstTime && !(pWld->fFlags & WLDC_NO_EXTENDED_DICTIONARY_SEARCH))
                libLoadDict(pLib);
            else
                libCloseDict(pLib);

            /*
             * Enumerate undefined symbols and try load them from this library.
             */
            do
            {
                param.cLoaded = 0;
                param.pLib = pLib;
                if (pLib->pDict)
                    rc = symEnum(pWld, &pWld->Global, WLDSF_UNDEF, WLDSF_TYPEMASK | WLDSF_WEAK, symSearchLibEnum, &param);
                else
                {
                    rc = libLoadUndefSymbols(pWld, pLib, NULL, &param.cLoaded);
                    if (rc == 42)
                        rc = 0;
                }
                cLoaded += param.cLoaded;
            } while (!rc && param.cLoaded > 0);

            /* close it */
            libClose(pLib);
            if (rc && rc != 42)
                return rc;
        }

        /* We only trust this if it's set. */
        fMore = symHaveUndefined(pWld);
        fFirstTime = 0;
    } while (fMore && cLoaded > 0);

    /* @todo: proper warning? */
    if (fMore)
        symPrintUnDefs(pWld);

    return fMore ? 42 : 0;
}



/**
 * Write the start of an weak alias object.
 *
 * @returns 0 on success
 * @returns -1 on failure.
 * @param   pWld    Linker instance.
 * @param   phFile  Output file.
 * @param   pszName Object name.
 * @remark  Link386 accepts anything as long as THEADR/LHEADR is there.
 *          VACxxx want's more stuff.
 */
static int  wldObjStart(PWLD pWld, FILE *phFile, const char *pszName)
{
    #pragma pack(1)
    struct omfstuff
    {
        OMFREC      hdr;
        char        ach[128];
    }       omf;
    #pragma pack()
    int     cch;
#if 0
    int     i;
    static const char * apszLNAMEs[] =
    {
        "",                             /* 1 */
        "TEXT32",                       /* 2 */
        "DATA32",                       /* 3 */
        "BSS32",                        /* 4 */
        "CODE",                         /* 5 */
        "DATA",                         /* 6 */
        "BSS",                          /* 7 */
        "FLAT",                         /* 8 */
        "DGROUP",                       /* 9 */
    };
#endif

    /* THEADR */
    omf.hdr.chType = THEADR;
    cch = strlen(pszName);
    omf.hdr.cb = 2 + cch;
    omf.ach[0] = cch;
    memcpy(&omf.ach[1], pszName, cch);
    omf.ach[cch + 1] = 0;
    if (fwrite(&omf, omf.hdr.cb + sizeof(OMFREC), 1, phFile) != 1)
    {
        wldErr(pWld, "Error occured while writing weak aliases. (4)");
        return -1;
    }

    /* COMENT - IDM */
    omf.hdr.chType = COMENT;
    cch = strlen("INNIDM");
    omf.hdr.cb = cch + 5;
    omf.ach[0] = 0x00;                  /* flags */
    omf.ach[1] = CLASS_IDMDLL;          /* class */
    omf.ach[2] = cch;
    memcpy(&omf.ach[3], "INNIDM", cch);
    omf.ach[3+cch] = 0;
    if (fwrite(&omf, omf.hdr.cb + sizeof(OMFREC), 1, phFile) != 1)
    {
        wldErr(pWld, "Error occured while writing weak aliases. (5)");
        return -1;
    }
#if 0
    /* COMENT - DEBUG INFO */
    omf.hdr.chType = COMENT;
    omf.hdr.cb = 6;
    omf.ach[0] = 0x80;                  /* flags */
    omf.ach[1] = CLASS_DBGTYPE;         /* class */
    omf.ach[2] = 3;
    omf.ach[3] = 'H';
    omf.ach[4] = 'L';
    omf.ach[5] = 0;
    if (fwrite(&omf, omf.hdr.cb + sizeof(OMFREC), 1, phFile) != 1)
    {
        wldErr(pWld, "Error occured while writing weak aliases. (6)");
        return -1;
    }

    /* LNAMES for VACxxx */
    omf.hdr.chType = LNAMES;
    omf.hdr.cb = 1;                     /* crc */
    for (i = 0; i < sizeof(apszLNAMEs) / sizeof(apszLNAMEs[0]); i++)
    {
        cch = strlen(apszLNAMEs[i]);
        omf.ach[omf.hdr.cb - 1] = cch;
        memcpy(&omf.ach[omf.hdr.cb], apszLNAMEs[i], cch);
        omf.hdr.cb += cch + 1;
    }
    omf.ach[omf.hdr.cb - 1] = 0;        /* crc */
    if (fwrite(&omf, omf.hdr.cb + sizeof(OMFREC), 1, phFile) != 1)
    {
        wldErr(pWld, "Error occured while writing weak aliases. (7)");
        return -1;
    }

    /* SEGDEF - TEXT32 */
    omf.hdr.chType = SEGDEF;
    omf.hdr.cb = 7;
    omf.ach[0] = 0x69;                  /* attributes */
    omf.ach[1] = omf.ach[2] = 0;        /* segment size (0) */
    omf.ach[3] = 2;                     /* "TEXT32" LNAME index. */
    omf.ach[4] = 5;                     /* "CODE" LNAME index. */
    omf.ach[5] = 1;                     /* "" LNAME index. */
    omf.ach[6] = 0;                     /* crc */
    if (fwrite(&omf, omf.hdr.cb + sizeof(OMFREC), 1, phFile) != 1)
    {
        wldErr(pWld, "Error occured while writing weak aliases. (8)");
        return -1;
    }

    /* SEGDEF - DATA32 */
    omf.hdr.chType = SEGDEF;
    omf.hdr.cb = 7;
    omf.ach[0] = 0x69;                  /* attributes */
    omf.ach[1] = omf.ach[2] = 0;        /* segment size (0) */
    omf.ach[3] = 3;                     /* "TEXT32" LNAME index. */
    omf.ach[4] = 6;                     /* "CODE" LNAME index. */
    omf.ach[5] = 1;                     /* "" LNAME index. */
    omf.ach[6] = 0;                     /* crc */
    if (fwrite(&omf, omf.hdr.cb + sizeof(OMFREC), 1, phFile) != 1)
    {
        wldErr(pWld, "Error occured while writing weak aliases. (9)");
        return -1;
    }

    /* SEGDEF - BSS32 */
    omf.hdr.chType = SEGDEF;
    omf.hdr.cb = 7;
    omf.ach[0] = 0x69;                  /* attributes */
    omf.ach[1] = omf.ach[2] = 0;        /* segment size (0) */
    omf.ach[3] = 4;                     /* "TEXT32" LNAME index. */
    omf.ach[4] = 6;                     /* "CODE" LNAME index. */
    omf.ach[5] = 1;                     /* "" LNAME index. */
    omf.ach[6] = 0;                     /* crc */
    if (fwrite(&omf, omf.hdr.cb + sizeof(OMFREC), 1, phFile) != 1)
    {
        wldErr(pWld, "Error occured while writing weak aliases. (10)");
        return -1;
    }

    /* GROUP - FLAT */
    omf.hdr.chType = GRPDEF;
    omf.hdr.cb = 2;
    omf.ach[0] = 8;                     /* "FLAT" LNAME index */
    omf.ach[1] = 0;                     /* crc */
    if (fwrite(&omf, omf.hdr.cb + sizeof(OMFREC), 1, phFile) != 1)
    {
        wldErr(pWld, "Error occured while writing weak aliases. (11)");
        return -1;
    }

    /* GROUP - DGROUP  */
    omf.hdr.chType = GRPDEF;
    omf.hdr.cb = 6;
    omf.ach[0] = 9;                     /* "DGROUP" LNAME index */
    omf.ach[1] = 0xff;
    omf.ach[2] = 3;                     /* "BSS32" SEGDEF index */
    omf.ach[3] = 0xff;
    omf.ach[4] = 2;                     /* "DATA32" SEGDEF index */
    omf.ach[5] = 0;                     /* crc */
    if (fwrite(&omf, omf.hdr.cb + sizeof(OMFREC), 1, phFile) != 1)
    {
        wldErr(pWld, "Error occured while writing weak aliases. (12)");
        return -1;
    }
#endif

    return 0;
}



/**
 * Write the end of object stuff for an weak alias object.
 *
 * @returns 0 on success.
 * @returns -1 on failure.
 * @param   pWld    Linker instance.
 * @param   phFile  Output file.
 * @param   cbPage  Library page size.
 */
static int wldObjEnd(PWLD pWld, FILE *phFile, int cbPage)
{
    #pragma pack(1)
    struct omfstuff
    {
        OMFREC  hdr;
        char        ach[32];
    }       omf;
    #pragma pack()

#if 0
    /* PASS1 end COMENT */
    omf.hdr.chType = COMENT;
    omf.hdr.cb = 4;
    omf.ach[0] = 0x80;                  /* flags */
    omf.ach[1] = CLASS_PASS;            /* class */
    omf.ach[2] = 1;                     /* subclass */
    omf.ach[3] = 0;                     /* crc */
    if (fwrite(&omf, omf.hdr.cb + sizeof(OMFREC), 1, phFile) != 1)
    {
        wldErr(pWld, "Error occured while writing weak aliases. (1)");
        return -1;
    }
#endif

    /* MODEND */
    omf.hdr.chType = MODEND | REC32;
    omf.hdr.cb = 2;
    memset(&omf.ach[0], 0, sizeof(omf.ach));
    if (fwrite(&omf, omf.hdr.cb + sizeof(OMFREC), 1, phFile) != 1)
    {
        wldErr(pWld, "Error occured while writing weak aliases. (2)");
        return -1;
    }

    /* padding */
    while (ftell(phFile) % cbPage)
    {
        if (fputc('\0', phFile) < 0)
        {
            wldErr(pWld, "Error occured while writing weak aliases. (3)");
            return -1;
        }
    }
    return 0;
}


/** Parameter structure for wldGenerateObjEnum(). */
typedef struct wldGenerateObjEnum_param
{
    /** Output file stream */
    FILE *      phFile;
    /** Number of aliases in current object. (0 means first) */
    unsigned    cAliases;
    /** Library file number */
    unsigned    iLibFile;
} WLDGOEPARAM, *PWLDGOEPARAM;


/**
 * Symbol enumeration callback function of wld_generate_weakobj().
 * It writes alias for weak symbols.
 *
 * @returns 0 on success
 * @returns !0 on failure after writing sufficient error message.
 * @param   pWld    Linker instance.
 * @param   pSym    Symbol.
 * @param   pvUser  Pointer to a FILE stream.
 */
static int      wldGenerateObjEnum(PWLD pWld, PWLDSYM pSym, void *pvUser)
{
    #pragma pack(1)
    struct omfstuff
    {
        OMFREC  hdr;
        union
        {
            char        ach[640];
            OMFLIBHDRX  libhdr;
        };
    } omf;
    #pragma pack()
    PWLDGOEPARAM    pParam = (PWLDGOEPARAM)pvUser;
    int             cchAlias;
    int             cch;

    /*
     * Skip non-weak symbols (a bit of paranoia).
     */
    if (    !pSym->pszWeakName
        ||  pSym->pszName == pSym->pszWeakName)
    {
        WLDDBG(("not weak: '%s'\n", pSym->pszName));
        return 0;
    }

    /*
     * Create the alias record.
     */
    cchAlias   = strlen(pSym->pszName);
    cch        = strlen(pSym->pszWeakName);
    WLDINFO(pWld, ("using weak %s for %s", pSym->pszWeakName, pSym->pszName));

    /* paranoia */
    if (cchAlias > 255)
    {
        wldErr(pWld, "Symbol '%s' are too long (%d).", pSym->pszName, cchAlias);
        return -1;
    }
    if (cch > 255)
    {
        wldErr(pWld, "Symbol '%s' are too long (%d).", pSym->pszWeakName, cch);
        return -1;
    }

    /* end the current object? */
    if ((pWld->fFlags & WLDC_LINKER_LINK386) && pParam->cAliases >= 32)
    {
        int rc = wldObjEnd(pWld, pParam->phFile, 32);
        if (rc)
            return rc;
        pParam->cAliases = 0;
    }

    /* make new object ? */
    if (!pParam->cAliases)
    {
        sprintf(omf.ach, "wk%d.obj", pParam->iLibFile++);
        int rc = wldObjStart(pWld, pParam->phFile, omf.ach);
        if (rc)
            return rc;
    }

    /* Alias record */
    omf.hdr.chType = ALIAS;
    omf.hdr.cb = cchAlias + cch + 3;
    omf.ach[0] = cchAlias;
    memcpy(&omf.ach[1], pSym->pszName, cchAlias);           /* alias */
    omf.ach[cchAlias + 1] = cch;
    memcpy(&omf.ach[cchAlias + 2], pSym->pszWeakName, cch); /* subtitute */
    omf.ach[cchAlias + cch + 2] = 0; /* crc */
    if (fwrite(&omf, omf.hdr.cb + sizeof(OMFREC), 1, pParam->phFile) != 1)
    {
        wldErr(pWld, "Error occured while writing weak/trunc aliases. (2)");
        return -1;
    }
    pParam->cAliases++;

    return 0;
}


/** Parameter structure for wldGenerateDefCallback (module definition parser callback). */
typedef struct wldGenerateDefCallback_param
{
    /** Linker Instance. */
    PWLD        pWld;
    /** Read file stream of the current definition file. */
    FILE       *phOrgFile;
    /** Write file stream of the new, modified, defintion file. */
    FILE       *phNewFile;
    /** Our current linenumber index. */
    unsigned    iLine;
    /** Set if we have processed any weak __declspec(dllexport) symbols. */
    unsigned    fWeakDllExport;
} WLDGDCPARAM, *PWLDGDCPARAM;

/**
 * Callback function for doing weak aliasing of a module definition file.
 *
 * @returns 0 on success.
 * @returns -1 to stop the parsing.
 * @param   pMD     Pointer to module definition file handle.
 * @param   pStmt   Statement we're processing.
 * @param   eToken  Token we're processing.
 * @param   pvArg   Pointer to a WLDGDCPARAM structure.
 * @sketch
 *
 *  If it exports a weak symbol Then
 *      While linenumber(_md) > linenumber phOrgFile
 *          Copy line from phOrgFile to phNewFile
 *      Write modifed line to phNewFile and
 *      Skip a line in phOrgFile.
 *  Endif
 *
 *  The caller of _md_parse will make sure the last chunk of the file is
 *  copied to the new file.
 */
static int      wldGenerateDefCallback(struct _md *pMD, const _md_stmt *pStmt, _md_token eToken, void *pvArg)
{
    PWLDGDCPARAM     pParam = (PWLDGDCPARAM)pvArg;

    switch (eToken)
    {
        /*
         * One export.
         */
        case _MD_EXPORTS:
        {
            char        szTmp[1024];
            int         cch;
            PWLDSYM     pSymInt = NULL;
            PWLDSYM     pSymExp;

            pSymExp = symLookupExport(pParam->pWld, pStmt->export.entryname);
            if (pStmt->export.internalname[0])
                pSymInt = symLookup(pParam->pWld, pStmt->export.internalname);
            if (!pSymExp && !pSymInt)
            {
                wldWarn(pParam->pWld, "Failed to find exported symbols! (%s, %s, %d)",
                        pStmt->export.entryname, pStmt->export.internalname, pStmt->export.ordinal);
                return 0;               /* .ignore it. good idea? */
            }

            WLDDBG2(("wldGenerateDefCallback: '%s', '%s'", pSymExp->pszName, pSymInt ? pSymInt->pszName : "<the-same-name>"));

            /* mark the exported symbol exported. */
            pSymExp->fFlags |= WLDSF_EXPORT_DEF;

            /* Skip it all if neither of the symbols are weak. */
            if (    !(pSymExp->fFlags & WLDSF_WEAK)
                &&   (!pSymInt  || !(pSymInt->fFlags & WLDSF_WEAK)))
                return 0;

            /* Copy line from org to new so we're up to date. */
            while (pParam->iLine + 1 < pStmt->line_number)
            {
                if (!fgets(szTmp, 512 /* this is the size _md_* uses */, pParam->phOrgFile))
                {
                    wldErr(pParam->pWld, "Read error while reading original .def file.");
                    strcpy(szTmp, ";read error");
                }
                pParam->iLine++;
                if (fputs(szTmp, pParam->phNewFile) < 0)
                    return wldErr(pParam->pWld, "Write error.");
            }

            /* Set the correct $w$ internal symbol. */
            if (pSymInt && (pSymInt->fFlags & WLDSF_WEAK))
                cch = sprintf(szTmp, "  \"%s\" = \"%s\"", pStmt->export.entryname, pSymInt->pszWeakName);
            else
            {
                cch = sprintf(szTmp, "  \"%s\" = \"%s\"", pStmt->export.entryname, pSymExp->pszWeakName);
                if (!(pParam->pWld->fFlags & WLDC_LINKER_WLINK))
                    pSymExp->fFlags |= WLDSF_WEAKALIASDONE;
            }
            if (pStmt->export.flags & _MDEP_ORDINAL)
                cch += sprintf(&szTmp[cch], " @%d", pStmt->export.ordinal);
            if (pStmt->export.flags & _MDEP_RESIDENTNAME)
                cch += sprintf(&szTmp[cch], " RESIDENTNAME");
            if (pStmt->export.flags & _MDEP_NONAME)
                cch += sprintf(&szTmp[cch], " NONAME");
            if (pStmt->export.flags & _MDEP_NODATA)
                cch += sprintf(&szTmp[cch], " NODATA");
            if (pStmt->export.flags & _MDEP_PWORDS)
                cch += sprintf(&szTmp[cch], " %d", pStmt->export.pwords);
            cch += sprintf(&szTmp[cch], "   ; !weakld changed this!\n");

            if (fputs(szTmp, pParam->phNewFile) < 0)
                return wldErr(pParam->pWld, "Write error.");

            /* skip line */
            pParam->iLine++;
            fgets(szTmp, 512 /* this is the size _md_* uses */, pParam->phOrgFile);
            break;
        }

        /*
         * Parse error.
         */
        case _MD_parseerror:
            wldErr(pParam->pWld, "Parse error %d on line %d. (errorcode=%d stmt=%d)",
                   _md_get_linenumber(pMD), pStmt->error.code, pStmt->error.stmt);
            break;

        /*
         * Everything else is passed thru line by line.
         */
        default:
            break;
    }
    return 0;
}


/**
 * Checks the __declspec(dllexport) symbols for weak symbols.
 * When a weak export is found, it will be added to the .def file.
 *
 * @returns 0 on success
 * @returns !0 on failure after writing sufficient error message.
 * @param   pWld    Linker instance.
 * @param   pSym    Symbol.
 * @param   pvUser  Pointer to a WLDGDCPARAM struct.
 */
static int      wldGenerateDefExportEnum(PWLD pWld, PWLDSYM pSym, void *pvUser)
{
    PWLDGDCPARAM     pParam = (PWLDGDCPARAM)pvUser;

    if (!pParam->fWeakDllExport)
    {
        /* copy the rest of the file if any changes was made. */
        char szTmp[512];
        while (!feof(pParam->phOrgFile) && fgets(szTmp, sizeof(szTmp), pParam->phOrgFile))
            if (fputs(szTmp, pParam->phNewFile) < 0)
                return wldErr(pWld, "Write error.");

        if (fputs("\n"
                  "; weakld added weak __declspec(dllexport) exports\n"
                  "EXPORTS\n",
                  pParam->phNewFile) < 0)
            return wldErr(pWld, "Write error.");
        pParam->fWeakDllExport = 1;
    }

    /*
     * Now generate the export entry.
     */
    if (!pSym->pAliasFor)
    {
        WLDINFO(pWld, ("Adding __declspec(dllexport) weak symbol '%s' to the definition aliasing '%s'",
                       pSym->pszName, pSym->pszWeakName));
        fprintf(pParam->phNewFile, "    \"%s\" = \"%s\"\n",
                pSym->pszName, pSym->pszWeakName);
    }
    else
    {
        WLDINFO(pWld, ("Adding __declspec(dllexport) weak symbol '%s' to the definition aliasing '%s' (alias)",
                       pSym->pszName,
                       pSym->pAliasFor->pszWeakName ? pSym->pAliasFor->pszWeakName : pSym->pAliasFor->pszName));
        fprintf(pParam->phNewFile, "    \"%s\" = \"%s\"\n",
                pSym->pszName,
                pSym->pAliasFor->pszWeakName ? pSym->pAliasFor->pszWeakName : pSym->pAliasFor->pszName);
    }
    if (ferror(pParam->phNewFile))
        return wldErr(pWld, "Write error.");
    return 0;
}


/**
 * Generates an unique temporary file.
 *
 * @returns 0 on success.
 * @returns -1 on failure.
 * @param   pWld        Linker instance.
 * @param   pszFile     Where to put the filename.
 * @param   pszPrefix   Prefix.
 * @param   pszSuffix   Suffix.
 */
static int      wldTempFile(PWLD pWld, char *pszFile, const char *pszPrefix, const char *pszSuffix)
{
    struct stat     s;
    unsigned        c = 0;
    pid_t           pid = getpid();
    static char     s_szTmp[_MAX_PATH + 1];
    
    /* We need to apply _realrealpath to the tmpdir, so resolve that once and for all. */
    if (!s_szTmp[0])
    {
        const char *    pszTmp = getenv("TMP");
        if (!pszTmp)    pszTmp = getenv("TMPDIR");
        if (!pszTmp)    pszTmp = getenv("TEMP");
        if (!pszTmp)    pszTmp = ".";
        if (!_realrealpath(pszTmp, s_szTmp, sizeof(s_szTmp)))
        {
            printf("emxomfld: _realrealpath failed on '%s'!!!\n", pszTmp);
            exit(1);
        }
    }

    do
    {
        struct timeval  tv = {0,0};
        if (c++ >= 200)
            return -1;
        gettimeofday(&tv, NULL);
        sprintf(pszFile, "%s\\%s%x%lx%d%lx%s", s_szTmp, pszPrefix, pid, tv.tv_sec, c, tv.tv_usec, pszSuffix);
    } while (!stat(pszFile, &s));

    return 0;
}


/**
 * Generates a object file containing alias for resolving the weak
 * symbol references in the linked executable.
 * The filename of the object will be generated, but of course it
 * ends with '.obj'.
 *
 * @returns 0 on success.
 * @returns some unexplainable randomly picked number on failure.
 * @param   pWld        Linker instance.
 * @param   pszObjName  Where to put the name of the generated object file.
 *                      This is an empty string if no weak symbols were found!
 * @param   pszDefName  Where to put the name of the modified definition file.
 *                      This is an empty string if changes was required!
 */
int     WLDGenerateWeakAliases(PWLD pWld, char *pszObjName, char *pszDefName)
{
    FILE *      phFile;
    int         rc = 0;

    /* zero returns */
    if (pszObjName)
        *pszObjName = '\0';
    if (pszDefName)
        *pszDefName = '\0';

    /*
     * Do the definition file.
     */
    if (pWld->pDef && pszDefName)
    {
        rc = wldTempFile(pWld, pszDefName, "wk", ".def");
        if (!rc)
        {
            WLDINFO(pWld, ("Generating definition file '%s' for weak exports.", pszDefName));

            /* open output file */
            phFile = fopen(pszDefName, "w");
            if (phFile)
            {
                /* open original file */
                FILE *phOrgFile = fopen(pWld->pDef->pszModName, "r");
                if (phOrgFile)
                {
                    /* open original def file with the ModDef library.  */
                    struct _md *pMD = _md_open(pWld->pDef->pszModName);
                    if (pMD)
                    {
                        WLDGDCPARAM  param;
                        param.pWld = pWld;
                        param.phOrgFile = phOrgFile;
                        param.phNewFile = phFile;
                        param.iLine = 0;
                        param.fWeakDllExport = 0;

                        /* parse it */
                        _md_next_token(pMD);
                        rc = _md_parse(pMD, wldGenerateDefCallback, &param);
                        _md_close(pMD);

                        /* now see if there are any aliases in __declspec(dllexport) statements. */
                        if (!rc)
                        {
                            if (0)//pWld->fFlags & WLDC_LINKER_WLINK)
                                rc = symEnum(pWld, &pWld->Exported,
                                             WLDSF_EXPORT_DEF | WLDSF_WEAK, WLDSF_EXPORT_DEF | WLDSF_WEAK | WLDSF_WEAKALIASDONE,
                                             wldGenerateDefExportEnum, &param);
                            else
                                rc = symEnum(pWld, &pWld->Exported,
                                             WLDSF_WEAK, WLDSF_WEAK | WLDSF_EXPORT_DEF | WLDSF_WEAKALIASDONE,
                                             wldGenerateDefExportEnum, &param);
                        }

                        /* copy the rest of the file if any changes was made. */
                        if (!rc && param.iLine > 0)
                        {
                            char szTmp[512];
                            while (fgets(szTmp, sizeof(szTmp), phOrgFile))
                                if (fputs(szTmp, phFile) < 0)
                                    rc = wldErr(pWld, "Write error.");
                        }
                        else if (!rc && !param.fWeakDllExport)
                        {
                            /* no changes were made. */
                            WLDINFO(pWld, ("No weak exports, removing file."));
                            fclose(phFile);
                            phFile = NULL;
                            remove(pszDefName);
                            *pszDefName = '\0';
                        }
                    }
                    else
                        rc = wldErr(pWld, "_md_open on '%s'.\n", pszDefName);
                    fclose(phOrgFile);
                }
                else
                    rc = wldErr(pWld, "Failed to open '%s' for reading.\n", pWld->pDef->pszModName);
                if (phFile)
                    fclose(phFile);
            }
            else
                rc = wldErr(pWld, "Failed to open '%s' for writing.\n", pszDefName);
        }
        else
            wldErr(pWld, "Failed to generate temporary definition file for weak aliases.");
    }

    /* cleanup */
    if (rc && pszDefName && *pszDefName)
    {
        remove(pszDefName);
        *pszDefName = '\0';
    }


    /*
     * Generate the object file
     */
    if (!rc && pszObjName)
    {
        rc = wldTempFile(pWld, pszObjName, "wk", (pWld->fFlags & WLDC_LINKER_LINK386) ? ".lib" : ".obj");
        if (!rc)
        {
            WLDINFO(pWld, ("Generating object file '%s' for weak aliases.", pszObjName));

            /* open it */
            phFile = fopen(pszObjName, "wb");
            if (phFile)
            {
                WLDGOEPARAM     param;
                #pragma pack(1)
                struct omfstuff
                {
                    OMFREC  hdr;
                    union
                    {
                        char        ach[256];
                        OMFLIBHDRX  libhdr;
                    };
                }               omf;
                #pragma pack()
                off_t           offAlias;

                /* write start of file */
                if (pWld->fFlags & WLDC_LINKER_LINK386)
                {
                    /* Write library file header.
                     * Link386 have trouble with too many aliases in one object. So we
                     * generate a library of objects.
                     */
                    memset(&omf, 0, sizeof(omf));
                    omf.hdr.chType = LIBHDR;
                    omf.hdr.cb = 32 - 3;
                    omf.libhdr.offDict = 0;
                    omf.libhdr.cDictBlocks = 0;
                    omf.libhdr.fchFlags = 1;
                    if (fwrite(&omf, omf.hdr.cb + sizeof(OMFREC), 1, phFile) != 1)
                    {
                        wldErr(pWld, "Failed to write to '%s'.", pszObjName);
                        rc = -1;
                    }
                }

                /* successfully wrote start of file? */
                if (!rc)
                {
                    /*
                     * Make aliases
                     */
                    offAlias = ftell(phFile);       /* save this to see if anything is added. */
                    param.phFile = phFile;
                    param.cAliases = 0;
                    param.iLibFile = 0;
                    rc = symEnum(pWld, &pWld->Global,
                                 WLDSF_WEAK, WLDSF_WEAK | WLDSF_WEAKALIASDONE,
                                 wldGenerateObjEnum, &param);
                    if (!rc)
                    {
                        /* Check if we need to fake a lot's of externals for working around an ILINK bug.
                         * See defect #483 for details. Short summary: an array calculation in ilink is
                         * assuming that library objects have less EXTDEFs than the object ones. So, for
                         * pass2 an EXTDEF array may become too small.
                         */
                        WLDINFO(pWld, ("cWeakAliases=%d cMaxObjExts=%d cMaxLibExts=%d",
                                       param.cAliases + (param.iLibFile - 1) * 32, pWld->cMaxObjExts, pWld->cMaxLibExts));
                        if (pWld->cMaxObjExts < pWld->cMaxLibExts + 32 && !(pWld->fFlags & WLDC_LINKER_LINK386))
                        {
                            int i;
                            if (!param.cAliases)
                            {
                                rc = wldObjStart(pWld, phFile, "extdefshack.obj");
                                param.cAliases++;
                            }

                            /* issue a lot's of extdefs. */
                            omf.hdr.chType = EXTDEF;
                            omf.hdr.cb = 0;
                            for (i = 0; i < 10; i++)
                            {
                                omf.ach[omf.hdr.cb] = 9;                            /* string length  */
                                memcpy(&omf.ach[omf.hdr.cb + 1], "WEAK$ZERO", 9);   /* external */
                                omf.ach[omf.hdr.cb + 9 + 1] = 0;                    /* typeidx */
                                omf.hdr.cb += 9 + 2;
                            }
                            omf.ach[omf.hdr.cb++] = 0;                              /* crc */
                            for (i = pWld->cMaxLibExts + 32; i > 0; i -= 10)
                                if (fwrite(&omf, omf.hdr.cb + sizeof(OMFREC), 1, phFile) != 1)
                                {
                                    wldErr(pWld, "Failed to write to '%s'.", pszObjName);
                                    rc = -1;
                                    break;
                                }
                        }

                        /* Did we actually write anything to the file? */
                        if (ftell(phFile) != offAlias)
                        {
                            /* Complete last object file? */
                            if (pWld->fFlags & WLDC_LINKER_LINK386)
                            {
                                if (param.cAliases)
                                    rc = wldObjEnd(pWld, phFile, 32);
                                if (!rc)
                                {
                                    /* write end of library */
                                    memset(&omf, 0, sizeof(omf));
                                    omf.hdr.chType = LIBEND;
                                    omf.hdr.cb = 32 - 3;
                                    if (fwrite(&omf, omf.hdr.cb + sizeof(OMFREC), 1, phFile) != 1)
                                    {
                                        wldErr(pWld, "Failed to write to '%s'.", pszObjName);
                                        rc = -1;
                                    }
                                }
                            }
                            else if (param.cAliases)
                                rc = wldObjEnd(pWld, phFile, 1);
                        }
                        else
                        {
                            WLDINFO(pWld, ("No weak alias needed, removing file."));
                            fclose(phFile);
                            phFile = NULL;
                            remove(pszObjName);
                            *pszObjName = '\0';
                        }
                    }
                }
                if (phFile)
                    fclose(phFile);
            }
            else
            {
                wldErr(pWld, "Failed to open '%s' for writing.", pszObjName);
                *pszObjName = '\0';
            }
        }
        else
            wldErr(pWld, "Failed to generate temporary object file for weak aliases.");
    }

    /* cleanup */
    if (rc && pszObjName && *pszObjName)
    {
        remove(pszObjName);
        *pszObjName = '\0';
    }

    return rc;
}

