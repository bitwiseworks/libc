#include <stdio.h>
#include "throw.h"

bar::bar(int i) : i(i)
{
    dfprintf((stderr, "bar::constructor 1\n"));
}

bar::bar() throw(int) : i(1)
{
    dfprintf((stderr, "bar::constructor 2\n"));
    throw(1);
}

int bar::get() const
{
    return i;
}

int bar::getThrow() const throw(expt)
{
    throw(expt(1, ""));
    return 1;
}



expt::expt(int i, const char *psz) : i(i), psz(psz)
{
}

int expt::get() const
{
    return i;
}


