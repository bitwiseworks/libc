/* $Id: complex.c 292 2003-06-04 03:02:11Z bird $
 * Complex numbers.
 * C99???
 */

#include <stdio.h>
/* do we have complex.h? let's skip that for now and do the #define ourself. */
#define complex _Complex

typedef struct { int real,imag; }           FakeComplexInt;
typedef struct { float real,imag; }         FakeComplexFloat;
typedef struct { double real,imag; }        FakeComplexDouble;
typedef struct { long double real,imag; }   FakeComplexLongDouble;

void main(void)
{
    complex int             ComplexInt;
    complex float           ComplexFloat;
    complex double          ComplexDouble;
    complex long double     ComplexLongDouble;

    printf("sizeof(complex int        ) = %2dbytes %3dbits\n", sizeof(ComplexInt       ), sizeof(ComplexInt       )*8);
    printf("sizeof(complex float      ) = %2dbytes %3dbits\n", sizeof(ComplexFloat     ), sizeof(ComplexFloat     )*8);
    printf("sizeof(complex double     ) = %2dbytes %3dbits\n", sizeof(ComplexDouble    ), sizeof(ComplexDouble    )*8);
    printf("sizeof(complex long double) = %2dbytes %3dbits\n", sizeof(ComplexLongDouble), sizeof(ComplexLongDouble)*8);
    printf("\nFake types:\n");
    printf("sizeof(FakeComplexInt       ) = %2dbytes %3dbits\n", sizeof(FakeComplexInt       ), sizeof(FakeComplexInt       )*8);
    printf("sizeof(FakeComplexFloat     ) = %2dbytes %3dbits\n", sizeof(FakeComplexFloat     ), sizeof(FakeComplexFloat     )*8);
    printf("sizeof(FakeComplexDouble    ) = %2dbytes %3dbits\n", sizeof(FakeComplexDouble    ), sizeof(FakeComplexDouble    )*8);
    printf("sizeof(FakeComplexLongDouble) = %2dbytes %3dbits\n", sizeof(FakeComplexLongDouble), sizeof(FakeComplexLongDouble)*8);
}
