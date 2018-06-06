/* ulsitem.h,v 1.3 2004/09/14 22:27:36 bird Exp */
/** @file
 * IGCC.
 */
/*
 * Legalesy-free Unicode API interface for OS/2
 * Defines for use with UniQueryLocaleItem
 *
 * Written by Andrew Zabolotny <bit@eltech.ru>
 *
 * This file is put into public domain. You are free to do
 * literally anything you wish with it: modify, print, sell,
 * rent, eat, throw out of window: in all (esp. in later)
 * cases I am not responsible for any damage it causes.
 */

#ifndef __ULSITEM_H__
#define __ULSITEM_H__

typedef int LocaleItem;

#define D_T_FMT		1
#define D_FMT		2
#define T_FMT		3
#define AM_STR		4
#define PM_STR		5

#define ABDAY_1		6
#define ABDAY_2		7
#define ABDAY_3		8
#define ABDAY_4		9
#define ABDAY_5		10
#define ABDAY_6		11
#define ABDAY_7		12

#define DAY_1		13
#define DAY_2		14
#define DAY_3		15
#define DAY_4		16
#define DAY_5		17
#define DAY_6		18
#define DAY_7		19

#define ABMON_1		20
#define ABMON_2		21
#define ABMON_3		22
#define ABMON_4		23
#define ABMON_5		24
#define ABMON_6		25
#define ABMON_7		26
#define ABMON_8		27
#define ABMON_9		28
#define ABMON_10	29
#define ABMON_11	30
#define ABMON_12	31

#define MON_1		32
#define MON_2		33
#define MON_3		34
#define MON_4		35
#define MON_5		36
#define MON_6		37
#define MON_7		38
#define MON_8		39
#define MON_9		40
#define MON_10		41
#define MON_11		42
#define MON_12		43

#define RADIXCHAR	44
#define THOUSEP		45
#define YESSTR		46
#define NOSTR		47
#define CRNCYSTR	48
#define CODESET		49

/* Additional constants defined in XPG4 */

#define T_FMT_AMPM	55
#define ERA		56
#define ERA_D_FMT	57
#define ERA_D_T_FMT	58
#define ERA_T_FMT	59
#define ALT_DIGITS	60
#define YESEXPR		61
#define NOEXPR		62

/* LSA feature */
#define DATESEP		63
#define TIMESEP		64
#define LISTSEP		65


/* OS/2 phun */

