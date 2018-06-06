/* $Id: locale_time.c 2161 2005-07-03 00:31:40Z bird $ */
/** @file
 *
 * Locale - time data.
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

/** Date / time formatting rules. */
__LIBC_LOCALETIME        __libc_gLocaleTime =
{
  .smonths = { "Jan",    "Feb",     "Mar",  "Apr",  "May","Jun", "Jul", "Aug",   "Sep",      "Oct",    "Nov",     "Dec"},
  .lmonths = { "January","February","March","April","May","June","July","August","September","October","November","December" },
  .swdays =  { "Sun",   "Mon",   "Tue",    "Wed",      "Thu",     "Fri",   "Sat" },
  .lwdays =  { "Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday" },
  .date_time_fmt = "%a %b %e %H:%M:%S %Y",
  .date_fmt = "%m/%d/%y",
  .time_fmt = "%H:%M:%S",
  .am =       "AM",
  .pm =       "PM",
  .ampm_fmt = "%I:%M:%S %p",
  .era =      "",
  .era_date_fmt = "",
  .era_date_time_fmt = "",
  .era_time_fmt = "",
  .alt_digits = "",
  .datesep =  "/", //?
  .timesep =  ":", //?
  .listsep =  "",  //??????
  .fConsts = 1
};


/** Date / time formatting rules for the C/POSIX locale. */
const __LIBC_LOCALETIME __libc_gLocaleTimeDefault =
{
  .smonths = { "Jan",    "Feb",     "Mar",  "Apr",  "May","Jun", "Jul", "Aug",   "Sep",      "Oct",    "Nov",     "Dec"},
  .lmonths = { "January","February","March","April","May","June","July","August","September","October","November","December" },
  .swdays =  { "Sun",   "Mon",   "Tue",    "Wed",      "Thu",     "Fri",   "Sat" },
  .lwdays =  { "Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday" },
  .date_time_fmt = "%a %b %e %H:%M:%S %Y",
  .date_fmt = "%m/%d/%y",
  .time_fmt = "%H:%M:%S",
  .am =       "AM",
  .pm =       "PM",
  .ampm_fmt = "%I:%M:%S %p",
  .era =      "",
  .era_date_fmt = "",
  .era_date_time_fmt = "",
  .era_time_fmt = "",
  .alt_digits = "",
  .datesep =  "/", //?
  .timesep =  ":", //?
  .listsep =  "",  //??????
  .fConsts = 1
};

