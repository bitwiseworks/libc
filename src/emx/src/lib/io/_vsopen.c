/* _vsopen.c (emx+gcc) -- Copyright (c) 1990-1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdarg.h>
#include <io.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <emx/io.h>
#include <emx/syscalls.h>
#include <InnoTekLIBC/backend.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_IO
#include <InnoTekLIBC/logstrict.h>

/* Bugs: O_TRUNC|O_RDONLY not implemented - bird: rejecting it makes more sense, just ask the OpenBSD guys. */
/*       O_TEXT|O_WRONLY  does/can not overwrite Ctrl-Z */

int _fmode_bin = 0;                     /* Set non-zero to make binary mode default */

#define SH_MASK           0x70

/**
 * This function may be called as follows (assuming that fOpen does
 *  not include O_CREAT nor O_SIZE):
 *
 *  _vsopen(pszName, fOpen, fShare)
 *  _vsopen(pszName, fOpen | O_CREAT, fShare, pmode)
 *  _vsopen(pszName, fOpen | O_SIZE, fShare, off_t size)
 *  _vsopen(pszName, fOpen | O_CREAT | O_SIZE, fShare, pmode, off_t size)
 *
 * @remark O_SIZE means a off_t sized argument not unsigned long as emxlib say.
 */
int _vsopen(const char *pszName, int fOpen, int fShare, va_list va)
{
    LIBCLOG_ENTER("pszName=%p:{%s} fOpen=%#x fShare=%#x ...\n", (void *)pszName, pszName, fOpen, fShare);

    /*
     * Validate input
     */
    if ((fOpen & O_ACCMODE) == O_ACCMODE)
    {
        errno = EINVAL;
        LIBCLOG_ERROR_RETURN_INT(-1);
    }

    /*
     * Build fLibc
     *  - It's pretty much fOpen but we need to remove irrelevant flags and add O_TEXT/O_BINARY and such.
     */
    unsigned fLibc = fOpen & (O_ACCMODE | O_NDELAY | O_APPEND | O_SYNC | O_DIRECT | O_NOINHERIT | O_CREAT | O_EXCL | O_TRUNC | O_NOINHERIT | O_SIZE);
    if (   !(fOpen & O_BINARY)
        && ((fOpen & O_TEXT) || !_fmode_bin))
        fLibc |= O_TEXT;
    else
        fLibc |= O_BINARY;
    if ((fLibc & (O_EXCL | O_CREAT)) == O_EXCL)
    {
        fLibc &= ~O_EXCL;               /* O_EXCL doesn't make sense without O_CREATE. */
        LIBCLOG_MSG("No O_CREAT, stripping O_EXCL.\n");
    }

    /*
     * If the caller requests to open a text file for appending in write-only
     * we have some trouble reading and stripping away any Ctrl-Z that may
     * reside at the end of the file. Thus, first open the file in O_RDWR mode
     * to do the stripping, close it, and reopen it in O_WRONLY mode.
     */
    int fCtrlZKludge = 0;
    if ((fLibc & (O_TEXT | O_APPEND | O_ACCMODE | O_EXCL)) == (O_TEXT | O_APPEND | O_WRONLY))
    {
        fCtrlZKludge = 1;
        fLibc = (fLibc & ~O_ACCMODE) | O_RDWR;
        LIBCLOG_MSG("Ctrl-Z: Applying the kludge.\n");
    }

    /*
     * The file mode is only available if O_CREAT is specified.
     */
    mode_t  Mode = 0;
    if (fLibc & O_CREAT)
    {
        Mode = va_arg(va, mode_t);
        LIBCLOG_MSG("O_CREATE: Mode=0%o\n", Mode);
    }

    /*
     * The initial file size is only available if O_SIZE is specified.
     */
    off_t cbInitial = 0;
    if (fLibc & O_SIZE)
    {
        cbInitial = va_arg(va, off_t);
        LIBCLOG_MSG("O_SIZE: cbInitial=%#llx\n", cbInitial);
    }

    /*
     * Open - note Ctrl-Z hack.
     */
    int     saved_errno = errno;
    PLIBCFH pFH;
    int     fd = __libc_Back_ioFileOpen(pszName, fLibc, fShare, cbInitial, Mode, &pFH);
    if (fd == -EACCES && fCtrlZKludge)
    {
        LIBCLOG_MSG("Ctrl-Z: Open failed, retries without the kludge.\n");
        fCtrlZKludge = 0;
        fLibc = (fLibc & ~O_ACCMODE) | O_WRONLY;
        fd = __libc_Back_ioFileOpen(pszName, fLibc, fShare, cbInitial, Mode, &pFH);
    }
    if (fd < 0)
    {
        errno = -fd;
        LIBCLOG_ERROR_RETURN_INT(-1);
    }
    fLibc = pFH->fFlags;                /* get the updated flags. */

    /*
     * Check what we got a handle to.
     */
    if (   (fLibc & O_APPEND)
        && (fLibc & __LIBC_FH_TYPEMASK) != F_FILE)
    {
        fLibc &= ~O_APPEND;
        LIBCLOG_MSG("Not a F_FILE, stripping O_APPEND!\n"); /** @todo __libc_Back_ioFileOpen should strip this! */
    }

    /*
     * For text files we shall remove the eventual Ctrl-Z at the end of the file.
     */
    if (    (fLibc & (__LIBC_FH_TYPEMASK | O_TEXT | O_BINARY)) == (F_FILE | O_TEXT)
        &&  (fLibc & O_ACCMODE) != O_RDONLY)
    {
        /* Remove the last character of the file if it is a Ctrl-Z.  Do
           this even if O_SIZE is given as we don't know whether the
           file existed before; removing a spurious Ctrl-Z due to O_SIZE
           should be harmless. */
        off_t cbSize = __libc_Back_ioSeek(fd, -1, SEEK_END);
        if (cbSize >= 0)
        {
            char chDummy = 127;
            if (    __read(fd, &chDummy, 1) == 1
                &&  chDummy == 0x1a)
            {
                int rc2 = __libc_Back_ioFileSizeSet(fd, cbSize, 0); /* Remove Ctrl-Z */
                LIBCLOG_MSG("Ctrl-Z: Strip attempt rc=%d, cbSize=%#llx\n", rc2, cbSize); (void)rc2;
            }
            else
                LIBCLOG_MSG("Ctrl-Z: Strip attempt chDummy=0x%02x, cbSize=%#llx.\n", chDummy, cbSize);
            __libc_Back_ioSeek(fd, 0, SEEK_SET);
        }
        else
            LIBCLOG_MSG("Ctrl-Z: Failed to search to end of file, cbSize=%lld\n", cbSize);
    }

    /*
     * Reopen the file in write-only mode if Ctrl-Z hack applied.
     */
    if (fCtrlZKludge)
    {
        LIBCLOG_MSG("Ctrl-Z: Reopening the file in O_WRONLY mode.\n");
        __close(fd);
        int fLibcSaved = fLibc;
        fLibc = (fLibc & ~(O_ACCMODE | O_EXCL | __LIBC_FH_TYPEMASK)) | O_WRONLY; /* Ignore exclusive open. */
        fd = __libc_Back_ioFileOpen(pszName, fLibc, fShare, cbInitial, Mode, &pFH);
        if (fd < 0)
        {
            errno = -fd;
            LIBCLOG_ERROR_RETURN_INT(-1);
        }
        if (fLibcSaved & O_EXCL)
            pFH->fFlags |= O_EXCL;
        fLibc = pFH->fFlags;
    }

    /*
     * When opening a file for appending, move to the end of the file.
     * This is required for passing the file to a child process.
     */
    if (    (fLibc & __LIBC_FH_TYPEMASK) == F_FILE
        &&  (fLibc & O_APPEND))
    {
        off_t cbSize = __libc_Back_ioSeek(fd, 0, SEEK_END);
        LIBCLOG_MSG("O_APPEND: seek cbSize=%#llx\n", cbSize); (void)cbSize;
    }

    errno = saved_errno;
    LIBCLOG_RETURN_INT(fd);
}

