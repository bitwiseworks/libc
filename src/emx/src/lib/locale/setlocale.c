/* $Id: setlocale.c 3936 2014-10-26 15:30:04Z bird $ */
/** @file
 *
 * Locale support implementation through OS/2 Unicode API.
 *
 * Implementation of the setlocale() function.
 *
 *
 * Copyright (c) 2003 InnoTek Systemberatung GmbH
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
*   Header Files                                                               *
*******************************************************************************/
#include "libc-alias.h"
#include <InnoTekLIBC/locale.h>         /* must be included before ctype. */
#include <ctype.h>

#include <stdlib.h>

#include <alloca.h>
#include <string.h>
#include <errno.h>
#include <386/builtin.h>
#include <sys/param.h>
#include <sys/smutex.h>
#include <InnoTekLIBC/errno.h>
#include <InnoTekLIBC/fork.h>

#define INCL_DOS
#define INCL_FSMACROS
#include <os2emx.h>
#include <unidef.h>
#include <uconv.h>

#define __LIBC_LOG_GROUP    __LIBC_LOG_GRP_LOCALE
#include <InnoTekLIBC/logstrict.h>


/*******************************************************************************
*   Defined Constants And Macros                                               *
*******************************************************************************/
/** Instead of strcmp since it's faster. */
#define IS_C_LOCALE(s)      (((s)[0] == 'C') && (!(s)[1]))
/** For simplisity. */
#define IS_POSIX_LOCALE(s)  (   (s)[0] == 'P' \
                             && (s)[1] == 'O' \
                             && (s)[2] == 'S' \
                             && (s)[3] == 'I' \
                             && (s)[4] == 'X' \
                             && (s)[5] == '\0' )
#define IS_EURO(s)         ((   (s)[0] == 'E' \
                             && (s)[1] == 'U' \
                             && (s)[2] == 'R' \
                             && (s)[3] == 'O' \
                             && (s)[4] == '\0' \
                            ) || ( \
                                (s)[0] == 'e' \
                             && (s)[1] == 'u' \
                             && (s)[2] == 'r' \
                             && (s)[3] == 'o' \
                             && (s)[4] == '\0') )
/** Max lenght we anticipate of a codepage name. */
#define CODEPAGE_MAX_LENGTH     64

/** Macro for swapping generic struct field. */
#define SWAP_SIMPLE_MEMBERS(a_pHot, a_pCold, a_Type, a_Member) \
    do { \
        a_Type const Tmp    = (a_pHot)->a_Member; \
        (a_pHot)->a_Member  = (a_pCold)->a_Member; \
        (a_pCold)->a_Member = Tmp; \
    } while (0)

/** Macro for swapping a string pointer if the new value differs or differs in
 *  it's readonlyness. */
#define MAYBE_SWAP_STRING_MEMBERS(a_pHot, a_pCold, a_Member, a_fOverride) \
    do { \
        char *pszTmp = (a_pHot)->a_Member; \
        if ((a_fOverride) || strcmp(pszTmp, (a_pCold)->a_Member) != 0) \
        { \
            (a_pHot)->a_Member  = (a_pCold)->a_Member; \
            (a_pCold)->a_Member = pszTmp; \
        } \
    } while (0)

/** Macro for swapping a fixed array of string pointer if the new value differs
 *  or differs in it's readonlyness. */
#define MAYBE_SWAP_STRING_ARRAY_MEMBERS(a_pHot, a_pCold, a_apszMember, a_fOverride) \
    do { \
        unsigned iMember = sizeof((a_pHot)->a_apszMember) / sizeof((a_pHot)->a_apszMember[0]); \
        while (iMember-- > 0) \
            MAYBE_SWAP_STRING_MEMBERS(a_pHot, a_pCold, a_apszMember[iMember], a_fOverride); \
    } while (0)


/*******************************************************************************
*   Structures and Typedefs                                                    *
*******************************************************************************/
/** Structure used while sorting codepage characters by their weights. */
struct __collate_weight
{
    /* Character code. */
    unsigned char code;
    /* Actual weight length. */
    unsigned char len;
    /* The weight itself. */
    UniChar       ucsWeight[7];
};


/**
 * There is one global object of this type that contains integral
 * information about last selected (with setlocale()) locale.
 * The locale information itself is split into parts to avoid linking
 * unused data into programs that use just the "C" locale and just
 * a few functions that use locale data (such as strdate()).
 */
typedef struct __libc_LocaleGlobal
{
  /** Category names. */
  char         *apszNames[_LC_LAST + 1];
  /* Lock for multi-threaded operations. */
  _smutex       lock;
} __LIBC_LOCALEGLOBAL, *__LIBC_PLOCALEGLOBAL;


/**
 * Internal working structure which we modify while
 * performing the setlocale() operation.
 */
struct temp_locale
{
    /** Which we have processed.*/
    int                         afProcessed[_LC_LAST + 1];
    /** The global data. */
    __LIBC_LOCALEGLOBAL         Global;
    /** Collate data. */
    __LIBC_LOCALECOLLATE        Collate;
    /** Ctype data. */
    __LIBC_LOCALECTYPE          Ctype;
    /** Ctype conversion functions. */
    __LIBC_PCLOCALECTYPEFUNCS   pCtypeFuncs;
    /** Time data. */
    __LIBC_LOCALETIME           Time;
    /** Numeric and monetary data. */
    __LIBC_LOCALELCONV          Lconv;
    /** Messages data. */
    __LIBC_LOCALEMSG            Msg;
};


/*******************************************************************************
*   Global Variables                                                           *
*******************************************************************************/
/** Array of local categories. Use their defined value + 1 as index (LC_ALL is -1). */
static const char       gaszCategories[_LC_LAST + 1][16] =
{
    "LC_ALL",                           /* -1 */
    "LC_COLLATE",                       /* 0 */
    "LC_CTYPE",
    "LC_MONETARY",
    "LC_NUMERIC",
    "LC_TIME",
    "LC_MESSAGES"
};
/** Array of the lengths corresponding to the entries in the above array. */
static const unsigned char gacchCategories[_LC_LAST + 1] =
{
    sizeof("LC_ALL") - 1,
    sizeof("LC_COLLATE") - 1,
    sizeof("LC_CTYPE") - 1,
    sizeof("LC_MONETARY") - 1,
    sizeof("LC_NUMERIC") - 1,
    sizeof("LC_TIME") - 1,
    sizeof("LC_MESSAGES") - 1
};

/** "C" string. */
static const char       gszC[] = "C";

/** "POSIX" string. */
static const char       gszPOSIX[] = "POSIX";

/** The currnet locale specifications. */
static __LIBC_LOCALEGLOBAL gLocale =
{
    .apszNames =
    {
        (char *)gszC,    /* LC_ALL      */
        (char *)gszC,    /* LC_COLLATE  */
        (char *)gszC,    /* LC_CTYPE    */
        (char *)gszC,    /* LC_NUMERIC  */
        (char *)gszC,    /* LC_MONETARY */
        (char *)gszC,    /* LC_TIME     */
        (char *)gszC     /* LC_MESSAGES */
    },
    .lock = 0
};

/** "ISO8859-1" UniChar string. */
static const UniChar    gucsISO8859_1[] = {'I', 'S', 'O', '8', '8', '5', '9', '-', '1', '\0' };

/** @page pg_env
 * @subsection pg_env_sub1_LIBC_SETLOCALE_OLDSTYLE      LIBC_SETLOCALE_OLDSTYLE
 *
 * When the LIBC_SETLOCALE_OLDSTYLE environment variable is present in the
 * environemnt LIBC will ask OS/2 about the country and codepage so the right
 * default locale can be found when none of the POSIX variables are present.
 *
 * The default behaviour (i.e. when LIBC_SETLOCALE_OLDSTYLE is not present) is
 * to use the 'C' locale if none of the LANG or LC_* variables can be found.
 */

/** Whether or not to use the old style where we query extra stuff from OS/2.
 * The new style is more conforming to POSIX and with VAC.
 * The presense of LIBC_SETLOCALE_OLDSTYLE forces the old style.
 *
 * If the value is negative Then init is required.
 */
static int  gfOldStyle = -1;


/*******************************************************************************
*   Internal Functions                                                         *
*******************************************************************************/
static int unierr2errno(int rc);
static int convert_ucs(UconvObject uobj, UniChar *in, char **out);
static void Ucs2Sb(UniChar *ucs, char *sbs, size_t cch);
static const char *getDefaultLocale(const char *pszCategory, char *pszBuffer, int *pfDefault);
static int getCodepage(const char *pszCodepage, const char *pszLocale, LocaleObject lobj, UniChar *pucsCodepage, unsigned cucCodepage);
static int query_mbcs(UconvObject uobj, char *mbcs, unsigned char *au2MBCSPrefixs);

static int localeCollateDo(__LIBC_PLOCALECOLLATE pCollate, UconvObject uobj, LocaleObject lobj, const char *pszLocale);
static void localeCollateFree(__LIBC_PLOCALECOLLATE pCollate);
static inline unsigned char Transform(LocaleObject lobj, UconvObject uobj,
                                      UniChar (*pfnTransFunc) (LocaleObject, UniChar),
                                      UniChar uc, unsigned char uchFallback);
static int localeCtypeDo(__LIBC_PLOCALECTYPE pCtype, UconvObject uobj, LocaleObject lobj, const char *pszLocale, const char *pszCodeset);
static void localeCtypeFree(__LIBC_PLOCALECTYPE pCtype);
static int query_item(LocaleObject lobj, UconvObject uobj, LocaleItem iItem, char **ppszOut);
static int query_array(LocaleObject lobj, UconvObject uobj, int cElements, LocaleItem iFirst, char **papszOut);
static int localeTimeDo(__LIBC_PLOCALETIME pTime, UconvObject uobj, LocaleObject lobj, const char *pszLocale);
static void localeTimeFree(__LIBC_PLOCALETIME pTime);
static void localeNumericFree(__LIBC_PLOCALELCONV pLconv);
static void localeMonetaryFree(__LIBC_PLOCALELCONV pLconv);
static int localeNumericDo(__LIBC_PLOCALELCONV pLconv, UconvObject uobj, struct UniLconv *pULconv, const char *pszLocale);
static int localeMonetaryDo(__LIBC_PLOCALELCONV pLconv, UconvObject uobj, LocaleObject lobj, struct UniLconv *pULconv, const char *pszLocale, const char *pszModifier);
static void localeGlobalFree(__LIBC_PLOCALEGLOBAL pGlobal, int iCategory);
static int localeParseLocale(char *pszLocale, const char **ppszCodepage, const char **ppszModifier);
static int localeDoOne(struct temp_locale *pTemp, int iCategory, const char *pszLocale, const char *pszCodepage, const char *pszModifier);
static int localeDo(struct temp_locale *pTemp, int iCategory, char *pszLocale, int fDefaultValue);
static char *localeCommit(struct temp_locale *pTemp, int iCategory);
static void localeFree(struct temp_locale *pTemp);

