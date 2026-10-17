#include <stdio.h>
#include <InnoTekLibc/locale.h>

int main( void )
{
    unsigned char prefixes[ 256 / 4 ] = { 0, };
    int failed;
    int errors = 0;
    int i;

    printf("Testing if a length of MBCS prefix is retrived correctly...\n");

    for( i = 0; i < 256; i++ )
        SET_MBCS_PREFIX( prefixes, i, i % 4 );

    for( i = 0; i < 256; i++ )
    {
        failed = LEN_MBCS_PREFIX( prefixes, i ) != i % 4;
        printf("%s: Prefix length of code point %d = %d(%d)\n",
               failed ? "FAILED" : "PASSED",
               i, LEN_MBCS_PREFIX( prefixes, i ), i % 4 );

        errors += failed;
    }

    return !!errors;
}
