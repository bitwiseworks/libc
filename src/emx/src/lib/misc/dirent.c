/* dirent.c (emx+gcc) */

#include "libc-alias.h"
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <emx/io.h>
#include <emx/syscalls.h>
#include <emx/umalloc.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_IO
#include <InnoTekLIBC/logstrict.h>


static int _readdir_ino = 31415926;

/*******************************************************************************
*   Internal Functions                                                         *
*******************************************************************************/
static void free_dircontents(struct _dircontents *dp);
static char *getdirent(const char *dir, struct _find *pfind, const char *base);


static void free_dircontents(struct _dircontents *dp)
{
    while (dp != NULL)
    {
        void *pv = dp;
        dp = dp->_d_next;
        free(pv);
    }
}

static char *getdirent(const char *dir, struct _find *pfind, const char *base)
{
    int rc;

    if (dir != NULL)
        rc = __findfirst(dir, A_DIR | A_HIDDEN | A_SYSTEM, pfind);
    else
        rc = __findnext(pfind);
    if (rc)
        return NULL;

    _fnlwr2(pfind->szName, base);
    return pfind->szName;
}

DIR *_STD(opendir)(const char *name)
{
    LIBCLOG_ENTER("name=%p:{%s}\n", (void *)name, name);
    struct stat             statb;
    struct _find            find;
    DIR *                   pdir;
    char *                  psz;
    char                    nbuf[MAXPATHLEN+4];
    int                     cchname;
    int                     saved_errno = errno;

    /*
     * Put directory name in nbuf.
     */
    cchname = strlen(name);
    if (cchname > MAXPATHLEN)
    {
        errno = ENAMETOOLONG;
        LIBCLOG_ERROR_RETURN_P(NULL);
    }
    memcpy(nbuf, name, cchname + 1);
    psz = nbuf + cchname;
    if (_trslash (nbuf, cchname, 0))
    {
        nbuf[cchname++] = '.';        /* psz now points to '.' */
        nbuf[cchname] = 0;
    }

    /*
     * Check that the directory exists and is a directory.
     */
    if (stat(nbuf, &statb) < 0)
        LIBCLOG_ERROR_RETURN_P(NULL);
    if ((statb.st_mode & S_IFMT) != S_IFDIR)
    {
        errno = ENOTDIR;
        LIBCLOG_ERROR_RETURN_P(NULL);
    }

    /*
     * Start enumeration.
     * We're reading _everything_ now, which isn't such an good idea for
     * large directories actually. (I wouldn't try this on the FreeDB tree.)
     */
    pdir = malloc(sizeof(DIR));
    if (pdir == NULL)
    {
        errno = ENOMEM;
        LIBCLOG_ERROR_RETURN_P(NULL);
    }
    if (!*psz) /* If dot was added. */
        *psz++ = '\\';
    strcpy(psz, "*.*");
    pdir->dd_loc = 0;
    pdir->dd_contents = NULL;
    pdir->dd_cp = NULL;
    psz = getdirent(nbuf, &find, name);
    if (psz == NULL)
    {
        errno = ENOMEM;
        LIBCLOG_ERROR_RETURN_P(pdir); /* why no free and return? */
    }

    do
    {
        struct _dircontents *   dp;
        int                     cch;

        /*
         * Allocate memory for the directory entry.
         * To limit number of heap blocks and malloc+free calls, we allocate
         * on chunk of memory, placing the name after the struct.
         */
        cch = strlen(psz) + 1;
        dp = _hmalloc(sizeof(struct _dircontents) + cch);
        if (dp == NULL)
        {
            free_dircontents(pdir->dd_contents);
            errno = ENOMEM;
            LIBCLOG_RETURN_P(NULL);
        }
        dp->_d_entry = (char*)(dp + 1);

        /*
         * Enter the data.
         */
        memcpy(dp->_d_entry, psz, cch);
        if (pdir->dd_contents != NULL)
            pdir->dd_cp->_d_next = dp;
        else
            pdir->dd_contents = dp;
        pdir->dd_cp = dp;
        dp->_d_next = NULL;
        dp->_d_size = find.cbFile;
        dp->_d_attr = find.attr;
        dp->_d_time = find.time;
        dp->_d_date = find.date;
        dp->_d_ino  = find.ino;

        /*
         * Next entry.
         */
        psz = getdirent(NULL, &find, name);
    } while (psz != NULL);

    /*
     * Position at start
     */
    pdir->dd_cp = pdir->dd_contents;
    errno = saved_errno; /* getdirent/findnext sets errno when done */
    LIBCLOG_RETURN_P(pdir);
}


