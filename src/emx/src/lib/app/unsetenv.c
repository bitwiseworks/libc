/* $Id: unsetenv.c 2254 2005-07-17 12:25:44Z bird $ */
/** @file
 *
 * LIBC - unsetenv()
 *
 * Copyright (c) 2005 knut st. osmundsen <bird@anduin.net>
 * Copyright (c) 2003 InnoTek Systemberatung GmbH
 * Author: knut st. osmundsen <bird-srcspam@anduin.net>
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
#include <emx/time.h>
#define __LIBC_LOG_GROUP  __LIBC_LOG_GRP_ENV
#include <InnoTekLIBC/logstrict.h>



/**
 * Delete environment variable.
 *
 * @returns 0 on success.
 * @returns -1 and errno=EINVAL if name is invalid in any way.
 * @param   name    Name of environment variable to unset.
 *                  Shall not be NULL, empty string or contain '='.
 * @remark  Leaks memory, but that's what BSD does too.
 * @author  knut st. osmundsen <bird-srcspam@anduin.net>
 */
int _STD(unsetenv)(const char *name)
{
    LIBCLOG_ENTER("name=%s\n", name);
    int     lenname;
    char ** p;

    /* validate input */
    if (name == NULL || *name == '\0' || strchr(name, '=') != NULL)
    {
        errno = EINVAL;
        LIBCLOG_ERROR("name(%p='%s') is invalid\n", name, name);
        LIBCLOG_RETURN_INT(-1);
    }

    /* search (thru all the environment in case of multiple defintions). */
    lenname = strlen(name);
    p = environ;
    while (*p != NULL)
    {
        char *s = *p;
        if (    strncmp(s, name, lenname) == 0
            && (    s[lenname] == '\0'
                ||  s[lenname] == '='))
        { /* shift down the remaining entries. */
            char **p2 = p;
            for (;;p2++)
                if ((p2[0] = p2[1]) == NULL)
                    break;
            LIBCLOG_MSG("deleted '%s'\n", s);
            _tzset_flag = 0;            /* Call tzset() */
        }
        else
        {
            /* next */
            p++;
        }
    }

    LIBCLOG_RETURN_INT(0);
}
