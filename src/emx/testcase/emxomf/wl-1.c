#include <stdio.h>

int getflag( void );

int main( void )
{
    printf("Testing if emxomfld converts IMPORTS of .def to IMPORT of WLINK correctly...\n");

    /* This program is running. So conversion was done correctly. */
    printf("PASSED: flag = %d\n", getflag());

    return 0;
}
