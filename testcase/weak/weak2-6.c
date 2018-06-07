/* weak external symbol with unresolvable default. */
int bar6 = 6;

extern int weakexternal6(void);
#pragma weak weakexternal6 = weakexternaldefault6

int weakexternaldefault6(void)
{
    return 12;
}

int foo6(void)
{
    return weakexternal6();
}
