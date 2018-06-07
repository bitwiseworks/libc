/* $Id: strspn-1.c 1530 2004-09-28 02:56:29Z bird $ */
/** @file
 *
 * strspn() test for /bin/stat problem.
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

#include <stdio.h>
#include <string.h>



int main(int argc, char **argv)
{
    char dest[256];
    char *b = strdup(
      "  File: %N\n"
      "  Size: %-10s\tBlocks: %-10b IO Block: %-6o %F\n"
      "Device: %Dh/%dd\tInode: %-10i  Links: %h\n"
      "Access: (%04a/%10.10A)  Uid: (%5u/%8U)   Gid: (%5g/%8G)\n"
      "Access: %x\n" "Modify: %y\n" "Change: %z");
    while (b)
      {
        char *p = strchr (b, '%');
        if (p != NULL)
          {
            size_t len;
            *p++ = '\0';
            fputs (b, stdout);

            len = strspn (p, "#-+.I 0123456789");
            dest[0] = '%';
            memcpy (dest + 1, p, len);
            dest[1 + len] = 0;
            p += len;

            b = p + 1;
            switch (*p)
              {
              case '\0':
                b = NULL;
                /* fall through */
              case '%':
                putchar ('%');
                break;
              default:
                //print_func (dest, *p, filename, data);
                break;
              }
          }
        else
          {
            fputs (b, stdout);
            b = NULL;
          }
      }

    return 0;
}
