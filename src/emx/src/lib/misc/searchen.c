/* $Id: cabsl.c 3889 2014-06-28 16:06:20Z bird $ */
/** @file
 * kLibC - Implementation of _searchenv() and _searchenv2().
 *
 * @copyright   Copyright (C) 2014 knut st. osmundsen <bird-klibc-spam-xiv@anduin.net>
 * @licenses    MIT, BSD2, BSD3, BSD4, LGPLv2.1, LGPLv3, LGPLvFuture.
 */


/*******************************************************************************
* Header Files                                                                 *
*******************************************************************************/
#include "libc-alias.h"
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <emx/io.h>
#include <sys/cdefs.h>
#include <sys/fcntl.h>
#include <sys/syslimits.h>


/**
 * Worker for _searchenv2_value() and _path2() that looks for a file in the
 * given path.
 *
 * @returns 0 on success. -1 is returned on failure together with errno and an
 *          empty string in @a pszDst (if possible).
 * @errno   ENOENT if not found
 * @errno   EISDIR if found but is directory or special file.
 * @errno   EOVERFLOW if any of the suffix combinations caused us to overlow
 *          (lower priority than EISDIR).
 *
 * @param   pszDst              The input / output buffer.  This contains a
 *                              filename of length @a cchName on input.  This
 *                              filename may have a suffix from @a pszSuffixes
 *                              appended on successful return.  On failure, it
 *                              will be set to an empty string (if there is room
 *                              for it).
 * @param   cbDst               The size of the buffer @a pszDst points to.
 * @param   cchName             The length of the incoming name.
 * @param   fFlags              The search flags, see _SEARCHENV2_F_XXX.
 * @param   pszSuffixes         Semicolon separated list of suffixes to append
 *                              to the filename.  Pass NULL if no suffixes
 *                              should be appended.
 *
 * @remarks The caller is expected to have used _searchenv2_has_suffix() to
 *          check if the input filename already has an extension.
 *
 * @since   kLIBC v0.6.6
 */
int _searchenv2_one_file(char *pszDst, size_t cbDst, size_t cchName, unsigned fFlags, const char *pszSuffixes)
{
    int rc;
    if (   cchName > 0
        && cchName < cbDst)
    {
        int iRetErrno = ENOENT;
        for (;;)
        {
            /*
             * Check the accesss, making sure it's a regular file (no
             * directories or other weird files).
             */
            if (_STD(eaccess)(pszDst, fFlags & _SEARCHENV2_F_EXEC_FILE ? X_OK : R_OK) == 0)
            {
                struct stat St;
                rc = _STD(stat)(pszDst, &St);
                if (rc == 0 && S_ISREG(St.st_mode))
                    return 0;
                iRetErrno = EISDIR;
            }

            /*
             * Try another suffix?
             */
            if (!pszSuffixes)
                break;

            /* Find start and end of the next suffix (if there is one). */
            while (*pszSuffixes == ';')
                pszSuffixes++;
            if (!*pszSuffixes)
                break;
            const char *pszEnd = pszSuffixes;
            while (*pszEnd && *pszEnd != ';')
                pszEnd++;

            /* Alter the filename. */
            size_t cchSuffix = pszEnd - pszSuffixes;
            if (cchSuffix + cchName < cbDst)
            {
                _STD(memcpy)(&pszDst[cchName], pszSuffixes, cchSuffix);
                pszDst[cchName + cchSuffix] = '\0';
            }
            /* Lazy bird: Retry with the same file name on overflow. */
            else if (iRetErrno == ENOENT)
                iRetErrno = EOVERFLOW;

            /* Advance suffixes. */
            pszSuffixes = pszEnd;
        }

        /* Failed. */
        *pszDst = '\0';
        errno = iRetErrno;
        rc = -1;
    }
    else
    {
        if (cbDst > 0 && pszDst)
            *pszDst = '\0';
        errno = ENOENT;
        rc = -1;
    }

    return rc;
}


/**
 * Checks if @a pszName already has a suffix from the @a pszSuffix list.
 *
 * @returns 1 if it has a suffix from the list, 0 if not.
 *
 * @param   pszName             The filename name.
 * @param   cchName             The length of the name.
 * @param   pszSuffixes         The semicolon separated list of suffixes.  Can
 *                              be NULL.
 * @since   kLIBC v0.6.6
 */