static int setlocalForkChild1(__LIBC_PFORKHANDLE pForkHandle, __LIBC_FORKOP enmOperation);






/**
 * Converts from Uni api error code to errno.
 *
 * @returns errno
 * @param   rc      Uni api error code.
 */
static int unierr2errno(int rc)
{
    switch (rc)
    {
        case ULS_SUCCESS:           return 0;
        case ULS_MAXFILESPERPROC:   return -EMFILE;
        case ULS_MAXFILES:          return -ENFILE;
        case ULS_BADOBJECT:         return -EBADF;
        case ULS_BADHANDLE:         return -EBADF;
        case ULS_NOTIMPLEMENTED:    return -ENOSYS;
        case ULS_RANGE:             return -ERANGE;
        case ULS_NOMEMORY:          return -ENOMEM;
        case ULS_OTHER:
        case ULS_ILLEGALSEQUENCE:
        case ULS_NOOP:
        case ULS_TOOMANYKBD:
        case ULS_KBDNOTFOUND:
        case ULS_NODEAD:
        case ULS_NOSCAN:
        case ULS_INVALIDSCAN:
        case ULS_INVALID:
        case ULS_NOTOKEN:
        case ULS_NOMATCH:
        case ULS_BUFFERFULL:
        case ULS_UNSUPPORTED:
        case ULS_BADATTR:
        case ULS_VERSION:
        default:
            return -EINVAL;
    }
}

static int convert_ucs(UconvObject uobj, UniChar *in, char **out)
{
    size_t usl = UniStrlen (in) + 1;
    /* Allocate twice as much as we need - just in case every character is DBCS
       or desired encoding is UCS-2. */
    size_t osl = usl * 2;
    size_t in_left = usl;
    size_t nonid, out_left = osl;
    char *tmp = malloc (osl);
    UniChar *inbuf = in;
    void *outbuf = tmp;
    int try_count = 0;
    FS_VAR();

    FS_SAVE_LOAD();
    try_again:

    if (try_count > 10)
    {
        /* Well... nobody will say we gave it no chance ... */
        free(tmp);
        FS_RESTORE();
        return -1;
    }

    switch (UniUconvFromUcs (uobj, &inbuf, &in_left, &outbuf, &out_left, &nonid))
    {
        case 0:
            break;

        case UCONV_E2BIG:
            /* Out buffer too small, make one larger */
            inbuf = in; in_left = usl;
            out_left = (osl *= 2);
            outbuf = tmp = realloc (tmp, osl);
            try_count++;
            goto try_again;

        default:
            /* Unexpected error. */
            free (tmp);
            FS_RESTORE();
            return -1;
    }

    usl = (char *)outbuf - (char *)tmp;
    (*out) = (char *)malloc (usl);
    memcpy (*out, tmp, usl);
    free (tmp);
    FS_RESTORE();

    return 0;
}

static void Ucs2Sb(UniChar *ucs, char *sbs, size_t cch)
{
    while (cch--)
        *sbs++ = *ucs++;
}


static int query_mbcs(UconvObject uobj, char *mbcs, unsigned char *au2MBCSPrefixs)
{
    unsigned            i;
    uconv_attribute_t   uconv_attr;
    unsigned char       uchSeqlen[256];
    int                 rc;

    /*
     * Query data.
     */
    rc = UniQueryUconvObject(uobj, &uconv_attr, sizeof(uconv_attr), (char *)&uchSeqlen[0], NULL, NULL);
    if (rc)
        return -unierr2errno(rc);

    /*
     * Create the return values.
     */
    *mbcs = uconv_attr.mb_max_len > 1 ? uconv_attr.mb_max_len : 0;

    bzero(au2MBCSPrefixs, 256/4);
    for (i = 0; i < 256; i++)
        if (uchSeqlen[i] != 255)
            SET_MBCS_PREFIX(au2MBCSPrefixs, i, uchSeqlen[i]);

    return 0;
}

/**
 * Sets the LC_COLLATE part of the locale.
 *
 * @returns 0 on success.
 * @returns negated errno on failure.
 * @param   pCollate        The collate structure to operate on.
 * @param   uobj            The UconvObject to use. Collate is responsible for freeing it.
 * @param   lobj            The LocaleObject to use. Collate is responsible for freeing it.
 * @param   pszLocale       Pointer to the locale base specifier.
 */
static int localeCollateDo(__LIBC_PLOCALECOLLATE pCollate, UconvObject uobj, LocaleObject lobj, const char *pszLocale)
{
    int rc;

    /* Cleanup in case of some special LC_ALL call. */
    localeCollateFree(pCollate);

    /*
     * For "C" / "POSIX" we can simply use the static default locale.
     */
    if (    IS_C_LOCALE(pszLocale)
        ||  IS_POSIX_LOCALE(pszLocale))
    {
        memcpy(pCollate, &__libc_gLocaleCollateDefault, sizeof(*pCollate));
        return 0;
    }

    /*
     * Query multi-byte related stuff.
     */
    rc = query_mbcs(uobj, &pCollate->mbcs, &pCollate->au2MBCSPrefixs[0]);
    if (rc)
        return rc;

    if (pCollate->mbcs)
    {
        /*
         * In MBCS mode we just borrow the conversion and locale objects
         * and leave the real work to the Unicode subsystem.
         */
        pCollate->lobj          = lobj;
        pCollate->uobj          = uobj;
    }
    /*
        * In SBCS we query the weight of every character and use the
        * weights directly, without the need to invoke the Unicode API.
        *
        * NOTE: Also do this for 1-byte MBCS characters since strcoll relies on
        * that for optimisation (e.g. strings containing only 1-byte chars).
        */
    struct __collate_weight     aCW[256];
    int                         i, c;

    /* Initialize character weights. */
    for (i = 0; i < 256; i++)
    {
        UniChar ucs[2];
        if (!__libc_ucs2To(uobj, (unsigned char *)&i, 1, &ucs[0]))
        {
            LIBCLOG_ERROR2("__libc_ucs2To failed for char %d\n", i);
            ucs[0] = (UniChar)i;
        }
        ucs[1] = '\0';

        aCW[i].code = i;
        aCW[i].len  = UniStrxfrm(lobj, aCW[i].ucsWeight, &ucs[0], sizeof(aCW[i].ucsWeight) / sizeof(aCW[i].ucsWeight[0]));
        if (aCW[i].len >= sizeof(aCW[i].ucsWeight) / sizeof(aCW[i].ucsWeight[0]))
        {
            LIBC_ASSERTM_FAILED("This cannot happen... :-) i=%d len=%d \n", i, aCW[i].len);
            aCW[i].len = sizeof(aCW[i].ucsWeight) / sizeof(aCW[i].ucsWeight[0]);
        }

        /* Workaround for bug / undocumented UniStrxfrm feature. See strxfrm.c & mozilla sources. */
        aCW[i].len = MIN(aCW[i].len * 2, sizeof(aCW[i].ucsWeight) / sizeof(aCW[i].ucsWeight[0]));
        while (!aCW[i].ucsWeight[aCW[i].len - 1])
            aCW[i].len--;
    }

    /*
        * Do bubble sorting since qsort() doesn't guarantee that the order
        * of equal elements stays the same.
        */
    c = 256 - 1;
    do
    {
        int j = 0;
        for (i = 0; i < c; i++)
            if (UniStrncmp(aCW[i].ucsWeight, aCW[i + 1].ucsWeight, MIN(aCW[i].len, aCW[i + 1].len)) > 0)
            {
                _memswap(&aCW[i], &aCW[i + 1], sizeof(aCW[i]));
                j = i;
            }
        c = j;
    } while (c);

    /*
        * Store the result.
        */
    for (i = 0; i < 256; i++)
        pCollate->auchWeight[aCW[i].code] = i;

    if (!pCollate->mbcs)
    {
        /* cleanup */
        UniFreeUconvObject(uobj);
        UniFreeLocaleObject(lobj);
    }

    return 0;
}

/**
 * Swaps the content of two locale collate structures.
 *
 * @param   pHot        The hot structure (target).
 * @param   pCold       The cold structure (source, will be freed).
 */
static void localeCollateSwap(__LIBC_LOCALECOLLATE *pHot, __LIBC_PLOCALECOLLATE pCold)
{
    __LIBC_LOCALECOLLATE Tmp;
    memcpy(&Tmp, pHot, sizeof(Tmp));
    memcpy(pHot, pCold, sizeof(Tmp));
    memcpy(pCold, &Tmp, sizeof(Tmp));
}

/**
 * Frees system and CRT resources assocated with a collate structure.
 * @param   pCollate    The collate structure.
 */
static void localeCollateFree(__LIBC_PLOCALECOLLATE pCollate)
{
    /* Free old pszLocale objects, if any */
    if (pCollate->uobj)
    {
        UniFreeUconvObject(pCollate->uobj);
        pCollate->uobj = NULL;
    }
    if (pCollate->lobj)
    {
        UniFreeLocaleObject(pCollate->lobj);
        pCollate->lobj = NULL;
    }
}

static inline unsigned char Transform(LocaleObject lobj, UconvObject uobj,
                                      UniChar (APIENTRY *pfnTransFunc) (const LocaleObject, UniChar),
                                      UniChar uc, unsigned char uchFallback)
{
    unsigned char   sbcs;
    int nb = __libc_ucs2From(uobj, pfnTransFunc(lobj, uc), &sbcs, 1);
    return (nb == 1) ? sbcs : uchFallback;
}


/**
 * Sets the LC_TYPE part of the locale.
 *
 * @returns 0 on success.
 * @returns negated errno on failure.
 * @param   pCtype          The Ctype structure to operate on.
 * @param   uobj            The UconvObject to use. Ctype is responsible for freeing it.
 * @param   lobj            The LocaleObject to use. Ctype is responsible for freeing it.
 * @param   pszLocale       Pointer to the locale base specifier.
 * @param   pszCodeset      The codeset used.
 */
