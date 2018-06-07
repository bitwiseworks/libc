#include <stdio.h>
#include <float.h>
#include <signal.h>
#ifdef OS2
#define INCL_BASE
#define INCL_PM
#include <os2.h>
#endif

#define JSDOUBLE_HI32_SIGNBIT   0x80000000
#define JSDOUBLE_HI32_EXPMASK   0x7ff00000
#define JSDOUBLE_HI32_MANTMASK  0x000fffff
#define JSVAL_INT_POW2(n)       ((unsigned long)1 << (n))
#define JSVAL_INT_MAX           (JSVAL_INT_POW2(30) - 1)
#define JSVAL_INT_MIN           ((unsigned long)1 - JSVAL_INT_POW2(30))

typedef unsigned uint32;
typedef uint32   jsuint;
typedef int      jsint;


#define JSDOUBLE_HI32(x)        (((uint32 *)&(x))[1])
#define JSDOUBLE_LO32(x)        (((uint32 *)&(x))[0])

#define JSDOUBLE_IS_NEGZERO(d)  (JSDOUBLE_HI32(d) == JSDOUBLE_HI32_SIGNBIT && \
				 JSDOUBLE_LO32(d) == 0)
#define JSDOUBLE_IS_FINITE(x)                                                 \
    ((JSDOUBLE_HI32(x) & JSDOUBLE_HI32_EXPMASK) != JSDOUBLE_HI32_EXPMASK)

#define JSDOUBLE_IS_INT(d, i) (JSDOUBLE_IS_FINITE(d)                          \
                               && !JSDOUBLE_IS_NEGZERO(d)                     \
			       && ((d) == (i = (jsint)(d))))

#define INT_FITS_IN_JSVAL(i)    ((jsuint)((i)+JSVAL_INT_MAX) <= 2*JSVAL_INT_MAX)


int js_NewNumberValue(void *cx, double d, int *rval)
{
    int i = -1;
#if 1
    if (    JSDOUBLE_IS_INT(d, i)
        &&  INT_FITS_IN_JSVAL(i))
    {
        *rval = i;
        return 1;
    }

#else
    if (((((unsigned *)&d)[1]) & JSDOUBLE_HI32_EXPMASK) != JSDOUBLE_HI32_EXPMASK )
        if ( !((((unsigned *)&d)[1]) == JSDOUBLE_HI32_SIGNBIT && (((unsigned *)&d)[0]) == 0))
            if (d == (i = (int)d))
                if ((unsigned long)((i)+JSVAL_INT_MAX) <= 2*JSVAL_INT_MAX)
                    return i;
#endif
    *rval = i;
    return 0;
}

void sigfun(int sig)
{
    printf("SIGFPE!\n");
}

int main()
{
    double rd1 = 0.0;
    double rd2 = 1.000000000e8;
    double rd3 = 2.147746133e9;
    int l = -1;
#ifdef OS2
    PPIB ppib;
    DosGetInfoBlocks(NULL, &ppib);
    ppib->pib_ultype = 3;
    WinCreateMsgQueue(WinInitialize(0), 0);
    WinMessageBox(HWND_DESKTOP, HWND_DESKTOP, "blah", "Blah!", 0, MB_OK);
#else
    _control87(0x262, 0xFFF);
#endif
    signal(SIGFPE, sigfun);

    l = -1;
    js_NewNumberValue(NULL, rd1, &l);
    printf("%11d  %f\n", l, rd1);

    l = -1;
    js_NewNumberValue(NULL, rd2, &l);
    printf("%11d  %f\n", l, rd2);

    l = -1;
    js_NewNumberValue(NULL, rd3, &l);
    printf("%11d  %f\n", l, rd3);
    return 0;
}

