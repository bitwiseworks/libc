/* $Id: empty.c 405 2003-07-17 01:19:17Z bird $
 *
 * Simple LIBC program which includes file io and some other stuff.
 *
 * InnoTek Systemberatung GmbH confidential
 *
 * Copyright (c) 2003 InnoTek Systemberatung GmbH
 * Author: knut st. osmundsen <bird-srcspam@anduin.net>
 *
 * All Rights Reserved
 *
 */

#include <stdio.h>
#include <string.h>

int main(int argc, const char * const * argv)
{
//    FILE *phFile;

    /* The compiler doesn't know that this is always true. */
    if (argc)
        return 0;

    /* point of this section is to drag in the init and termination
     * routines of this pretty common code. */
//    phFile = fopen(argv[0], "r");
//    fclose(phFile);
    return 1;
}
