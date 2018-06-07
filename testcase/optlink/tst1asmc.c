/* $Id: tst1asmc.c 678 2003-09-09 19:21:51Z bird $
 *
 * Optlink testcase no. 1.
 * C source of the ASM file.
 *
 * InnoTek Systemberatung GmbH confidential
 *
 * Copyright (c) 2003 InnoTek Systemberatung GmbH
 * Author: knut st. osmundsen <bird-srcspam@anduin.net>
 *
 * All Rights Reserved
 *
 */

extern int _Optlink asmfoo (int i1, int i2, int i3, double rf1, double rf2, double rf3, double rf4)
{
    if (i1 != 1)
        return 1;
    if (i2 != 2)
        return 2;
    if (i3 != 3)
        return 3;
    if (rf1 != 1.1)
        return 4;
    if (rf2 != 1.2)
        return 5;
    if (rf3 != 1.3)
        return 6;
    if (rf4 != 1.4)
        return 7;
    return 0;
}

