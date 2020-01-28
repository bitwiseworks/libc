/* emx/io.h (emx+gcc) */

#ifndef _EMX_IO_H
#define _EMX_IO_H

#include <sys/cdefs.h>
#include <sys/types.h>
#include <InnoTekLIBC/fork.h>
#include <alloca.h>
#include <stdio.h>

__BEGIN_DECLS

#if !defined (NULL)
#if defined (__cplusplus)
#define NULL 0
#else
#define NULL ((void *)0)
#endif
#endif

/** @defgroup   libc_ioflags    Low Level I/O Flags
 *
 * These low level I/O flags are kept in the fFlags member of the LIBCFH
 * structure. The O_* flags are defined in sys/fcntl.h, the F_* flags are
 * internal to LIBC and defined in emx/io.h (they should've been decorated
 * with the usual prefix but it's already EMX legacy), and the FD_* flag(s)
 * is(/are) defined
 *
 * @{
 */
/*      O_RDONLY    0x00000000 */
/*      O_WRONLY    0x00000001 */
/*      O_RDWR      0x00000002 */
/*      O_ACCMODE   0x00000003 */
/*      O_NONBLOCK  0x00000004 */
/*      O_APPEND    0x00000008 */
/*      reserved    0x00000010 O_SHLOCK */
/*      reserved    0x00000020 O_EXLOCK */
/*      reserved    0x00000040 O_ASYNC  */
/*      O_(F)SYNC   0x00000080 */
/*      O_NOFOLLOW  0x00000100 */
/*      O_CREAT     0x00000200 */
/*      O_TRUNC     0x00000400 */
/*      O_EXCL      0x00000800 */
/* emx  O_NOINHERIT 0x00001000 */
/*      free?       0x00002000 */
/*      free?       0x00004000 */
/*      O_NOCTTY    0x00008000 */
/*      O_DIRECT    0x00010000 */
/* emx  O_BINARY    0x00020000 */
/* emx  O_TEXT      0x00040000 */
/* emx  O_SIZE      0x00080000 */
#define F_EOF       0x00100000
#define F_TERMIO    0x00200000
#define F_WRCRPEND  0x00400000
#define F_CRLF      0x00800000
/** Type - Regular file. */
#define F_FILE      0x01000000
/** Type - Characater device. */
#define F_DEV       0x02000000
/** Type - Pipe. */
#define F_PIPE      0x03000000
/** Type - Socket. */
#define F_SOCKET    0x04000000
/** Type - Directory. */
#define F_DIR       0x05000000
/*      FD_CLOEXEC  0x10000000 (when shifted) */
/** The shift for the file descriptor part of __LIBC_FH::fFlags. */
#define __LIBC_FH_FDFLAGS_SHIFT     28

/** File status/open flag mask. */
#define __LIBC_FH_OFLAGS_MASK       0x00ffffff
/** File handle type mask. */
#define __LIBC_FH_TYPEMASK          0x0f000000
/** File descriptor flags mask. */
#define __LIBC_FH_FDFLAGS_MASK      0xf0000000

/** The mask of flags settable using fcntl(,F_SETFL,). */
#define __LIBC_FH_SETFL_MASK        (O_NONBLOCK | O_APPEND /*| O_ASYNC*/ | O_SYNC | O_DIRECT | O_BINARY | O_TEXT)
/** The mask of flags gettable using fcntl(,F_GETFL,). */
#define __LIBC_FH_GETFL_MASK        (__LIBC_FH_SETFL_MASK | O_ACCMODE | O_NOINHERIT)

/** @} */

/* stdio */

/*      _IOREAD     0x00000001 */
/*      _IOWRT      0x00000002 */
/*      _IORW       0x00000004 */
/*      _IOEOF      0x00000008 */
/*      _IOERR      0x00000010 */
/*      _IOLBF      0x00000020 */
/*      _IONBF      0x00000040 */

/* This bit is set if the stream is open. */