static int localeCtypeDo(__LIBC_PLOCALECTYPE pCtype, UconvObject uobj, LocaleObject lobj, const char *pszLocale, const char *pszCodeset)
{
    int         rc;
    unsigned    i;

    /* Cleanup in case of some special LC_ALL call. */
    localeCtypeFree(pCtype);

    /*
     * For "C" / "POSIX" we can simply use the static default locale.
     */
    if (    IS_C_LOCALE(pszLocale)
        ||  IS_POSIX_LOCALE(pszLocale))
    {
        memcpy(pCtype, &__libc_GLocaleCtypeDefault, sizeof(*pCtype));
        return 0;
    }

    /*
     * Query multi-byte related stuff.
     */
    rc = query_mbcs(uobj, &pCtype->mbcs, &pCtype->au2MBCSPrefixs[0]);
    if (rc)
        return rc;

    /*
     * Set codeset and query encoding.
     */
    strncpy(pCtype->szCodeSet, pszCodeset, sizeof(pCtype->szCodeSet));
    pCtype->szCodeSet[sizeof(pCtype->szCodeSet) - 1] = '\0';

    uconv_attribute_t   attr;
    rc = UniQueryUconvObject(uobj, &attr, sizeof(attr), NULL, NULL, NULL);
    if (rc)
        return -unierr2errno(rc);
    switch (attr.esid)
    {
        case ESID_sbcs_data:
        case ESID_sbcs_pc:
        case ESID_sbcs_ebcdic:
        case ESID_sbcs_iso:
        case ESID_sbcs_windows:
        case ESID_sbcs_alt:
            __libc_localeFuncsSBCS(&pCtype->CtypeFuncs);
            break;

        case ESID_dbcs_data:
        case ESID_dbcs_pc:
        case ESID_dbcs_ebcdic:
            __libc_localeFuncsDBCS(&pCtype->CtypeFuncs);
            break;

        case ESID_mbcs_data:
        case ESID_mbcs_pc:
        case ESID_mbcs_ebcdic:
            __libc_localeFuncsMBCS(&pCtype->CtypeFuncs);
            break;

        case ESID_ucs_2:
            __libc_localeFuncsUCS2(&pCtype->CtypeFuncs);
            break;

        case ESID_utf_8:
            __libc_localeFuncsUTF8(&pCtype->CtypeFuncs);
            break;

        case ESID_upf_8:
        case ESID_ugl:
        default:
            __libc_localeFuncsDefault(&pCtype->CtypeFuncs);
            break;
    }

    /*
     * For speeding up isXXX() and lower/upper case mapping functions
     * we'll cache the type of every character into a variable.
     *
     * Do every character separately to avoid errors that could result
     * when some character in the middle cannot be conerted from or to
     * Unicode - this would lead in a shift of the entire string.
     */
    /** @todo Aren't there apis for getting all this at once? */
    bzero(pCtype->auchToSBCS0To128, sizeof(pCtype->auchToSBCS0To128));
    bzero(pCtype->aSBCSs, sizeof(pCtype->aSBCSs));
    pCtype->cSBCSs = 0;
    for (i = 0; i < 256; i++)
    {
        unsigned        ufType = 0;
        unsigned char   uchUpper = i;
        unsigned char   uchLower = i;
        UniChar         uc = 0xffff;

        /* isxxx() does not support MBCS characters at all. */
        if (!pCtype->mbcs || !IS_MBCS_PREFIX(pCtype, i))
        {
            if (__libc_ucs2To(uobj, (unsigned char *)&i, 1, &uc))
            {
                /* ASSUMES that lower/upper are paired! */
                /* ASSUMES that there are no difference between the locale and the unicode spec! */
                UNICTYPE *pUniType = UniQueryCharType(uc);
                if (pUniType)
                {
                    ufType = ___wctype_uni(pUniType, uc);
                    if (ufType & __CT_LOWER)
                        uchUpper = Transform(lobj, uobj, UniTransUpper, uc, i);
                    if (ufType & __CT_UPPER)
                        uchLower = Transform(lobj, uobj, UniTransLower, uc, i);

                    /*
                     * Add to conversion table.
                     */
                    if (uc < 128)
                        pCtype->auchToSBCS0To128[uc] = i;
                    else
                    {
                        /*
                         * Try fit it into an existing chunk.
                         */
                        int iChunk;
                        for (iChunk = 0; iChunk < pCtype->cSBCSs; iChunk++)
                        {
                            int cFree = sizeof(pCtype->aSBCSs[iChunk].auch) / sizeof(pCtype->aSBCSs[iChunk].auch[0]) - pCtype->aSBCSs[iChunk].cChars;
                            if (cFree > 0)
                            {
                                int off = (int)uc - (int)pCtype->aSBCSs[iChunk].usStart;
                                if (off < sizeof(pCtype->aSBCSs[iChunk].auch) / sizeof(pCtype->aSBCSs[iChunk].auch[0]))
                                {
                                    if (off >= 0)
                                    {
                                        if (pCtype->aSBCSs[iChunk].cChars <= off)
                                            pCtype->aSBCSs[iChunk].cChars = off + 1;
                                        pCtype->aSBCSs[iChunk].auch[off] = i;
                                        break;
                                    }
                                    /* Relocate the chunk up to 3 bytes if that will help. (might cause overlapping areas!) */
                                    else if (off >= -3 && -off < cFree)
                                    {
                                        off = -off;
                                        memmove(&pCtype->aSBCSs[iChunk].auch[off], &pCtype->aSBCSs[iChunk].auch[0], pCtype->aSBCSs[iChunk].cChars * sizeof(pCtype->aSBCSs[iChunk].auch[0]));

                                        pCtype->aSBCSs[iChunk].usStart -= off;
                                        pCtype->aSBCSs[iChunk].cChars  += off;
                                        pCtype->aSBCSs[iChunk].auch[0]  = i;
                                        switch (off)
                                        {
                                            case 3: pCtype->aSBCSs[iChunk].auch[2] = 0;
                                            case 2: pCtype->aSBCSs[iChunk].auch[1] = 0; break;
                                        }
                                        break;
                                    }
                                }
                            }
                        } /* foreach chunk */

                        /*
                         * Add new chunk?
                         */
                        if (    iChunk == pCtype->cSBCSs
                            &&  iChunk < sizeof(pCtype->aSBCSs) / sizeof(pCtype->aSBCSs[0]))
                        {
                            pCtype->aSBCSs[iChunk].usStart = uc;
                            pCtype->aSBCSs[iChunk].cChars  = 1;
                            pCtype->aSBCSs[iChunk].auch[0] = i;
                            pCtype->cSBCSs++;
                        }
                    }
                } /* Unicode type data, ok. */
            }
            else
                uc = 0xffff;
        }

        /* Store the data in the locale structure. */
        pCtype->aufType[i]      = ufType;
        pCtype->auchUpper[i]    = uchUpper;
        pCtype->auchLower[i]    = uchLower;
        pCtype->aucUnicode[i]   = uc;
    } /* foreach char 0..255 */

    /*
     * Store the objects.
     */
    pCtype->uobj          = uobj;
    if (pCtype->mbcs)
    {
        /*
         * In MBCS mode we just borrow the local object and leave the
         * real work to the Unicode subsystem.
         */
        pCtype->lobj      = lobj;
    }
    else
        UniFreeLocaleObject(lobj);

    return 0;
}

/**
 * Swaps the content of two locale Ctype structures.
 *
 * @param   pHot        The hot structure (target).
 * @param   pCold       The cold structure (source, will be freed).
 */
static void localeCtypeSwap(__LIBC_LOCALECTYPE *pHot, __LIBC_PLOCALECTYPE pCold)
{
    __LIBC_LOCALECTYPE Tmp;
    memcpy(&Tmp, pHot, sizeof(Tmp));
    memcpy(pHot, pCold, sizeof(Tmp));
    memcpy(pCold, &Tmp, sizeof(Tmp));
}

/**
 * Frees system and CRT resources assocated with a ctype structure.
 * @param   pCtype    The collate structure.
 */
static void localeCtypeFree(__LIBC_PLOCALECTYPE pCtype)
{
    if (pCtype->uobj)
    {
        UniFreeUconvObject(pCtype->uobj);
        pCtype->uobj = NULL;
    }
    if (pCtype->lobj)
    {
        UniFreeLocaleObject(pCtype->lobj);
        pCtype->lobj = NULL;
    }
}

/**
 * Query one item from a locale object.
 */
static int query_item(LocaleObject lobj, UconvObject uobj, LocaleItem iItem, char **ppszOut)
{
    /*
     * Query item.
     */
    UniChar    *pucsItem;
    int rc = UniQueryLocaleItem(lobj, iItem, &pucsItem);
    if (rc)
    {
        LIBC_ASSERTM_FAILED("UniQueryLocaleItem(,%d,) -> rc=%d\n", iItem, rc);
        return -unierr2errno(rc);
    }

    /*
     * Convert from Ucs2.
     */
    rc = convert_ucs(uobj, pucsItem, ppszOut);

    UniFreeMem(pucsItem);
    return rc;
}

/**
 * Query an array of locale string items.
 */
static int query_array(LocaleObject lobj, UconvObject uobj, int cElements, LocaleItem iFirst, char **papszOut)
{
    int     i;
    for (i = 0; i < cElements; i++)
    {
        int rc = query_item(lobj, uobj, iFirst + i, &papszOut[i]);
        if (rc)
            return rc;
    }
    return 0;
}

/**
 * Sets the LC_TIME part of the locale.
 *
 * @returns 0 on success.
 * @returns negated errno on failure.
 * @param   pTime       The time structure to operate on.
 * @param   uobj        The UconvObject to use.
 * @param   lobj        The LocaleObject to use.
 * @param   pszLocale   Pointer to the locale base specifier.
 */
