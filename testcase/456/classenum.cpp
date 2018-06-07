/* $Id: classenum.cpp 733 2003-09-26 14:14:04Z bird $ */
/** @file
 * This testcase show a problem with forward references to enums.
 * GCC doesn't really make .stabs and enums types debuggable, but it's better
 * to not have nsUGenCategory - the type name - than loosing all the values.
 * So, I've switched off 'xe' output and we'll get values only, no type name.
 */
class ChildClass
{
public:
    typedef enum
    {
        kUGenCategory_Mark         = 1,
        kUGenCategory_Number       = 2,
        kUGenCategory_Separator    = 3,
        kUGenCategory_Other        = 4,
        kUGenCategory_Letter       = 5,
        kUGenCategory_Punctuation  = 6,
        kUGenCategory_Symbol       = 7
    } nsUGenCategory;

public:
    ChildClass(nsUGenCategory enm)
    {
    }
};

int main(void)
{
    ChildClass obj((ChildClass::nsUGenCategory)2);
    return 0;
}
