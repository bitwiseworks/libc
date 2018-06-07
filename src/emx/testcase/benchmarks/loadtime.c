#ifdef __OS2__
#define INCL_BASE
#endif
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <dlfcn.h>
#include "PrfTiming.h"


int main(int argc, char **argv)
{
    int argi;

    if (argc <= 1)
    {
        printf("syntax: loadtime <dll> [dll2 [..]]\n");
        return 1;
    }

    /*
     * Loop thru the libs.
     */
    for (argi = 1; argi < argc; argi++)
    {
        void       *pv1;
        void       *pv2;
        long double rdStart;
        long double rdEnd;
        long double rd;

        printf("loading %s...\n", argv[argi]);
        rdStart = gettime();
        pv1 = dlopen(argv[argi], 0);
        rdEnd = gettime();
        rd = rdEnd - rdStart;
        rd /= getHz();
        printf("1st %f ", (double)rd);    fflush(stdout);
        if (!pv1) printf("!err! ");

        rdStart = gettime();
        pv2 = dlopen(argv[argi], 0);
        rdEnd = gettime();
        rd = rdEnd - rdStart;
        rd /= getHz();
        printf("2nd %f ", (double)rd);    fflush(stdout);
        if (!pv2) printf("!err! ");

        rdStart = gettime();
        dlclose(pv2);
        rdEnd = gettime();
        rd = rdEnd - rdStart;
        rd /= getHz();
        printf("close2 %f ", (double)rd); fflush(stdout);

        rdStart = gettime();
        dlclose(pv1);
        rdEnd = gettime();
        rd = rdEnd - rdStart;
        rd /= getHz();
        printf("close1 %f ", (double)rd);
        printf("\n");
    }

    return 0;
}
