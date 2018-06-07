/* $Id: pathrewrite.c 3917 2014-10-24 20:02:16Z bird $ */
/** @file
 *
 * LIBC SYS Backend - Path Rewrite.
 *
 * Copyright (c) 2004 knut st. osmundsen <bird-srcspam@anduin.net>
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


/** @page pg_pathRewrite   Path Rewrite Feature
 *
 * The Path Rewrite Feature was designed as a replacement for the hardcoded
 * mapping of /dev/null to nul and /dev/tty to con in __open.c. It allows
 * the user of LIBC to automatically rewrite (map) paths given to LIBC APIs.
 * The user can dynamically add and remove rewrite rules. The environment
 * variable LIBC_HOOK_DLLS can be used to dynamically load DLLs containing
 * rewrite rules and add it to the process. See \ref pg_hooks for further
 * details.
 */

#if 0
/** This enables a testcase where we rewrite /etc. */
#define TESTCASE_ETC 1
#endif

/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include <InnoTekLIBC/pathrewrite.h>
#include <InnoTekLIBC/libc.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_BACK_IO
#include <InnoTekLIBC/logstrict.h>

#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <emx/startup.h>
#include <emx/umalloc.h>

#define INCL_BASE
#include <os2.h>


/*******************************************************************************
*   Defined Constants And Macros                                               *
*******************************************************************************/
/** @group __libc_PathReWrite_direntry_type     Directory Entry Type Flags
 * @{ */
/** Directory entry is a file. */
#define DIRENTRY_TYPE_FILE  0x01
/** Directory entry is a subdirectory. */
#define DIRENTRY_TYPE_DIR   0x02
/** @} */

/** @group __libc_PathReWrite_direntry_flags    Directory Entry Flags
 * @{ */
/** Case sensitive compare. */
#define DIRENTRY_FLAGS_CASE     0x01
/** Builtin directory entry. */
#define DIRENTRY_FLAGS_BUILTIN  0x02
/** @} */



/*******************************************************************************
*   Structures and Typedefs                                                    *
*******************************************************************************/

/* forward refs */
struct __libc_prw_directory;
struct __libc_prw_directory_entry;
typedef struct __libc_prw_directory_entry  *PDIRENTRY;
typedef struct __libc_prw_directory        *PDIRECTORY;

/**
 * Directory entry.
 */
typedef struct __libc_prw_directory_entry
{
    /** Pointer to the next directory entry. */
    struct __libc_prw_directory_entry   *pNext;
    /** Number of rules referencing this directory entry. */
    unsigned                cRefs;
    /** Type specific data. */
    union
    {
        /** File: Pointer to the rule this belongs to. */
        __LIBC_PPATHREWRITE     pFile;
        /** Directory: Pointer to subdirectory entry. */
        PDIRECTORY              pDir;
    } u;

    /** Node type.
     * See DIRENTRY_TYPE_* defines.
     * The content of u depends on this field.
     */
    unsigned char           fType;
    /** If set the entry is case sensitive, If clear case insensitive. */
    unsigned char           fFlags;
    /** Length of the directory entry name. */
    unsigned char           cch;
    /** Name (this is a variable length field). */
    char                    szName[5];
} DIRENTRY;

/**
 * Look directory structure.
 */
typedef struct __libc_prw_directory
{
    /** List of directory entries. */
    PDIRENTRY               pEntries;
    /** Pointer to the rule rewriting this directory. */
    __LIBC_PPATHREWRITE     pRule;
    /** Number of rules referencing this directory. */
    unsigned                cRefs;
} DIRECTORY;


/*******************************************************************************
*   Global Variables                                                           *
*******************************************************************************/
/** @todo Read/Write lock - ARG!!! WE REALLY NEED THIS _NOW_!!! */

/** /dev/null -> /dev/nul rewrite rules */
static __LIBC_PATHREWRITE   gBltinRule_dev_null =
{   __LIBC_PRWF_CASE_SENSITIVE | __LIBC_PRWF_TYPE_FILE,     "/dev/null", 9,   "/dev/nul", 8 };
/** /dev/tty -> /dev/con rewrite rules */
static __LIBC_PATHREWRITE   gBltinRule_dev_tty =
{   __LIBC_PRWF_CASE_SENSITIVE | __LIBC_PRWF_TYPE_FILE,     "/dev/tty",  8,   "/dev/con", 8 };