static int localeTimeDo(__LIBC_PLOCALETIME pTime, UconvObject uobj, LocaleObject lobj, const char *pszLocale)
{
    int         rc;

    /* free old stuff. */
    localeTimeFree(pTime);

    /*
     * For "C" / "POSIX" we can simply use the static default locale.
     */
    if (    IS_C_LOCALE(pszLocale)
        ||  IS_POSIX_LOCALE(pszLocale))
    {
        memcpy(pTime, &__libc_gLocaleTimeDefault, sizeof(*pTime));
        return 0;
    }

    /* query the items. */
    if (    (rc = query_item( lobj, uobj,     D_T_FMT,      &pTime->date_time_fmt))
        ||  (rc = query_item( lobj, uobj,     D_FMT,        &pTime->date_fmt))
        ||  (rc = query_item( lobj, uobj,     T_FMT,        &pTime->time_fmt))
        ||  (rc = query_item( lobj, uobj,     AM_STR,       &pTime->am))
        ||  (rc = query_item( lobj, uobj,     PM_STR,       &pTime->pm))
        ||  (rc = query_item( lobj, uobj,     T_FMT_AMPM,   &pTime->ampm_fmt))
        ||  (rc = query_item( lobj, uobj,     ERA,          &pTime->era))
        ||  (rc = query_item( lobj, uobj,     ERA_D_FMT,    &pTime->era_date_fmt))
        ||  (rc = query_item( lobj, uobj,     ERA_D_T_FMT,  &pTime->era_date_time_fmt))
        ||  (rc = query_item( lobj, uobj,     ERA_T_FMT,    &pTime->era_time_fmt))
        ||  (rc = query_item( lobj, uobj,     ALT_DIGITS,   &pTime->alt_digits))
        ||  (rc = query_item( lobj, uobj,     DATESEP,      &pTime->datesep))
        ||  (rc = query_item( lobj, uobj,     TIMESEP,      &pTime->timesep))
        ||  (rc = query_item( lobj, uobj,     LISTSEP,      &pTime->listsep))
        ||  (rc = query_array(lobj, uobj,  7, DAY_1,        &pTime->lwdays[0]))
        ||  (rc = query_array(lobj, uobj,  7, ABDAY_1,      &pTime->swdays[0]))
        ||  (rc = query_array(lobj, uobj, 12, MON_1,        &pTime->lmonths[0]))
        ||  (rc = query_array(lobj, uobj, 12, ABMON_1,      &pTime->smonths[0]))
            )
    {
        return rc;
    }

    /*
     * Hacks for bad data in LOCALE.DLL.
     */
    if (!strcmp(pTime->date_time_fmt, "%a %e %b %H:%H:%S %Z %Y"))
        strcpy( pTime->date_time_fmt, "%a %e %b %H:%M:%S %Z %Y");

    return 0;
}

/**
 * Commits the changed LC_TIME locale attributes.
 *
 * @param   pHot        The hot structure (target).
 * @param   pCold       The cold structure (source, will be freed).
 */
static void localeTimeCommit(__LIBC_LOCALETIME *pHot, __LIBC_PLOCALETIME pCold)
{
    int const fOverride = pHot->fConsts != pCold->fConsts || pCold->fConsts;
    SWAP_SIMPLE_MEMBERS(pHot, pCold, int, fConsts);
    MAYBE_SWAP_STRING_ARRAY_MEMBERS(pHot, pCold, smonths, fOverride);
    MAYBE_SWAP_STRING_ARRAY_MEMBERS(pHot, pCold, lmonths, fOverride);
    MAYBE_SWAP_STRING_ARRAY_MEMBERS(pHot, pCold, swdays, fOverride);
    MAYBE_SWAP_STRING_ARRAY_MEMBERS(pHot, pCold, lwdays, fOverride);
    MAYBE_SWAP_STRING_MEMBERS(pHot, pCold, date_time_fmt, fOverride);
    MAYBE_SWAP_STRING_MEMBERS(pHot, pCold, date_fmt, fOverride);
    MAYBE_SWAP_STRING_MEMBERS(pHot, pCold, time_fmt, fOverride);
    MAYBE_SWAP_STRING_MEMBERS(pHot, pCold, am, fOverride);
    MAYBE_SWAP_STRING_MEMBERS(pHot, pCold, pm, fOverride);
    MAYBE_SWAP_STRING_MEMBERS(pHot, pCold, ampm_fmt, fOverride);
    MAYBE_SWAP_STRING_MEMBERS(pHot, pCold, era, fOverride);
    MAYBE_SWAP_STRING_MEMBERS(pHot, pCold, era_date_fmt, fOverride);
    MAYBE_SWAP_STRING_MEMBERS(pHot, pCold, era_date_time_fmt, fOverride);
    MAYBE_SWAP_STRING_MEMBERS(pHot, pCold, era_time_fmt, fOverride);
    MAYBE_SWAP_STRING_MEMBERS(pHot, pCold, alt_digits, fOverride);
    MAYBE_SWAP_STRING_MEMBERS(pHot, pCold, datesep, fOverride);
    MAYBE_SWAP_STRING_MEMBERS(pHot, pCold, timesep, fOverride);
    MAYBE_SWAP_STRING_MEMBERS(pHot, pCold, listsep, fOverride);
}

/**
 * Frees the CRT resources held up by a time structure.
 * @param   pTime   The time structure.
 */
static void localeTimeFree(__LIBC_PLOCALETIME pTime)
{
    if (!pTime->fConsts)
    {
        /*
         * Everything is pointers to heap here!
         */
        char **ppsz = (char **)pTime;
        char **ppszEnd = (char **)pTime->fConsts;
        while ((uintptr_t)ppsz < (uintptr_t)ppszEnd)
        {
            void *pv = *ppsz;
            if (pv)
            {
                free(pv);
                *ppsz = NULL;
            }
        }
    }
    pTime->fConsts = 0;
}


/**
 * Commits the changed LC_NUMERIC locale attributes.
 *
 * @param   pHot        The hot structure (target).
 * @param   pCold       The cold structure (source, will be freed).
 */
static void localeNumericCommit(__LIBC_LOCALELCONV volatile *pHot, __LIBC_PLOCALELCONV pCold)
{
    int const fOverride = pHot->fNumericConsts != pCold->fNumericConsts || pCold->fNumericConsts;
    SWAP_SIMPLE_MEMBERS(pHot, pCold, int, fNumericConsts);
    MAYBE_SWAP_STRING_MEMBERS(pHot, pCold, s.decimal_point, fOverride);
    MAYBE_SWAP_STRING_MEMBERS(pHot, pCold, s.thousands_sep, fOverride);
    MAYBE_SWAP_STRING_MEMBERS(pHot, pCold, s.grouping, fOverride);
}

/**
 * Frees all heap strings in the monetary part of the lconv structure.
 * @param   pLconv  What to work on.
 */
static void localeNumericFree(__LIBC_PLOCALELCONV pLconv)
{
#define FREE(x) do { if (pLconv->s.x && !pLconv->fNumericConsts) free(pLconv->s.x); pLconv->s.x = NULL; } while (0)
    FREE(decimal_point);
    FREE(thousands_sep);
    FREE(grouping);

    pLconv->fNumericConsts = 0;
#undef FREE
}

/**
 * Commits the changed LC_MONETARY locale attributes.
 *
 * @param   pHot        The hot structure (target).
 * @param   pCold       The cold structure (source, will be freed).
 */
static void localeMonetaryCommit(__LIBC_LOCALELCONV volatile *pHot, __LIBC_PLOCALELCONV pCold)
{
    int const fOverride = pHot->fMonetaryConsts != pCold->fMonetaryConsts || pCold->fMonetaryConsts;
    SWAP_SIMPLE_MEMBERS(pHot, pCold, int,    fMonetaryConsts);
    MAYBE_SWAP_STRING_MEMBERS(pHot, pCold,   s.int_curr_symbol, fOverride);
    MAYBE_SWAP_STRING_MEMBERS(pHot, pCold,   s.currency_symbol, fOverride);
    MAYBE_SWAP_STRING_MEMBERS(pHot, pCold,   s.mon_decimal_point, fOverride);
    MAYBE_SWAP_STRING_MEMBERS(pHot, pCold,   s.mon_thousands_sep, fOverride);
    MAYBE_SWAP_STRING_MEMBERS(pHot, pCold,   s.mon_grouping, fOverride);
    MAYBE_SWAP_STRING_MEMBERS(pHot, pCold,   s.positive_sign, fOverride);
    MAYBE_SWAP_STRING_MEMBERS(pHot, pCold,   s.negative_sign, fOverride);
    SWAP_SIMPLE_MEMBERS(pHot, pCold, char,   s.int_frac_digits);
    SWAP_SIMPLE_MEMBERS(pHot, pCold, char,   s.frac_digits);
    SWAP_SIMPLE_MEMBERS(pHot, pCold, char,   s.p_cs_precedes);
    SWAP_SIMPLE_MEMBERS(pHot, pCold, char,   s.p_sep_by_space);
    SWAP_SIMPLE_MEMBERS(pHot, pCold, char,   s.n_cs_precedes);
    SWAP_SIMPLE_MEMBERS(pHot, pCold, char,   s.n_sep_by_space);
    SWAP_SIMPLE_MEMBERS(pHot, pCold, char,   s.p_sign_posn);
    SWAP_SIMPLE_MEMBERS(pHot, pCold, char,   s.n_sign_posn);
    SWAP_SIMPLE_MEMBERS(pHot, pCold, char,   s.int_p_cs_precedes);
    SWAP_SIMPLE_MEMBERS(pHot, pCold, char,   s.int_n_cs_precedes);
    SWAP_SIMPLE_MEMBERS(pHot, pCold, char,   s.int_p_sep_by_space);
    SWAP_SIMPLE_MEMBERS(pHot, pCold, char,   s.int_n_sep_by_space);
    SWAP_SIMPLE_MEMBERS(pHot, pCold, char,   s.int_p_sign_posn);
    SWAP_SIMPLE_MEMBERS(pHot, pCold, char,   s.int_n_sign_posn);
    MAYBE_SWAP_STRING_MEMBERS(pHot, pCold,   pszCrncyStr, fOverride);
}

/**
 * Frees all heap strings in the monetary part of the lconv structure.
 * @param   pLconv  What to work on.
 */
static void localeMonetaryFree(__LIBC_PLOCALELCONV pLconv)
{
#define FREE(x) do { if (pLconv->x && !pLconv->fMonetaryConsts) free(pLconv->x); pLconv->x = NULL; } while (0)
    FREE(s.int_curr_symbol);
    FREE(s.currency_symbol);
    FREE(s.mon_decimal_point);
    FREE(s.mon_thousands_sep);
    FREE(s.mon_grouping);
    FREE(s.positive_sign);
    FREE(s.negative_sign);
    FREE(pszCrncyStr);

    pLconv->fMonetaryConsts = 0;
#undef FREE
}

