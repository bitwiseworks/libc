/* $Id: cabsl.c 3889 2014-06-28 16:06:20Z bird $ */
/** @file
 * kLibC - Implementation of _path() and _path2().
 *
 * @copyright   Copyright (C) 2014 knut st. osmundsen <bird-klibc-spam-xiv@anduin.net>
 * @licenses    MIT, BSD2, BSD3, BSD4, LGPLv2.1, LGPLv3, LGPLvFuture.
 */


/*******************************************************************************
* Header Files                                                                 *
*******************************************************************************/
#include "libc-alias.h"
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/cdefs.h>
#include <sys/syslimits.h>


/**
 * Find an executable file using the PATH variables.
 *
 * If the specified filename includes a volume or/and a path, the environment
 * variables will not be searched.  Instead the file name is taken as it is and
 * checked whether it is a file we can execute.
 *
 * If no volume or path is specified in the filename, the EMXPATH and PATH
 * environment variables will be searched.
 *
 * @returns 0 on success, -1 and errno (ENOENT, EISDIR, EINVAL, EOVERFLOW) on
 *          failure.
 * @param   pszName     The file to search for.
 * @param   pszSuffixes Semicolon separated list of suffixes to append to
 *                      the filename while searching.  Only done if the
 *                      specified filename does not already end with one of the
 *                      specified extensions.  Pass NULL to skip this feature.
 * @param   pszDst      Where to store the result if found.  This may be used
 *                      for temporary buffering while searching.  It will be an
 *                      empty string on failure.
 * @param   cbDst       The size of the destination buffer.
 *
 * @since   kLIBC v0.6.6
 */
int _path2(const char *pszName, const char *pszSuffixes, char *pszDst, size_t cbDst)
{
    int rc;

    /*
     * Validate the input.
     */
    if (   cbDst > 1
        && pszDst
        && (!pszSuffixes || *pszSuffixes)
        && pszName)
    {
        /*
         * Path or volume specified?
         */
        size_t cchName = strlen(pszName);
        if (   memchr(pszName, __KLIBC_PATH_SLASH, cchName) != NULL
#ifdef __KLIBC_PATH_SLASH_ALT
            || memchr(pszName, __KLIBC_PATH_SLASH_ALT,  cchName) != NULL
#endif
#ifdef __KLIBC_PATH_HAVE_DRIVE_LETTERS
            || (cchName >= 2 && pszName[1] == ':')
#endif
           )
        {
            if (cchName < cbDst)
            {
                memcpy(pszDst, pszName, cchName + 1);
                rc = _searchenv2_one_file(pszDst, cbDst, cchName, _SEARCHENV2_F_EXEC_FILE,
                                          _searchenv2_has_suffix(pszName, cchName, pszSuffixes) ? NULL : pszSuffixes);
            }
            else
            {
                errno = EOVERFLOW;
                rc = -1;
            }
        }
        /*
         * No path or volume, search the environment variables.
         */
        else
        {
            rc = _searchenv2("EMXPATH", pszName, _SEARCHENV2_F_EXEC_FILE, pszSuffixes, pszDst, cbDst);
            if (rc == -1 && errno == ENOENT)
                rc = _searchenv2("PATH", pszName, _SEARCHENV2_F_EXEC_FILE | _SEARCHENV2_F_SKIP_CURDIR,
                                 pszSuffixes, pszDst, cbDst);
        }
    }
    else
    {
        errno = EINVAL;
        rc = -1;
    }
    return rc;
}


/**
 * Find an executable file using the PATH variables (insane version).
 *
 * If the specified filename includes a volume or/and a path, the environment
 * variables will not be searched.  Instead the file name is taken as it is and
 * checked whether it is a file we can execute.
 *
 * If no volume or path is specified in the filename, the EMXPATH and PATH
 * environment variables will be searched.
 *
 * @returns 0 on success, -1 and errno (ENOENT, EISDIR, EOVERFLOW) on failure.
 * @param   pszDst      Where to store the result if found.  This may be used
 *                      for temporary buffering while searching.  It will be an
 *                      empty string on failure.  The length of the buffer is
 *                      assumed to be at least PATH_MAX.
 * @param   pszName     The file to search for.
 *
 * @since   The _path() function seems to be EMX specific.
 */
int _path(char *pszDst, const char *pszName)
{
    return _path2(pszName, NULL /*pszSuffixes*/, pszDst, PATH_MAX);
}

