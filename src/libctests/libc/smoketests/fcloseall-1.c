/* $Id: $ */
/** @file
 *
 *
 *
 * Copyright (c) 2006 knut st. osmundsen <bird@anduin.net>
 *
 *
 * This file is part of kLIBC.
 *
 * kLIBC is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * kLIBC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with kLIBC; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <sys/fmutex.h>
#include <stdio.h>
#include <errno.h>


int main(int argc, char **argv)
{
    int     rc;
    int     cErrors = 0;
    FILE   *pFile;

#ifdef __INNOTEK_LIBC__
    /*
     * Check that the fmutex padding is ok.
     */
    if (sizeof(pFile->__u.__fsem) != sizeof(pFile->__u.__rsem_ersatz))
    {
        printf("fcloseall-1: FAILURE - sizeof(pFile->__u.__fsem) [%u] != sizeof(pFile->__u.__rsem_ersatz) [%u]\n",
               sizeof(pFile->__u.__fsem), sizeof(pFile->__u.__rsem_ersatz));
        return 1;
    }
#endif

    /*
     * We can close all handle now, but nothing should really be done
     * since stdout, stdin, and stderr are not affected by this api.
     */
    errno = 0;
    rc = _fcloseall();
    if (rc != 0)
    {
        printf("fcloseall-1: FAILURE - _fcloseall failed rc=%d (expected 0) errno=%d\n", rc, errno);
        cErrors++;
    }
    errno = 0;
    rc = printf("fcloseall-1: first test succeeded\n");
    if (rc < 10)
    {
        printf("fcloseall-1: FAILURE - failed to write to stdout after _fcloseall\n", rc, errno);
        cErrors++;
    }

    /*
     * Now, let's open a file and then do _fcloseall;'
     */
    pFile = fopen(argv[0], "r");
    if (!pFile)
    {
        fprintf(stderr, "fcloseall-1: FAILURE - couldn't open the executable!!!\n");
        return 1;
    }
    errno = 0;
    rc = _fcloseall();
    if (rc != 1)
    {
        printf("fcloseall-1: FAILURE - _fcloseall failed rc=%d (expected 1) errno=%d\n", rc, errno);
        cErrors++;
    }

    /*
     * Summary.
     */
    if (!cErrors)
        printf("fcloseall-1: SUCCESS\n");
    else
        printf("fcloseall-1: FAILURE - %d errors\n", cErrors);
    return !!cErrors;
}

