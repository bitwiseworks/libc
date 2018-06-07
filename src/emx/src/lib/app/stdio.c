/* stdio.c (emx+gcc) -- Copyright (c) 1990-1996 by Eberhard Mattes
                     -- Copyright (c) 2003-2004 by knut st. osmundsen */

#include "libc-alias.h"
#include <sys/builtin.h>        /* For <sys/fmutex.h> */
#include <sys/fmutex.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <emx/io.h>
#include <emx/startup.h>
#include <emx/syscalls.h>
#define __LIBC_LOG_GROUP  __LIBC_LOG_GRP_STREAM
#include <InnoTekLIBC/logstrict.h>


/*******************************************************************************
*   Global Variables                                                           *
*******************************************************************************/
/** Preallocated streams
 * (public since stdin,stdout,stderr references this thru #defines.) */
static FILE                 gaPreFiles[_NFILES] = {{0}};

/** Vector entry for preallocated streams. */
static struct streamvec     gPreStreamVec = {_NFILES, NULL, &gaPreFiles[0], NULL, _NFILES};

/** The buffer for stdin. */
static char                 gachStdIn[BUFSIZ];

/** Mutex protecting the stream vector. */
_fmutex                     _streamv_fmutex;    /* See io/_newstre.c */

/*
 * (These variables must be initialized as no extended dictionary
 * entries are built for COMDEF.)
 */

/** Head of the stream vector list. */
struct streamvec           *_streamvec_head = NULL;
/** Tail of the stream vector list. */
struct streamvec           *_streamvec_tail = NULL;

/** Standard input stream pointer. */
FILE                       *__stdinp = &gaPreFiles[0];
/** Standard output stream pointer. */
FILE                       *__stdoutp = &gaPreFiles[1];
/** Standard error stream pointer. */
FILE                       *__stderrp = &gaPreFiles[2];


/**
 * Initialize the streams -- this function will be called by
 * _CRT_init() via the __crtinit1__ set vector.
 */
CRT_DATA_USED
void _init_streams(void);
CRT_DATA_USED
void _init_streams(void)
{
    LIBCLOG_ENTER("\n");
    FILE               *pFile;
    int                 i;
    static char         fInited = 0;

    /*
     * Paranoia!
     */
    if (fInited)
        LIBCLOG_RETURN_VOID();
    fInited = 1;
    LIBCLOG_MSG("_NFILES=%d\n", _NFILES);

    /*
     * Init the stream vector back pointer.
     */
    pFile   = &gaPreFiles[0];
    for (i = 0; i < _NFILES; i++, pFile++)
        pFile->__pSV = &gPreStreamVec;

    /*
     * Create the mutex protecting the stream list and insert the table.
     * ASSUME: we're ALONE at this point.
     */
    _streamvec_head     = &gPreStreamVec;
    _streamvec_tail     = &gPreStreamVec;
    _fmutex_checked_create(&_streamv_fmutex, 0);

    /*
     * As far as I know there is no reason to initiate a bunch of streams
     * here as EM used to do. All that is required is the three standard
     * streams.
     */
    for (i = 0; i < 3; i++)
    {
        PLIBCFH pFH = __libc_FH(i);
        if (!pFH)
            LIBCLOG_MSG("No open file for %d\n", i);

        /*
         * Common init - recall everything is ZERO.
         */
        gPreStreamVec.cFree--;
        gaPreFiles[i].__uVersion = _FILE_STDIO_VERSION;
        gaPreFiles[i]._flags    |= _IOOPEN | _IONOCLOSEALL;
        gaPreFiles[i]._handle    = i;
        gaPreFiles[i]._flush     = _flushstream;
        if (_fmutex_create2(&gaPreFiles[i].__u.__fsem, 0, "LIBC stream") != 0)
        {
            LIBC_ASSERTM_FAILED("_setmore failed for i=%d\n", i);
            abort();
        }

        /*
         * Specific init.
         */
        switch (i)
        {
            case 0:
                /* stdin is always buffered. */
                gaPreFiles[0]._flags    |= _IOREAD | _IOFBF | _IOBUFUSER;
                gaPreFiles[0]._ptr       = gachStdIn;
                gaPreFiles[0]._buffer    = gachStdIn;
                gaPreFiles[0]._buf_size  = BUFSIZ;
                LIBCLOG_MSG("__stdinp=%p\n", (void *)__stdinp);
                break;

            case 1:
                /* stdout is buffered unless it's connected to a device. */
                gaPreFiles[1]._flags |= _IOWRT | _IOBUFNONE
                    | (pFH && (pFH->fFlags & __LIBC_FH_TYPEMASK) == F_DEV ? _IONBF : _IOFBF);
                LIBCLOG_MSG("__stdoutp=%p flags=%#x (%s)\n", (void *)__stdoutp, gaPreFiles[1]._flags,
                            pFH && (pFH->fFlags & __LIBC_FH_TYPEMASK) == F_DEV ? "dev" : "none-dev");
                break;

            case 2:
                /* stderr is always unbuffered. */
                gaPreFiles[2]._flags |= _IOWRT | _IOBUFNONE | _IONBF;
                LIBCLOG_MSG("__stderrp=%p\n", (void *)__stderrp);
                break;
        }
    } /* for standard handles. */
    LIBCLOG_RETURN_VOID();
}


/* Shutdown the streams -- this function will be called by _CRT_term()
   via the __crtexit1__ set vector.  Do not close the streams as this
   function is called by abort() and the signal handler might want to
   print something.  (ISO 9899-1990 lets this undefined.) */

CRT_DATA_USED
void _exit_streams(void);
CRT_DATA_USED
void _exit_streams(void)
{
    flushall();
    _rmtmp();
}

_CRT_INIT1(_init_streams)
_CRT_EXIT1(_exit_streams)



/**
 * Fatal stream error.
 *
 * @returns 0.
 * @param   f           Stream connected to the fatal error.
 * @param   pszMsg      Message to print.
 */
int __stream_abort(FILE *f, const char *pszMsg)
{
    static const char szMsg[] = "\r\nLIBC fatal error - streams: ";
    __write(2, szMsg, sizeof(szMsg) - 1);
    __write(2, pszMsg, strlen(pszMsg));
    if (f->__uVersion != _FILE_STDIO_VERSION)
    {
        static const char szMsg2[] = " (stream version mismatch (too))";
        __write(2, szMsg2, sizeof(szMsg2) - 1);
    }
    __write(2, "\r\n", 2);
    abort();
    return 0;
}

