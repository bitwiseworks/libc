/* $Id: namelessunions.c 1456 2004-09-05 10:17:06Z bird $ */
/** @file
 *
 * Testcase for nameless unions.
 *
 * Copyright (c) 2004 knut st. osmundsen <bird-srcspam@anduin.net>
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

struct bar
{
    short   s1;
    char    ch1;
    short   s2;
    char    ch2;
    short   s3;
    char    ch3;
};

struct foo
{
    int         i1;
    union
    {
        char        ach[20];
        struct  bar s;
    };
    void       *pv;
};


struct foo  gFoo;
struct foo *gpFoo = &gFoo;


int main()
{
    return gpFoo->i1;
}
