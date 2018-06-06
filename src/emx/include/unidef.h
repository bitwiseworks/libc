/*
 * Legalesy-free Unicode API interface for OS/2
 * Interface definitions for basic Unicode API functions
 *
 * Written by Andrew Zabolotny <bit@eltech.ru>
 *
 * This file is put into public domain. You are free to do
 * literally anything you wish with it: modify, print, sell,
 * rent, eat, throw out of window: in all (esp. in later)
 * cases I am not responsible for any damage it causes.
 */

#ifndef __UNIDEF_H__
#define __UNIDEF_H__

#include <sys/cdefs.h>
#include <stddef.h>
#include <time.h>

#include <ulserrno.h>
#include <ulsitem.h>

typedef unsigned short UniChar;
typedef void *LocaleObject;
typedef unsigned int LocaleToken;
typedef void *AttrObject;
typedef void *XformObject;
typedef int ulsBool;

#ifndef TRUE
# define TRUE	1
#endif
#ifndef FALSE
# define FALSE	0
#endif
#ifndef APIENTRY
# ifdef _System
#  define APIENTRY _System
# else
#  define APIENTRY
# endif
#endif

#define UNI_TOKEN_POINTER	1
#define UNI_MBS_STRING_POINTER	2
#define UNI_UCS_STRING_POINTER	3

/* Locale types */
#define UNI_SYSTEM_LOCALES      1
#define UNI_USER_LOCALES        2

/* Max length of a locale name null included. */
#define ULS_LNAMEMAX            32    

/* Locale categories */
#undef LANG
#undef LC_ALL
#undef LC_COLLATE
#undef LC_CTYPE
#undef LC_NUMERIC
#undef LC_MONETARY
#undef LC_TIME
#undef LC_MESSAGES

#define LANG			(-2)
#define LC_ALL			(-1)
#define LC_COLLATE		0
#define LC_CTYPE		1
#define LC_NUMERIC		2
#define LC_MONETARY		3
#define LC_TIME			4
#define LC_MESSAGES		5
/* Number of locale categories. */
#define N_LC_CATEGORIES         6


/** @group Character Type Consts (itype)
 * @{ */
#define CT_UPPER            0x0001      /** Upper case. */
#define CT_LOWER            0x0002      /** Lower case. */
#define CT_DIGIT            0x0004      /** Digits (0-9). */
#define CT_SPACE            0x0008      /** White space and line ends. */
#define CT_PUNCT            0x0010      /** Punctuation marks. */
#define CT_CNTRL            0x0020      /** Control and format characters. */
#define CT_BLANK            0x0040      /** Space and tab. */
#define CT_XDIGIT           0x0080      /** Hex digits. */
#define CT_ALPHA            0x0100      /** Any linguistic character. */
#define CT_ALNUM            0x0200      /** Alphanumeric. */
#define CT_GRAPH            0x0400      /** All except controls and space. */
#define CT_PRINT            0x0800      /** Everything except controls. */
#define CT_NUMBER           0x1000      /** Integral number. */
#define CT_SYMBOL           0x2000      /** Symbol. */
#define CT_ASCII            0x8000      /** In standard ASCII set. */
/** @} */


/** @group CType 1 Flag Bits.
 * @{ */
#define C1_ALPHA            CT_ALPHA    /** Any linguistic character. */
#define C1_BLANK            CT_BLANK    /** Blank characters. */
#define C1_CNTRL            CT_CNTRL    /** Control characters. */
#define C1_DIGIT            CT_DIGIT    /** Decimal digits. */
#define C1_LOWER            CT_LOWER    /** Lower case. */
#define C1_PUNCT            CT_PUNCT    /** Punctuation characters. */
#define C1_SPACE            CT_SPACE    /** Spacing characters. */
#define C1_UPPER            CT_UPPER    /** Upper case. */
#define C1_XDIGIT           CT_XDIGIT   /** Hex digits. */
/** @} */


