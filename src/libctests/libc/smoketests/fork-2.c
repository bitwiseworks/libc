/* $Id: fork-2.c 2055 2005-06-19 08:04:00Z bird $ */
/** @file
 *
 * Simple fork testcase.
 *
 * Copyright (c) 2004 knut st. osmundsen <bird-srcspam@anduin.net>
 *
 *
 * This file is part of InnoTek LIBC.
 *
 * InnoTek LIBC is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * InnoTek LIBC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with InnoTek LIBC; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */


/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <locale.h>
#include <sys/socket.h>
#include <ctype.h>
#include <iconv.h>
#include <string.h>
#include <sys/wait.h>

/*******************************************************************************
*   Internal Functions                                                         *
*******************************************************************************/
static int tstLocale(const char *pszCtx);
static int tstIconv(iconv_t pConv, const char *pszCtx);

/* fixme! */
static int tstLocale(const char *pszCtx)
{
#ifdef OTHER
    static char szTest[] = "ëõÜ\tíùè\tÑéîô\n"; /* 850 */
#else
    static char szTest[] = "tÊ¯Â\t∆ÿ≈\t‰ƒˆ÷\n"; /* 8859-1 */
#endif
    char   *psz;

    printf("%s: ", pszCtx);
    for (psz = szTest; *psz; psz++)
    {
        if (*psz == '\t')
            printf("\n%s: ", pszCtx);
        else if (*psz == '\n')
            printf("\n");
        else
        {
            printf("'%c'(%02x): %d,%d,%d%s",
                   *psz, (unsigned char)*psz, !!isalpha(*psz), !!isupper(*psz), !!islower(*psz), psz[1] ? " " : "");
        }
    }
    return 0;
}


static int tstIconv(iconv_t pIconv, const char *pszCtx)
{
    size_t  cbIn = 4;
    char    achOut[20];
    size_t  cbOut = sizeof(achOut);
    size_t  cb;
    const char *pszIn = "abc";
    char   *pszOut = &achOut[0];
    cb = iconv(pIconv, &pszIn, &cbIn, &pszOut, &cbOut);
    if (!cb && !strcmp(achOut, "abc"))
        return 0;
    printf("error (%s): iconv failed. cb=%d errno=%d\n", pszCtx, cb, errno);
    return 1;
}


int main(int argc, char **argv)
{
    int     rcRet = 0;
    int     afds[2] = {-1,-1};
    int     fh;
    pid_t   pid;
    int     rc;
    iconv_t pIconv;

    /* filehandles */
    fh = open(argv[0], O_RDONLY
#ifdef O_BINARY
              | O_BINARY
#endif
              , 0777);
    if (fh < 0)
    {
        printf("fatal error: failed to set locale to en_US.UTF-8!\n");
        return 1;
    }
    if (fcntl(fh, F_SETFD, FD_CLOEXEC))
        printf("fcntl(%d,) failed! errno=%d\n", fh, errno), rcRet++;

    /* locale */
    if (!setlocale(LC_ALL, "en_US.UTF-8"))
    {
        printf("fatal error: failed to set locale to en_US.UTF-8!\n");
        return 1;
    }

    /* sockets */
    rcRet = socketpair(PF_LOCAL, SOCK_STREAM, 0, &afds[0]);
    if (rcRet)
    {
        printf("fatal error: socketpair failed! errno=%d\n", errno);
        return 1;
    }
    printf("info: created socket pair %d %d\n", afds[0], afds[1]);
    if (fcntl(afds[0], F_SETFD, FD_CLOEXEC))
        printf("error: fcntl(%d,) failed! errno=%d\n", afds[0], errno), rcRet++;
    if ((rc = fcntl(afds[0], F_GETFD, 0)) != FD_CLOEXEC)
        printf("error: the filehandle %d (socket) was messed up! (flags=%d)\n", afds[0], rc), rcRet++;
    if ((rc = fcntl(afds[1], F_GETFD, 0)) != 0)
        printf("error: the filehandle %d (socket) was messed up! (flags=%d)\n", afds[1], rc), rcRet++;

    /* iconv */
    pIconv = iconv_open("IBM850", "UTF-8");
    if (!pIconv)
    {
        printf("fatal error: iconv_open(\"IBM850\", \"UTF-8\") failed! errno=%d\n", errno);
        return 1;
    }


    /*
     * fork the child process.
     */
    pid = fork();
    if (!pid)
    {
        /* child */
        printf("child!\n");
        if ((rc = fcntl(afds[0], F_GETFD, 0)) != FD_CLOEXEC)
            printf("error (child): the filehandle %d (socket) was messed up! (flags=%d)\n", afds[0], rc), rcRet++;
        #if 0
        close(afds[0]);
        usleep(100000);
        #endif
        if ((rc = fcntl(afds[1], F_GETFD, 0)) != 0)
            printf("error (child): the filehandle %d (socket) was messed up! (flags=%d)\n", afds[1], rc), rcRet++;
        if ((rc = fcntl(fh, F_GETFD, 0)) != FD_CLOEXEC)
            printf("error (child): the filehandle %d was messed up! (flags=%d)\n", fh, rc), rcRet++;

        rcRet += tstLocale("child");
        rcRet += tstIconv(pIconv, "child");
    }
    else if (pid < 0)
    {
        printf("error: fork() failed pid=%d errno=%d\n", pid, errno);
        return 1;
    }
    else
    {
        /* parent */
        printf("created child %d\n", pid);
        if ((rc = fcntl(afds[0], F_GETFD, 0)) != FD_CLOEXEC)
            printf("error (parent): the filehandle %d (socket) was messed up! (flags=%d)\n", afds[0], rc), rcRet++;
        if ((rc = fcntl(afds[1], F_GETFD, 0)) != 0)
            printf("error (parent): the filehandle %d (socket) was messed up! (flags=%d)\n", afds[1], rc), rcRet++;
        if ((rc = fcntl(fh, F_GETFD, 0)) != FD_CLOEXEC)
            printf("error (parent): the filehandle %d was messed up! (flags=%d)\n", fh, rc), rcRet++;

        /* check if the socket is still around after the child closed its handle. */
        waitpid(pid, NULL, 0);
        rc = write(afds[0], "howdy!", 6);
        if (rc <= 0)
            printf("error (parent): write(%d,,) -> %d errno=%d\n", afds[0], rc, errno), rcRet++;

        rcRet += tstLocale("parent");
        rcRet += tstIconv(pIconv, "parent");
    }

    /*
     * Summary
     */
    if (rcRet)
        printf(" %d failed tests\n", rcRet);
    else
        printf("all tests completed successfully\n");

    return !!rcRet;
}
