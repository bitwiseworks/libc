/* popen.c (emx+gcc) -- Copyright (c) 1992-1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <process.h>
#include <unistd.h>
#include <io.h>
#include <fcntl.h>
#include <errno.h>
#include <InnotekLIBC/backend.h>

static void restore (int org_handle, int org_private, int handle)
{
  int saved_errno;

  saved_errno = errno;
  dup2 (org_handle, handle);
  close (org_handle);
  fcntl (handle, F_SETFD, org_private);
  errno = saved_errno;
}

static FILE *make_pipe (int pipe_local, int pipe_remote, int handle,
                        const char *command, const char *mode)
{
  int i, org_handle, org_private;
  FILE *f;
  char szShell[260];
  size_t offShellArg;
  char szCmdLineOpt[8];
  int rc;

  fcntl (pipe_local, F_SETFD, FD_CLOEXEC);
  org_private = fcntl (handle, F_GETFD, 0);
  if (org_private == -1)
    return NULL;
  org_handle = dup (handle);
  if (org_handle == -1)
    return NULL;
  fcntl (org_handle, F_SETFD, FD_CLOEXEC);

  i = dup2 (pipe_remote, handle);
  if (i != handle)
    {
      restore (org_handle, org_private, handle);
      errno = EBADF;
      return NULL;
    }
  if (close (pipe_remote) == -1)
    {
      restore (org_handle, org_private, handle);
      return NULL;
    }
  f = fdopen (pipe_local, mode);
  if (f == NULL)
    {
      restore (org_handle, org_private, handle);
      return NULL;
    }
  rc = __libc_Back_processGetDefaultShell (&szShell[0], sizeof(szShell), &offShellArg, &szCmdLineOpt[0], sizeof(szCmdLineOpt));
  if (rc != 0)
    {
      fclose (f);
      restore (org_handle, org_private, handle);
      errno = -rc;
      return NULL;
    }
  i = spawnlp (P_NOWAIT, szShell, &szShell[offShellArg], szCmdLineOpt, command, (char *)0);
  if (i == -1)
    {
      fclose (f);
      f = NULL;
    }
  else
    f->_pid = i;
  restore (org_handle, org_private, handle);
  return f;
}


FILE *_STD(popen) (const char *command, const char *mode)
{
  int ph[2];
  FILE *stream;
  int saved_errno;

  if (mode[0] != 'r' && mode[0] != 'w')
    {
      errno = EINVAL;
      return NULL;
    }
  if (pipe (ph) == -1)
    return NULL;
  if (fcntl (ph[0], F_SETFD, FD_CLOEXEC) == -1)
    return NULL;
  if (fcntl (ph[1], F_SETFD, FD_CLOEXEC) == -1)
    return NULL;
  if (mode[0] == 'r')
    stream = make_pipe (ph[0], ph[1], STDOUT_FILENO, command, mode);
  else
    stream = make_pipe (ph[1], ph[0], STDIN_FILENO, command, mode);
  if (stream == NULL)
    {
      saved_errno = errno;
      close (ph[0]);
      close (ph[1]);
      errno = saved_errno;
    }
  return stream;
}