/** @group CType 2 Flag Bits. (bidi)
 * @{ */
#define C2_NOTAPPLICABLE    0x00        /** NA - Not a character. */
#define C2_LEFTTORIGHT      0x01        /** L  - Left to Right. */
#define C2_RIGHTTOLEFT      0x02        /** R  - Right to Left. */
#define C2_EUROPENUMBER     0x03        /** EN - European number. */
#define C2_EUROPESEPARATOR  0x04        /** ES - European separator. */
#define C2_EUROPETERMINATOR 0x05        /** ET - European terminator. */
#define C2_ARABICNUMBER     0x06        /** AN - Arabic number. */
#define C2_COMMONSEPARATOR  0x07        /** CS - Common separator. */
#define C2_BLOCKSEPARATOR   0x08        /** B  - Block separator. */
#define C2_WHITESPACE       0x0a        /** WS - Whitespace. */
#define C2_OTHERNEUTRAL     0x0b        /** ON - Other neutral. */
#define C2_MIRRORED         0x0c        /** M  - Symetrical (not Win32). */
/** @} */


/** @group CType 3 Flag Bits.
 * @{ */
#define C3_NONSPACING       0x0001      /** Nonspacing mark. */
#define C3_DIACRITIC        0x0002      /** Diacritic mark. */
#define C3_NSDIACRITIC      0x0003
#define C3_VOWELMARK        0x0004      /** Vowel mark. */
#define C3_NSVOWEL          0x0005
#define C3_SYMBOL           0x0008      /** Symbol (see CT_SYMBOL). */
#define C3_KATAKANA         0x0010      /** Katakana character (jap). */
#define C3_HIRAGANA         0x0020      /** Hiragana character (jap). */
#define C3_HALFWIDTH        0x0040      /** Half-width varient. */
#define C3_FULLWIDTH        0x0080      /** Full-width varient. */
#define C3_IDEOGRAPH        0x0100      /** Kanji/Han character (asian). */
#define C3_KASHIDA          0x0200      /** Arabic enlonger. */
#define C3_ALPHA            0x8000      /** Alphabetic. */
#define C3_MASK             0x83FF      /** Mask for Win32 bits. */
/** @} */


/** @group Character Set Values. (charset)
 * Linguistic groups and subtypes.
 * @{ */
#define CHS_NONCHAR         0x00000000
#define CHS_OTHER           0x00000001
#define CHS_LATIN           0x00000002
#define CHS_CYRILLIC        0x00000003
#define CHS_ARABIC          0x00000004
#define CHS_GREEK           0x00000005
#define CHS_HEBREW          0x00000006
#define CHS_THAI            0x00000007
#define CHS_KATAKANA        0x00000008
#define CHS_HIRAGANA        0x00000009
#define CHS_HANGUEL         0x0000000a
#define CHS_BOPOMOFO        0x0000000b
#define CHS_DEVANAGARI      0x0000000c
#define CHS_TELUGU          0x0000000d
#define CHS_BENGALI         0x0000000e
#define CHS_GUJARATI        0x0000000f
#define CHS_GURMUKHI        0x00000010
#define CHS_TAMIL           0x00000011
#define CHS_LAO             0x00000012
/* .. */
#define CHS_PUNCTSTART      0x00000020
#define CHS_PUNCTEND        0x00000021
#define CHS_DINGBAT         0x00000022
#define CHS_MATH            0x00000023
#define CHS_APL             0x00000024
#define CHS_ARROW           0x00000025
#define CHS_BOX             0x00000026
#define CHS_DASH            0x00000027
#define CHS_CURRENCY        0x00000028
#define CHS_FRACTION        0x00000029
#define CHS_LINESEP         0x0000002a
#define CHS_USERDEF         0x0000002b
/** @} */


/** @group UGL Codepage Consts. (codepage)
 * @{ */