#ifdef TESTCASE_ETC /* for testing purposes */
/** /etc -> c:/mptn/etc rewrite rules */
static __LIBC_PATHREWRITE   gBltinRule_etc =
{   __LIBC_PRWF_CASE_SENSITIVE | __LIBC_PRWF_TYPE_DIR,      "/etc",      4,   "c:/mptn/etc", 11 };
/** /etc directory */
static DIRECTORY            gBltinDir_etc =
{   NULL,                               &gBltinRule_etc, 1 };
#endif

/** /dev directory entries. */
static DIRENTRY             gBltinEntries_dev[2] =
{
    { &gBltinEntries_dev[1],            1,  {.pFile=&gBltinRule_dev_null},  DIRENTRY_TYPE_FILE, DIRENTRY_FLAGS_CASE | DIRENTRY_FLAGS_BUILTIN,   4, "null" },
    { NULL,                             1,  {.pFile=&gBltinRule_dev_tty},   DIRENTRY_TYPE_FILE, DIRENTRY_FLAGS_CASE | DIRENTRY_FLAGS_BUILTIN,   3, "tty"  },
};
/** /dev directory. */
static DIRECTORY            gBltinDir_dev =
{   &gBltinEntries_dev[0],              (__LIBC_PPATHREWRITE)NULL, 2 };

