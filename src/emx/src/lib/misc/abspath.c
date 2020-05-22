/* $Id: searchen.c 3904 2014-10-23 21:45:51Z bird $ */
/** @file
 * kLibC - Implementation of _abspath().
 *
 * @copyright   Copyright (C) 2014 knut st. osmundsen <bird-klibc-spam-xiv@anduin.net>
 * @licenses    MIT, BSD2, BSD3, BSD4, LGPLv2.1, LGPLv3, LGPLvFuture.
 */


/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include "libc-alias.h"
#include <stdlib.h>

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <alloca.h>
#include <sys/cdefs.h>
#include <sys/param.h>
#include <InnoTekLIBC/libc.h>
#include <InnoTekLIBC/locale.h>
#include <InnoTekLIBC/backend.h>
#ifdef __OS2__
# include <InnoTekLIBC/pathrewrite.h>
extern int  __libc_gcchUnixRoot; /* fs.c  / b_fs.h */
#endif
#include "backend.h"
#include "b_fs.h"

/** Macro that advance the a_pszSrc variable past all leading slashes. */
#define _ABSPATH_SKIP_SLASHES(a_pszSrc) \
    do { \
        char ch = *(a_pszSrc); \
        while (__KLIBC_PATH_IS_SLASH(ch)) \
            ch = *++(a_pszSrc); \
    } while (0)


/**
 * Sets errno to ERANGE and returns -1.
 *
 * Convenience for dropping three lines of code for each overflow check and
 * return.
 *
 * @returns -1
 */
static int _abspath_overflow(void)
{
    errno = ERANGE;
    return -1;
}


/**
 * Figures out the length of the directory component @a pszSrc points to.
 *
 * @returns Length in bytes.
 * @param   pszSrc      Pointer to the path component which length we're
 *                      interested in.  This should not point to a slash but may
 *                      point to the zero terminator character.
 */
static int _abspath_component_length(const char *pszSrc)
{
    char const *psz = pszSrc;
    char        ch;
    assert(!__KLIBC_PATH_IS_SLASH(*psz));

    if (__libc_GLocaleCtype.mbcs)
    {
        /* May contain multibyte chars. */
        int cchMultibyteCodepoint;
        while ((ch = *psz) != '\0')
        {
            if (   CHK_MBCS_PREFIX(&__libc_GLocaleCtype, ch, cchMultibyteCodepoint)
                && psz[1] != '\0')
                psz += cchMultibyteCodepoint;
            else if (__KLIBC_PATH_IS_SLASH(ch))
                break;
            else
                psz++;
        }
    }
    else
    {
        /* All chars are single byte. */
        while ((ch = *psz) != '\0' && !__KLIBC_PATH_IS_SLASH(ch))
            psz++;
    }
    return (int)(psz - pszSrc);
}


/**
 * Fixes the slashes of a current directory we got from the backend.
 *
 * @returns Length of the current returned directory.
 * @param   pszDst      The destination buffer which slashes we should fix.
 * @param   chSlash     The prefereed slash.
 */
static int _abspath_slashify(char *pszDst, char chSlash)
{
#ifdef __KLIBC_PATH_SLASH_ALT
    char *psz = pszDst;
    char  ch;
    if (__libc_GLocaleCtype.mbcs)
    {
        /* May contain multibyte chars. */
        int cchMultibyteCodepoint;
        while ((ch = *psz) != '\0')
        {
            if (   CHK_MBCS_PREFIX(&__libc_GLocaleCtype, ch, cchMultibyteCodepoint)
                && psz[1] != '\0')
                psz += cchMultibyteCodepoint;
            else if (__KLIBC_PATH_IS_SLASH(ch))
                *psz++ = chSlash;
            else
                psz++;
        }
    }
    else
    {
        /* All chars are single byte. */
        while ((ch = *psz) != '\0')
        {
            if (__KLIBC_PATH_IS_SLASH(ch))
                *psz = chSlash;
            psz++;
        }
    }
    return (int)(psz - pszDst);
#else
    return strlen(pszDst);
#endif
}


#ifdef __OS2__
/**
 * Checks if @a pszSrc points something that could be an OS/2 device name.
 *
 * @returns 1 if likely OS/2 device name, 0 if not.
 * @param   pszSrc          Pointer to the char after "/DEV/".
 */
static int _abspath_is_likely_device_name(const char *pszSrc)
{
    _ABSPATH_SKIP_SLASHES(pszSrc);
    size_t cch = _abspath_component_length(pszSrc);
    if (cch > 0 && cch <= 8 && !pszSrc[cch])
    {
        if (cch == 1)
            return pszSrc[0] != '.';
        if (cch == 2)
            return pszSrc[0] != '.' || pszSrc[1] != '.';
        return 1;
    }
    return 0;
}
#endif