int _searchenv2_has_suffix(const char *pszName, size_t cchName, const char *pszSuffixes)
{
    /*
     * Enumerate the suffix list (if we have one).
     */
    if (pszSuffixes)
    {
        for (;;)
        {
            /*
             * Find the start and end of the next suffix.
             */
            while (*pszSuffixes == ';')
                pszSuffixes++;
            if (!*pszSuffixes)
                break;

            const char *pszEnd = pszSuffixes;
            char ch;
            while ((ch = *pszEnd) != '\0' && ch != ';')
                pszEnd++;

            /*
             * Check if the filename has this suffix.
             */
            size_t cchSuffix = pszEnd - pszSuffixes;
            if (   cchSuffix < cchName
                && _STD(strnicmp)(pszName + cchName - cchSuffix, pszSuffixes, cchSuffix) == 0)
            {
                /** @todo check that we didn't barge into a multibyte sequence! */
                return 1;
            }

            /*
             * Next.
             */
            pszSuffixes = pszEnd;
        }
    }
    return 0;
}


/**
 * Search for a file (@a pszName) using an search path (@a pszSearchPath) and
 * optionally a list of suffixes.
 *
 * The search will first check the @a pszName without any path prepended, unless
 * _SEARCHENV2_F_SKIP_CURDIR is specified.  Then environment variable value will
 * be taken, split up into a list of paths on semicolon and colon (except drive
 * letters of course) boundraries, and each path is checked out individually in
 * the order they appear.
 *
 * @returns 0 on success. On failure -1 is returned together with errno and an
 *          empty return string.
 * @errno   EINVAL if @a pszName or @a pszDst is NULL.
 * @errno   ENOENT if not found or if @a pszName is an empty string.
 * @errno   EOVERFLOW if pszName and any suffix is longer than the buffer.
 *
 * @param   pszSearchPath       The search path.
 * @param   pszName             The name of the file we're searching for.
 * @param   fFlags              Search flags, see _SEARCHENV2_F_XXX.
 * @param   pszSuffixes         Semicolon separated list of suffixes to append
 *                              to @a pszName while searching.  If @a pszName
 *                              already include one of the suffixes in this list
 *                              (case insensitive check), no suffixes will be
 *                              appended.  Pass NULL to use @a pszName as it is.
 * @param   pszDst              Where to store the filename we find, set to empty string
 *                              if not found (errno set).
 * @param   cbDst               The size of the buffer @a pszDst points to.
 *
 *
 * @sa      _searchenv2()
 *
 * @since   kLIBC v0.6.6
 */
int _searchenv2_value(const char *pszSearchPath, const char *pszName, unsigned fFlags, const char *pszSuffixes,
                      char *pszDst, size_t cbDst)
{
    int rc;

    /*
     * Validate input.
     */
    if (   pszName
        && *pszName
        && (!pszSuffixes || *pszSuffixes)
        && pszDst
        && cbDst > 0)
    {
        /*
         * If the filename already is using one of the specified suffixes, drop
         * the suffix search.  Quicker doing this up front than for each search
         * path member.
         */
        size_t const cchName = _STD(strlen)(pszName);
        if (_searchenv2_has_suffix(pszName, cchName, pszSuffixes))
            pszSuffixes = NULL;

        /*
         * First try the file directly.  If it overflows, we won't bother
         * trying the search path since it'll just construct even longer names.
         */
        if (cchName < cbDst)
        {
            if (!(fFlags & _SEARCHENV2_F_SKIP_CURDIR))
            {
                _STD(memcpy)(pszDst, pszName, cchName + 1);
                rc = _searchenv2_one_file(pszDst, cbDst, cchName, fFlags, pszSuffixes);
                if (rc == -1 && errno == EISDIR)
                    errno = ENOENT; /* Hide EISDIR. */
            }
            else
            {
                errno = ENOENT;
                rc = -1;
            }
            if (   rc == -1
                && errno != EOVERFLOW
                && pszSearchPath)
            {
                /*
                 * Work the search path.  We respect both ';' and ':' as
                 * separators here.
                 */
                for (;;)
                {
                    /*
                     * Skip leading spaces and empty paths (we've already checked without
                     * a path above and if the caller used _SEARCHENV2_F_SKIP_CURDIR, we're
                     * still doing the correct thing as such).
                     */
                    char ch;
                    while (   (ch = *pszSearchPath) == ';'
                           || ch == ':'
                           || isblank(ch))
                        pszSearchPath++;
                    if (!ch)
                        break;

                    /*
                     * Find the end of the current path, using both colon (unix) and
                     * semicolon (DOS) as separators.
                     *
                     * The first char will not be a separator. With the second character
                     * we have to be careful with colon on platforms with drive letters.
                     */
                    const char *pszStart = pszSearchPath;
                    const char *pszEnd = ++pszSearchPath;
                    ch = *pszSearchPath;
                    if (   ch != '\0'
                        && ch != ';'
#ifdef __KLIBC_PATH_HAVE_DRIVE_LETTERS
                        && (ch != ':' || isalpha(*pszStart))
#else
                        && ch != ':'
#endif
                       )
                    {
                        do
                            pszSearchPath++;
                        while ((ch = *pszSearchPath) != '\0' && ch != ';' && ch != ':');
                        pszEnd = pszSearchPath;

                        /* Strip trailing blanks. */
                        while ((uintptr_t)pszEnd > (uintptr_t)pszStart && isblank(pszEnd[-1]))
                            pszEnd--;
                    }

                    /*
                     * Try construct a new name.
                     */
                    size_t cchPath  = pszEnd - pszStart;
                    int    cchSlash = _trslash(pszStart, cchPath, 0) == 0;
                    if (cchPath + cchSlash + cchName < cbDst)
                    {
                        _STD(memcpy)(pszDst, pszStart, cchPath);
                        if (cchSlash)
                            pszDst[cchPath++] = __KLIBC_PATH_SLASH;
                        _STD(memcpy)(&pszDst[cchPath], pszName, cchName + 1);

                        /*
                         * Test the filename + suffixes.
                         */
                        rc = _searchenv2_one_file(pszDst, cbDst, cchPath + cchName, fFlags, pszSuffixes);
                        if (!rc)
                            return rc;
                    }
                    /* else: we overflowed - ignore. */
                }

                /* Not found.  We ignore EOVERFLOW and EISDIR errors here. */
                *pszDst = '\0';
                errno = ENOENT;
                rc = -1;
            }
            /* else: maybe we found it, maybe we overflowed. */
        }
        else
        {
            /* Buffer too small for the name. */
            *pszDst = '\0';
            errno = EOVERFLOW;
            rc = -1;
        }
    }
    else
    {
        /* Invalid parameter(s). */
        if (cbDst > 0 && pszDst)
            *pszDst = '\0';
        errno = pszName && !*pszName ? ENOENT : EINVAL;
        rc = -1;
    }
    return rc;
}


