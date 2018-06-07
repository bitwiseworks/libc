/* $Id: boolparam.cpp 824 2003-10-08 19:30:03Z bird $ */
/** @file
 *
 * GCC does weird stuff in respect to the C++ bool type when passed as
 * a parameter. In this testcsae we should see the values changing in
 * variable monitor. The bool variables should be represented as if
 * enums with values true or false.
 *
 * InnoTek Systemberatung GmbH confidential
 *
 * Copyright (c) 2003 InnoTek Systemberatung GmbH
 * Author: knut st. osmundsen <bird-srcspam@anduin.net>
 *
 * All Rights Reserved
 */

#ifdef __IBMCPP__
typedef char bool;
#define false 0
#define true 1
#endif

bool foo(bool f1, bool f2)
{
    f1 = true;
    f1 = false;
    f2 = false;
    f2 = true;
    return f2 && !f1;
}

bool bar(int i)
{
    {
    int i = 1;
    i++;
    i++;
    i++;
    i *= 2;
    return i;
    }
}

int main()
{
    int rc = !foo(false, true);
    if (!bar(0))
        rc = -1;
    return rc;
}