#define _IOOPEN     0x00000080

/* Mask for the buffer type. */

#define _IOBUFMASK  0x00000700

/* This buffer type is set until a buffer has been assigned. */

#define _IOBUFNONE  0x00000000

/* This buffer type is set by setvbuf() if a user-allocated buffer is
   used. */

#define _IOBUFUSER  0x00000100

/* This buffer type is set by _fbuf() if a single-character buffer is
   used. */

#define _IOBUFCHAR  0x00000200

/* This buffer type is set by _fbuf() if the stream buffer has been
   allocated by the library, with malloc().  The buffer will be
   deallocated by fclose(). */

#define _IOBUFLIB   0x00000300

/* This buffer type is set by _tmpbuf1() to indicate that a temporary
   buffer has been assigned to the stream. */

#define _IOBUFTMP   0x00000400

/* This bit is set by tmpfile() to indicate a temporary file, to be
   deleted by fclose(). */

#define _IOTMP      0x00000800

/* This bit is set for special streams which don't have an underlying
   file. */

#define _IOSPECIAL  0x00001000

/* This bit is set by ungetc() to indicate that pushed-back characters
   have been stored to the stream buffer -- fseek() must cause the
   buffer to be reread. */

#define _IOUNGETC   0x00002000

/* This bit is set by _newstream() to avoid reusing the same slot in
   another thread. */

#define _IONEW      0x00004000

/* This bit is set for byte-oriented streams. */

#define _IOBYTE     0x00008000

/* This bit is set for wide-oriented streams. */

#define _IOWIDE     0x00010000

/** This bit is set for standard streams which should not be closed by _fcloseall(). */
#define _IONOCLOSEALL   0x00020000

/** This bit is set when the stream is locked by flockfile() or ftrylockfile().
 * !!TEMPORARY HACK!!
 * @todo replace the fmutex on the stream with a recursive lock! */
#define _IOLOCKED       0xff000000

#define _FLUSH_FLUSH  (-1)
#define _FLUSH_FILL   (-2)

#define nbuf(s) (((s)->_flags & _IOBUFMASK) == _IOBUFNONE)
#define cbuf(s) (((s)->_flags & _IOBUFMASK) == _IOBUFCHAR)
#define ubuf(s) (((s)->_flags & _IOBUFMASK) == _IOBUFUSER)
#define lbuf(s) (((s)->_flags & _IOBUFMASK) == _IOBUFLIB)
#define tbuf(s) (((s)->_flags & _IOBUFMASK) == _IOBUFTMP)

#define bbuf(s) (ubuf (s) || lbuf (s) || tbuf (s))

#define _tmpbuf(s,b) (nbuf (s) || cbuf (s) \
                      ? b = alloca (BUFSIZ), _tmpbuf1 (s, b) : 0)
#define _endbuf(s) (tbuf (s) ? _endbuf1 (s) : 0)

struct streamvec
{
    /** Number of free entries in the vector. */
    int               cFree;
    /** Pointer to the previous node in the list. */
    struct streamvec *pPrev;
    /** Array of file stream structure. */
    struct __sFILE     *aFiles;
    /** Pointer to the next entry in the list */
    struct streamvec *pNext;
    /** Number of entries in the vector. */
    int               cFiles;
};

struct fdvec
{
  int *flags;
  int *lookahead;
  struct fdvec *next;
  int n;
};

extern struct streamvec    *_streamvec_head;
extern struct streamvec    *_streamvec_tail;
extern int                  _io_ninherit;
extern struct fdvec         _fdvec_head;

#if defined (_SYS_FMUTEX_H)
/* This semaphore (defined in app/stdio.c) protects _streamv[].  Only
   concurrent access by _newstream(), _setmore(), and freopen() in
   different threads must be prevented to avoid using one element of
   _streamv[] for multiple streams (in different threads).  All other
   functions may concurrently access _streamv[], even concurrently to
   _newstream(), _setmore(), and freopen(). */

