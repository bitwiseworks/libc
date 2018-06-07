/* $Id: cppll.cpp 814 2003-10-06 16:08:53Z bird $
 *
 * Check if cout handles long longs.
 *
 * InnoTek Systemberatung GmbH confidential
 *
 * Copyright (c) 2003 InnoTek Systemberatung GmbH
 * Author: knut st. osmundsen <bird-srcspam@anduin.net>
 *
 * All Rights Reserved
 *
 */



#include <iostream.h>

int main()
{
    long long ll = 111111111111111;
    cout << "ll=" <<ll << "\n";
    return 0;
}
