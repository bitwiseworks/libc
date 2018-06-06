/* _newstre.c (emx+gcc) -- Copyright (c) 1990-1996 by Eberhard Mattes
                        -- Copyright (c) 2003 by Knut St. Osmundsen */

#include "libc-alias.h"
#include <stdlib.h>
#include <sys/builtin.h>        /* For <sys/fmutex.h> */
#include <sys/fmutex.h>
#include <stdio.h>
#include <string.h>
#include <emx/io.h>
#include <emx/umalloc.h>

#define INC     64
#define ALIGN(p, a) (((unsigned long)(p) + (a) - 1) & ~((a) - 1))

FILE *_newstream (void)
{
    int                 i;
    struct streamvec   *pSV;

    /*
     * Search thru the list of streams.
     * Backwards as we're more likely to find free itmes at the end.
     */
    STREAMV_LOCK;
    for (pSV = _streamvec_tail; pSV != NULL; pSV = pSV->pPrev)
    {
        if (pSV->cFree > 0)
        {
            for (i = 0; i < pSV->cFiles; ++i)
            {
                if (!(pSV->aFiles[i]._flags & (_IOOPEN|_IONEW)))
                {
                    pSV->aFiles[i]._flags = _IONEW;
                    pSV->cFree--;
                    STREAMV_UNLOCK;

                    /*
                     * We zero all members up till the __pSV member
                     * before returning the handle to the caller.
                     */
                    bzero(&pSV->aFiles[i], offsetof(FILE, __pSV));
                    pSV->aFiles[i].__uVersion = _FILE_STDIO_VERSION;
                    return &pSV->aFiles[i];
                }
            }
        }
    }
    STREAMV_UNLOCK;

    /*
     * Enlarge our table.
     * (As the entries must not move, we add another segment.)
     */
    pSV = _hcalloc(1, sizeof(*pSV) + 15
                   + INC * sizeof(FILE)
                   + 15
                   );
    if (pSV)
    {
        FILE *pFile;

        /*
         * Init the structures in the segment.
         * Remember everything is ZEROed by calloc()!
         */
        pSV->aFiles = pFile = (FILE *)ALIGN(pSV + 1, 16);
        pSV->aFiles         = pFile;
        pSV->cFiles         = INC;
        pSV->cFree          = INC - 1;
        for (i = 0; i < INC; i++, pFile++)
            pFile->__pSV = pSV;

        /*
         * Allocate the first one.
         */
        pFile = &pSV->aFiles[0];
        pFile->_flags = _IONEW;
        pFile->__uVersion = _FILE_STDIO_VERSION;

        /*
         * Insert the new chunk.
         */
        STREAMV_LOCK;
        pSV->pPrev = _streamvec_tail;
        /* pNext is NULLed by calloc(). */
        if (_streamvec_tail)
            _streamvec_tail = _streamvec_tail->pNext = pSV;
        else
            /* Someone fopen'ed something before streams were initated. */
            _streamvec_head = _streamvec_tail = pSV;
        STREAMV_UNLOCK;
        return pFile;
    }

    return NULL;
}


/**
 * Closes a stream freeing the associated mutex semaphore.
 *
 * @param   stream  Stream to close.
 */
void _closestream(FILE *stream)
{
    if (stream->__uVersion == _FILE_STDIO_VERSION)
    {
        stream->_flags = 0;
        /* Assumption hare made about how the dummy stuff is made. */
        if (stream->__u.__fsem.hev)
            _fmutex_close(&stream->__u.__fsem);

        if (stream->__pSV)
            ((struct streamvec *)stream->__pSV)->cFree++;
    }
}