/**
 * Converts a grouping array.
 */
static int localeConvertGrouping(short *pasGrouping, char **pachRet)
{
    short  *ps;
    char   *pch;
    int     cch;

    for (cch = 1, ps = pasGrouping; *ps && *ps != -1; ps++)
        cch++;
    *pachRet = pch = malloc(cch);
    if (!pch)
        return -ENOMEM;
    for (ps = pasGrouping; cch > 0; cch--)
        *pch++ = (char)*ps++;

    return 0;
}

/**
 * Sets the LC_NUMERIC part of the locale.
 *
 * @returns 0 on success.
 * @returns negated errno on failure.
 * @param   pLconv      The lconv structure to operate on.
 * @param   uobj        The UconvObject to use.
 * @param   pULconv     Pointer to the pULconv structure to get the data from.
 * @param   pszLocale   Pointer to the locale base specifier.
 */
static int localeNumericDo(__LIBC_PLOCALELCONV pLconv, UconvObject uobj, struct UniLconv *pULconv, const char *pszLocale)
{
    int     rc;

    /* free any old stuff. */
    localeNumericFree(pLconv);

    /*
     * For "C" / "POSIX" we can simply use the static default locale.
     */
    if (    IS_C_LOCALE(pszLocale)
        ||  IS_POSIX_LOCALE(pszLocale))
    {
#define COPY(m) do { pLconv->m = __libc_gLocaleLconvDefault.m; } while (0)
	pLconv->fNumericConsts        = 1;
        COPY(s.decimal_point);
        COPY(s.thousands_sep);
        COPY(s.grouping);
#undef COPY
        return 0;
    }

    /*
     * Convert the stuff.
     */
#define CONVERT_UCS(field) \
    do  { rc = convert_ucs(uobj, pULconv->field, &pLconv->s.field); if (rc) return rc; } while (0)
    CONVERT_UCS(decimal_point);
    CONVERT_UCS(thousands_sep);
#undef CONVERT_UCS

    return localeConvertGrouping(pULconv->grouping, &pLconv->s.grouping);
}


/**
 * Sets the LC_MONETARY part of the locale.
 *
 * @returns 0 on success.
 * @returns negated errno on failure.
 * @param   pLconv      The lconv structure to operate on.
 * @param   uobj        The UconvObject to use.
 * @param   pULconv     Pointer to the pULconv structure to get the data from.
 * @param   pszLocale   The locale base specifier.
 * @param   pszModifier The locale modifier.
 */
static int localeMonetaryDo(__LIBC_PLOCALELCONV pLconv, UconvObject uobj, LocaleObject lobj, struct UniLconv *pULconv, const char *pszLocale, const char *pszModifier)
{
    int rc;

    /* free any old stuff. */
    localeMonetaryFree(pLconv);

    /*
     * For "C" / "POSIX" we can simply use the static default locale.
     */
    if (    IS_C_LOCALE(pszLocale)
        ||  IS_POSIX_LOCALE(pszLocale))
    {
#define COPY(m) do { pLconv->m = __libc_gLocaleLconvDefault.m; } while (0)
        COPY(s.int_curr_symbol);
        COPY(s.currency_symbol);
        COPY(s.mon_decimal_point);
        COPY(s.mon_thousands_sep);
        COPY(s.positive_sign);
        COPY(s.negative_sign);
        COPY(pszCrncyStr);
        COPY(s.mon_grouping);
        COPY(s.int_frac_digits);
        COPY(s.frac_digits);
        COPY(s.p_cs_precedes);
        COPY(s.p_sep_by_space);
        COPY(s.n_cs_precedes);
        COPY(s.n_sep_by_space);
        COPY(s.p_sign_posn);
        COPY(s.n_sign_posn);
        COPY(s.int_p_cs_precedes);
        COPY(s.int_n_cs_precedes);
        COPY(s.int_p_sep_by_space);
        COPY(s.int_n_sep_by_space);
        COPY(s.int_p_sign_posn);
        COPY(s.int_n_sign_posn);
        pLconv->fMonetaryConsts = 1;
#undef COPY
        return 0;
    }
    /*
     * Convert the stuff.
     */
#define FIXMAX(val) \
    ( (val) != 0xff ? (val) : CHAR_MAX ) /* (assumes CHAR_MAX == 0x7f) */
#define CONVERT_UCS(field) \
    do  { rc = convert_ucs(uobj, pULconv->field, &pLconv->s.field); if (rc) return rc; } while (0)

    if (pszModifier && IS_EURO(pszModifier))
    {
        /** @todo check for specs on a standard EURO grouping and stuff. */
        pLconv->s.currency_symbol = strdup("EUR");
        pLconv->s.int_curr_symbol = strdup("EUR");
    }
    else
    {
        CONVERT_UCS(int_curr_symbol);
        CONVERT_UCS(currency_symbol);
    }
    CONVERT_UCS(mon_decimal_point);
    CONVERT_UCS(mon_thousands_sep);
    CONVERT_UCS(positive_sign);
    CONVERT_UCS(negative_sign);

    pLconv->s.int_frac_digits   = FIXMAX(pULconv->int_frac_digits);
    pLconv->s.frac_digits       = FIXMAX(pULconv->frac_digits);
    pLconv->s.p_cs_precedes     = FIXMAX(pULconv->p_cs_precedes);
    pLconv->s.p_sep_by_space    = FIXMAX(pULconv->p_sep_by_space);
    pLconv->s.n_cs_precedes     = FIXMAX(pULconv->n_cs_precedes);
    pLconv->s.n_sep_by_space    = FIXMAX(pULconv->n_sep_by_space);
    pLconv->s.p_sign_posn       = FIXMAX(pULconv->p_sign_posn);
    pLconv->s.n_sign_posn       = FIXMAX(pULconv->n_sign_posn);
    /* we fake the international variants here. */
    pLconv->s.int_p_cs_precedes = pLconv->s.p_cs_precedes;
    pLconv->s.int_n_cs_precedes = pLconv->s.n_cs_precedes;
    pLconv->s.int_p_sep_by_space= pLconv->s.p_sep_by_space;
    pLconv->s.int_n_sep_by_space= pLconv->s.n_sep_by_space;
    pLconv->s.int_p_sign_posn   = pLconv->s.p_sign_posn;
    pLconv->s.int_n_sign_posn   = pLconv->s.n_sign_posn;

#undef FIXMAX
#undef CONVERT_UCS

    /*
     * Extra stuff (which I don't know which member corresponds to).
     */
    if ((rc = query_item(lobj, uobj, CRNCYSTR, &pLconv->pszCrncyStr)))
        return rc;

    return localeConvertGrouping(pULconv->mon_grouping, &pLconv->s.mon_grouping);
}

/**
 * Commits the changed LC_MESSAGES locale attributes.
 *
 * @param   pHot        The hot structure (target).
 * @param   pCold       The cold structure (source, will be freed).
 */
static void localeMessagesCommit(__LIBC_LOCALEMSG *pHot, __LIBC_PLOCALEMSG pCold)
{
    int const fOverride = pHot->fConsts != pCold->fConsts || pCold->fConsts;
    SWAP_SIMPLE_MEMBERS(pHot, pCold, int, fConsts);
    MAYBE_SWAP_STRING_MEMBERS(pHot, pCold, pszYesExpr, fOverride);
    MAYBE_SWAP_STRING_MEMBERS(pHot, pCold, pszNoExpr, fOverride);
    MAYBE_SWAP_STRING_MEMBERS(pHot, pCold, pszYesStr, fOverride);
    MAYBE_SWAP_STRING_MEMBERS(pHot, pCold, pszNoStr, fOverride);
}

/**
 * Frees all heap strings in the monetary part of the lconv structure.
 * @param   pLconv  What to work on.
 */
static void localeMessagesFree(__LIBC_PLOCALEMSG pMsg)
{
    if (!pMsg->fConsts)
    {
#define FREE(x) do { if (pMsg->x) free(pMsg->x); pMsg->x = NULL; } while (0)
        FREE(pszYesExpr);
        FREE(pszNoExpr);
        FREE(pszYesStr);
        FREE(pszNoStr);
#undef FREE
    }
    pMsg->fConsts = 0;
}


/**
 * Sets the LC_MESSAGES part of the locale.
 *
 * @returns 0 on success.
 * @returns negated errno on failure.
 * @param   pMsg        The messages locale info structure to operate on.
 * @param   uobj        The UconvObject to use.
 * @param   lobj        The LocaleObject to use.
 * @param   pszLocale   Pointer to the locale base specifier.
 */
static int localeMessagesDo(__LIBC_PLOCALEMSG pMsg, UconvObject uobj, LocaleObject lobj, const char *pszLocale)
{
    int     rc;
    /* free any old stuff. */
    localeMessagesFree(pMsg);

    /*
     * For "C" / "POSIX" we can simply use the static default locale.
     */
    if (    IS_C_LOCALE(pszLocale)
        ||  IS_POSIX_LOCALE(pszLocale))
    {
        memcpy(pMsg, &__libc_gLocaleMsgDefault, sizeof(*pMsg));
        return 0;
    }

    /* query the items. */
    if (    (rc = query_item(lobj, uobj,     YESEXPR,      &pMsg->pszYesExpr))
        ||  (rc = query_item(lobj, uobj,     NOEXPR,       &pMsg->pszNoExpr))
        ||  (rc = query_item(lobj, uobj,     YESSTR,       &pMsg->pszYesStr))
        ||  (rc = query_item(lobj, uobj,     NOSTR,        &pMsg->pszNoStr))
            )
    {
        return rc;
    }

    return 0;
}


/**
 * Swaps the locate name between two locale instances unless they are equal.
 *
 * @param   pHot        The hot structure (target).
 * @param   pCold       The cold structure (source, will be freed).
 * @param   iCategory   The cateogry which name shall be swapped.
 */
static void localeGlobalNameCommit(__LIBC_LOCALEGLOBAL volatile *pHot, __LIBC_PLOCALEGLOBAL pCold, int iCategory)
{
    MAYBE_SWAP_STRING_MEMBERS(pHot, pCold, apszNames[iCategory + 1], 0);
}


