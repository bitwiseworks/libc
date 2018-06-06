/* $Id: 567main.cpp 514 2003-08-02 23:11:16Z bird $
 *
 * Defect 567: ctype and negative offsets.
 *
 * InnoTek Systemberatung GmbH confidential
 *
 * Copyright (c) 2003 InnoTek Systemberatung GmbH
 * Author: knut st. osmundsen <bird-srcspam@anduin.net>
 *
 * All Rights Reserved
 *
 */


#include <ctype.h>

int main()
{
    int     i;
    int     rc = 0;
    char    ch[] = {'a', 'b',  'c', 'z',
                     12, -12, -127, 127,
                    0};

    for (i = 0; ch[i]; i++)
    {
        if (isalpha(ch[i]))
            rc++;
        else
            rc--;
    }
    return rc;
}
