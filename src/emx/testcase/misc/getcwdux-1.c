/*
 * Regression test for #184: _getcwdux() omits the drive prefix with both
 * an allocated result and a caller-supplied buffer.
 */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

int main( void )
{
    char *cwd = NULL;
    char buf[ PATH_MAX ];
    int failed;
    int errors = 0;

    printf("Testing if _getcwdux() returns CWD excluding the drive name...\n");

    cwd = _getcwdux( NULL, 0 );
    failed = cwd == NULL || cwd[ 0 ] != '/';
    printf("%s: _getcwdux( NULL ) = [%s]\n", failed ? "FAILED" : "PASSED",
           cwd );

    if( cwd != NULL )
        free( cwd );

    errors += failed;

    cwd = _getcwdux( buf, sizeof buf );
    failed = cwd == NULL || cwd[ 0 ] != '/';
    printf("%s: _getcwdux( buf ) = [%s]\n", failed ? "FAILED" : "PASSED",
           cwd );

    errors += failed;

    return !!errors;
}