/**
 * Free an entry in the global locale data array.
 */
static void localeGlobalFree(__LIBC_PLOCALEGLOBAL pGlobal, int iCategory)
{
    if (pGlobal->apszNames[iCategory + 1])
    {
        if (   pGlobal->apszNames[iCategory + 1] != gszC
            && pGlobal->apszNames[iCategory + 1] != gszPOSIX)
            free(pGlobal->apszNames[iCategory + 1]);
        pGlobal->apszNames[iCategory + 1] = NULL;
    }
}


/**
 * Parses out the locale spec and the code page spec.
 */
static int localeParseLocale(char *pszLocale, const char **ppszCodepage, const char **ppszModifier)
{
    /*
     * Modifier.
     */
    char *psz = strchr(pszLocale, '@');
    if (psz)
    {
        *psz++ = '\0';
        LIBCLOG_MSG2("Using locale modifier '%s'\n", psz);
    }
    *ppszModifier = psz;

    /*
     * Codepage.
     */
    psz = strchr(pszLocale, '.');
    if (psz)
        *psz++ = '\0';
    *ppszCodepage = psz;

    return 0;
}

/**
 * Get the default local specification.
 * @returns Pointer to default local (can be environment or it could be pszBuffer).
 * @param   pszBuffer   Where to store the default local.
 */
static const char *getDefaultLocale(const char *pszCategory, char *pszBuffer, int *pfDefault)
{
    /* Copy pszLocale to a local storage since we'll modify it during parsing.
       If pszLocale value is a empty string, user wants the defaults fetched from
       environment. */
    const char *pszRet = getenv("LC_ALL");
    if (pszRet && *pszRet)
        *pfDefault = 0;                 /* LC_ALL is not default, it's an override of everything else. */
    else
    {
        *pfDefault = 1;
        pszRet = getenv(pszCategory);
        if (!pszRet)
        {
            pszRet = getenv("LANG");
            if (!pszRet)
            {
                /*
                 * The default is 'C' or 'POSIX'.
                 *
                 * But if old style is enabled we'll be using the country
                 * info to get a locale.
                 */
                pszRet = gszC;
                if (gfOldStyle < 0)
                    gfOldStyle = getenv("LIBC_SETLOCALE_OLDSTYLE") != NULL;
                if (gfOldStyle)
                {
                    /*
                     * Not specified nor in environment, use country info.
                     * This is actually wrong in POSIX sense, but it makes "OS/2 sense". :)
                     */
                    COUNTRYCODE ctryc = {0,0};
                    COUNTRYINFO ctryi = {0};
                    ULONG       cb;
                    int         rc;
                    FS_VAR();

                    FS_SAVE_LOAD();
                    rc = DosQueryCtryInfo(sizeof(ctryi), &ctryc, &ctryi, &cb);
                    if (!rc /*|| rc == ERROR_COU*/)
                    {
                        UniChar     ucs[50];
                        rc = UniMapCtryToLocale(ctryi.country, ucs, sizeof(ucs));
                        if (!rc)
                        {
                            Ucs2Sb(ucs, pszBuffer, UniStrlen(ucs) + 1);
                            pszRet = pszBuffer;
                        }
                        else
                            LIBC_ASSERTM_FAILED("UniMapCtryToLocale(%ld) failed rc=%d\n", ctryi.country, rc);
                    }
                    else
                        LIBC_ASSERTM_FAILED("DosQueryCtryInfo failed rc=%d\n", rc);
                    FS_RESTORE();
                }
            }
        }
    }
    return pszRet;
}

/**
 * Extracts the code page from the locale spec or gets the default
 * code page.
 * @returns 0 on success.
 * @returns negated errno on failure.
 * @param   pszCodepage     Pointer to where the codepage specifier starts.
 * @param   pszLocale       Pointer to the locale base specifier.
 * @param   lobj            The locale object handle.
 * @param   pucsCodepage    Where to store the code page.
 * @param   cucCodepage     Number of UniChar's in the buffer.
 */
static int getCodepage(const char *pszCodepage, const char *pszLocale, LocaleObject lobj, UniChar *pucsCodepage, unsigned cucCodepage)
{
    /*
     * Look at what the user provides.
     */
    if (pszCodepage && *pszCodepage)
        __libc_TranslateCodepage(pszCodepage, pucsCodepage);
    else
    {
#if 0 /* See ticket #227, #240, #187 and probably more. */
        int      rc = -1;
        if (gfOldStyle < 0)
            gfOldStyle = getenv("LIBC_SETLOCALE_OLDSTYLE") != NULL;

        /*
         * The locale object contains codepage information.
         * We'll use that unless someone want's the old style.
         */
        if (!gfOldStyle)
        {
            UniChar *pucsItem;
            rc = UniQueryLocaleItem(lobj, LOCI_sISOCodepage, &pucsItem);
            if (!rc)
            {
                UniChar *pucs = pucsItem;
                while ( (*pucsCodepage++ = *pucs++) != '\0')
                    /* nada */;
                UniFreeMem(pucsItem);
                return 0;
            }
        }

        /*
         * Old style / fallback.
         */
#endif
        if (IS_C_LOCALE(pszLocale) || IS_POSIX_LOCALE(pszLocale))
            /*
             * The "C" character encoding maps to ISO8859-1 which is not quite true,
             * but Unicode API doesn't have a codepage that matches the POSIX "C"
             * pszCodepage, so that's what we presume when user requests the "C" pszCodepage.
             */
            memcpy(pucsCodepage, gucsISO8859_1, sizeof(gucsISO8859_1));
        else
        {
            /*
             * Consider current process codepage as default for specified language.
             */
            ULONG   aulCPs[5];
            ULONG   cb;
            int     rc;
            FS_VAR();

            FS_SAVE_LOAD();
            rc = DosQueryCp(sizeof(aulCPs), &aulCPs[0], &cb);
            if (rc)
            {
                FS_RESTORE();
                LIBC_ASSERTM_FAILED("DosQueryCp failed with rc=%d\n", rc);
                return -EDOOFUS;
            }
            LIBC_ASSERT(cb >= sizeof(ULONG));
            LIBCLOG_MSG2("locale: using process codepage %ld\n", aulCPs[0]);
            rc = UniMapCpToUcsCp(aulCPs[0], pucsCodepage, cucCodepage);
            FS_RESTORE();
            if (rc)
            {
                LIBC_ASSERTM_FAILED("UniMapCpToUcsCp(%ld,,) -> %d\n", aulCPs[0], rc);
                return -unierr2errno(rc);
            }
        }
    }
    return 0;
}


/**
 * Creates the libuni objects we need.
 */
int __libc_localeCreateObjects(const char *pszLocale, const char *pszCodepage, char *pszCodepageActual, LocaleObject *plobj, UconvObject *puobj)
{
    LIBCLOG_ENTER("pszLocale=%p:{%s} pszCodepage=%p:{%s} pszCodepageActual=%p plobj=%p puobj=%p\n",
                  pszLocale, pszLocale, pszCodepage, pszCodepage, pszCodepageActual, (void *)plobj, (void *)puobj);
    UniChar ucsCodepage[CODEPAGE_MAX_LENGTH];
    int     rc;

    /*
     * Create locale object.
     */
    if (IS_C_LOCALE(pszLocale) || IS_POSIX_LOCALE(pszLocale))
        rc = UniCreateLocaleObject(UNI_MBS_STRING_POINTER, gszC, plobj);
    else
        rc = UniCreateLocaleObject(UNI_MBS_STRING_POINTER, pszLocale, plobj);
    if (rc)
    {
        LIBCLOG_ERROR("UniCreateLocaleObject(,%p:{%s},) -> rc=%d\n", pszLocale, pszLocale, rc);
        rc = -unierr2errno(rc);
        LIBCLOG_ERROR_RETURN_INT(rc);
    }

    /*
     * Calc code page and create object.
     */
    rc = getCodepage(pszCodepage, pszLocale, *plobj, &ucsCodepage[0], sizeof(ucsCodepage) / sizeof(ucsCodepage[0]));
    if (!rc)
    {
        rc = UniCreateUconvObject(ucsCodepage, puobj);
        if (!rc)
        {
            if (pszCodepageActual)
            {
                size_t cchCodepageActual = UniStrlen(ucsCodepage);
                Ucs2Sb(ucsCodepage, pszCodepageActual, cchCodepageActual + 1);

                /*
                 * For some common codeset specs we'll normalize the naming.
                 */
                const char *psz;
                if ((psz = strstr(pszCodepageActual,
                                  "ibm-1208!" "Ibm-1208!" "IBm-1208!" "IBM-1208!" "IbM-1208!" "iBm-1208!" "iBM-1208!" "ibM-1208!"
                                  "utf-8!"    "Utf-8!"    "UTf-8!"    "UTF-8!"    "UtF-8!"    "uTf-8!"    "uTF-8!"    "utF-8!"
                                  "utf8!"     "Utf8!"     "UTf8!"     "UTF8!"     "UtF8!"     "uTf8!"     "uTF8!"     "utF8!")) != NULL
                    && psz[cchCodepageActual] == '!')
                    memcpy(pszCodepageActual, "UTF-8", sizeof("UTF-8"));
                else if ((psz = strstr(pszCodepageActual,
                                       "ibm-1200!" "Ibm-1200!" "IBm-1200!" "IBM-1200!" "IbM-1200!" "iBm-1200!" "iBM-1200!" "ibM-1200!"
                                       "ucs-2!"    "Ucs-2!"    "UCs-2!"    "UCS-2!"    "UcS-2!"    "uCs-2!"    "uCS-2!"    "ucS-2!"
                                       "ucs2!"     "Ucs2!"     "UCs2!"     "UCS2!"     "UcS2!"     "uCs2!"     "uCS2!"     "ucS2!")) != NULL
                         && psz[cchCodepageActual] == '!')
                    memcpy(pszCodepageActual, "UCS-2", sizeof("UCS-2"));
            }
            LIBCLOG_RETURN_MSG(rc, "ret 0 *plobj=%08x *puobj=%08x pszCodepageActual=%p:{%s}\n",
                               (unsigned)*plobj, (unsigned)*puobj, pszCodepageActual, pszCodepageActual);
        }

        LIBCLOG_ERROR("UniCreateUconvObject(%ls,) -> rc=%d\n", (wchar_t *)ucsCodepage, rc);
        rc = -unierr2errno(rc);
    }

    UniFreeLocaleObject(*plobj);
    LIBCLOG_ERROR_RETURN_INT(rc);
}



