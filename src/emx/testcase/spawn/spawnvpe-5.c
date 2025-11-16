/*
 * spawnvp() + pipe()/socketpair()/select() test program
 *
 * Copyright (C) 2025 Dmitriy Kuminov <coding@dmik.org>
 *
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://www.wtfpl.net/ for more details.
 */

#define INCL_BASE
#include <os2.h>

#include <errno.h>
#include <fcntl.h>
#include <process.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#include <emx/io.h>

// Use socketpair instead of pipe since kLIBC select can't handle them
#define pipe(p) socketpair (AF_UNIX, SOCK_STREAM, 0, (p))

#define LINE_BUFFERED

static int set_nonblock (int fd)
{
  int fl = fcntl (fd, F_GETFL, 0);
  if (fl < 0)
    return - 1;
  return fcntl (fd, F_SETFL, fl | O_NONBLOCK);
}

static int set_cloexec (int fd)
{
  int fl = fcntl (fd, F_GETFD, 0);
  if (fl < 0)
    return - 1;
  return fcntl (fd, F_SETFD, fl | FD_CLOEXEC);
}

static int save_stdin = -1;
static int save_stdout = -1;
static int save_stderr = -1;

static void restore_stdio ()
{
  dup2 (save_stdin,  STDIN_FILENO);
  dup2 (save_stdout, STDOUT_FILENO);
  dup2 (save_stderr, STDERR_FILENO);
  close (save_stdin);
  close (save_stdout);
  close (save_stderr);
}

