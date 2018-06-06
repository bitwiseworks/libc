#include <unistd.h>
#include <PrfTiming.h>
#include <stdio.h>
#include <errno.h>


#ifdef __EMX__
/* guesses, will fail on high system load */
#define MAXDIFF_BIG     32000
#define MAXDIFF_SMALL   10000
#else
#define MAXDIFF_BIG     20000
#define MAXDIFF_SMALL    2000
#endif




int main()
{
    struct __test
    {
        /** Number of seconds to sleep. */
        useconds_t      useconds;
        /** Accepted number variation (higher, not lower). */
        useconds_t      udiff;
        /** expected return code. */
        int             rc;
        /** expected errno. */
        int             err;
    }   aTests[] =
    {
        {        1, MAXDIFF_SMALL, 0, 0 },
        {     2000, MAXDIFF_SMALL, 0, 0 },
        {     4000, MAXDIFF_SMALL, 0, 0 },
        {    10000, MAXDIFF_SMALL, 0, 0 },
        {    20000, MAXDIFF_SMALL, 0, 0 },
        {    25000, MAXDIFF_SMALL, 0, 0 },
        {    30000, MAXDIFF_SMALL, 0, 0 },
        {    33000, MAXDIFF_SMALL, 0, 0 },
        {    32100, MAXDIFF_SMALL, 0, 0 },
        {    32200, MAXDIFF_SMALL, 0, 0 },
        {    32300, MAXDIFF_SMALL, 0, 0 },
        {    33000, MAXDIFF_SMALL, 0, 0 },
        {    32000, MAXDIFF_SMALL, 0, 0 },
        {    29000, MAXDIFF_SMALL, 0, 0 },
        {    29500, MAXDIFF_SMALL, 0, 0 },
        {    30000, MAXDIFF_SMALL, 0, 0 },
        {    30500, MAXDIFF_SMALL, 0, 0 },
        {    31000, MAXDIFF_SMALL, 0, 0 },
        {    31100, MAXDIFF_SMALL, 0, 0 },
        {    31300, MAXDIFF_SMALL, 0, 0 },
        {    31500, MAXDIFF_SMALL, 0, 0 },
        {    31700, MAXDIFF_SMALL, 0, 0 },
        {    31900, MAXDIFF_SMALL, 0, 0 },
        {    33000, MAXDIFF_SMALL, 0, 0 },
        {   200000, MAXDIFF_BIG,   0, 0 },
    };
    int         i;
    int         rcRet = 0;

    for (i = 0; i < sizeof(aTests)/sizeof(aTests[0]); i++)
    {
        int j;
        for (j = 0; j < 10; j++)
        {
            long double lrdEnd, lrdStart, lrdDiff;
            useconds_t  udiff;
            int         err;
            int         rc;

            errno = 0;
            lrdStart = gettime();
            rc = usleep(aTests[i].useconds);
            lrdEnd = gettime();
            err = errno;

            lrdDiff = lrdEnd - lrdStart;
            udiff = lrdDiff * 1000000 / getHz();

            /* check rc/errno */
            if (    rc != aTests[i].rc
                ||  err != aTests[i].err)
            {
                printf("usleep: %d: Invalid rc and/or errno. returned rc=%d expected %d, errno=%d, expected %d.\n",
                       i, rc, aTests[i].rc, err, aTests[i].err);
                rcRet++;
            }
            else if (!rc)
            {   /* check time */
                if (udiff < aTests[i].useconds)
                {
                    printf("usleep: %d: returned too early. sleept %d, requested %d\n",
                           i, udiff, aTests[i].useconds);
                    rcRet++;
                    break;
                }
                else if (udiff > aTests[i].useconds + aTests[i].udiff)
                {
                    printf("usleep: %d: returned too late. sleept %d, requested %d. overslept %d max expected diff %d\n",
                           i, udiff, aTests[i].useconds, udiff - aTests[i].useconds, aTests[i].udiff);
                    rcRet++;
                    break;
                }
            }
        }
    }

    if (!rcRet)
        printf("usleep: all tests succeeded.\n");
    else
        printf("usleep: %d failure%s\n", rcRet, rcRet > 1 ? "s" : "");

    return rcRet;
}

