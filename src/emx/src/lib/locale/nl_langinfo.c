/* $Id: nl_langinfo.c 2254 2005-07-17 12:25:44Z bird $ */
/** @file
 *
 * nl_langinfo.
 *
 * Copyright (c) 2004-2005 knut st. osmundsen <bird@anduin.net>
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
#include <InnoTekLIBC/locale.h>
#include <langinfo.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_LOCALE
#include <InnoTekLIBC/logstrict.h>


char *_STD(nl_langinfo)(nl_item Item)
{
    LIBCLOG_ENTER("Item=%d\n", Item);

    switch (Item)
    {
#define CASE_RETURN_STR(Item, String) \
    case Item: \
        LIBCLOG_RETURN_MSG(String, "ret %p:{%s} - %s\n", String, String, #Item)

        CASE_RETURN_STR(D_T_FMT, __libc_gLocaleTime.date_time_fmt);
        CASE_RETURN_STR(D_FMT, __libc_gLocaleTime.date_fmt);
        CASE_RETURN_STR(T_FMT, __libc_gLocaleTime.time_fmt);
        CASE_RETURN_STR(AM_STR, __libc_gLocaleTime.am);
        CASE_RETURN_STR(PM_STR, __libc_gLocaleTime.pm);

        CASE_RETURN_STR(ABDAY_1, __libc_gLocaleTime.swdays[ABDAY_1 - ABDAY_1]);
        CASE_RETURN_STR(ABDAY_2, __libc_gLocaleTime.swdays[ABDAY_2 - ABDAY_1]);
        CASE_RETURN_STR(ABDAY_3, __libc_gLocaleTime.swdays[ABDAY_3 - ABDAY_1]);
        CASE_RETURN_STR(ABDAY_4, __libc_gLocaleTime.swdays[ABDAY_4 - ABDAY_1]);
        CASE_RETURN_STR(ABDAY_5, __libc_gLocaleTime.swdays[ABDAY_5 - ABDAY_1]);
        CASE_RETURN_STR(ABDAY_6, __libc_gLocaleTime.swdays[ABDAY_6 - ABDAY_1]);
        CASE_RETURN_STR(ABDAY_7, __libc_gLocaleTime.swdays[ABDAY_7 - ABDAY_1]);

        CASE_RETURN_STR(DAY_1, __libc_gLocaleTime.lwdays[DAY_1 - DAY_1]);
        CASE_RETURN_STR(DAY_2, __libc_gLocaleTime.lwdays[DAY_2 - DAY_1]);
        CASE_RETURN_STR(DAY_3, __libc_gLocaleTime.lwdays[DAY_3 - DAY_1]);
        CASE_RETURN_STR(DAY_4, __libc_gLocaleTime.lwdays[DAY_4 - DAY_1]);
        CASE_RETURN_STR(DAY_5, __libc_gLocaleTime.lwdays[DAY_5 - DAY_1]);
        CASE_RETURN_STR(DAY_6, __libc_gLocaleTime.lwdays[DAY_6 - DAY_1]);
        CASE_RETURN_STR(DAY_7, __libc_gLocaleTime.lwdays[DAY_7 - DAY_1]);

        CASE_RETURN_STR(ABMON_1, __libc_gLocaleTime.smonths[ABMON_1 - ABMON_1]);
        CASE_RETURN_STR(ABMON_2, __libc_gLocaleTime.smonths[ABMON_2 - ABMON_1]);
        CASE_RETURN_STR(ABMON_3, __libc_gLocaleTime.smonths[ABMON_3 - ABMON_1]);
        CASE_RETURN_STR(ABMON_4, __libc_gLocaleTime.smonths[ABMON_4 - ABMON_1]);
        CASE_RETURN_STR(ABMON_5, __libc_gLocaleTime.smonths[ABMON_5 - ABMON_1]);
        CASE_RETURN_STR(ABMON_6, __libc_gLocaleTime.smonths[ABMON_6 - ABMON_1]);
        CASE_RETURN_STR(ABMON_7, __libc_gLocaleTime.smonths[ABMON_7 - ABMON_1]);
        CASE_RETURN_STR(ABMON_8, __libc_gLocaleTime.smonths[ABMON_8 - ABMON_1]);
        CASE_RETURN_STR(ABMON_9, __libc_gLocaleTime.smonths[ABMON_9 - ABMON_1]);
        CASE_RETURN_STR(ABMON_10, __libc_gLocaleTime.smonths[ABMON_10 - ABMON_1]);
        CASE_RETURN_STR(ABMON_11, __libc_gLocaleTime.smonths[ABMON_11 - ABMON_1]);
        CASE_RETURN_STR(ABMON_12, __libc_gLocaleTime.smonths[ABMON_12 - ABMON_1]);

        CASE_RETURN_STR(MON_1, __libc_gLocaleTime.lmonths[MON_1 - MON_1]);
        CASE_RETURN_STR(MON_2, __libc_gLocaleTime.lmonths[MON_2 - MON_1]);
        CASE_RETURN_STR(MON_3, __libc_gLocaleTime.lmonths[MON_3 - MON_1]);
        CASE_RETURN_STR(MON_4, __libc_gLocaleTime.lmonths[MON_4 - MON_1]);
        CASE_RETURN_STR(MON_5, __libc_gLocaleTime.lmonths[MON_5 - MON_1]);
        CASE_RETURN_STR(MON_6, __libc_gLocaleTime.lmonths[MON_6 - MON_1]);
        CASE_RETURN_STR(MON_7, __libc_gLocaleTime.lmonths[MON_7 - MON_1]);
        CASE_RETURN_STR(MON_8, __libc_gLocaleTime.lmonths[MON_8 - MON_1]);
        CASE_RETURN_STR(MON_9, __libc_gLocaleTime.lmonths[MON_9 - MON_1]);
        CASE_RETURN_STR(MON_10, __libc_gLocaleTime.lmonths[MON_10 - MON_1]);
        CASE_RETURN_STR(MON_11, __libc_gLocaleTime.lmonths[MON_11 - MON_1]);
        CASE_RETURN_STR(MON_12, __libc_gLocaleTime.lmonths[MON_12 - MON_1]);


        CASE_RETURN_STR(RADIXCHAR, __libc_gLocaleLconv.s.decimal_point);
        CASE_RETURN_STR(THOUSEP, __libc_gLocaleLconv.s.thousands_sep);

        CASE_RETURN_STR(YESSTR, __libc_gLocaleMsg.pszYesStr);
        CASE_RETURN_STR(NOSTR, __libc_gLocaleMsg.pszNoStr);

        CASE_RETURN_STR(CRNCYSTR, __libc_gLocaleLconv.pszCrncyStr);

        CASE_RETURN_STR(CODESET, __libc_GLocaleCtype.szCodeSet);

        CASE_RETURN_STR(T_FMT_AMPM, __libc_gLocaleTime.ampm_fmt);

        CASE_RETURN_STR(ERA, __libc_gLocaleTime.era);

        CASE_RETURN_STR(ERA_D_FMT, __libc_gLocaleTime.era_date_fmt);

        CASE_RETURN_STR(ERA_D_T_FMT, __libc_gLocaleTime.era_date_time_fmt);

        CASE_RETURN_STR(ERA_T_FMT, __libc_gLocaleTime.era_time_fmt);

        CASE_RETURN_STR(ALT_DIGITS, __libc_gLocaleTime.alt_digits);

        CASE_RETURN_STR(YESEXPR, __libc_gLocaleMsg.pszYesExpr);
        CASE_RETURN_STR(NOEXPR, __libc_gLocaleMsg.pszNoExpr);

        CASE_RETURN_STR(DATESEP, __libc_gLocaleTime.datesep);

        CASE_RETURN_STR(TIMESEP, __libc_gLocaleTime.timesep);

        CASE_RETURN_STR(LISTSEP, __libc_gLocaleTime.listsep);

#undef CASE_RETURN_STR
    }

    LIBCLOG_ERROR_RETURN_MSG("", "ret \"\" - Unknown item %d\n", Item);
}

