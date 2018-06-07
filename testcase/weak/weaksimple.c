#define WEAKBSS
/* must be compiled with -fno-common! */

#ifdef WEAKBSS
#pragma weak weakbss
int     weakbss;
#endif

int     weakdata = 1;
#pragma weak weakdata

#pragma weak weaktext
int weaktext(void)
{
    return -1;
}

int main()
{
    return weaktext()
#ifdef WEAKBSS
        + weakbss
#endif
        + weakdata;
}

