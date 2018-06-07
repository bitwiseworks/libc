/* $Id: pathrewrite.h 2021 2005-06-13 02:16:10Z bird $ */
/** @file
 *
 * InnoTek LIBC - Path Rewrite.
 *
 * Copyright (c) 2004-2005 knut st. osmundsen <bird-srcspam@anduin.net>
 *
 *
 * This file is part of InnoTek LIBC.
 *
 * InnoTek LIBC is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * InnoTek LIBC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with InnoTek LIBC; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */


#ifndef __libc_InnoTekLIBC_pathrewrite_h__
#define __libc_InnoTekLIBC_pathrewrite_h__
/** @group __libc_PathReWrite
 * @{ */

/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include <sys/cdefs.h>


/*******************************************************************************
*   Structures and Typedefs                                                    *
*******************************************************************************/
/**
 * Path rewrite.
 */
typedef struct __libc_PathRewrite
{
    /** Flags.
     * If __LIBC_PRWF_CASE_SENSITIVE is not set pszFrom *must* be in all
     * lower case. */
    unsigned        fFlags;
    /** From path. Unix slashes! */
    const char     *pszFrom;
    /** Length of from path. */
    unsigned        cchFrom;
    /** To path. */
    const char     *pszTo;
    /** Length of to path. */
    unsigned        cchTo;
    /** Reserved 0 - intended for function pointer to special open. */
    unsigned        uReserved0;
    /** Reserved 1 - intended for function pointer to special parser. */
    unsigned        uReserved1;
    /** Reserved 2 - 8 byte alignment*/
    unsigned        uReserved2;
} __LIBC_PATHREWRITE, *__LIBC_PPATHREWRITE;


/** @group __libc_PathReWrite_flags
 * @{ */
/** The rule rewrites a file. */
#define __LIBC_PRWF_TYPE_FILE           0x00000001
/** The rule rewrites a directory. */
#define __LIBC_PRWF_TYPE_DIR            0x00000002

/** From path is case sensitive. */
#define __LIBC_PRWF_CASE_SENSITIVE      0x00000010
/** From path is case insensitive. */
#define __LIBC_PRWF_CASE_INSENSITIVE    0x00000000
/** @} */

__BEGIN_DECLS

/**
 * Adds a set of new rules to the rewrite rule set.
 *
 * The rules will be validated before any of them are processed
 * and added to the current rewrite rule set. Make sure the rules
 * conform to the specifications.
 *
 * The rewrites are 1:1, no nested processing.
 *
 * @returns 0 on success.
 * @returns -1 and errno set on failure.
 * @param   paRules     Pointer to an array of rules to add.
 *                      The rules will _not_ be duplicated but used until
 *                      a remove call is issued.
 * @param   cRules      Number of rules in the array.
 */
int __libc_PathRewriteAdd(const __LIBC_PPATHREWRITE paRules, unsigned cRules);


/**
 * Removes a set of rules from the current rewrite rule set.
 *
 * The specified rule array must be the eact same as for the add call.
 *
 * @returns 0 on success.
 * @returns -1 and errno set on failure.
 * @param   paRules     Pointer to an array of rules to remove.
 * @param   cRules      Number of rules in the array.
 */
int __libc_PathRewriteRemove(const __LIBC_PPATHREWRITE paRules, unsigned cRules);


/**
 * Rewrites a path using a caller supplied buffer.
 *
 * @returns 0 if no rewrite was necessary.
 * @returns length of the rewritten name including the terminating zero.
 * @returns -1 on failure, errno not set.
 * @param   pszPath     Path to rewrite.
 * @param   pszBuf      Where to store the rewritten path.
 *                      This can be NULL.
 * @param   cchBuf      Size of the buffer.
 */
int  __libc_PathRewrite(const char *pszPath, char *pszBuf, unsigned cchBuf);


__END_DECLS

/** @} */
#endif