extern _fmutex _streamv_fmutex;

#define STREAMV_LOCK    _fmutex_checked_request(&_streamv_fmutex, _FMR_IGNINT)
#define STREAMV_UNLOCK  _fmutex_checked_release(&_streamv_fmutex)
#endif /* _SYS_FMUTEX_H */

#if defined (__FILE_FSEM_DECLARED)

int __stream_abort(struct __sFILE *f, const char *pszMsg);

static int __inline__ stream_validate(struct __sFILE *stream)
{
    if (stream->__uVersion != _FILE_STDIO_VERSION)
    {
        __stream_abort(stream, "version error");
        return 0;
    }
    return 1;
}

static int __inline__ stream_lock(struct __sFILE *stream)
{
    if (!stream_validate(stream))
        return 0;
    if (   !(stream->_flags & _IOLOCKED)
        || !_fmutex_is_owner(&stream->__u.__fsem))
    {
        if (_fmutex_request(&stream->__u.__fsem, _FMR_IGNINT) != 0)
        {
            __stream_abort(stream, "fmutex_request failed");
            return 0;
        }
        stream->_flags = 0x01000000 | (stream->_flags & ~_IOLOCKED);
    }
    else
        stream->_flags = ((stream->_flags & _IOLOCKED) + 0x01000000) | (stream->_flags & ~_IOLOCKED);
    return 1;
}

static int __inline__ stream_trylock(struct __sFILE *stream)
{
    if (!stream_validate(stream))
        return 0;
    if (   !(stream->_flags & _IOLOCKED)
        || !_fmutex_is_owner(&stream->__u.__fsem))
    {
        if (_fmutex_request(&stream->__u.__fsem, _FMR_NOWAIT) != 0)
        {
            __stream_abort(stream, "fmutex_request failed");
            return 0;
        }
        stream->_flags = 0x01000000 | (stream->_flags & ~_IOLOCKED);
    }
    else
        stream->_flags = ((stream->_flags & _IOLOCKED) + 0x01000000) | (stream->_flags & ~_IOLOCKED);
    return 1;
}

static int __inline__ stream_unlock(struct __sFILE *stream)
{
    if (!stream_validate(stream))
        return 0;
    if (!(stream->_flags & _IOLOCKED))
    {
        __stream_abort(stream, "unlock, 0 count");
        return 0;
    }
#ifdef __LIBC_STRICT
    if (!_fmutex_is_owner(&stream->__u.__fsem))
    {
        __stream_abort(stream, "unlock, not owner");
        return 0;
    }
#endif
    stream->_flags = ((stream->_flags & _IOLOCKED) - 0x01000000) | (stream->_flags & ~_IOLOCKED);
    if (   !(stream->_flags & _IOLOCKED)
        && _fmutex_release(&stream->__u.__fsem) != 0)
    {
        __stream_abort(stream, "fmutex_release failed");
        return 0;
    }
    return 1;
}

#define STREAM_LOCK(f)          stream_lock(f)
#define STREAM_UNLOCK(f)        stream_unlock(f)
#define STREAM_LOCK_NOWAIT(f)   stream_trylock(f)
#define STREAM_UNLOCKED(f)      ((f)->__uVersion == _FILE_STDIO_VERSION && _fmutex_available(&(f)->__u.__fsem))

#endif /* __FILE_FSEM_DECLARED */

struct __libc_FileHandle;

/**
 * Information struct about a mounted file system.
 *
 * Used to track down the filesystem of an open file handle and
 * so we can optimize certain operations like expanding a file.
 */
