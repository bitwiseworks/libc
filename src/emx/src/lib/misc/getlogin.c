/* getlogin.c (emx+gcc) -- Copyright (c) 1994-1995 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

char *_STD(getlogin)(void)
{
    static char tmp_name[L_cuserid];
    if (!getlogin_r(tmp_name, sizeof(tmp_name)))
        return tmp_name;
    return NULL;
}

int _STD(getlogin_r)(char *buf, int size)
{
    if (size >= 0)
    {
        /** @todo this isn't correct, use the uid stuff in SPM */
        const char *pszName = getenv("LOGNAME");
        if (!pszName)
            pszName = getenv("USER");
        if (!pszName)
            pszName = "root";
        int cchName = strlen(pszName);
        if (cchName <= size)
        {
            memcpy(buf, pszName, cchName + 1);
            return 0;
        }
        memcpy(buf, pszName, --size);
        buf[size] = '\0';
    }
    errno = ERANGE;
    return -1;
}