int main (int argc, char * argv [])
{
  {
    char buf [1024];
    APIRET arc;
    strcpy (buf, "<error>");
    arc = DosQueryExtLIBPATH (buf, BEGIN_LIBPATH);
    printf ("BEGINLIBPATH=[%s] (arc %ld)\n", buf, arc);
    strcpy (buf, "<error>");
    arc = DosQueryExtLIBPATH (buf, END_LIBPATH);
    printf ("ENDLIBPATH=[%s] (arc %ld)\n", buf, arc);
    memset (buf, '\0', 2); // LIBPATHSTRICT returns just T or nothing
    arc = DosQueryExtLIBPATH (buf, LIBPATHSTRICT);
    printf ("LIBPATHSTRICT=[%s] (arc %ld)\n", buf, arc);
  }

  save_stdin  = dup (STDIN_FILENO);
  save_stdout = dup (STDOUT_FILENO);
  save_stderr = dup (STDERR_FILENO);

  if (save_stdin < 0 || save_stdout < 0 || save_stderr < 0)
  {
    restore_stdio ();
    perror ("dup");
    return 1;
  }

  int pin [2];
  int pout [2];
  int perr [2];

  if (pipe (pin) || pipe (pout) || pipe (perr))
  {
    restore_stdio ();
    perror ("pipe");
    return 1;
  }

  if (dup2 (pin [0], STDIN_FILENO)  < 0 ||
      dup2 (pout [1], STDOUT_FILENO) < 0 ||
      dup2 (perr [1], STDERR_FILENO) < 0)
  {
    restore_stdio ();
    perror ("dup2");
    return 1;
  }

  close (pin [0]);
  close (pout [1]);
  close (perr [1]);

  int in_w  = pin [1];
  int out_r = pout [0];
  int err_r = perr [0];

  if (set_cloexec (in_w) || set_cloexec (out_r) || set_cloexec (err_r))
  {
    restore_stdio ();
    perror ("fcntl FD_CLOEXEC");
    return 1;
  }

  const char * argv_1 [] =
  {
    "cmd", "/c",
    "dash", "-c", // Use dash since it's unlikely to be running (useful for LIBPATHSTRICT=T)
    "while read line; do echo $line; echo $line 1>&2; done",
    NULL
  };

  const char * argv_2 [] =
  {
    "dash", "-c",
    "wl",
    NULL
  };

  const char * argv_3 [] =
  {
    "dash", "-c", // Use dash since it's unlikely to be running (useful for LIBPATHSTRICT=T)
    "while read line; do echo $line; echo $line 1>&2; done",
    NULL
  };

  const char * const * argv_c;

  if (argc <= 1 || ! strcmp (argv [1], "1"))
    argv_c = argv_1;
  else if (! strcmp (argv [1], "2"))
    argv_c = argv_2;
  else if (! strcmp (argv [1], "3"))
    argv_c = argv_3;
  else
    argv_c = argv_1;

  int pid = spawnvp (P_NOWAIT, argv_c [0], (char * const *) argv_c);

  restore_stdio ();

  if (pid < 0)
  {
    perror ("spawnvp");
    return 1;
  }

  if (set_nonblock (out_r) || set_nonblock (err_r))
  {
    perror ("fcntl O_NONBLOCK");
    return 1;
  }

  int out_open = 1;
  int err_open = 1;
  int child_exited = 0;

  char buf [4096];

  struct sigaction sa = {0};
  sa.sa_handler = SIG_IGN;
  sigaction(SIGPIPE, &sa, NULL);

  /* Write a few lines to child's stdin */
  const char * input_lines [] = { "one\n", "two\n", "three\n", NULL };
  for (int i = 0; input_lines [i]; ++ i)
  {
    size_t len = strlen (input_lines [i]);
    if (write (in_w, input_lines [i], len) != (ssize_t) len)
      perror ("write stdin");
  }
  close (in_w); /* EOF on child's stdin */

#ifdef LINE_BUFFERED
  FILE * out_r_fp = fdopen (out_r, "rt");
  setvbuf (out_r_fp, NULL, _IOLBF, 0);

  FILE * err_r_fp = fdopen (err_r, "rt");
  setvbuf (err_r_fp, NULL, _IOLBF, 0);
#endif

  int status;

  while (! child_exited || out_open || err_open)
  {
    fd_set rfds;
    FD_ZERO (& rfds);
    int maxfd = - 1;

    if (out_open)
    {
      FD_SET (out_r, & rfds);
      if (out_r > maxfd)
        maxfd = out_r;
    }

    if (err_open)
    {
      FD_SET (err_r, & rfds);
      if (err_r > maxfd)
        maxfd = err_r;
    }

    struct timeval tv = { 1, 0 };
    int n = select (maxfd + 1, & rfds, NULL, NULL, & tv);
    if (n < 0)
    {
      if (errno == EINTR)
        continue;
      perror ("select");
      break;
    }

    if (n > 0 && out_open && FD_ISSET (out_r, & rfds))
    {
      for (;;)
      {
#ifdef LINE_BUFFERED
        char * r = fgets (buf, sizeof buf, out_r_fp);
        if (r)
#else
        ssize_t r = read (out_r, buf, sizeof buf);
        if (r > 0)
#endif
        {
          fputs ("[stdout] ", stdout);
#ifdef LINE_BUFFERED
          fputs (buf, stdout);
#else
          fwrite (buf, 1, (size_t) r, stdout);
#endif
          fflush (stdout);
        }
#ifdef LINE_BUFFERED
        else if (feof (out_r_fp))
        {
          fclose (out_r_fp);
#else
        else if (r == 0)
        {
          close (out_r);
#endif
          out_open = 0;
          break;
        }
        else // if (ferror (out_r_fp)) for LINE_BUFFERED
        {
          if (errno == EAGAIN || errno == EWOULDBLOCK)
            break;
          else if (errno == EINTR)
            continue;
          else
          {
            perror ("fgets stdout");
#ifdef LINE_BUFFERED
            fclose (out_r_fp);
#else
            close (out_r);
#endif
            out_open = 0;
            break;
          }
        }
      }
    }

    if (n > 0 && err_open && FD_ISSET (err_r, & rfds))
    {
      for (;;)
      {
#ifdef LINE_BUFFERED
        char * r = fgets (buf, sizeof buf, err_r_fp);
        if (r)
#else
        ssize_t r = read (err_r, buf, sizeof buf);
        if (r > 0)
#endif
        {
          fputs ("[stderr] ", stdout);
#ifdef LINE_BUFFERED
          fputs (buf, stdout);
#else
          fwrite (buf, 1, (size_t) r, stdout);
#endif
          fflush (stdout);
        }
#ifdef LINE_BUFFERED
        else if (feof (err_r_fp))
        {
          fclose (err_r_fp);
#else
        else if (r == 0)
        {
          close (err_r);
#endif
          err_open = 0;
          break;
        }
        else // if (ferror (err_r_fp)) for LINE_BUFFERED
        {
          if (errno == EAGAIN || errno == EWOULDBLOCK)
            break;
          else if (errno == EINTR)
            continue;
          else
          {
            perror ("read stderr");
#ifdef LINE_BUFFERED
            fclose (err_r_fp);
#else
            close (err_r);
#endif
            err_open = 0;
            break;
          }
        }
      }
    }

    if (! child_exited)
    {
      int w = waitpid (pid, & status, WNOHANG);
      if (w == pid)
      {
        child_exited = 1;
      }
      else if (w < 0)
      {
        perror ("waitpid");
        break;
      }
    }
  }

  if (child_exited)
  {
    if (WIFEXITED (status))
      fprintf (stdout, "[child exited %d]\n", WEXITSTATUS (status));
    else if (WIFSIGNALED (status))
      fprintf (stdout, "[child killed by signal %d]\n", WTERMSIG (status));
    fflush (stdout);
  }

  return 0;
}