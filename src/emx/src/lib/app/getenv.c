/* getenv.c (emx+gcc) -- Copyright (c) 1990-1995 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdlib.h>
#include <string.h>
#define __LIBC_LOG_GROUP  __LIBC_LOG_GRP_ENV
#include <InnoTekLIBC/logstrict.h>

char *_STD(getenv)(const char *name)
{
    LIBCLOG_ENTER("name=%p:{%s}\n", name, name);
    int len;
    char **p;

    if (!name)
    {
        LIBCLOG_ERROR("invalid parameter name (NULL)!\n");
        LIBCLOG_RETURN_P(NULL);
    }
    if (environ == NULL)
    {
        LIBCLOG_ERROR("environment is not configured!!!\n");
        LIBCLOG_RETURN_P(NULL);
    }

    LIBCLOG_ERROR_CHECK(!strchr(name, '='), "name contains '=' ('%s')\n", name);
    LIBCLOG_ERROR_CHECK(*name, "name is empty\n");

    len = strlen(name);
    for (p = environ; *p != NULL; ++p)
    {
        if (    strncmp(*p, name, len) == 0
            &&  (*p)[len] == '=')
        {
            char *pszRet = *p + len + 1;
            LIBCLOG_RETURN_MSG(pszRet, "ret %p:{%s}\n", pszRet, pszRet);
        }
    }
    LIBCLOG_RETURN_P(NULL);
}
