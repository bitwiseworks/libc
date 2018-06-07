/* $Id: duplicate1.cpp 1332 2004-04-04 17:55:18Z bird $ */
/** @file
 *
 * Testcase for emxbind problem with differenting symbols by case.
 *
 * Copyright (c) 2004 knut st. osmundsen <bird-srcspam@anduin.net>
 *
 * All Rights Reserved
 *
 */

class wxBaseArrayInt
{
public:
    wxBaseArrayInt() {}

    void Clear();
    void clear();

};

void wxBaseArrayInt::Clear()
{
    return;
}

void wxBaseArrayInt::clear()
{
    return;
}

int main()
{
    wxBaseArrayInt foo();
    return 0;
}
