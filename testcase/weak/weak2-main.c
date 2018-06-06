/* test more complex weak stuff. Both weak symbols and weak external symbols. */
#include <stdio.h>
extern int bar1;
extern int bar2;
extern int bar3;
extern int bar4;
extern int bar5;
extern int bar6;

extern int weak_not_defined(void);
#pragma weak weak_not_defined

extern int weak_defined(void);
#pragma weak weak_not_defined

int main()
{
    int i;
    int cErrors = 0;

    i = foo();
    if (i != 1)
    {
        printf("foo() returned %d, expected 1\n", i);
        cErrors++;
    }

    i = foo3();
    if (i != 1)
    {
        printf("foo3() returned %d, expected 1\n", i);
        cErrors++;
    }

    i = foo4();
    if (i != -1)
    {
        printf("foo4() returned %d, expected -1\n", i);
        cErrors++;
    }

    i = foo5();
    if (i != 1)
    {
        printf("foo5() returned %d, expected 1\n", i);
        cErrors++;
    }

    i = foo6();
    if (i != 12)
    {
        printf("foo6() returned %d, expected 12\n", i);
        cErrors++;
    }

    i = bar1 + bar2 + bar3 + bar4 + bar5 + bar6;
    if (i != 21)
    {
        printf("bar sum is %d expected 21\n", i);
        cErrors++;
    }

    if (weak_not_defined)
    {
        printf("weak_not_defined %#p expected NULL\n", weak_not_defined);
        cErrors++;
    }

    if (!weak_defined)
    {
        printf("weak_not_defined %#p expected non-zero\n", weak_defined);
        cErrors++;
    }

    return cErrors;
}