#define LOCI_sDateTime                	D_T_FMT
#define LOCI_sShortDate                 D_FMT
#define LOCI_sTimeFormat                T_FMT
#define LOCI_s1159                      AM_STR
#define LOCI_s2359                      PM_STR
#define LOCI_sAbbrevDayName7            ABDAY_1
#define LOCI_sAbbrevDayName1            ABDAY_2
#define LOCI_sAbbrevDayName2            ABDAY_3
#define LOCI_sAbbrevDayName3            ABDAY_4
#define LOCI_sAbbrevDayName4            ABDAY_5
#define LOCI_sAbbrevDayName5            ABDAY_6
#define LOCI_sAbbrevDayName6            ABDAY_7
#define LOCI_sDayName7                  DAY_1
#define LOCI_sDayName1                  DAY_2
#define LOCI_sDayName2                  DAY_3
#define LOCI_sDayName3                  DAY_4
#define LOCI_sDayName4                  DAY_5
#define LOCI_sDayName5                  DAY_6
#define LOCI_sDayName6                  DAY_7
#define LOCI_sAbbrevMonthName1          ABMON_1
#define LOCI_sAbbrevMonthName2          ABMON_2
#define LOCI_sAbbrevMonthName3          ABMON_3
#define LOCI_sAbbrevMonthName4          ABMON_4
#define LOCI_sAbbrevMonthName5          ABMON_5
#define LOCI_sAbbrevMonthName6          ABMON_6
#define LOCI_sAbbrevMonthName7          ABMON_7
#define LOCI_sAbbrevMonthName8          ABMON_8
#define LOCI_sAbbrevMonthName9          ABMON_9
#define LOCI_sAbbrevMonthName10         ABMON_10
#define LOCI_sAbbrevMonthName11         ABMON_11
#define LOCI_sAbbrevMonthName12         ABMON_12
#define LOCI_sMonthName1                MON_1
#define LOCI_sMonthName2                MON_2
#define LOCI_sMonthName3                MON_3
#define LOCI_sMonthName4                MON_4
#define LOCI_sMonthName5                MON_5
#define LOCI_sMonthName6                MON_6
#define LOCI_sMonthName7                MON_7
#define LOCI_sMonthName8                MON_8
#define LOCI_sMonthName9                MON_9
#define LOCI_sMonthName10               MON_10
#define LOCI_sMonthName11               MON_11
#define LOCI_sMonthName12               MON_12
#define LOCI_sDecimal                   RADIXCHAR
#define LOCI_sThousand                  THOUSEP
#define LOCI_sYesString                 YESSTR
#define LOCI_sNoString                  NOSTR
#define LOCI_sCurrency                  CRNCYSTR
#define LOCI_sCodeSet                   CODESET
#define LOCI_xLocaleToken               50
#define LOCI_xWinLocale                 51
#define LOCI_iLocaleResnum              52
#define LOCI_sNativeDigits              53
#define LOCI_iMaxItem                   54
#define LOCI_sTimeMark                  T_FMT_AMPM
#define LOCI_sEra                       ERA
#define LOCI_sAltShortDate              ERA_D_FMT
#define LOCI_sAltDateTime               ERA_D_T_FMT
#define LOCI_sAltTimeFormat             ERA_T_FMT
#define LOCI_sAltDigits                 ALT_DIGITS
#define LOCI_sYesExpr                   YESEXPR
#define LOCI_sNoExpr                    NOEXPR
#define LOCI_sDate                      DATESEP
#define LOCI_sTime                      TIMESEP
#define LOCI_sList                      LISTSEP
#define LOCI_sMonDecimalSep             66
#define LOCI_sMonThousandSep            67
#define LOCI_sGrouping                  68
#define LOCI_sMonGrouping               69
#define LOCI_iMeasure                   70
#define LOCI_iPaper                     71
#define LOCI_iDigits                    72
#define LOCI_iTime                      73
#define LOCI_iDate                      74
#define LOCI_iCurrency                  75
#define LOCI_iCurrDigits                76
#define LOCI_iLzero                     77
#define LOCI_iNegNumber                 78
#define LOCI_iLDate                     79
#define LOCI_iCalendarType              80
#define LOCI_iFirstDayOfWeek            81
#define LOCI_iFirstWeekOfYear           82
#define LOCI_iNegCurr                   83
#define LOCI_iTLzero                    84
#define LOCI_iTimePrefix                85
#define LOCI_iOptionalCalendar          86
#define LOCI_sIntlSymbol                87
#define LOCI_sAbbrevLangName            88
#define LOCI_sCollate                   89
#define LOCI_iUpperType                 90
#define LOCI_iUpperMissing              91
#define LOCI_sPositiveSign              92
#define LOCI_sNegativeSign              93
#define LOCI_sLeftNegative              94
#define LOCI_sRightNegative             95
#define LOCI_sLongDate                  96
#define LOCI_sAltLongDate               97
#define LOCI_sMonthName13               98
#define LOCI_sAbbrevMonthName13         99
#define LOCI_sName                      100
#define LOCI_sLanguageID                101
#define LOCI_sCountryID                 102
#define LOCI_sEngLanguage               103
#define LOCI_sLanguage                  104
#define LOCI_sEngCountry                105
#define LOCI_sCountry                   106
#define LOCI_sNativeCtryName            107
#define LOCI_iCountry                   108
#define LOCI_sISOCodepage               109
#define LOCI_iAnsiCodepage              110
#define LOCI_iCodepage                  111
#define LOCI_iAltCodepage               112
#define LOCI_iMacCodepage               113
#define LOCI_iEbcdicCodepage            114
#define LOCI_sOtherCodepages            115
#define LOCI_sSetCodepage               116
#define LOCI_sKeyboard                  117
#define LOCI_sAltKeyboard               118
#define LOCI_sSetKeyboard               119
#define LOCI_sDebit                     120
#define LOCI_sCredit                    121
#define LOCI_sLatin1Locale              122
#define LOCI_wTimeFormat                123
#define LOCI_wShortDate                 124
#define LOCI_wLongDate                  125
#define LOCI_jISO3CountryName           126
#define LOCI_jPercentPattern            127
#define LOCI_jPercentSign               128
#define LOCI_jExponent                  129
#define LOCI_jFullTimeFormat            130
#define LOCI_jLongTimeFormat            131
#define LOCI_jShortTimeFormat           132
#define LOCI_jFullDateFormat            133
#define LOCI_jMediumDateFormat          134
#define LOCI_jDateTimePattern           135
#define LOCI_jEraStrings                136
#define LOCI_MAXITEM                    136
#define LOCI_NOUSEROVERRIDE             0x00008000

#endif /* __ULSITEM_H__ */

