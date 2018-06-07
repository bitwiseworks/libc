/* very simple testcase/experiment for weak symbols. */
extern int bar;

int foo(void)
{
    return 1;
}


#pragma weak weak_in_main
int weak_in_main(void)
{
    return 3;
}

int main()
{
    int i;
    int cErrors = 0;

    i = foo();
    if (i != 1)
    {
        printf("foo -> %d expected 1\n", i);
        cErrors++;
    }

    i = doo();
    if (i != -2)
    {
        printf("doo -> %d expected -2\n", i);
        cErrors++;
    }

    if (bar != -1)
    {
        printf("bar == %d expected -1\n", bar);
        cErrors++;
    }

    i = weak_in_main();
    if (i != -3)
    {
        printf("weak_in_main == %d expected -3\n", i);
        cErrors++;
    }

    return cErrors;
}
