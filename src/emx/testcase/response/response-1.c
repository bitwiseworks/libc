#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <process.h>

#define LINE_LENGTH ( 1000 * 1000 )

#define RESPONSE_NAME "_response.rsp"

static int spawn( const char *prog )
{
    FILE *f;
    int i;
    int rc;

    f = fopen( RESPONSE_NAME, "wt");

    for( i = 0; i < LINE_LENGTH - 1; i++ )
        fputc('X', f );
    fputc('1', f );
    fputc('\n', f );

    for( i = 0; i < LINE_LENGTH - 1; i++ )
        fputc('X', f );
    fputc('2', f );

    fclose( f );

    rc = spawnlp( P_WAIT, prog, prog, "@" RESPONSE_NAME, NULL );

    remove( RESPONSE_NAME );

    return rc;
}

#define WANTED_ARG_COUNT   3

int main( int argc, char *argv[])
{
    _response( &argc, &argv );

    if( argc > 1 )
    {
        int errors = 0;
        int failed;

        int i;

        failed = argc != WANTED_ARG_COUNT;
        if( failed )
            errors++;

        fprintf( stderr, "%s: argc = %d(%d)\n",
                 failed ? "FAILED" : "PASSED", argc, WANTED_ARG_COUNT );

        for( i = 1; i < WANTED_ARG_COUNT; i++ )
        {
            const char *arg = argv[ i ];
            int len = strlen( arg );

            failed = len != LINE_LENGTH || arg[ len - 1 ] != ('0' + i );
            if( failed )
                errors++;

            fprintf( stderr,
                     "%s: argv[ %d ]: len = %d(%d), last char = %c(%c)\n",
                     failed ? "FAILED" : "PASSED", i, len, LINE_LENGTH,
                     arg[ len - 1 ], '0' + i );
        }

        return !!errors;
    }

    printf("Testing _response() can read a line longer than 8192 bytes...\n");

    return spawn( argv[ 0 ]);
}
