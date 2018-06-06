/* $Id: longnames.cpp 825 2003-10-08 19:31:15Z bird $ */
/** @file
 *
 * This testcase expose the mangling problem we face with the IBM debuggers;
 * more specificly the 106 char limit. The testcase must cause both VAC and
 * GCC to genereate C++ mangled names to exceed this limit.
 *
 * InnoTek Systemberatung GmbH confidential
 *
 * Copyright (c) 2003 InnoTek Systemberatung GmbH
 * Author: knut st. osmundsen <bird-srcspam@anduin.net>
 *
 * All Rights Reserved
 */

#define NULL 0

struct StructureWithALongNameA;
struct StructureWithALongNameB;
struct StructureWithALongNameC;
struct StructureWithALongNameD;
struct StructureWithALongNameE;
struct StructureWithALongNameF;
struct StructureWithALongNameG;
struct StructureWithALongNameH;
struct StructureWithALongNameI;
struct StructureWithALongNameJ;
struct StructureWithALongNameK;
struct StructureWithALongNameL;
struct StructureWithALongNameM;
struct StructureWithALongNameN;
struct StructureWithALongNameO;

class bar
{
public:
    bar()
    { }

    struct StructureWithALongNameA *foomethod(
        StructureWithALongNameA *a = NULL,
        StructureWithALongNameB *b = NULL,
        StructureWithALongNameC *c = NULL,
        StructureWithALongNameD *d = NULL,
        StructureWithALongNameE *e = NULL,
        StructureWithALongNameF *f = NULL,
        StructureWithALongNameG *g = NULL,
        StructureWithALongNameH *h = NULL,
        StructureWithALongNameI *i = NULL,
        StructureWithALongNameJ *j = NULL,
        StructureWithALongNameK *k = NULL,
        StructureWithALongNameK *l = NULL,
        StructureWithALongNameK *m = NULL,
        StructureWithALongNameK *n = NULL,
        StructureWithALongNameK *o = NULL)
    {
        return NULL;
    }

    static struct StructureWithALongNameA *foostatic(
        StructureWithALongNameA *a = NULL,
        StructureWithALongNameB *b = NULL,
        StructureWithALongNameC *c = NULL,
        StructureWithALongNameD *d = NULL,
        StructureWithALongNameE *e = NULL,
        StructureWithALongNameF *f = NULL,
        StructureWithALongNameG *g = NULL,
        StructureWithALongNameH *h = NULL,
        StructureWithALongNameI *i = NULL,
        StructureWithALongNameJ *j = NULL,
        StructureWithALongNameK *k = NULL,
        StructureWithALongNameK *l = NULL,
        StructureWithALongNameK *m = NULL,
        StructureWithALongNameK *n = NULL,
        StructureWithALongNameK *o = NULL)
    {
        return NULL;
    }
};

class looooooooooooooooooooooooooooooonnnnnnnnnnnnnnnnnnnnnnnnnggggggggggggggggggggggggggccccccccccccccccccccccccccccccccccccllllllllllllllllllllllllaaaaaaaaaaaaaaassssssssssssss
{
public:
    looooooooooooooooooooooooooooooonnnnnnnnnnnnnnnnnnnnnnnnnggggggggggggggggggggggggggccccccccccccccccccccccccccccccccccccllllllllllllllllllllllllaaaaaaaaaaaaaaassssssssssssss()
    {
    }
static long ppppppppppppppppppprrrrrrrrrrrrreeeeeeeeeettttttttttttyyyyyyyyyylllllllloooooooooonnnnnnnnnnnngggggggssssssssstttttttttttaaaaaaaaaaaaatttttttiiiiiiiiiicccccccccc;
    int getstatic()
    {
        return ppppppppppppppppppprrrrrrrrrrrrreeeeeeeeeettttttttttttyyyyyyyyyylllllllloooooooooonnnnnnnnnnnngggggggssssssssstttttttttttaaaaaaaaaaaaatttttttiiiiiiiiiicccccccccc;
    }
};

long looooooooooooooooooooooooooooooonnnnnnnnnnnnnnnnnnnnnnnnnggggggggggggggggggggggggggccccccccccccccccccccccccccccccccccccllllllllllllllllllllllllaaaaaaaaaaaaaaassssssssssssss::ppppppppppppppppppprrrrrrrrrrrrreeeeeeeeeettttttttttttyyyyyyyyyylllllllloooooooooonnnnnnnnnnnngggggggssssssssstttttttttttaaaaaaaaaaaaatttttttiiiiiiiiiicccccccccc
    = -1;

struct StructureWithALongNameA *foo(
    StructureWithALongNameA *a = NULL,
    StructureWithALongNameB *b = NULL,
    StructureWithALongNameC *c = NULL,
    StructureWithALongNameD *d = NULL,
    StructureWithALongNameE *e = NULL,
    StructureWithALongNameF *f = NULL,
    StructureWithALongNameG *g = NULL,
    StructureWithALongNameH *h = NULL,
    StructureWithALongNameI *i = NULL,
    StructureWithALongNameJ *j = NULL,
    StructureWithALongNameK *k = NULL,
    StructureWithALongNameK *l = NULL,
    StructureWithALongNameK *m = NULL,
    StructureWithALongNameK *n = NULL,
    StructureWithALongNameK *o = NULL)
{
    return NULL;
}


int main()
{
    bar     obj;
    looooooooooooooooooooooooooooooonnnnnnnnnnnnnnnnnnnnnnnnnggggggggggggggggggggggggggccccccccccccccccccccccccccccccccccccllllllllllllllllllllllllaaaaaaaaaaaaaaassssssssssssss obj2;
    bar::foostatic();
    obj.foomethod();
    obj2.getstatic();
    foo();
    return 0;
}
