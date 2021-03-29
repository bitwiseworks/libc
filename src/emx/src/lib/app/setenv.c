/* $Id: setenv.c 2254 2005-07-17 12:25:44Z bird $ */
/** @file
 *
 * LIBC APP - setenv().
 *
 * Copyright (c) 2003 InnoTek Systemberatung GmbH
 * Author: knut st. osmundsen <bird-srcspam@anduin.net>
 * Copyright (c) 2005 knut st. osmundsen <bird@anduin.net>
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
#include "libc-alias.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <emx/startup.h>
#include <emx/time.h>           /* _tzset_flag */
#define __LIBC_LOG_GROUP  __LIBC_LOG_GRP_ENV
#include <InnoTekLIBC/logstrict.h>


/**
 * Set environment variable.
 *
 * @returns 0 on success.
 * @returns -1 on failure with errno set to either EINVAL or ENOMEM.
 * @param   envname     Name of environment variable to set.
 *                      Shall not be NULL, empty string or contain '='.
 * @param   envval      The value to set.
 * @param   overwrite   If set any existing variable should be overwritten.
 *                      If clear return successfully without changing any
 *                      existing variable.
 *                      If there is not existing variable it is added
 *                      regardless of the state of this flag.
 * @author  knut st. osmundsen <bird-srcspam@anduin.net>
 * @remark  We skip the first equal in the environment value like BSD does.
 * @remark  Make environment updating and such thread safe!
 */
int _STD(setenv)(const char *envname, const char *envval, int overwrite)
{
    LIBCLOG_ENTER("envname=%p:{%s} envval=%p:{%s} overwrite=%d\n", envname, envname, envval, envval, overwrite);
    char **p;
    char *psz;
    int   lenname;
    int   lenval;
    int   env_size;

    /* validate */
    if (envname == NULL || *envname == '\0' || strchr(envname, '=') != NULL)
    {
        LIBCLOG_ERROR("Invalid argument envname %p:{%s}!\n", envname, envname);
        errno = EINVAL;
        LIBCLOG_RETURN_INT(-1);
    }
    _tzset_flag = 0;              /* Call tzset() */

    /* BSD Compatability. */
    if (*envval == '=')
        envval++;

    /* search for existing variable iinstance  */
    lenname = strlen(envname);
    p = environ;
    env_size = 0;
    if (p != NULL)
        while (*p != NULL)
        {
            char *s = *p;
            if (    strncmp(s, envname, lenname) == 0
                && (    s[lenname] == '\0'
                    ||  s[lenname] == '='))
                break;
            ++p; ++env_size;
        }

    /* found it? */
    lenval = strlen(envval);
    if (p != NULL && *p != NULL)
    {
        if (!overwrite)
        {
            LIBCLOG_MSG("exists, requested not to overwrite it\n");
            LIBCLOG_RETURN_INT(0);
        }
        /* if the older is larger then overwrite it */
        LIBCLOG_MSG("exists, overwrite\n");
        if (*(*p + lenname) == '=' && strlen(*p + lenname + 1) >= lenval)
        {
            memcpy(*p + lenname + 1, envval, lenval + 1);
            LIBCLOG_RETURN_INT(0);
        }
    }
    else
    {
        /* new variable: reallocate the environment pointer block */
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

        /* Add to end. */
        p = &environ[env_size + 0];
        environ[env_size + 1] = NULL;
    }

    /* Allocate space for new variable and assign it. */
    psz = malloc(lenname + lenval + 2);
    if (!psz)
        LIBCLOG_ERROR_RETURN_INT(-1);
    memcpy(psz, envname, lenname);
    psz[lenname] = '=';
    memcpy(psz + lenname + 1, envval, lenval + 1);
    *p = psz;
    LIBCLOG_RETURN_INT(0);
}

