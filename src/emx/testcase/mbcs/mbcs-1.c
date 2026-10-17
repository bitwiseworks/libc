#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <locale.h>

int main( void )
{
    /* "Hangul!" (U+D55C U+AE00 '!') encoded in CP949. */
    const char *s = "\xC7\xD1\xB1\xDB!";
    wchar_t ws[ 4 ] = { 1, 1, 1, 1 };
    size_t rc;
    const size_t wlen = 3;
    int errors = 0;
    int failed;

    printf("Testing mbstowcs() returns in wide-characters...\n");

    if( setlocale( LC_CTYPE, "ko_KR.CP949" ) == NULL )
    {
        fprintf( stderr, "FAILED: cannot set ko_KR.CP949 locale\n" );
        return 1;
    }

    rc = mbstowcs( ws, s, sizeof( ws ) / sizeof( ws[ 0 ] ) );
    if( rc == (size_t)-1 )
    {
        perror( "FAILED: mbstowcs(non-NULL)" );
        return 1;
    }
    failed = rc != wlen;

    printf("%s: non-NULL: rc = %lu(%lu)\n",
           failed ? "FAILED" : "PASSED",
           (unsigned long)rc, (unsigned long)wlen );

    if( failed )
        errors++;

    if( wmemchr( ws, L'\0', sizeof( ws ) / sizeof( ws[ 0 ] ) ) == NULL )
    {
        fprintf( stderr, "FAILED: converted string is not null-terminated\n" );
        errors++;
    }
    else
    {
        rc = wcslen( ws );
        failed = rc != wlen;

        printf("%s: wcslen: rc = %lu(%lu)\n",
               failed ? "FAILED" : "PASSED",
               (unsigned long)rc, (unsigned long)wlen );

        if( failed )
            errors++;
    }

    rc = mbstowcs( NULL, s, 0 );
    if( rc == (size_t)-1 )
    {
        perror( "FAILED: mbstowcs(NULL)" );
        return 1;
    }
    failed = rc != wlen;

    printf("%s: NULL: rc = %lu(%lu)\n",
           failed ? "FAILED" : "PASSED",
           (unsigned long)rc, (unsigned long)wlen );

    if( failed )
        errors++;

    return !!errors;
}
