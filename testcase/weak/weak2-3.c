int bar3 = 3;

int foo(void);
#pragma weak foo

int foo(void)
{
    return 3;
}

int foo3(void)
{
    return foo();
}