int _STD(readdir_r)(DIR *dirp, struct dirent *pdent, struct dirent **ppdent)
{
    LIBCLOG_ENTER("dirp=%p pdent=%p ppdent=%p\n", (void *)dirp, (void *)pdent, (void *)ppdent);
    int cch;
    struct _dircontents *pd;
    /* done? */
    pd = dirp->dd_cp;
    if (pd == NULL)
    {
        //errno = ENOENT;
        *ppdent = NULL;
        LIBCLOG_RETURN_P(0);
    }

    /*
     * Copy the data over to the entry.
     */
    cch = strlen(pd->_d_entry);
    memcpy(pdent->d_name, pd->_d_entry, cch + 1);
    pdent->d_namlen = cch;
    pdent->d_reclen = pdent->d_namlen;
    if (pd->_d_ino)
        pdent->d_ino = pd->_d_ino;
    else
    {
        pdent->d_ino = _readdir_ino++;
        if (_readdir_ino == 0)
            _readdir_ino = 1;
    }
    pdent->d_type = (pd->_d_attr & A_DIR) ? DT_DIR : (pd->_d_attr & A_SYMLINK) ? DT_LNK : DT_REG;
    pdent->d_size = pd->_d_size;
    pdent->d_time = pd->_d_time;
    pdent->d_date = pd->_d_date;
    pdent->d_attr = pd->_d_attr;

    /*
     * Advance the stream and return successfully.
     */
    dirp->dd_cp = pd->_d_next;
    dirp->dd_loc++;
    *ppdent = pdent;
    LIBCLOG_RETURN_P(0);
}


struct dirent *_STD(readdir)(DIR *dirp)
{
    struct dirent *p = &dirp->dd_dirent;
    if (!readdir_r(dirp, p, &p))
        return p;
    return NULL;
}

/**
 * Adopted from https://github.com/komh/os2compat/blob/57fdbf88086bb48e79012a52fad5c7b7b227bafb/io/scandir.c
 * Copyright (C) 2016-2021 KO Myung-Hun <komh@chollian.net>
 */
int  _STD(scandir)(const char *dir, struct dirent ***namelist,
      int (*sel)(const struct dirent *), int (*compar)(const struct dirent **,
      const struct dirent **))
{
    LIBCLOG_ENTER("dir=%s namelist=%p sel=%p compar=%p\n", dir, namelist, sel, compar);
    DIR *dp;
    struct dirent **list = NULL;
    size_t list_size = 0;
    int list_count = 0;
    struct dirent *d;
    int saved_errno;

    dp = opendir(dir);
    if (dp == NULL)
        LIBCLOG_ERROR_RETURN_INT(-1);

    /* Save original errno */
    saved_errno = errno;

    /* Clear errno to test later */
    errno = 0;

    while ((d = readdir(dp)) != NULL)
    {
        int selected = !sel || sel(d);

        /* Clear errno modified by sel() */
        errno = 0;

        if (selected)
        {
            struct dirent *d1;
            size_t len;

            /* List full ? */
            if (list_count == list_size)
            {
                struct dirent **new_list;

                if (list_size == 0)
                    list_size = 20;
                else
                    list_size += list_size;

                new_list = (struct dirent **)realloc(list,
                                                     list_size * sizeof(*list));
                if (!new_list)
                {
                    errno = ENOMEM;
                    break;
                }

                list = new_list;
            }

            /*
             * On OS/2 kLIBC, d_name is not the last member of struct dirent.
             * So just allocate as many as the size of struct dirent.
             */
            len = sizeof(struct dirent);
            d1 = (struct dirent *)malloc(len);
            if (!d1)
            {
                errno = ENOMEM;
                break;
            }

            memcpy(d1, d, len);

            list[list_count++] = d1;
        }
    }

    /* Error ? */
    if (errno)
    {
        /* Store errno to use later */
        saved_errno = errno;

        /* Free a directory list */
        while (list_count > 0)
            free(list[--list_count]);
        free(list);

        /* Indicate an error */
        list_count = -1;
    }
    else
    {
        /* If compar is present, sort */
        if (compar != NULL)
            qsort(list, list_count, sizeof(*list),
                  (int (*)(const void *, const void *))compar);

        *namelist = list;
    }

    /* Ignore error of closedir() */
    closedir(dp);

    errno = saved_errno;

    if (list_count == -1)
        LIBCLOG_ERROR_RETURN_INT(-1);
    LIBCLOG_RETURN_INT(list_count);
}


int _STD(alphasort)(const struct dirent **d1, const struct dirent **d2)
{
    return strcoll((*d1)->d_name, (*d2)->d_name);
}


void _STD(seekdir)(DIR *dirp, long off)
{
    LIBCLOG_ENTER("dirp=%p off=%ld\n", (void *)dirp, off);
    if (off >= 0)
    {
        long                i;
        struct _dircontents *dp;

        i = 0;
        for (dp = dirp->dd_contents; i < off && dp != NULL; dp = dp->_d_next)
            ++i;
        dirp->dd_loc = i;
        dirp->dd_cp = dp;
        LIBCLOG_RETURN_VOID();
    }
    errno = EINVAL;
    LIBCLOG_ERROR_RETURN_VOID();
}


void _STD(rewinddir)(DIR *dirp)
{
    _seekdir(dirp, 0);
}


long _STD(telldir)(DIR *dirp)
{
    LIBCLOG_ENTER("dirp=%p\n", (void *)dirp);
    LIBCLOG_RETURN_LONG(dirp->dd_loc);
}


int _STD(closedir)(DIR *dirp)
{
    LIBCLOG_ENTER("dirp=%p\n", (void *)dirp);
    free_dircontents(dirp->dd_contents);
    free(dirp);
    LIBCLOG_RETURN_INT(0);
}