/** Root directory entries. */
#ifndef TESTCASE_ETC
static DIRENTRY            gBltinEntries_UnixRoot_dev[1] =
{
#else
static DIRENTRY            gBltinEntries_UnixRoot_dev[2] =
{
    { &gBltinEntries_UnixRoot_dev[1],   1,  {.pDir=&gBltinDir_etc},          DIRENTRY_TYPE_DIR,  DIRENTRY_FLAGS_CASE | DIRENTRY_FLAGS_BUILTIN,   3, "etc" },
#endif
    { NULL,                             2,  {.pDir=&gBltinDir_dev},          DIRENTRY_TYPE_DIR,  DIRENTRY_FLAGS_CASE | DIRENTRY_FLAGS_BUILTIN,   3, "dev" },
};
/** The Unix Root directory. */
static DIRECTORY            gBltinDir_UnixRoot =
{   &gBltinEntries_UnixRoot_dev[0],    (__LIBC_PPATHREWRITE)NULL, 2 };


/** Root directory entries. */
static DIRENTRY             gBltinEntries_Root[1] =
{
    { NULL,                             2,  {.pDir=&gBltinDir_UnixRoot},     DIRENTRY_TYPE_DIR,  DIRENTRY_FLAGS_CASE | DIRENTRY_FLAGS_BUILTIN,   0, "" }
};
/** The Root directory (for other roots). */
static DIRECTORY            gRootDir =
{   &gBltinEntries_Root[0],             (__LIBC_PPATHREWRITE)NULL, 2 };


/*******************************************************************************
*   Internal Functions                                                         *
*******************************************************************************/
static void __libc_pathRewriteDeleteDir(PDIRECTORY pDir);

/**
 * Validates a set of rules.
 *
 * @returns 0 if valid.
 * @returns -1 if invalid.
 * @param   paRules     Array of rules
 * @param   cRules      Number of rules in array.
 */
static int __libc_pathRewriteValidate(const __LIBC_PPATHREWRITE paRules, unsigned cRules)
{
    LIBCLOG_ENTER("paRules=%p cRules=%d\n", (void*)paRules, cRules);
    int i;

    /*
     * Validation.
     */
    for (i = 0; i < cRules; i++)
    {
        unsigned            cch;
        PDIRECTORY          pDir;
        const char         *psz;

        /*
         * Flags
         */
        if (    (paRules[i].fFlags & (__LIBC_PRWF_TYPE_FILE | __LIBC_PRWF_TYPE_DIR)) == (__LIBC_PRWF_TYPE_FILE | __LIBC_PRWF_TYPE_DIR)
            ||  !(paRules[i].fFlags & (__LIBC_PRWF_TYPE_FILE | __LIBC_PRWF_TYPE_DIR))
            ||  (paRules[i].fFlags & ~(__LIBC_PRWF_TYPE_FILE | __LIBC_PRWF_TYPE_DIR | __LIBC_PRWF_CASE_SENSITIVE | __LIBC_PRWF_CASE_INSENSITIVE)))
            LIBCLOG_ERROR_RETURN(-1, "ret -1 - rule %d: Invalid flags %#x\n", i, paRules[i].fFlags);

        /*
         * FROM
         */
        cch = strlen(paRules[i].pszFrom);
        if (cch != paRules[i].cchFrom)
            LIBCLOG_ERROR_RETURN(-1, "ret -1 - rule %d: invalid FROM length. actual=%d given=%d\n", i, cch, paRules[i].cchFrom);
        if (    paRules[i].pszFrom[cch - 1] == '\\'
            ||  paRules[i].pszFrom[cch - 1] == '/')
            LIBCLOG_ERROR_RETURN(-1, "ret -1 - rule %d: invalid FROM ends with slash.\n",i );

        /*
         * TO
         */
        cch = strlen(paRules[i].pszTo);
        if (cch != paRules[i].cchTo || !cch)
            LIBCLOG_ERROR_RETURN(-1, "ret -1 - rule %d: invalid TO length. actual=%d given=%d\n", i, cch, paRules[i].cchTo);
        if (    !(cch == 1  || (cch == 3 && paRules[i].pszTo[1] == ':'))
            && (    paRules[i].pszTo[cch - 1] == '\\'
                ||  paRules[i].pszTo[cch - 1] == '/') )
            LIBCLOG_ERROR_RETURN(-1, "ret -1 - rule %d: invalid TO ends with slash.\n", i);

        /*
         * Check if the type mismatches existing rule
         * and some pecularities.
         */
        for (pDir = &gRootDir, psz = paRules[i].pszFrom; ; )
        {
            PDIRENTRY       pEntry;
            const char     *pszEnd;
            char            ch;

            /*
             * Find the end of the path component.
             */
            pszEnd = psz;
            while ((ch = *pszEnd) && ch != '/' && ch != '\\')
                pszEnd++;
            cch    = pszEnd - psz;
            if (cch > 255)
                LIBCLOG_ERROR_RETURN(-1, "ret -1 - rule %d: path component is too long (%d bytes, 255 is max)\n", i, cch);

            /*
             * Lookup this component in the current directory.
             */
            for (pEntry = pDir->pEntries; pEntry; pEntry = pEntry->pNext)
            {
                int iDiff;
                if (pEntry->cch != cch)
                    continue;
                if (pEntry->fFlags & DIRENTRY_FLAGS_CASE)
                    iDiff = memcmp(pEntry->szName, psz, cch);
                else
                    iDiff = memicmp(pEntry->szName, psz, cch);
                if (!iDiff)
                    break;
            }

            /*
             * Found anything?
             */
            if (!pEntry)
                break;                  /* excellent, no match. */
            /*
             * Basic test, case sensitivity.
             */
            if (    (pEntry->fFlags & DIRENTRY_FLAGS_CASE)
                !=  (paRules[i].fFlags & __LIBC_PRWF_CASE_SENSITIVE ? DIRENTRY_FLAGS_CASE : 0))
                LIBCLOG_ERROR_RETURN(-1, "ret -1 - rule %d: collides with an exiting rule in terms of case sensitivity.\n", i);

            /*
             * Complete match? It must be of the same type and have
             * the same idea about what to rewrite to.
             */
            if (ch == '\0')
            {
                if (pEntry->fType != (paRules[i].fFlags & __LIBC_PRWF_TYPE_FILE ? DIRENTRY_TYPE_FILE : DIRENTRY_TYPE_DIR))
                    LIBCLOG_ERROR_RETURN(-1, "ret -1 - rule %d: collides with existing rule for '%s' (not same type)\n", i, paRules[i].pszFrom);

                __LIBC_PPATHREWRITE pRule = pEntry->fType == DIRENTRY_TYPE_FILE
                    ? pEntry->u.pFile : pEntry->u.pDir->pRule;
                if (    pRule
                    &&  (   pRule->cchTo != paRules[i].cchTo
                         || memicmp(pRule->pszTo, paRules[i].pszTo, paRules[i].cchTo)))
                    LIBCLOG_ERROR_RETURN(-1, "ret -1 - rule %d: collides with existing rule for '%s', different rewriting: existing='%s' new='%s'\n",
                                         i, paRules[i].pszFrom, pRule->pszTo, paRules[i].pszTo);
                break;                  /* excellent, complete match of just an simple subdirectory. */
            }

            /*
             * Next - We got a match on a non final component,
             *        this must be a directory.
             */
            if (pEntry->fType != DIRENTRY_TYPE_DIR)
                LIBCLOG_ERROR_RETURN(-1, "ret -1 - rule %d: collides with a file rule for '%s'\n", i, pEntry->u.pFile->pszFrom);
            psz  = pszEnd + 1;
            pDir = pEntry->u.pDir;
        } /* iterate thru the path components. */

        /* the rule is ok, let's log it. */
        LIBCLOG_MSG("rule %d: %#x '%s' -> '%s'\n", i, paRules[i].fFlags, paRules[i].pszFrom, paRules[i].pszTo);
    } /* for all rules */

    LIBCLOG_RETURN_INT(0);
}


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
 * @todo    Make this thread safe?
 */
int __libc_PathRewriteAdd(const __LIBC_PPATHREWRITE paRules, unsigned cRules)
{
    LIBCLOG_ENTER("paRules=%p cRules=%d\n", (void*)paRules, cRules);
    int i;

    /*
     * Validation.
     */
    if (__libc_pathRewriteValidate(paRules, cRules))
    {
        errno = EINVAL;
        LIBCLOG_RETURN_INT(-1);
    }

    /*
     * Add the rules.
     */
    for (i = 0; i < cRules; i++)
    {
        const char *psz;
        PDIRECTORY  pDir;
        for (pDir = &gRootDir, psz = paRules[i].pszFrom; ; )
        {
            PDIRENTRY       pEntry;
            unsigned        cch;
            const char     *pszEnd;
            char            ch;

            /* Increment reference counter. */
            pDir->cRefs++;

            /*
             * Find the end of the path component.
             */
            pszEnd = psz;
            while ((ch = *pszEnd) && ch != '/' && ch != '\\')
                pszEnd++;
            cch    = pszEnd - psz;

            /*
             * Lookup this component in the current directory.
             */
            for (pEntry = pDir->pEntries; pEntry; pEntry = pEntry->pNext)
            {
                int iDiff;
                if (pEntry->cch != cch)
                    continue;
                if (pEntry->fFlags & DIRENTRY_FLAGS_CASE)
                    iDiff = memcmp(pEntry->szName, psz, cch);
                else
                    iDiff = memicmp(pEntry->szName, psz, cch);
                if (!iDiff)
                    break;
            }

            /*
             * No match? Then add an entry.
             */
            if (!pEntry)
            {
                /* entry */
                pEntry = _hmalloc(sizeof(*pEntry) - sizeof(pEntry->szName) + cch + 1);
                LIBC_ASSERTM(pEntry, "out of memory!\n");
                if (!pEntry)
                    LIBCLOG_ERROR_RETURN_INT(-1);
                pEntry->cRefs       = 0;
                pEntry->fFlags      = paRules[i].fFlags & __LIBC_PRWF_CASE_SENSITIVE ? DIRENTRY_FLAGS_CASE : 0;
                pEntry->cch         = cch;
                memcpy(pEntry->szName, psz, cch);
                pEntry->szName[cch] = '\0';

                if (ch != '\0' || (paRules[i].fFlags & __LIBC_PRWF_TYPE_DIR))
                {   /* directory */
                    PDIRECTORY  pNewDir = _hmalloc(sizeof(*pNewDir));
                    LIBC_ASSERTM(pNewDir, "out of memory!\n");
                    if (!pNewDir)
                    {
                        free(pEntry);
                        LIBCLOG_RETURN_INT(-1);
                    }
                    pNewDir->cRefs      = 0;
                    pNewDir->pEntries   = NULL;
                    pNewDir->pRule      = NULL;
                    pEntry->u.pDir      = pNewDir;
                    pEntry->fType       = __LIBC_PRWF_TYPE_DIR;
                }
                else
                {
                    pEntry->fType       = __LIBC_PRWF_TYPE_FILE;
                    pEntry->u.pFile     = &paRules[i];
                }

                /* link it in. */
                pEntry->pNext   = pDir->pEntries;
                pDir->pEntries  = pEntry;
            }

            /* Increment reference counter for the entry. */
            pEntry->cRefs++;
            if (ch != '\0')
            {
                /*
                 * Matched a directory and there is more left to parse.
                 * go to next path component.
                 */
                psz  = pszEnd + 1;
                pDir = pEntry->u.pDir;
                continue;
            }
            else
            {
                /*
                 * Complete match. We're done!
                 */
                if (    pEntry->fType == DIRENTRY_TYPE_DIR
                    &&  !pEntry->u.pDir->pRule)
                {
                    pEntry->u.pDir->pRule = &paRules[i];
                    pEntry->u.pDir->cRefs++;
                }
                break;
            }

        } /* parse path */
    } /* for all rules */

    LIBCLOG_RETURN_INT(0);
}


/**
 * Removes a set of rules from the current rewrite rule set.
 *
 * The specified rule array must be the eact same as for the add call.
 *
 * @returns 0 on success.
 * @returns -1 and errno set on failure.
 * @param   paRules     Pointer to an array of rules to remove.
 * @param   cRules      Number of rules in the array.
 * @todo    Make this thread safe?
 */
int __libc_PathRewriteRemove(const __LIBC_PPATHREWRITE paRules, unsigned cRules)
{
    LIBCLOG_ENTER("paRules=%p cRules=%d\n", (void*)paRules, cRules);
    int i;

    /*
     * Validation.
     */
    if (__libc_pathRewriteValidate(paRules, cRules))
    {
        errno = EINVAL;
        LIBCLOG_RETURN_INT(-1);
    }

    /*
     * Add the rules.
     */
    for (i = 0; i < cRules; i++)
    {
        const char *psz;
        PDIRECTORY  pDir;
        for (pDir = &gRootDir, psz = paRules[i].pszFrom; ; )
        {
            PDIRENTRY       pEntry;
            unsigned        cch;
            const char     *pszEnd;
            char            ch;

            /* Decrement reference counter. */
            if (pDir->cRefs)
                pDir->cRefs--;
            LIBC_ASSERT(pDir->cRefs);

            /*
             * Find the end of the path component.
             */
            pszEnd = psz;
            while ((ch = *pszEnd) && ch != '/' && ch != '\\')
                pszEnd++;
            cch    = pszEnd - psz;

            /*
             * Lookup this component in the current directory.
             */
            for (pEntry = pDir->pEntries; pEntry; pEntry = pEntry->pNext)
            {
                int iDiff;
                if (pEntry->cch != cch)
                    continue;
                if (pEntry->fFlags & DIRENTRY_FLAGS_CASE)
                    iDiff = memcmp(pEntry->szName, psz, cch);
                else
                    iDiff = memicmp(pEntry->szName, psz, cch);
                if (!iDiff)
                    break;
            }

            /*
             * No match? we're done then.
             */
            if (!pEntry)
                break;

            /*
             * Decrement the reference counter for the entry.
             */
            if (pEntry->cRefs)
                pEntry->cRefs--;
            if (!pEntry->cRefs)
            {
                /*
                 * time to kill this entry and all it's decendants.
                 */

                /* first step: unlink it from the directory. */
                if (pDir->pEntries == pEntry)
                    pDir->pEntries = pEntry->pNext;
                else
                {
                    PDIRENTRY   pPrev;
                    for (pPrev = pDir->pEntries; pPrev && pPrev->pNext != pEntry; )
                        pPrev = pPrev->pNext;
                    if (pPrev)
                        pPrev->pNext = pEntry->pNext;
                }

                /* second step: kill subdirs. */
                if (pEntry->fType == DIRENTRY_TYPE_DIR)
                    __libc_pathRewriteDeleteDir(pEntry->u.pDir);

                /* third step: free it. */
                if (!(pEntry->fFlags & DIRENTRY_FLAGS_BUILTIN))
                    free(pEntry);
                break; /* done */
            }

            /* next entry */
            psz  = pszEnd + 1;
            pDir = pEntry->u.pDir;
        } /* parse path */
    } /* for all rules */

    LIBCLOG_RETURN_INT(0);
}


/**
 * Recursivly deletes a directory tree.
 *
 * @param   pDir    Tree to delete.
 */
static void __libc_pathRewriteDeleteDir(PDIRECTORY pDir)
{
    PDIRENTRY   pEntry = pDir->pEntries;
    while (pEntry)
    {
        PDIRENTRY   pNext = pEntry->pNext;
        if (pEntry->fType == DIRENTRY_TYPE_DIR)
            __libc_pathRewriteDeleteDir(pEntry->u.pDir);
        if (!(pEntry->fFlags & DIRENTRY_FLAGS_BUILTIN))
            free(pEntry);
        pEntry = pNext;
    }
}


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
int  __libc_PathRewrite(const char *pszPath, char *pszBuf, unsigned cchBuf)
{
    LIBCLOG_ENTER("pszPath=%s pszBuf=%p cchBuf=%u\n", pszPath, pszBuf, cchBuf);
    PDIRECTORY          pDir;
    __LIBC_PPATHREWRITE pRuleBest;
    const char         *psz;
    const char         *pszBest;

    /*
     * Validate input.
     */
    if (!pszPath || !*pszPath)
        LIBCLOG_ERROR_RETURN(-1, "ret -1 - bad path\n");

    /*
     * Parse path.
     */
    for (pDir = &gRootDir, psz = pszPath, pRuleBest = NULL, pszBest = NULL; ; )
    {
        PDIRENTRY       pEntry;
        unsigned        cch;
        unsigned char   fType;
        const char     *pszEnd;
        char            ch;

        /*
         * Find the end of the path component.
         */
        pszEnd = psz;
        while ((ch = *pszEnd) && ch != '/' && ch != '\\')
            pszEnd++;
        cch = (unsigned)(pszEnd - psz);
        if (cch > 255)
        {
            LIBCLOG_ERROR_RETURN(-1, "ret -1 - path component is too long (%d bytes, 255 is max)\n", cch);
            break;                      /* path component is to long, we don't accept rules like this one!! */
        }
        fType = ch == '\0' ? DIRENTRY_TYPE_FILE | DIRENTRY_TYPE_DIR : DIRENTRY_TYPE_DIR;

        /*
         * Lookup this component in the current directory.
         */
        for (pEntry = pDir->pEntries; pEntry; pEntry = pEntry->pNext)
        {
            int iDiff;
            if (    pEntry->cch != cch
                ||  !(pEntry->fType & fType))
                continue;
            if (pEntry->fFlags & DIRENTRY_FLAGS_CASE)
                iDiff = memcmp(pEntry->szName, psz, cch);
            else
                iDiff = memicmp(pEntry->szName, psz, cch);
            if (!iDiff)
                break;
        }

        /*
         * Found anything?
         */
        if (pEntry)
        {
            if (ch != '\0')
            {
                /*
                 * Matched a directory and there is more left to parse.
                 * go to next path component.
                 *
                 * If we have a partial match, save it.
                 */
                if (pEntry->u.pDir->pRule)
                {
                    pRuleBest = pEntry->u.pDir->pRule;
                    pszBest   = pszEnd + 1;
                }
                psz  = pszEnd + 1;
                pDir = pEntry->u.pDir;
                continue;
            }

            /*
             * Complete match. We're done!
             */
            if (    pEntry->fType == DIRENTRY_TYPE_FILE
                ||  pEntry->u.pDir->pRule /* DIRENTRY_TYPE_DIR */)
            {
                __LIBC_PPATHREWRITE pRule = pEntry->fType == DIRENTRY_TYPE_FILE
                                          ? pEntry->u.pFile : pEntry->u.pDir->pRule;
                cch = pRule->cchTo + 1;
                if (pszBuf)
                {
                    if (cchBuf < cch)
                        LIBCLOG_ERROR_RETURN(-1, "ret -1 - Buffer is too small (%d < %d)\n", cchBuf, cch);
                    memcpy(pszBuf, pRule->pszTo, cch);
                    LIBCLOG_MSG("rewritten: '%s' (complete)\n", pszBuf);
                }
                LIBCLOG_RETURN_INT(cch);
            }
            /* not a match, the directory wasn't rewritten, quit searching. */
        }
        /* no match, we're done. */
        break;
    } /* iterate thru the path components. */

    /*
     * No complete match or no match at all.
     *
     * We might have a partial match with a directory from earlier
     * and use that for the rewrite, if not there is no rewrite.
     */
    if (pRuleBest)
    {
        unsigned cch = strlen(pszBest);
        unsigned cchTotal = cch + pRuleBest->cchTo + 2; /* 2 = slash + '\0'. */
        if (pszBuf)
        {
            char *pszBufIn = pszBuf;
            if (cchBuf < cchTotal)
                LIBCLOG_ERROR_RETURN(-1, "ret -1 - Buffer is too small (%d < %d)\n", cchBuf, cchTotal);
            memcpy(pszBuf, pRuleBest->pszTo, pRuleBest->cchTo);
            pszBuf += pRuleBest->cchTo;
            if (    pszBuf[-1] != '/'
                &&  pszBuf[-1] != '\\')
                *pszBuf++ = '\\';
            memcpy(pszBuf, pszBest, cch + 1);
            LIBCLOG_MSG("rewritten: '%s' (partial)\n", pszBufIn); (void)pszBufIn;
        }
        LIBCLOG_RETURN_INT(cchTotal);
    }

    /* no rewrite */
    LIBCLOG_RETURN_INT(0);
}