/**
 * Search for a file (@a pszName) using an environment variable (@a pszEnvVar)
 * and optionally a list of suffixes.
 *
 * The search will first check the @a pszName without any path prepended, unless
 * _SEARCHENV2_F_SKIP_CURDIR is specified.  Then environment variable value will
 * be taken, split up into a list of paths on semicolon and colon (except drive
 * letters of course) boundraries, and each path is checked out individually in
 * the order they appear.
 *
 * @returns 0 on success. On failure -1 is returned together with errno and an
 *          empty return string.
 * @errno   EINVAL if @a pszName or @a pszDst is NULL.
 * @errno   ENOENT if not found or if @a pszName is an empty string.
 * @errno   EOVERFLOW if pszName and any suffix is longer than the buffer.
 *
 * @param   pszEnvVar           The environment variable.
 * @param   pszName             The name of the file we're searching for.
 * @param   fFlags              Search flags, see _SEARCHENV2_F_XXX.
 * @param   pszSuffixes         Semicolon separated list of suffixes to append
 *                              to @a pszName while searching.  If @a pszName
 *                              already include one of the suffixes in this list
 *                              (case insensitive check), no suffixes will be
 *                              appended.  Pass NULL to use @a pszName as it is.
 * @param   pszDst              Where to store the filename we find, set to empty string
 *                              if not found (errno set).
 * @param   cbDst               The size of the buffer @a pszDst points to.
 *
 * @sa      _searchenv2_value()
 * @since   kLIBC v0.6.6
 */
int _searchenv2(const char *pszEnvVar, const char *pszName, unsigned fFlags, const char *pszSuffixes,
                 char *pszDst, size_t cbDst)
{
    return _searchenv2_value(_STD(getenv)(pszEnvVar), pszName, fFlags, pszSuffixes, pszDst, cbDst);
}


/**
 * Search for a file (@a pszName) using an environment variable (@a pszEnvVar),
 * insane version.
 *
 * @errno   EINVAL if @a pszName or @a pszDst is NULL.
 * @errno   ENOENT if not found or if @a pszName is an empty string.
 * @errno   EOVERFLOW if pszName and any suffix is longer than the buffer.
 *
 * @param   pszName     The name of the file we're searching for.
 * @param   pszEnvVar   The environment variable name.
 * @param   pszDst      Where to store the filename we find, set to empty string
 *                      if not found (errno set). This buffer must be able to
 *                      hold PATH_MAX chars (including the string terminator).
 *
 * @sa      _searchenv2(), _searchenv2_value()
 *
 * @since   The _searchenv() also exists in the microsoft runtime library and
 *          probably from the DOS days common to both OS/2 and windows
 *          compilers.
 */
void _searchenv(const char *pszName, const char *pszEnvVar, char *pszDst)
{
    _searchenv2(pszEnvVar, pszName, _SEARCHENV2_F_EXEC_FILE, NULL /*pszSuffixes*/, pszDst, PATH_MAX);
}

