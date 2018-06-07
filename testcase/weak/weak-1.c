/* everything is weak in this testcase */
#pragma weak bar
#pragma weak foo
#pragma weak doo
int bar = -1;

int foo(void)
{
    return -1;
}

int doo(void)
{
    return -2;
}

int weak_in_main(void)
{
    return -3;
}

