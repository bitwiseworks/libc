/*
 * Test fcntl(O_NONBLOCK) on a connected OS/2 named pipe. Check that the
 * libc flags match the native pipe state and that an empty non-blocking
 * read returns EAGAIN.
 *
 * Pass a non-zero first argument to also fill the pipe and check that a
 * non-blocking client write returns without waiting. This check is skipped
 * by default because it can hang when the O_NONBLOCK fix is absent.
 */

#define INCL_DOS
#include <os2.h>

#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <fcntl.h>
#include <errno.h>

#define PIPE_NAME "\\PIPE\\FCNTL\\NAMED_PIPE"

static int named_pipe( int *ph )
{
    HPIPE hpipe;
    HFILE hpipeWrite;
    ULONG ulAction;

    DosCreateNPipe( PIPE_NAME,
                    &hpipe,
                    NP_ACCESS_DUPLEX,
                    NP_NOWAIT | NP_TYPE_BYTE | NP_READMODE_BYTE | 1,
                    32768, 32768, 0 );

    DosConnectNPipe( hpipe );

    DosOpen( PIPE_NAME, &hpipeWrite, &ulAction, 0, FILE_NORMAL,
             OPEN_ACTION_OPEN_IF_EXISTS,
             OPEN_SHARE_DENYREADWRITE | OPEN_ACCESS_WRITEONLY,
             NULL );

    ph[ 0 ] = hpipe;
    ph[ 1 ] = hpipeWrite;

    return 0;

}

int check( const char *msg, int fd )
{
    ULONG ulState;
    int fl;
    int failed;
    ULONG rc;

    rc = DosQueryNPHState( fd, &ulState );
    if(!rc)
    {
        fl = fcntl(fd, F_GETFL);

        rc = fl != -1 ? 0 : 1;
    }

    failed = rc ||
             ((( ulState & NP_NOWAIT ) == NP_NOWAIT ) !=
              (( fl & O_NONBLOCK ) == O_NONBLOCK ));

    printf("%s: %s, fcntl() = %s(%s)\n",
           failed ? "FAILED" : "PASSED", msg,
           ( fl & O_NONBLOCK ) ? "Non-block" : "Block",
           ( ulState & NP_NOWAIT ) ? "Non-block" : "Block");

    return failed;
}

static int check_nonblocking_read( const char *msg, int fd )
{
    char ch;
    int n;
    int saved_errno;
    int failed;

    errno = 0;
    n = read( fd, &ch, 1 );
    saved_errno = errno;
    failed = n != -1 || saved_errno != EAGAIN;

    printf( "%s: %s, empty read() = %d, errno = %d\n",
            failed ? "FAILED" : "PASSED", msg, n, saved_errno );

    return failed;
}

static int check_nonblocking_write( const char *msg, int fd )
{
    char buf[4096] = { 0 };
    int i;
    int n = -1;
    int saved_errno = 0;
    int total = 0;
    int failed = 1;

    /* Fill the connected pipe without reading from its server end. */
    for ( i = 0; i < 256; ++i )
    {
        errno = 0;
        n = write( fd, buf, sizeof( buf ) );
        saved_errno = errno;
        if ( n > 0 )
        {
            total += n;
            continue;
        }

        /* OS/2 may report a full non-blocking pipe as a zero-byte write. */
        failed = total == 0 || ( n != 0 && !( n == -1 && saved_errno == EAGAIN ) );
        break;
    }

    printf( "%s: %s, wrote %d bytes, final write() = %d, errno = %d\n",
            failed ? "FAILED" : "PASSED", msg, total, n, saved_errno );

    return failed;
}

int main( int argc, char **argv )
{
    int ph[ 2 ];
    int fd;
    int fl;
    int errors = 0;

    printf("Testing fcntl(O_NONBLOCK) for a named pipe\n");

    named_pipe( ph );

    fd = ph[ 0 ];
    fl = fcntl( fd, F_GETFL );
    errors += check("S: Import", fd );
    errors += check_nonblocking_read("S: Import non-blocking read", fd );

    fcntl( fd, F_SETFL, fl & ~O_NONBLOCK );
    errors += check("S: Set to BLOCK", fd );

    fcntl( fd, F_SETFL, fl | O_NONBLOCK );
    errors += check("S: Set to NON-BLOCK", fd );
    errors += check_nonblocking_read("S: Set to NON-BLOCK read", fd );

    fd = ph[ 1 ];
    fl = fcntl( fd, F_GETFL );
    errors += check("C: Import", fd );

    fcntl( fd, F_SETFL, fl & ~O_NONBLOCK );
    errors += check("C: Set to BLOCK", fd );

    fcntl( fd, F_SETFL, fl | O_NONBLOCK );
    errors += check("C: Set to NON-BLOCK", fd );
    if ( argc > 1 && atoi( argv[ 1 ] ) != 0 )
        errors += check_nonblocking_write("C: Set to NON-BLOCK write", fd );

    close( ph[ 0 ]);
    close( ph[ 1 ]);

    return !!errors;
}