/**
 * Perform the locale operation on one category.
 */
static int localeDoOne(struct temp_locale *pTemp, int iCategory, const char *pszLocale, const char *pszCodepage, const char *pszModifier)
{
    LIBCLOG_ENTER("pTemp=%p iCategory=%d (%s) pszLocale=%p:{%s} pszCodepage=%p:{%s} pszModifier=%p:{%s}\n",
                  (void *)pTemp, iCategory, gaszCategories[iCategory + 1], pszLocale, pszLocale, pszCodepage, pszCodepage, pszModifier, pszModifier);
    char            szCodepageActual[CODEPAGE_MAX_LENGTH];
    UconvObject     uobj;
    LocaleObject    lobj;
    int             rc;
    int             fFree;


    /*
     * Create the objects.
     */
    rc = __libc_localeCreateObjects(pszLocale, pszCodepage, &szCodepageActual[0], &lobj, &uobj);
    if (rc)
        return rc;

    /*
     * Call the worker for the locale category.
     */
    fFree = 1;
    pTemp->afProcessed[iCategory + 1] = 1;
    switch (iCategory)
    {
        case LC_COLLATE:
            rc = localeCollateDo(&pTemp->Collate, uobj, lobj, pszLocale);
            fFree = rc != 0;
            break;

        case LC_CTYPE:
            rc = localeCtypeDo(&pTemp->Ctype, uobj, lobj, pszLocale, &szCodepageActual[0]);
            fFree = rc != 0 || IS_C_LOCALE(pszLocale) || IS_POSIX_LOCALE(pszLocale);
            break;

        case LC_TIME:
            rc = localeTimeDo(&pTemp->Time, uobj, lobj, pszLocale);
            break;

        case LC_NUMERIC:
        case LC_MONETARY:
        {
            /*
             * Query the unicode data..
             */
            struct UniLconv *pULconv;
            rc = UniQueryLocaleInfo(lobj, &pULconv);
            if (!rc)
            {
                if (iCategory == LC_NUMERIC)
                    rc = localeNumericDo(&pTemp->Lconv, uobj, pULconv, pszLocale);
                else
                    rc = localeMonetaryDo(&pTemp->Lconv, uobj, lobj, pULconv, pszLocale, pszModifier);
                UniFreeLocaleInfo(pULconv);
            }
            else
            {
                LIBCLOG_ERROR("UniQueryLocaleInfo -> %d\n", rc);
                rc = -unierr2errno(rc);
            }
            break;
        }

        case LC_MESSAGES:
            rc = localeMessagesDo(&pTemp->Msg, uobj, lobj, pszLocale);
            break;

        default:
            rc = 0;
            break;
    }


    /*
     * Cleanup.
     */
    if (fFree)
    {
        UniFreeLocaleObject(lobj);
        UniFreeUconvObject(uobj);
    }

    if (!rc)
    {
        /*
         * Build and set catagory value.
         * The 'pszLocale' variable already contains language and country.
         */
        localeGlobalFree(&pTemp->Global, iCategory);
        if ((!pszCodepage || !*pszCodepage) && IS_C_LOCALE(pszLocale))
            pTemp->Global.apszNames[iCategory + 1] = (char *)gszC;
        else if ((!pszCodepage || !*pszCodepage) && IS_POSIX_LOCALE(pszLocale))
            pTemp->Global.apszNames[iCategory + 1] = (char *)gszPOSIX;
        else
	{
            /* pszLocale + "." + szCodepageActual [+ "@" + pszModifier] */
            int     cch1 = strlen(pszLocale);
            int     cch2 = strlen(szCodepageActual);
            int     cch3 = pszModifier ? strlen(pszModifier) + 1 : 0;
            char   *psz = pTemp->Global.apszNames[iCategory + 1] = malloc(cch1 + cch2 + cch3 + 2);
            if (!psz)
                return -ENOMEM;
            memcpy(psz, pszLocale, cch1);
            psz += cch1;

            *psz++ = '.';
            memcpy(psz, szCodepageActual, cch2);
            psz += cch2;
            *psz = '\0';

            if (cch3)
            {
                *psz++ = '@';
                memcpy(psz, pszModifier, cch3);
            }

            LIBCLOG_MSG2("Setting iCategory='%d''%s' locale value to '%s'\n",
                         iCategory, gaszCategories[iCategory + 1], pTemp->Global.apszNames[iCategory + 1]);
	}
    }

    LIBCLOG_RETURN_INT(rc);
}



/**
 * Perform the more complex setlocale() operations which requires that
 * failure doesn't change anything. It will use an auto variable for
 * the temporary locale structure comitting it if all goes fine.
 *
 * @returns 0 on success.
 * @returns Negative errno on failure.
 */
static int localeDo(struct temp_locale *pTemp, int iCategory, char *pszLocale, int fDefaultValue)
{
    LIBCLOG_ENTER("pTemp=%p iCategory=%d (%s) pszLocale=%p:{%s} fDefaultValue=%d\n",
                  (void *)pTemp, iCategory, gaszCategories[iCategory + 1], pszLocale, pszLocale, fDefaultValue);
    const char *pszCodepage;
    const char *pszModifier;
    char       *pszNext;
    int         rc;

    /*
     * Process it.
     */
    if (iCategory != LC_ALL)
    {
        rc = localeParseLocale(pszLocale, &pszCodepage, &pszModifier);
        if (!rc)
            rc = localeDoOne(pTemp, iCategory, pszLocale, pszCodepage, pszModifier);
    }
    else
    {
        /*
         * Parse the pszLocale string user passed to us. This is either a string
         * in the XPG format (see below) or a list of values of the
         * form "CATEGORY1=value1;CATEGORY2=value2[;...]", where values are
         * also in XPG format: "language[_territory[.codeset]][@modifier]".
         * Currently we're only handling the @EURO/@euro modifiers and ignoring the rest.
         */
        pszNext = strchr(pszLocale, ';');
        if (pszNext)
        {
            /*
             * User supplied a list of iCategory=value statements separated with ';'.
             */
            char *pszCur = pszLocale;
            *pszNext++ = '\0';          /* remove the ';'. */
            for (;;)
            {
                int iCat;
                /* Search for the variable, ignoring those we cannot find.s */
                for (rc = 0, iCat = LC_ALL; iCat < _LC_LAST; iCat++)
                {
                    unsigned    cch = gacchCategories[iCat + 1];
                    if (    strncmp(pszCur, gaszCategories[iCat + 1], cch) == 0
                        &&  pszCur[cch] == '=')
                    {
                        char        *pszVal = &pszCur[cch + 1];
                        const char  *pszValCp;
                        const char  *pszValQual;
                        /* parse the locale value. */
                        rc = localeParseLocale(pszVal, &pszValCp, &pszValQual);
                        if (!rc)
                        {
                            if (iCat != LC_ALL)
                                rc = localeDoOne(pTemp, iCat, pszVal, pszValCp, pszValQual);
                            else /* Iterate all categories except LC_ALL. */
                                for (iCat = LC_ALL + 1; !rc && iCat < _LC_LAST; iCat++)
                                    rc = localeDoOne(pTemp, iCat, pszVal, pszValCp, pszValQual);
                        }
                        break;
                    }
                }

                /* next */
                if (!pszNext || rc < 0)
                    break;
                pszCur = pszNext;
                pszNext = strchr(pszCur, ';');
                if (pszNext)
                    *pszNext++ = '\0';
            }
        }
        else
        {
            /*
             * Set all pszLocale categories to given value.
             * Parse it first to save time.
             */
            rc = localeParseLocale(pszLocale, &pszCodepage, &pszModifier);
            if (!rc)
            {
                int iCat;
                for (iCat = LC_ALL + 1; !rc && iCat < _LC_LAST; iCat++)
                {
                    const char *pszEnv;
                    /*
                     * If user wants default values, we must check environment first.
                     */
                    if (fDefaultValue && (pszEnv = getenv(gaszCategories[iCat + 1])) != NULL)
                    {
                        const char *pszCodepageEnv;
                        const char *pszModifierEnv;
                        char *pszCopy = alloca(strlen(pszEnv) + 1);
                        if (!pszCopy)
                            LIBCLOG_RETURN_INT(-ENOMEM);
                        rc = localeParseLocale(strcpy(pszCopy, pszEnv), &pszCodepageEnv, &pszModifierEnv);
                        if (!rc)
                            rc = localeDoOne(pTemp, iCat, pszCopy, pszCodepageEnv, pszModifierEnv);
                    }
                    else
                        rc = localeDoOne(pTemp, iCat, pszLocale, pszCodepage, pszModifier);
                }
            }
        }
    }
    LIBCLOG_RETURN_INT(rc);
}


/**
 * Commits a temporary local and updates the global locale strings.
 */