/**
 * Internal _abspath entry with optional FS mutex locking.
 */
int __libc_abspath(char *pszDst, const char *pszSrc, int cbDst, int fLock)
{
    /*
     * Note! This code must mimick fsResolveUnix and fsResolveOS2
     *
     * Note2! The above comment is irrelevant now as this code is called
     * to process relative paths and resolve .. and . right before calling
     * fsResolveUnix and fsResolveOS2. For more info see
     * https://github.com/bitwiseworks/libc/issues/73.
     */
#ifdef __KLIBC_PATH_SLASH_ALT
    char const chSlash = !__libc_gfNoUnix ? __KLIBC_PATH_SLASH_ALT : __KLIBC_PATH_SLASH;
#else
    char const chSlash = __KLIBC_PATH_SLASH;
#endif
#ifdef __KLIBC_PATH_HAVE_DRIVE_LETTERS
    char chDrive;
#endif

    /*
     * Make a copy of the input if it overlaps the destination buffer
     * OR if the
     */
    size_t cchSrc = strlen(pszSrc);
    if (   (uintptr_t)pszDst <= (uintptr_t)pszSrc + cchSrc
        && (uintptr_t)pszDst + (uintptr_t)cbDst > (uintptr_t)pszSrc)
    {
        char *pszSrcCopy = (char *)alloca(cchSrc + 1);
        memcpy(pszSrcCopy, pszSrc, cchSrc + 1);
        pszSrc = pszSrcCopy;
    }

    /*
     * Deal with the start of the path. This will skip
     */
    int cch;
    int offDst     = 0;
#define _ABSPATH_ERANGE_RETURN_CHECK(a_cbNeeded) if (cbDst <= (a_cbNeeded)) return _abspath_overflow(); else do {} while (0)
    int offDstRoot = 0;
    /* Root slash? */
    if (__KLIBC_PATH_IS_SLASH(*pszSrc))
    {
#ifdef __KLIBC_PATH_HAVE_UNC
        /*
         * Check for and deal with UNC.
         * Note! We do not allow '..' to advance higher than the share name,
         *       i.e. "//server/share/.." becomes "//server/share/".
         */
        /** @todo Should "//server/share/.." become "//server/share" rather than "//server/share/" ?? */
        if (   __KLIBC_PATH_IS_SLASH(pszSrc[1])
            && !__KLIBC_PATH_IS_SLASH(pszSrc[2])
            && pszSrc[2])
        {
            /* The server part. */
            cch = _abspath_component_length(&pszSrc[2]);
            _ABSPATH_ERANGE_RETURN_CHECK(2 + cch);
            pszDst[0] = chSlash;
            pszDst[1] = chSlash;
            memcpy(&pszDst[2], &pszSrc[2], cch);
            offDst = 2 + cch;
            pszSrc += 2 + cch;

            /* The share/resource too, if present. */
            if (*pszSrc)
            {
                pszSrc++;
                _ABSPATH_SKIP_SLASHES(pszSrc);
                cch = _abspath_component_length(pszSrc);
                _ABSPATH_ERANGE_RETURN_CHECK(offDst + 1 + cch);
                pszDst[offDst++] = chSlash;
                memcpy(&pszDst[offDst], pszSrc, cch);
                offDst += cch;
                pszSrc += cch;
                if (*pszSrc)
                {
                    _ABSPATH_ERANGE_RETURN_CHECK(offDst + 1);
                    pszDst[offDst++] = chSlash;
                    pszSrc++;
                    _ABSPATH_SKIP_SLASHES(pszSrc);
                }
            }
            offDstRoot = offDst;
        }
        else
#endif
        {
            /* Skip leading slashes before continuing. */
            pszSrc++;
            _ABSPATH_SKIP_SLASHES(pszSrc);

            if (0)
            { /* nothing */ }
#ifdef __OS2__
            /*
             * The \pipe\ path is special on OS/2, all named pipes are invisibly
             * living under it.  May include subdirectories.  Unsure how '..' is
             * handled, but assuming the caller wants to straighten those out.
             */
            else if (   (pszSrc[0] == 'p' || pszSrc[0] == 'P')
                     && (pszSrc[1] == 'i' || pszSrc[1] == 'I')
                     && (pszSrc[2] == 'p' || pszSrc[2] == 'P')
                     && (pszSrc[3] == 'e' || pszSrc[3] == 'E')
                     && __KLIBC_PATH_IS_SLASH(pszSrc[4]) )
            {
                _ABSPATH_ERANGE_RETURN_CHECK(6);
                pszDst[0] = chSlash;
                pszDst[1] = 'P';
                pszDst[2] = 'I';
                pszDst[3] = 'P';
                pszDst[4] = 'E';
                pszDst[5] = chSlash;
                offDst = 6;
                pszSrc += 5;
                _ABSPATH_SKIP_SLASHES(pszSrc);
            }
            /*
             * The \dev\ path is also special on OS/2, all devices are visibly
             * (via stat but not readdir) living under it.  Names are limited
             * to 8 chars and there should not be any subdirs AFAIK.  Since we
             * may misidentify kNIX pseudo device path here, we do not
             * uppercase the prefix like we did for the pipes.
             */
            else if (   (pszSrc[0] == 'd' || pszSrc[0] == 'D')
                     && (pszSrc[1] == 'e' || pszSrc[1] == 'E')
                     && (pszSrc[2] == 'v' || pszSrc[2] == 'V')
                     && __KLIBC_PATH_IS_SLASH(pszSrc[3])
                     && _abspath_is_likely_device_name(&pszSrc[4]))
            {
                _ABSPATH_ERANGE_RETURN_CHECK(5);
                pszDst[0] = chSlash;
                pszDst[1] = pszSrc[0];
                pszDst[2] = pszSrc[1];
                pszDst[3] = pszSrc[2];
                pszDst[4] = chSlash;
                offDst = 5;
                pszSrc += 4;
                _ABSPATH_SKIP_SLASHES(pszSrc);
            }
            /*
             * If the path rewriter triggers on the path, we will not prefix
             * it with the current driver letter.
             */
            else if (__libc_PathRewrite(pszSrc - 1, NULL, 0) > 0)
            {
                _ABSPATH_ERANGE_RETURN_CHECK(1);
                pszDst[0] = chSlash;
                offDst = 1;
            }
#endif /* __OS2__ */
#ifdef __KLIBC_PATH_HAVE_DRIVE_LETTERS
            /*
             * Add the current drive letter before the root slash, unless we've
             * done chroot() or similar that means '/' refers to the unixroot
             * instead of the root of the current drive.
             */
            else if (   __libc_gcchUnixRoot == 0
                     && (chDrive = _getdrive()) > 0) /** @todo UNC CWD */
            {
                _ABSPATH_ERANGE_RETURN_CHECK(3);
                pszDst[0] = chDrive;
                pszDst[1] = ':';
                pszDst[2] = chSlash;
                offDst = 3;
            }
#endif
            /*
             * Unix root slash.
             */
            else
            {
                _ABSPATH_ERANGE_RETURN_CHECK(1);
                pszDst[0] = chSlash;
                offDst = 1;
            }
            offDstRoot = offDst;
        }
    }
#ifdef __KLIBC_PATH_HAVE_DRIVE_LETTERS
    /*
     * Drive letter?
     */
    else if (   pszSrc[0] != '\0'
             && pszSrc[1] == ':'
             && (chDrive = _fngetdrive(pszSrc)) )
    {
        if (__KLIBC_PATH_IS_SLASH(pszSrc[2]))
        {
            /*
             * Absolute path, great :-).
             */
            _ABSPATH_ERANGE_RETURN_CHECK(3);
            pszDst[0] = chDrive;
            pszDst[1] = ':';
            pszDst[2] = chSlash;
            offDstRoot = offDst = 3;
            pszSrc += 3;
            _ABSPATH_SKIP_SLASHES(pszSrc);
        }
        else
        {
            /*
             * Drive letter relative path.  Try add the current directory for
             * that drive.  If we cannot get it (drive letter exists or
             * something, return the relative path.
             */
            pszSrc += 2;
            int rc = fLock ? __libc_Back_fsDirCurrentGet(pszDst, cbDst, chDrive, 0 /*fFlags*/)
                           : __libc_back_fsDirCurrentGet(pszDst, cbDst, chDrive, 0 /*fFlags*/);
            if (rc == -ERANGE)
                return _abspath_overflow();
            if (rc == 0)
            {
                offDst = _abspath_slashify(pszDst, chSlash);
                if (*pszSrc && pszDst[offDst - 1] != chSlash)
                {
                    _ABSPATH_ERANGE_RETURN_CHECK(offDst + 1);
                    pszDst[offDst++] = chSlash;
                }
                offDstRoot = 3;
            }
            else
            {
                pszDst[0] = chDrive;
                pszDst[1] = ':';
                offDstRoot = offDst = 2;
            }
        }
    }
#endif
    /*
     * The path is relative to the current directory, so try add it.
     */
    else if (*pszSrc)
    {
        int rc = fLock ? __libc_Back_fsDirCurrentGet(pszDst, cbDst, '\0' /* current drive */, 0 /*fFlags*/)
                       : __libc_back_fsDirCurrentGet(pszDst, cbDst, '\0' /* current drive */, 0 /*fFlags*/);
        if (rc == -ERANGE)
            return _abspath_overflow();
        if (rc == 0)
        {
            offDst = _abspath_slashify(pszDst, chSlash);
            if (pszDst[offDst - 1] != chSlash)
            {
                _ABSPATH_ERANGE_RETURN_CHECK(offDst + 1);
                pszDst[offDst++] = chSlash;
            }

#ifdef __KLIBC_PATH_HAVE_DRIVE_LETTERS
            if (_fngetdrive(pszDst))
                offDstRoot = __KLIBC_PATH_IS_SLASH(pszDst[2]) ? 3 : 2;
            else
#endif
#ifdef __KLIBC_PATH_HAVE_UNC
            if (   __KLIBC_PATH_IS_SLASH(pszDst[0])
                && __KLIBC_PATH_IS_SLASH(pszDst[1]))
            {
                offDstRoot = 2 + _abspath_component_length(&pszSrc[2]);
                if (pszDst[offDstRoot])
                    offDstRoot += 1 + _abspath_component_length(&pszSrc[offDstRoot]);
            }
            else
#endif
                offDstRoot = 1;
        }
        else
            offDstRoot = offDst = 0;
    }
    /*
     * Input is empty. Return empty path and failure.
     */
    else
    {
        if (cbDst)
            *pszDst = '\0';
        errno = EINVAL;
        return -1;
    }

    /*
     * Straighten out '.', '..' and slashes.  There shall be no slash at the
     * head of the input string at this point, that way we can more easily
     * handle the trailing directory slash correctly.  It is also assumed that
     * offDstRoot - 1 is a slash or similar, so we can back up all the way to
     * offDstRoot without needing to thing about slashes.
     */
    int offParentDir = -1;
    for (;;)
    {
        assert(!__KLIBC_PATH_IS_SLASH(*pszSrc));
        cch = _abspath_component_length(pszSrc);
        if (!cch)
            break;

        /*
         * "." - No change.
         */
        if (cch == 1 && pszSrc[0] == '.')
        {
            pszSrc++;
            if (!*pszSrc)
            {
                if (offDst > offDstRoot)
                    offDst--; /* No trailing slash. */
                break;
            }
            pszSrc++;
        }
        /*
         * ".." - Drop the last directory in the destination path (unless we're
         * constructing a relative path because we couldn't for some reason or
         * other resolve the current directory).
         */
        else if (cch == 2 && pszSrc[0] == '.' && pszSrc[1] == '.' && offDstRoot != 0)
        {
            if (offDst > offDstRoot)
            {
                if (offParentDir > 0)
                    offDst = offParentDir;
                else
                {
                    pszDst[offDst - 1] = '\0'; /* terminate and drop trailing slash. */
                    offDst = (int)(_getname(&pszDst[offDstRoot]) - pszDst);
                }
            }
            /* else: Hit the root, it's ".." link doesn't go anywhere. So ignore it. */
            offParentDir = -1;

            pszSrc += 2;
            if (!*pszSrc)
            {
                if (offDst > offDstRoot)
                    offDst--; /* No trailing slash. */
                break;
            }
            pszSrc++;
        }
        /*
         * Append the component and trailing slash, if any.
         */
        else
        {
            offParentDir = offDst; /* Cache the last dir offset for efficient '..' handling. */
            if (!pszSrc[cch])
            {
                _ABSPATH_ERANGE_RETURN_CHECK(offDst + cch);
                memcpy(&pszDst[offDst], pszSrc, cch);
                offDst += cch;
                break;
            }

            _ABSPATH_ERANGE_RETURN_CHECK(offDst + cch + 1);
            memcpy(&pszDst[offDst], pszSrc, cch);
            offDst += cch;
            pszDst[offDst++] = chSlash;
            pszSrc += cch + 1;
        }

        /* Skip extra slashes before we go on to the next component or reach
           the end of the string. */
        _ABSPATH_SKIP_SLASHES(pszSrc);
    }

    /*
     * Add the terminator and return.
     */
    if (offDst < cbDst)
    {
        pszDst[offDst] = '\0';
        return 0;
    }
    return _abspath_overflow();
}


int _abspath(char *pszDst, const char *pszSrc, int cbDst)
{
    return __libc_abspath(pszDst, pszSrc, cbDst, 1);
}