typedef struct __libc_FileSystemInfo
{
    /** Number of references to this file system info object.
     * The structure can be shared by many handles. */
    volatile int32_t    cRefs;
    /** Does the file system automatically zero the new space when a file is extended? */
    unsigned            fZeroNewBytes : 1;
    /** Does the file system provide sufficient EA support for UNIX attributes? */
    unsigned            fUnixEAs : 1;
    /** _PC_CHOWN_RESTRICTED - Is chown and chgrp restricted the typical unix way. */
    unsigned            fChOwnRestricted : 1;
    /** _PC_NO_TRUNC - Indicates whether too long names will cause errors or be truncated. */
    unsigned            fNoNameTrunc : 1;
    /** _PC_FILESIZEBITS - The bitsize of the type required to store the max file size. */
    unsigned            cFileSizeBits : 7;
    /** _PC_PATH_MAX - The maximum path length. */
    unsigned short      cchMaxPath;
    /** _PC_NAME_MAX - The maximum path length. */
    unsigned short      cchMaxName;
    /** _PC_SYMLINK_MAX - The maximum symlink length. */
    unsigned short      cchMaxSymlink;
    /** _PC_LINK_MAX - The maximum number of hard links per file. */
    unsigned short      cMaxLinks;
    /** _PC_MAX_CANON - The maximum number of bytes in a terminal canonical input queue. */
    unsigned short      cchMaxTermCanon;
    /** _PC_MAX_INPUT - The maximum number of bytes in a canonical input queue. */
    unsigned short      cchMaxTermInput;
    /** _PC_PIPE_BUF  - The maximum number of bytes that is guaranteed to be atomic when writing to a pipe. */
    unsigned            cbPipeBuf;
    /** _PC_ALLOC_SIZE_MIN - The (smallest) block allocation size of the file system.  */
    unsigned            cbBlock;
    /** _PC_REC_XFER_ALIGN - The recommended buffer alignment for transfers. */
    unsigned short      uXferAlign;
    /** _PC_REC_INCR_XFER_SIZE - The increments to walk between the min and max transfer sizes. */
    unsigned short      cbXferIncr;
    /** _PC_REC_MAX_XFER_SIZE  - The maximum recommended transfer size. */
    unsigned long       cbXferMax;
    /** _PC_REC_MIN_XFER_SIZE  - The minimum recommended transfer size. */
    unsigned long       cbXferMin;
    /** Device number of the device the filesystem resides on.
     * On OS/2 the device number is derived from the driveletter. */
    dev_t               Dev;
    /** The filesystem driver name. */
    char                szName[16];
    /** The mount point - may extend beyond the 4 bytes on some OSes. */
    char                szMountpoint[4];
} __LIBC_FSINFO;
/** Pointer to information about an open filesystem. */
typedef __LIBC_FSINFO *__LIBC_PFSINFO;


/**
 * Filehandle type.
 */
typedef enum __libc_FileHandleType
{
    /** Anything which is supported by the OS/2 file API.
     * (not used at present as those handles doesn't need special ops). */
    enmFH_File,
    /** Socket handle (BSD 4.3 stack). */
    enmFH_Socket43,
    /** Socket handle (BSD 4.4 stack). */
    enmFH_Socket44,
    /** Directory handle. */
    enmFH_Directory
} __LIBC_FHTYPE;

/**
 * File handle Operations.
 * (for non standard handles (like sockets))
 */
