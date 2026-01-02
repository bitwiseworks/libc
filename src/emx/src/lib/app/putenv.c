/* putenv.c (emx+gcc) -- Copyright (c) 1990-1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <emx/startup.h>
#include <emx/time.h>           /* _tzset_flag */
#define __LIBC_LOG_GROUP  __LIBC_LOG_GRP_ENV
#include <InnoTekLIBC/logstrict.h>

int _STD(putenv)(char *string)
{
    LIBCLOG_ENTER("string=%p:{%s}\n", string, string);
    char *s, **p;
    int len, env_size;

    if (string == NULL)
    {
        LIBCLOG_ERROR("Invalid argument string (NULL).\n");
        errno = EINVAL;
        LIBCLOG_RETURN_INT(-1);
    }

    s = strchr(string, '=');
    if (s == NULL)
    {
        /* remove string, let unset do the work */
        int rc = unsetenv(string);
        LIBCLOG_RETURN_INT(rc);
    }

    /* insert/replace */
    len = s - string;
    p = environ;
    env_size = 0;
    if (p != NULL)
    {
        while (*p != NULL)
        {
            s = *p;
            if (    strncmp(s, string, len) == 0
                &&  (   s[len] == '\0'
                     || s[len] == '='))
                break;
            ++p;
            ++env_size;
        }
    }
    else
        LIBC_ASSERTM_FAILED("environment not initiated!\n");

    if (p == NULL || *p == NULL)
    {
        if (environ == _org_environ)
        {
            LIBCLOG_MSG("env_size=%d; new, first new\n", env_size);
            p = malloc((env_size + 2) * sizeof (char *));
            if (p == NULL)
                LIBCLOG_ERROR_RETURN_INT(-1);
            environ = p;
            if (env_size != 0)
                memcpy(environ, _org_environ, env_size * sizeof (char *));
        }
        else
        {
            LIBCLOG_MSG("env_size=%d; new, resize environ array\n", env_size);
            p = realloc(environ, (env_size + 2) * sizeof (char *));
            if (p == NULL)
                LIBCLOG_ERROR_RETURN_INT(-1);
            environ = p;
        }
        environ[env_size + 0] = string;
        environ[env_size + 1] = NULL;
    }
    else
    {
        LIBCLOG_MSG("replacing '%s' with '%s'\n", *p, string);
        *p = string;
    }

    _tzset_flag = 0;                    /* Call tzset() */
    LIBCLOG_RETURN_INT(0);
}