static char *localeCommit(struct temp_locale *pTemp, int iCategory)
{
    char           *pszRet;
    char           *pszAll;
    int             cch;
    int             iCat;

    /*
     * Lock the structure.
     */
    _smutex_request(&gLocale.lock);

    /*
     * Swap all the data (caller frees the old data).
     */
    if (pTemp->afProcessed[LC_COLLATE + 1])
    {
        localeCollateSwap(&__libc_gLocaleCollate, &pTemp->Collate);
        localeGlobalNameCommit(&gLocale, &pTemp->Global, LC_COLLATE);
    }

    if (pTemp->afProcessed[LC_CTYPE + 1])
    {
        MB_CUR_MAX = pTemp->Ctype.mbcs ? pTemp->Ctype.mbcs : 1;
        if (    IS_C_LOCALE(pTemp->Global.apszNames[LC_CTYPE + 1])
            ||  IS_POSIX_LOCALE(pTemp->Global.apszNames[LC_CTYPE + 1]))
            __libc_GLocaleWCtype.uMask = ~0x7fU;
        else
            __libc_GLocaleWCtype.uMask = ~0xffU;

        localeCtypeSwap(&__libc_GLocaleCtype, &pTemp->Ctype);
        localeGlobalNameCommit(&gLocale, &pTemp->Global, LC_CTYPE);
    }

    if (pTemp->afProcessed[LC_TIME + 1])
    {
        localeTimeCommit(&__libc_gLocaleTime, &pTemp->Time);
        localeGlobalNameCommit(&gLocale, &pTemp->Global, LC_TIME);
    }

    if (pTemp->afProcessed[LC_NUMERIC + 1])
    {
        localeNumericCommit(&__libc_gLocaleLconv, &pTemp->Lconv);
        localeGlobalNameCommit(&gLocale, &pTemp->Global, LC_NUMERIC);
    }

    if (pTemp->afProcessed[LC_MONETARY + 1])
    {
        localeMonetaryCommit(&__libc_gLocaleLconv, &pTemp->Lconv);
        localeGlobalNameCommit(&gLocale, &pTemp->Global, LC_MONETARY);
    }
    if (pTemp->afProcessed[LC_MESSAGES + 1])
    {
        localeMessagesCommit(&__libc_gLocaleMsg, &pTemp->Msg);
        localeGlobalNameCommit(&gLocale, &pTemp->Global, LC_MESSAGES);
    }

    /*
     * Now we must build the LC_ALL string.
     *
     * If all the entries are not identical we must make a "category=value"
     * string for LC_ALL. Else we can just duplicate one of the others.
     */
    pszAll = gLocale.apszNames[1];
    cch = gacchCategories[1] + 1 + strlen(pszAll) + 1 + 1;
    for (iCat = 2; iCat <= _LC_LAST; iCat++)
    {
        int cchCat = strlen(gLocale.apszNames[iCat]);
        cch += gacchCategories[iCat] + 1 + cchCat + 1;
        if (pszAll && strcmp(pszAll, gLocale.apszNames[iCat]))
            pszAll = NULL;
    }

    if (!pszAll)
    {
        /*
         * Not identical. Generate composite value.
         */
        char *psz = pszAll = malloc(cch); /* If we're out of memory here, then it's just too bad :-/ */
        for (iCat = 1; iCat <= _LC_LAST; iCat++) /* (iCat is array index not lc idx this time) */
        {
            int cchCat = gacchCategories[iCat];
            memcpy(psz, gaszCategories[iCat], cchCat);
            psz += cchCat;
            *psz++ = '=';
            cchCat = strlen(gLocale.apszNames[iCat]);
            memcpy(psz, gLocale.apszNames[iCat], cchCat);
            psz += cchCat;
            *psz++ = ';';
        }
        *psz = '\0';
        pTemp->Global.apszNames[LC_ALL + 1] = pszAll;
        localeGlobalNameCommit(&gLocale, &pTemp->Global, LC_ALL);
    }
    else if (strcmp(gLocale.apszNames[LC_ALL + 1], pszAll))
    {
        pTemp->Global.apszNames[LC_ALL + 1] = gLocale.apszNames[LC_ALL + 1];
        gLocale.apszNames[LC_ALL + 1] = strdup(pszAll);
    }

    /*
     * Unlock and returns.
     */
    pszRet = gLocale.apszNames[iCategory + 1];
    _smutex_release(&gLocale.lock);

    return pszRet;
}


/**
 * Clean up the temporary locale instance structure.
 * @param   pTemp   Stuff to cleanup.
 */
static void localeFree(struct temp_locale *pTemp)
{
    int iCat;
    if (pTemp->afProcessed[LC_COLLATE + 1])
        localeCollateFree(&pTemp->Collate);
    if (pTemp->afProcessed[LC_CTYPE + 1])
        localeCtypeFree(&pTemp->Ctype);
    if (pTemp->afProcessed[LC_TIME + 1])
        localeTimeFree(&pTemp->Time);
    if (pTemp->afProcessed[LC_NUMERIC + 1])
        localeNumericFree(&pTemp->Lconv);
    if (pTemp->afProcessed[LC_MONETARY + 1])
        localeMonetaryFree(&pTemp->Lconv);
    if (pTemp->afProcessed[LC_MESSAGES + 1])
        localeMessagesFree(&pTemp->Msg);
    localeGlobalFree(&pTemp->Global, LC_ALL);
    for (iCat = 0; iCat < _LC_LAST; iCat++)
        if (pTemp->afProcessed[iCat + 1])
            localeGlobalFree(&pTemp->Global, iCat);
}


/**
 * This setlocale() implementation differs from the specs in that any
 * options specified by '@...' other than EURO/euro are ignored.
 */
char *_STD(setlocale)(int iCategory, const char *pszLocale)
{
    LIBCLOG_ENTER("iCategory=%d pszLocale=%p:{%s}\n", iCategory, pszLocale, pszLocale);
    char            szTmpBuf[64];
    int             fDefaultValue;
    size_t          cch;
    char           *pszLocaleCopy;
    char           *pszRet;
    int             rc;

    /*
     * Validate input.
     */
    if (iCategory < LC_ALL || iCategory >= _LC_LAST)
    {
        errno = EINVAL;
        LIBCLOG_ERROR_RETURN_MSG(NULL, "ret %p - iCategory=%d is invalid!\n", (void *)NULL, iCategory);
    }

    /*
     * Check if user just queries current pszLocale.
     */
    if (!pszLocale)
    {
        pszRet = gLocale.apszNames[iCategory + 1];
        LIBCLOG_RETURN_MSG(pszRet, "ret %p:{%s}\n", pszRet, pszRet);
    }

    /*
     * Check if user wants we to do the same job twice.
     */
    if (strcmp(pszLocale, gLocale.apszNames[iCategory + 1]) == 0)
    {
        /* We have to return the value of LC_ALL */
        pszRet = gLocale.apszNames[iCategory + 1];
        LIBCLOG_RETURN_MSG(pszRet, "ret %p:{%s} (already set)\n", pszRet, pszRet);
    }


    /*
     * If pszLocale value is a empty string, user wants the defaults fetched from
     * environment.
     */
    fDefaultValue = *pszLocale == '\0';
    if (fDefaultValue)
        pszLocale = getDefaultLocale(gaszCategories[iCategory + 1], szTmpBuf, &fDefaultValue);

    /*
     * Copy pszLocale to a local storage since we'll modify it during parsing.
     */
    cch = strlen(pszLocale) + 1;
    pszLocaleCopy = (char *)alloca(cch);
    if (!pszLocaleCopy)
    {
        errno = ENOMEM;
        LIBCLOG_ERROR_RETURN_P(NULL);
    }
    memcpy(pszLocaleCopy, pszLocale, cch);


    /*
     * Allocate a temporary locale state and perform
     * the locale operation on that.
     */
    struct temp_locale *pTemp = alloca(sizeof(struct temp_locale));
    bzero(pTemp, sizeof(struct temp_locale));

    rc = localeDo(pTemp, iCategory, pszLocaleCopy, fDefaultValue);

    /*
     * If successful commit the temporary locale.
     */
    if (!rc)
        pszRet = localeCommit(pTemp, iCategory);
    else
    {
        errno = -rc;
        pszRet = NULL;
    }

    /*
     * Cleanup and exit.
     */
    localeFree(pTemp);

    if (pszRet)
        LIBCLOG_RETURN_MSG(pszRet, "ret %p:{%s}\n", pszRet, pszRet);
    LIBCLOG_ERROR_RETURN_P(NULL);
}

#undef ERROR

#undef  __LIBC_LOG_GROUP
#define __LIBC_LOG_GROUP    __LIBC_LOG_GRP_FORK


_FORK_CHILD1(0xffffff00, setlocalForkChild1);

/**
 * Create any unicode objects used by the locale stuff.
 *
 * !describe me!
 */
static int setlocalForkChild1(__LIBC_PFORKHANDLE pForkHandle, __LIBC_FORKOP enmOperation)
{
    LIBCLOG_ENTER("pForkHandle=%p enmOperation=%d\n", (void *)pForkHandle, enmOperation);
    int rc;
    switch (enmOperation)
    {
        case __LIBC_FORK_OP_FORK_CHILD:
        {
            gLocale.lock = 0;

            rc = 0;
            if (    __libc_GLocaleCtype.lobj
                ||  __libc_GLocaleCtype.uobj
                ||  __libc_gLocaleCollate.lobj
                ||  __libc_gLocaleCollate.uobj
                )
            {
                LocaleObject    lobj;
                UconvObject     uobj;
                const char *pszCodepage;
                const char *pszModifier;
                int     cch1 = strlen(gLocale.apszNames[LC_CTYPE + 1]) + 1;
                int     cch2 = strlen(gLocale.apszNames[LC_COLLATE + 1]) + 1;
                int     cch = cch1 > cch2 ? cch1 : cch2;
                char   *psz = alloca(cch);
                if (!psz)
                    LIBCLOG_RETURN_INT(-ENOMEM);

                if (    __libc_GLocaleCtype.lobj
                    ||  __libc_GLocaleCtype.uobj)
                {
                    memcpy(psz, gLocale.apszNames[LC_CTYPE + 1], cch);
                    localeParseLocale(psz, &pszCodepage, &pszModifier);
                    rc = __libc_localeCreateObjects(psz, pszCodepage, NULL, &lobj, &uobj);
                    if (rc)
                        LIBCLOG_RETURN_INT(rc);

                    if (__libc_GLocaleCtype.lobj)
                        __libc_GLocaleCtype.lobj = lobj;
                    else
                        UniFreeLocaleObject(lobj);

                    if (__libc_GLocaleCtype.uobj)
                        __libc_GLocaleCtype.uobj = uobj;
                    else
                        UniFreeUconvObject(uobj);
                }

                if (    __libc_gLocaleCollate.lobj
                    ||  __libc_gLocaleCollate.uobj)
                {
                    memcpy(psz, gLocale.apszNames[LC_COLLATE + 1], cch);
                    localeParseLocale(psz, &pszCodepage, &pszModifier);
                    rc = __libc_localeCreateObjects(psz, pszCodepage, NULL, &lobj, &uobj);
                    if (rc)
                        LIBCLOG_RETURN_INT(rc);

                    if (__libc_gLocaleCollate.lobj)
                        __libc_gLocaleCollate.lobj = lobj;
                    else
                        UniFreeLocaleObject(lobj);

                    if (__libc_gLocaleCollate.uobj)
                        __libc_gLocaleCollate.uobj = uobj;
                    else
                        UniFreeUconvObject(uobj);
                }
            }
            break;
        }
        default:
            rc = 0;
            break;
    }
    LIBCLOG_RETURN_INT(rc);
}