typedef struct __libc_FileHandleOperations
{
    /** Handle type. */
    __LIBC_FHTYPE       enmType;
    /** Close operation.
     * @returns 0 on success.
     * @returns OS/2 error code or negated errno on failure.
     * @param   pFH         Pointer to the handle structure to operate on.
     * @param   fh          It's associated filehandle.
     */
    int (*pfnClose)(struct __libc_FileHandle *pFH, int fh);
    /** Read operation.
     * @returns 0 on success.
     * @returns OS/2 error code or negated errno on failure.
     * @param   pFH         Pointer to the handle structure to operate on.
     * @param   fh          It's associated filehandle.
     * @param   pvBuf       Pointer to the buffer to read into.
     * @param   cbRead      Number of bytes to read.
     * @param   pcbRead     Where to store the count of bytes actually read.
     */
    int (*pfnRead)(struct __libc_FileHandle *pFH, int fh, void *pvBuf, size_t cbRead, size_t *pcbRead);
    /** Write operation.
     * @returns 0 on success.
     * @returns OS/2 error code or negated errno on failure.
     * @param   pFH         Pointer to the handle structure to operate on.
     * @param   fh          It's associated filehandle.
     * @param   pvBuf       Pointer to the buffer which contains the data to write.
     * @param   cbWrite     Number of bytes to write.
     * @param   pcbWritten  Where to store the count of bytes actually written.
     */
    int (*pfnWrite)(struct __libc_FileHandle *pFH, int fh, const void *pvBuf, size_t cbWrite, size_t *pcbWritten);
    /** Duplicate handle operation.
     * @returns 0 on success, OS/2 error code on failure.
     * @param   pFH         Pointer to the handle structure to operate on.
     * @param   fh          It's associated filehandle.
     * @param   pfhNew      Where to store the duplicate filehandle.
     *                      The input value describe how the handle is to be
     *                      duplicated. If it's -1 a new handle is allocated.
     *                      Any other value will result in that value to be
     *                      used as handle. Any existing handle with that
     *                      value will be closed.
     */
    int (*pfnDuplicate)(struct __libc_FileHandle *pFH, int fh, int *pfhNew);
    /** File Control operation.
     * @returns 0 on success.
     * @returns OS/2 error code or negated errno on failure.
     * @param   pFH         Pointer to the handle structure to operate on.
     * @param   fh          It's associated filehandle.
     * @param   iRequest    Which file file descriptior request to perform.
     * @param   iArg        Argument which content is specific to each
     *                      iRequest operation.
     * @param   prc         Where to store the value which upon success is
     *                      returned to the caller.
     */
    int (*pfnFileControl)(struct __libc_FileHandle *pFH, int fh, int iRequest, int iArg, int *prc);
    /** I/O Control operation.
     * @returns 0 on success.
     * @returns OS/2 error code or negated errno on failure.
     * @param   pFH         Pointer to the handle structure to operate on.
     * @param   fh          It's associated filehandle.
     * @param   iIOControl  Which I/O control operation to perform.
     * @param   iArg        Argument which content is specific to each
     *                      iIOControl operation.
     * @param   prc         Where to store the value which upon success is
     *                      returned to the caller.
     */
    int (*pfnIOControl)(struct __libc_FileHandle *pFH, int fh, int iIOControl, int iArg, int *prc);
    /** Select operation.
     * The select operation is only performed if all handles have the same
     * select routine (the main worker checks this).
     *
     * @returns 0 on success.
     * @returns OS/2 error code or negated errno on failure.
     * @param   cFHs        Range of handles to be tested.
     * @param   pRead       Bitmap for file handles to wait upon to become ready for reading.
     * @param   pWrite      Bitmap for file handles to wait upon to become ready for writing.
     * @param   pExcept     Bitmap of file handles to wait on (error) exceptions from.
     * @param   tv          Timeout value.
     * @param   prc         Where to store the value which upon success is
     *                      returned to the caller.
     */
    int (*pfnSelect)(int cFHs, struct fd_set *pRead, struct fd_set *pWrite, struct fd_set *pExcept, struct timeval *tv, int *prc);
    /** Fork notification - parent context.
     * If NULL it's assumed that no notifiction is needed.
     *
     * @returns 0 on success.
     * @returns OS/2 error code or negated errno on failure.
     * @param   pFH             Pointer to the handle structure to operate on.
     * @param   fh              It's associated filehandle.
     * @param   pForkHandle     The fork handle.
     * @param   enmOperation    The fork operation.
     */
    int (*pfnForkParent)(struct __libc_FileHandle *pFH, int fh, __LIBC_PFORKHANDLE pForkHandle, __LIBC_FORKOP enmOperation);
    /** Fork notification - child context.
     * Only the __LIBC_FORK_OP_FORK_CHILD operation is forwarded atm.
     * If NULL it's assumed that no notifiction is needed.
     *
     * @returns 0 on success.
     * @returns OS/2 error code or negated errno on failure.
     * @param   pFH             Pointer to the handle structure to operate on.
     * @param   fh              It's associated filehandle.
     * @param   pForkHandle     The fork handle.
     * @param   enmOperation    The fork operation.
     */
    int (*pfnForkChild)(struct __libc_FileHandle *pFH, int fh, __LIBC_PFORKHANDLE pForkHandle, __LIBC_FORKOP enmOperation);

} __LIBC_FHOPS;
/** Pointer to file handle operations. */
typedef __LIBC_FHOPS *__LIBC_PFHOPS;
/** Pointer to const file handle operations. */
typedef const __LIBC_FHOPS *__LIBC_PCFHOPS;

