/* spawnve.c (emx+gcc) -- Copyright (c) 1990-1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdlib.h>
#include <string.h>
#include <process.h>
#include <errno.h>
#include <alloca.h>
#include <sys/syslimits.h>
#include <emx/startup.h>
#include <emx/syscalls.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_PROCESS
#include <InnoTekLIBC/logstrict.h>

int _STD(spawnve)(int mode, const char *name, char * const argv[], char * const envp[])
{
    LIBCLOG_ENTER("mode=%#x name=%s argv=%p envp=%p\n", mode, name, (void *)argv, (void *)envp);
    struct _new_proc np;
    int i, size, n, rc;
    const char * const *p;
    char *d;
    char exe[PATH_MAX];

    /*
     * Init the syscall struct np.
     */
    /* mode */
    np.mode = mode;
    /* exe name */
    if (strlen(name) >= sizeof(exe) - 4)
    {
        errno = ENAMETOOLONG;
        LIBCLOG_ERROR_RETURN(-1, "ret -1 - name is too long, %d bytes: %s\n", strlen(name), name);
    }
    strcpy(exe, name);
#if 0/*  This isn't the place and time, however the backend assumes there is sufficient space in the buffer. */
    _defext(exe, "exe");
    LIBCLOG_MSG("exe=%s\n", exe); */
#endif
    np.fname_off = (unsigned long)exe;

    /* calc environment size */
    if (envp == NULL)
        envp = environ;
    size = 1; n = 0;
    for (p = (const char * const *)envp; *p != NULL; ++p)
    {
        ++n;
        size += 1 + strlen(*p);
    }
    d = alloca(size);
    LIBCLOG_MSG("environment: %d bytes %d entries block=%p\n", size, n, d);
    np.env_count = n; np.env_size = size;
    np.env_off = (unsigned long)d;

    /* copy environment */
    for (p = (const char * const *)envp; *p != NULL; ++p)
    {
        i = strlen(*p);
        /*
         * Skip empty strings to prevent DosExecPgm from interpreting them
         * as end of list and ignoring the rest (see #100).
         */
        if (!i)
            continue;
        memcpy(d, *p, i + 1);
        d += i + 1;
    }
    *d = 0;

    /* calc argument size */
    size = 0; n = 0;
    for (p = (const char * const *)argv; *p != NULL; ++p)
    {
        ++n;
        size += 2 + strlen(*p);
    }
    d = alloca(size);
    LIBCLOG_MSG("arguments: %d bytes %d entries block=%p\n", size, n, d);
    np.arg_count = n; np.arg_size = size;
    np.arg_off = (unsigned long)d;

    /* copy arguments */
    for (p = (const char * const *)argv; *p != NULL; ++p)
    {
        i = strlen(*p);
        *(unsigned char *)d++ = _ARG_NONZERO;
        memcpy(d, *p, i + 1);
        d += i + 1;
    }

    /*
     * Call syscall.
     */
    rc = __spawnve(&np);
    LIBCLOG_RETURN_INT(rc);
}