#define CCP_437             0x01  /** US PC. */
#define CCP_850             0x02  /** Multilingual PC. */
#define CCP_SYMB            0x04  /** PostScript Symbol. */
#define CCP_1252            0x08  /** Windows Latin 1. */
#define CCP_1250            0x10  /** Windows Latin 2. */
#define CCP_1251            0x20  /** Windows Cyrillic. */
#define CCP_1254            0x40  /** Windows Turkish. */
#define CCP_1257            0x80  /** Windows Baltic. */
/** @} */


/** @group Character Type (kind/UniQueryStringType)
 * @{ */
#define CT_ITYPE            0x0001
#define CT_BIDI             0x0002
#define CT_CHARSET          0x0003
#define CT_EXTENDED         0x0004
#define CT_CODEPAGE         0x0005
#define CT_INDEX            0x0006
#define CT_CTYPE1           0x0007  /** C1_* - Win32 compat xpg4. */
#define CT_CTYPE2           0x0008  /** C2_* - Win32 compat bidi. */
#define CT_CTYPE3           0x0009  /** C3_* - Win32 compat extended. */
/** @} */


/** Locale conventions structure */
typedef struct UniLconv
{
  UniChar *decimal_point;	/* non-monetary decimal point */
  UniChar *thousands_sep;	/* non-monetary thousands separator */
  short   *grouping;		/* non-monetary size of grouping */
  UniChar *int_curr_symbol;	/* int'l currency symbol and separator */
  UniChar *currency_symbol;	/* local currency symbol */
  UniChar *mon_decimal_point;	/* monetary decimal point */
  UniChar *mon_thousands_sep;	/* monetary thousands separator */
  short   *mon_grouping;	/* monetary size of grouping */
  UniChar *positive_sign;	/* non-negative values sign */
  UniChar *negative_sign;	/* negative values sign */
  short   int_frac_digits;	/* no of fractional digits int currency */
  short   frac_digits;		/* no of fractional digits loc currency */
  short   p_cs_precedes;	/* nonneg curr sym 1-precedes,0-succeeds */
  short   p_sep_by_space;	/* nonneg curr sym 1-space,0-no space */
  short   n_cs_precedes;	/* neg curr sym 1-precedes,0-succeeds */
  short   n_sep_by_space;	/* neg curr sym 1-space 0-no space */
  short   p_sign_posn;		/* positioning of nonneg monetary sign */
  short   n_sign_posn;		/* positioning of negative monetary sign */
  short   os2_mondecpt;		/* os2 curr sym positioning */
  UniChar *debit_sign;		/* non-neg-valued monetary sym - "DB" */
  UniChar *credit_sign;		/* negative-valued monetary sym - "CR" */
  UniChar *left_parenthesis;	/* negative-valued monetary sym - "(" */
  UniChar *right_parenthesis;	/* negative-valued monetary sym - ")" */
} UNILCONV;


/** Char/String Type (QueryCharType and UniQueryStringType). */
typedef struct UniCType {
    unsigned short  itype;              /** XPG/4 type attributes (C1_*). */
    unsigned char   bidi;               /** BiDi type attributes (C2_*). */
    unsigned char   charset;            /** Character set (CHS_*). */
    unsigned short  extend;             /** Win32 Extended attributes (C3_*). */
    unsigned short  codepage;           /** Codepage bits. (CCP_*) */
} UNICTYPE;


__BEGIN_DECLS
/* Locale Management Function Prototypes */
int APIENTRY UniCreateLocaleObject (int locale_spec_type, const void *locale_spec,
  LocaleObject *locale_object_ptr);
int APIENTRY UniQueryLocaleObject (const LocaleObject locale_object, int category,
  int locale_spec_type, void **locale_spec_ptr);
int APIENTRY UniFreeLocaleObject (LocaleObject locale_object);
int APIENTRY UniFreeMem (void *memory_ptr);
int APIENTRY UniLocaleStrToToken (int locale_string_type, const void *locale_string,
  LocaleToken *locale_token_ptr);
