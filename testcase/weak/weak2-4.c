/* weak external symbol, no defaults, not resolved. */
int bar4 = 4;

extern int weakexternal4(void);
#pragma weak weakexternal4

int foo4(void)
{
    return weakexternal4 ? weakexternal4() : -1;
}