/**
 * Common part of a per 'file' handle structure.
 */
typedef struct __libc_FileHandle
{
    /** Handle flags.
     * See group @ref libc_ioflags in include/emx/io.h.
     * @remark For thread safety update this atomically. */
    volatile unsigned int   fFlags;

    /** Lookahead. (whoever uses that?)
     * Previously represented by _files / *fd_vec->flags.
     * @remark For thread safety update this atomically. */
    volatile int            iLookAhead;

    /** Pointer to the operations one can perform on the handle.
     * Only for special handles not supported by the OS/2 file API. */
    __LIBC_PCFHOPS          pOps;

    /** Device number of the device containing the file.
     * The device number is also determined from the path, more correctly the
     * driveletter determins this. */
    dev_t                   Dev;
    /** Inode number of the file.
     * Since the inode number is usually calculated from the path we need to
     * determin this at handle creation time. */
    ino_t                   Inode;
    /** Pointer to the filesystem information object for the filesystem which
     * this file resides on. This might be NULL... */
    __LIBC_PFSINFO          pFsInfo;
    /** Pointer to the native path to this file as specified in the open call.
     * This is required to read the unix attributes from EAs if the file isn't
     * opened with exclusive access. */
    char                   *pszNativePath;
} __LIBC_FH;
/** Pointer to filehandle. */
typedef __LIBC_FH *__LIBC_PFH;
/* fixme!! */
#define LIBCFH __LIBC_FH
#define PLIBCFH __LIBC_PFH


int     __libc_FHEnsureHandles(int fh);
int     __libc_FHMoreHandles(void);
int     __libc_FHAllocate(int fh, unsigned fFlags, int cb, __LIBC_PCFHOPS pOps, PLIBCFH *ppFH, int *pfh);
int     __libc_FHClose(int fh);
PLIBCFH __libc_FH(int fh);
int     __libc_FHEx(int fh, __LIBC_PFH *ppFH);
int     __libc_FHSetFlags(__LIBC_PFH pFH, int fh, unsigned fFlags);


int _endbuf1 (struct __sFILE *);
void _fbuf (struct __sFILE *);
#define _fd_flags(a)        please do not use this use this! ##a
#define _fd_init(a)         please do not use this use this! ##a
#define _fd_lookahead(a)    please do not use this use this! ##a
struct __sFILE *_openstream (struct __sFILE *, const char *, const char *,
    int, int);
int _flushstream (struct __sFILE *, int);
void _closestream (struct __sFILE *);
int _fseek_unlocked (struct __sFILE *, off_t, int);
off_t _ftello_unlocked (struct __sFILE *);
int _input (struct __sFILE *, const char *, char *);
struct __sFILE *_newstream (void);
int _output (struct __sFILE *, const char *, char *);
int _woutput (struct __sFILE *, const wchar_t *, char *);
int _stream_read (int, void *, size_t);
int _stream_write (int, const void *, size_t);
int _tmpbuf1 (struct __sFILE *, void *);
int _trslash (const char *, size_t, int);
int _ungetc_nolock (int, struct __sFILE *);
int _vsopen (const char *, int, int, char *);


__END_DECLS

#endif /* not _EMX_IO_H */
