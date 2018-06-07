/* $Id: locale_lconv.c 3936 2014-10-26 15:30:04Z bird $ */
/** @file
 *
 * Locale - lconv data.
 *
 * Copyright (c) 2003 InnoTek Systemberatung GmbH
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

#include <InnoTekLIBC/locale.h>
#include <limits.h>

/* Locale information structure. */
__LIBC_LOCALELCONV       __libc_gLocaleLconv =
{
    .s =
    {
        ".",                            //*decimal_point;         /** non-monetary decimal point */
        "",                             //*thousands_sep;         /** non-monetary thousands separator */
        "",                             //*grouping;              /** non-monetary size of grouping */
        "",                             //*int_curr_symbol;       /** international currency symbol and separator */
        "",                             //*currency_symbol;       /** local currency symbol */
        "",                             //*mon_decimal_point;     /** monetary decimal point */
        "",                             //*mon_thousands_sep;     /** monetary thousands separator */
        "",                             //*mon_grouping;          /** monetary size of grouping */
        "",                             //*positive_sign;         /** non-negative values sign */
        "" ,                            //*negative_sign;         /** negative values sign */
        CHAR_MAX,                       //int_frac_digits;        /** number of fractional digits - int currency */
        CHAR_MAX,                       //frac_digits;            /** number of fractional digits - local currency */
        CHAR_MAX,                       //p_cs_precedes;          /** (non-neg curr sym) 1-precedes, 0-succeeds */
        CHAR_MAX,                       //p_sep_by_space;         /** (non-neg curr sym) 1-space, 0-no space */
        CHAR_MAX,                       //n_cs_precedes;          /** (neg curr sym) 1-precedes, 0-succeeds */
        CHAR_MAX,                       //n_sep_by_space;         /** (neg curr sym) 1-space, 0-no space */
        CHAR_MAX,                       //p_sign_posn;            /** positioning of non-negative monetary sign */
        CHAR_MAX,                       //n_sign_posn;            /** positioning of negative monetary sign */
        CHAR_MAX,                       //int_p_cs_precedes;
        CHAR_MAX,                       //int_p_sep_by_space;
        CHAR_MAX,                       //int_n_cs_precedes;
        CHAR_MAX,                       //int_n_sep_by_space;
        CHAR_MAX,                       //int_p_sign_posn;
        CHAR_MAX                        //int_n_sign_posn;
    },

    .pszCrncyStr = "",
    .fNumericConsts = 1,
    .fMonetaryConsts = 1
};


/* Locale information structure for the 'C'/'POSIX' locale. */
const __LIBC_LOCALELCONV       __libc_gLocaleLconvDefault =
{
    .s =
    {
        ".",                            //*decimal_point;         /** non-monetary decimal point */
        "",                             //*thousands_sep;         /** non-monetary thousands separator */
        "",                             //*grouping;              /** non-monetary size of grouping */
        "",                             //*int_curr_symbol;       /** international currency symbol and separator */
        "",                             //*currency_symbol;       /** local currency symbol */
        "",                             //*mon_decimal_point;     /** monetary decimal point */
        "",                             //*mon_thousands_sep;     /** monetary thousands separator */
        "",                             //*mon_grouping;          /** monetary size of grouping */
        "",                             //*positive_sign;         /** non-negative values sign */
        "" ,                            //*negative_sign;         /** negative values sign */
        CHAR_MAX,                       //int_frac_digits;        /** number of fractional digits - int currency */
        CHAR_MAX,                       //frac_digits;            /** number of fractional digits - local currency */
        CHAR_MAX,                       //p_cs_precedes;          /** (non-neg curr sym) 1-precedes, 0-succeeds */
        CHAR_MAX,                       //p_sep_by_space;         /** (non-neg curr sym) 1-space, 0-no space */
        CHAR_MAX,                       //n_cs_precedes;          /** (neg curr sym) 1-precedes, 0-succeeds */
        CHAR_MAX,                       //n_sep_by_space;         /** (neg curr sym) 1-space, 0-no space */
        CHAR_MAX,                       //p_sign_posn;            /** positioning of non-negative monetary sign */
        CHAR_MAX,                       //n_sign_posn;            /** positioning of negative monetary sign */
        CHAR_MAX,                       //int_p_cs_precedes;
        CHAR_MAX,                       //int_p_sep_by_space;
        CHAR_MAX,                       //int_n_cs_precedes;
        CHAR_MAX,                       //int_n_sep_by_space;
        CHAR_MAX,                       //int_p_sign_posn;
        CHAR_MAX                        //int_n_sign_posn;
    },

    .pszCrncyStr = "",
    .fNumericConsts = 1,
    .fMonetaryConsts = 1
};

