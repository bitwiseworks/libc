#include <stdio.h>
#include "throw.h"

class foo
{
    int     i;
public:
    foo(int i) : i(i)
    {
        dfprintf((stderr, "foo::constructor 1\n"));
    }

    foo() throw(int) : i(1)
    {
        dfprintf((stderr, "foo::constructor 2\n"));
        throw(1);
    }

    int get() const
    {
        return i;
    }

    int getthrow() const throw(int)
    {
        throw(2);
        return i;
    }
};


static foo  o2(2);
static bar  o3(3);

int main()
{
    int rc = 0;

    /* static foo - inline implementation */
    if (o2.get() == 2)
        dfprintf((stderr, "o2 ok\n"));
    else
    {
        rc++;
        dfprintf((stderr, "o2 failed\n"));
    }
    try
    {
        rc += o2.getthrow();
        printf("error: foo::getthrow() didn't throw!\n");
    }
    catch (int e)
    {
        dfprintf((stderr, "foo caught e=%d (ok)\n", e));
    }

    /* foo - inline implementation */
    try
    {
        dfprintf((stderr, "foo creating\n"));
        foo o;
        dfprintf((stderr, "foo no throw!\n"));
        printf("error: foo::foo() didn't throw!\n");
        rc += o.get();
    }
    catch (int e)
    {
        dfprintf((stderr, "foo caught e=%d (ok)\n", e));
    }


    /* static bar - external implementation */
    if (o3.get() == 3)
        dfprintf((stderr, "o3 ok\n"));
    else
    {
        rc++;
        dfprintf((stderr, "o3 failed\n"));
    }
    try
    {
        rc += o3.getThrow();
        printf("error: bar:getThrow() didn't throw!\n");
    }
    catch (expt e)
    {
        dfprintf((stderr, "foo caught e=%d (ok)\n", e.get()));
    }

    /* bar - external implementation */
    try
    {
        dfprintf((stderr, "bar creating\n"));
        bar o;
        dfprintf((stderr, "bar no throw!\n"));
        printf("error: bar::bar() didn't throw!\n");
        rc += o.get();
    }
    catch (int e)
    {
        dfprintf((stderr, "bar caught e=%d (ok)\n", e));
    }

    return rc;
}

