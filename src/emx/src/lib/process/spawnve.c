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
    char exe[PATH_MAX + 4 /* room for .exe extension probed by __spawnve */];

    /*
     * Init the syscall struct np.
     */
    /* mode */
    np.mode = mode & 0x7FFFFFFF;
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
        i = strlen(*p);
        /*
         * Skip empty strings to prevent DosExecPgm from interpreting them
         * as end of list and ignoring the rest (see #100).
         */
        if (!i)
            continue;
        /*
         * Also skip variables with no `=` as they are dead anyway (inaccessible
         * as if never existed), see #102.
         */
        if (memchr(*p, '=', i) == NULL)
            continue;
        size += 1 + i;
    }
    d = alloca(size);
    LIBCLOG_MSG("environment: %d bytes %d entries block=%p\n", size, n, d);
    np.env_count = n; np.env_size = size;
    np.env_off = (unsigned long)d;
    np.env_size2 = size >> 16;
    if (np.env_size2)
        np.mode |= 0x80000000;

    /* copy environment */
    for (p = (const char * const *)envp; *p != NULL; ++p)
    {
        i = strlen(*p);
        /* see above */
        if (!i)
            continue;
        /* see above */
        if (memchr(*p, '=', i) == NULL)
            continue;
        memcpy(d, *p, i + 1);
        d += i + 1;
    }
    *d = 0;

    /* calc argument size */
    size = 1; n = 0;
    for (p = (const char * const *)argv; *p != NULL; ++p)
    {
        ++n;
        size += 2 + strlen(*p);
    }
    d = alloca(size);
    LIBCLOG_MSG("arguments: %d bytes %d entries block=%p\n", size, n, d);
    np.arg_count = n; np.arg_size = size;
    np.arg_off = (unsigned long)d;
    np.arg_size2 = size >> 16;
    if (np.arg_size2)
        np.mode |= 0x80000000;

    /* copy arguments */
    for (p = (const char * const *)argv; *p != NULL; ++p)
    {
        i = strlen(*p);
        *(unsigned char *)d++ = _ARG_NONZERO;
        memcpy(d, *p, i + 1);
        d += i + 1;
    }
    *d = 0;

    /*
     * Call syscall.
     */
    rc = __spawnve(&np);
    LIBCLOG_RETURN_INT(rc);
}