int APIENTRY UniLocaleTokenToStr (const LocaleToken locale_token,
  int locale_string_type, void **locale_string_ptr);

/* Locale Information Function Prototypes */
int APIENTRY UniQueryLocaleInfo (const LocaleObject locale_object,
  struct UniLconv **unilconv_addr_ptr);
int APIENTRY UniFreeLocaleInfo (struct UniLconv *unilconv_addr);
int APIENTRY UniQueryLocaleItem (const LocaleObject locale_object, LocaleItem item,
  UniChar **info_item_addr_ptr);
int APIENTRY UniQueryLocaleValue (const LocaleObject locale_object, LocaleItem item, int *info_item);

/* Date and Time Function Prototypes */
size_t APIENTRY UniStrftime (const LocaleObject locale_object, UniChar *ucs,
  size_t maxsize, const UniChar *format, const struct tm *time_ptr);
UniChar *APIENTRY UniStrptime (const LocaleObject locale_object, const UniChar *buf,
  const UniChar *format, struct tm *time_ptr);

/* Monetary Formatting Function Prototype */
int APIENTRY UniStrfmon (const LocaleObject locale_object, UniChar *ucs, size_t maxsize,
  const UniChar *format, ...);

/* String/Character Function Prototypes */
UniChar *APIENTRY UniStrcat (UniChar *ucs1, const UniChar *ucs2);
UniChar *APIENTRY UniStrchr (const UniChar *ucs, UniChar uc);
int APIENTRY UniStrcmp (const UniChar *ucs1, const UniChar *ucs2);
UniChar *APIENTRY UniStrcpy (UniChar *ucs1, const UniChar *ucs2);
size_t APIENTRY UniStrcspn (const UniChar *ucs1, const UniChar *ucs2);
size_t APIENTRY UniStrlen (const UniChar *ucs1);
UniChar *APIENTRY UniStrncat (UniChar *ucs1, const UniChar *ucs2, size_t n);
int APIENTRY UniStrncmp (const UniChar *ucs1, const UniChar *ucs2, size_t n);
UniChar *APIENTRY UniStrncpy (UniChar *ucs1, const UniChar *ucs2, size_t n);
UniChar *APIENTRY UniStrpbrk (const UniChar *ucs1, const UniChar *ucs2);
UniChar *APIENTRY UniStrrchr (const UniChar *ucs, UniChar uc);
size_t APIENTRY UniStrspn (const UniChar *ucs1, const UniChar *ucs2);
UniChar *APIENTRY UniStrstr (const UniChar *ucs1, const UniChar *ucs2);
UniChar *APIENTRY UniStrtok (UniChar *ucs1, const UniChar *ucs2);

/* Character Attribute Function Prototypes */
int APIENTRY UniCreateAttrObject (const LocaleObject locale_object,
  const UniChar *attr_name, AttrObject *attr_object_ptr);
int APIENTRY UniQueryCharAttr (AttrObject attr_object, UniChar uc);
int APIENTRY UniScanForAttr (AttrObject attr_object, const UniChar *ucs,
  size_t num_elems, ulsBool inverse_op, size_t *offset_ptr);
int APIENTRY UniFreeAttrObject (AttrObject attr_object);
int APIENTRY UniQueryAlnum (const LocaleObject locale_object, UniChar uc);
int APIENTRY UniQueryAlpha (const LocaleObject locale_object, UniChar uc);
int APIENTRY UniQueryBlank (const LocaleObject locale_object, UniChar uc);
int APIENTRY UniQueryCntrl (const LocaleObject locale_object, UniChar uc);
int APIENTRY UniQueryDigit (const LocaleObject locale_object, UniChar uc);
int APIENTRY UniQueryGraph (const LocaleObject locale_object, UniChar uc);
int APIENTRY UniQueryLower (const LocaleObject locale_object, UniChar uc);
int APIENTRY UniQueryPrint (const LocaleObject locale_object, UniChar uc);
int APIENTRY UniQueryPunct (const LocaleObject locale_object, UniChar uc);
int APIENTRY UniQuerySpace (const LocaleObject locale_object, UniChar uc);
int APIENTRY UniQueryUpper (const LocaleObject locale_object, UniChar uc);
int APIENTRY UniQueryXdigit (const LocaleObject locale_object, UniChar uc);

