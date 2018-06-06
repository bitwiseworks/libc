/* weak external symbol with resolvable default. */
int bar5 = 5;

extern int foo(void);
#ifndef __EMX__ /* we don't support defaults */
#pragma weak foo = foodefault

int foodefault(void)
{
    return 11;
}
#endif

int foo5(void)
{
    return foo();
}
