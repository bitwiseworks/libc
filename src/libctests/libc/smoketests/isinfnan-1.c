/* $Id: isinfnan-1.c 2091 2005-06-27 03:40:37Z bird $ */
/** @file
 *
 * LIBC - isnan/isinf linkage test.
 *
 * Copyright (c) 2005 knut st. osmundsen <bird@anduin.net>
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

#include <math.h>

double rd = 1.0;
float r = 1.0;
long double lrd = 1.0;

int main()
{
    return 0
        | isnan(r)
        | isnan(rd)
        | isnan(lrd)
        | isinf(r)
        | isinf(rd)
        | isinf(lrd)
        | !isfinite(r)
        | !isfinite(rd)
        | !isfinite(lrd)
        | !isnormal(r)
        | !isnormal(rd)
        | !isnormal(lrd);
}