/* String Transformation Function Prototypes */
int APIENTRY UniCreateTransformObject (const LocaleObject locale_object,
  const UniChar *xtype, XformObject *xform_object_ptr);
int APIENTRY UniTransformStr (XformObject xform_object, const UniChar *inp_buf,
  int *inp_size, UniChar *out_buf, int *out_size);
int APIENTRY UniFreeTransformObject (XformObject xform_object);
UniChar APIENTRY UniTransLower (const LocaleObject locale_object, UniChar uc);
UniChar APIENTRY UniTransUpper (const LocaleObject locale_object, UniChar uc);

/* String Conversion Function Prototypes */
int APIENTRY UniStrtod (const LocaleObject locale_object, const UniChar *ucs,
  UniChar **end_ptr, double *result_ptr);
int APIENTRY UniStrtol (const LocaleObject locale_object, const UniChar *ucs,
  UniChar **end_ptr, int base, long int *result_ptr);
int APIENTRY UniStrtoul (const LocaleObject locale_object, const UniChar *ucs,
  UniChar **end_ptr, int base, unsigned long int *result_ptr);

/* String Comparison Function Prototypes */
int APIENTRY UniStrcoll (const LocaleObject locale_object,
  const UniChar *ucs1, const UniChar *ucs2);
size_t APIENTRY UniStrxfrm (const LocaleObject locale_object, UniChar *ucs1,
  const UniChar *ucs2, size_t n);
int APIENTRY UniStrcmpi (const LocaleObject locale_object,
  const UniChar *ucs1, const UniChar *ucs2);
int APIENTRY UniStrncmpi (const LocaleObject locale_object,
  const UniChar *ucs1, const UniChar *ucs2, const size_t n);

/* Unicode Case Mapping Function Prototypes */
UniChar APIENTRY UniToupper (UniChar uc);
UniChar APIENTRY UniTolower (UniChar uc);
UniChar *APIENTRY UniStrupr (UniChar *ucs);
UniChar *APIENTRY UniStrlwr (UniChar *ucs);

int APIENTRY UniMapCtryToLocale (unsigned long Country, UniChar *LocaleName, size_t n);

/* Locale independent character classification. */
int             APIENTRY UniQueryChar (UniChar uc, unsigned long attr);
unsigned long   APIENTRY UniQueryAttr (UniChar * name);
unsigned long   APIENTRY UniQueryStringType (UniChar * ustr, int size, unsigned short *outstr, int kind);
struct UniCType*APIENTRY UniQueryCharType (UniChar uchr);
unsigned long   APIENTRY UniQueryCharTypeTable (unsigned long * count, struct UniCType **unictype);
int             APIENTRY UniQueryNumericValue (UniChar uc);

/* Functions for user locals designed to be used by WPShell (local object). */
int APIENTRY UniSetUserLocaleItem (UniChar * locale, int item, int type, void * value);
int APIENTRY UniMakeUserLocale (UniChar * name, UniChar * basename);
int APIENTRY UniDeleteUserLocale (UniChar * locale);
int APIENTRY UniCompleteUserLocale (void);
int APIENTRY UniQueryLocaleList (int, UniChar *, int);
int APIENTRY UniQueryLanguageName (UniChar *lang, UniChar *isolang, UniChar **infoitem);
int APIENTRY UniQueryCountryName (UniChar *country, UniChar *isolang, UniChar **infoitem);

__END_DECLS

#endif /* __UNIDEF_H__ */
