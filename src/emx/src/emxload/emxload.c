/* emxload.c -- Keep OS/2 programs in memory
  Copyright (c) 1993-1998 Eberhard Mattes

This file is part of emxload.

emxload is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

emxload is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with emxload; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

#define NO_EXAPIS
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <limits.h>
#include <time.h>
#include <alloca.h>
#include <sys/emxload.h>
#include <emx/emxload.h>
#define INCL_DOSPROCESS
#define INCL_DOSNMPIPES
#define INCL_DOSSEMAPHORES
#define INCL_DOSDEVICES
#define INCL_DOSMISC
#define INCL_DOSERRORS
#include <os2.h>

/* ------------------------------ SERVER ------------------------------ */

/* This is the maximum number of processes that can be managed by the
   emxload server. */

#define MAX_PROCS 256

/* A process table entry.  PID is the process ID, STOP is the time at
   which the program will be unloaded (0 means never unload), NAME is
   the path name of the executable.  If NAME is NULL, the slot is
   empty. */

typedef struct
{
  int pid;
  time_t stop;
  char *name;
} proc;

/* The name of the pipe. */
static char const pipe_name[] = _EMXLOAD_PIPENAME;

/* The table of the processes. */
static proc table[MAX_PROCS];

/* This many entries of the process table are initialized. */
static int procs = 0;

/* Mutex semaphore for protecting the process table. */
static HMTX hmtxTable;

/* This event semaphore is posted when the process table has been
   changed. */
static HEV hevReady;

/* The server pipe handle. */
static HPIPE hp;

/* These variables are used for debugging.  Use `-debug FNAME' instead
   of `-server' on the command line to start the server manually in
   debug mode.  You can also use the `EMXLOAD_DEBUG_FILE' environment
   variable to set the file name.  In that case, debugging output will
   also be enabled for `-server'.  If `debug_file' is non-NULL, the
   server will print informational messages and error message to
   FNAME.  Usually, the server is started detached and doesn't display
   anything. */
static char *debug_fname = NULL;
static FILE *debug_file = NULL;

/* Prototypes. */

static void s_unload_all (void);
static void debug (const char *fmt, ...)
#ifdef __GNUC__
    __attribute__ ((__format__ (__printf__, 1, 2)))
#endif
  ;

/* Debugging output.  This function is called like printf(). */

static void debug (const char *fmt, ...)
{
  va_list arg_ptr;

  if (debug_file == NULL)
    return;
  va_start (arg_ptr, fmt);
  vfprintf (debug_file, fmt, arg_ptr);
  va_end (arg_ptr);
  fflush (debug_file);
}


/* Load the program NAME into memory and create a process table entry.
   The program will be unloaded at the time indicated by STOP.  If
   STOP is 0, the program won't be unloaded automatically.  This
   function is called only while the process table is locked. */

static void s_load (const char *name, time_t stop)
{
  int i;
  RESULTCODES result;
  uDB_t dbgbuf;
  CHAR objbuf[16];
  PCHAR args;
  size_t len;
  APIRET rc;

  /* Find an unused slot in the process table. */

  for (i = 0; i < procs; ++i)
    if (table[i].name == NULL)
      break;
  if (i >= procs)
    {
      if (procs >= MAX_PROCS)
        {
          debug ("Too many programs\n");
          return;
        }
      i = procs++;
      table[i].pid = -1;
      table[i].name = NULL;
      table[i].stop = 0;
    }

  /* Build the command line. */

  len = strlen (name);
  args = alloca (len + 3);
  memcpy (args, name, len);
  args[len+0] = 0;
  args[len+1] = 0;
  args[len+2] = 0;

  /* Load the program without running it.  Originally, we used
     EXEC_LOAD, but emxload lost its children under certain
     circumstances, perhaps due to a bug in the OS/2 kernel. */

  rc = DosExecPgm (objbuf, sizeof (objbuf), EXEC_TRACE, args, NULL,
                   &result, name);
  if (rc == 0)
    {
      /* Now connect to the debuggee.  If we didn't do this, the child
         process would be killed by OS/2 after 10 minutes. */

      memset (&dbgbuf, 0, sizeof (dbgbuf));
      dbgbuf.Addr = 0;          /* Sever connection to avoid deadlock */
      dbgbuf.Pid = result.codeTerminate;
      dbgbuf.Tid = 0;           /* Reserved */
      dbgbuf.Cmd = DBG_C_Connect; /* Connect */
      dbgbuf.Value = DBG_L_386; /* Level */
      rc = DosDebug (&dbgbuf);
      if (rc == 0 && dbgbuf.Cmd == DBG_N_Success)
        {
          /* Fill-in the process table entry. */

          table[i].stop = stop;
          table[i].name = strdup (name);
          if (table[i].name == NULL)
            {
              debug ("Out of memory\n");
              s_unload_all ();
              exit (2);
            }
          table[i].pid = result.codeTerminate;

          /* Notify the other thread. */

          DosPostEventSem (hevReady);
        }
      else
        debug ("Cannot connect to %s, rc=%lu\n", name, rc);
    }
  else
    debug ("Cannot load %s, rc=%lu\n", name, rc);
}


/* Unload the program in process table entry I.  The process table
   entry may be unused, see unload_all().  After calling unload(), the
   process table entry will be marked unused. */

static void s_unload (int i)
{
  uDB_t dbgbuf;

  if (table[i].name != NULL)
    {
      memset (&dbgbuf, 0, sizeof (dbgbuf));
      dbgbuf.Pid = table[i].pid;
      dbgbuf.Cmd = DBG_C_Term;
      if (DosDebug (&dbgbuf) != 0)
        debug ("Cannot unload %s\n", table[i].name);
      free (table[i].name);
      table[i].name = NULL;
      table[i].pid = -1;
    }
}


/* Unload all programs. */

static void s_unload_all (void)
{
  int i;

  for (i = 0; i < procs; ++i)
    s_unload (i);
}


/* Automatically unload programs.  This function never returns. */

static void auto_unload (void)
{
  int i, next_set, more;
  ULONG timeout, post_count, rc;
  time_t now, next;

  for (;;)
    {

      /* Request exclusive access to the process table. */

      DosRequestMutexSem (hmtxTable, SEM_INDEFINITE_WAIT);
      do
        {

          /* Scan the table, unloading programs which have expired.
             Compute the time to sleep until the next program will
             expire.  To improve accuracy of the timing, we repeat
             this loop until no program has been unloaded. */

          time (&now);
          next = 0; next_set = FALSE; more = FALSE;
          for (i = 0; i < procs; ++i)
            if (table[i].name != NULL && table[i].stop != 0)
              {
                if (now >= table[i].stop)
                  {
                    debug ("Unloading %s\n", table[i].name);
                    s_unload (i);
                    more = TRUE;
                  }
                else if (!next_set)
                  {
                    next = table[i].stop;
                    next_set = TRUE;
                  }
                else if (table[i].stop < next)
                  next = table[i].stop;
              }
        } while (more);
      DosReleaseMutexSem (hmtxTable);

      if (next_set)
        debug ("Waiting for %u seconds\n", (unsigned)(next - now));
      else
        debug ("Waiting forever\n");

      /* Wait until next program expires or until a thread changes the
         process table. */

      timeout = (next_set ? (next - now) * 1000 : SEM_INDEFINITE_WAIT);
      rc = DosWaitEventSem (hevReady, timeout);
      rc = DosResetEventSem (hevReady, &post_count);
    }
}


/* Send answer to client.  Return 0 on success, -1 on failure. */

static int s_answer (const answer *ans)
{
  ULONG rc, cb;

  rc = DosWrite (hp, ans, sizeof (answer), &cb);
  return (rc == 0 ? 0 : -1);
}


/* Send acknowledge to client. */

static void s_ack (void)
{
  answer ans;

  ans.ans_code = 0;
  s_answer (&ans);
}


/* Load or unload a program.  This function is called by the thread
   which reads the pipe. */

static void s_load_unload (const request *req)
{
  int i;
  time_t now, stop;

  stop = 0;

  /* Compute the time when the program will be unloaded. */

  if (req->req_code == _EMXLOAD_LOAD && req->seconds != _EMXLOAD_INDEFINITE)
    {
      time (&now);
      stop = now + req->seconds;
    }

  /* Request exclusive access to the process table. */

  DosRequestMutexSem (hmtxTable, SEM_INDEFINITE_WAIT);

  /* Scan table for a matching entry.  If the program is already in
     the table, update the time-out or unload the program. */

  for (i = 0; i < procs; ++i)
    if (table[i].name != NULL && strcasecmp (table[i].name, req->name) == 0)
      {
        if (req->req_code != _EMXLOAD_LOAD)
          s_unload (i);
        else
          {
            table[i].stop = stop;
            DosPostEventSem (hevReady);
          }
        break;
      }

  /* Load the program if the program is not in the table. */

  if (req->req_code == _EMXLOAD_LOAD && i >= procs)
    s_load (req->name, stop);
  if (req->req_code == _EMXLOAD_UNLOAD_WAIT)
    s_ack ();
  DosReleaseMutexSem (hmtxTable);
}


/* Send a list of preloaded programs to the client. */

static void s_list (void)
{
  int i;
  time_t now;
  answer ans;

  /* Request exclusive access to the process table. */

  DosRequestMutexSem (hmtxTable, SEM_INDEFINITE_WAIT);

  /* Get the current time. */

  time (&now);

  /* List all the process table entries. */

  for (i = 0; i < procs; ++i)
    if (table[i].name != NULL)
      {
        if (table[i].stop == 0)
          ans.seconds = _EMXLOAD_INDEFINITE;
        else
          {
            ans.seconds = table[i].stop - now;
            if (ans.seconds < 0)
              ans.seconds = 0;
          }
        _strncpy (ans.name, table[i].name, sizeof (ans.name));
        ans.ans_code = 0;
        if (s_answer (&ans) != 0)
          break;
      }

  ans.ans_code = 1;
  s_answer (&ans);
  DosReleaseMutexSem (hmtxTable);
}


static void read_pipe (void)
{
  ULONG rc, len;
  request req;

  for (;;)
    {
      rc = DosRead (hp, &req, sizeof (req), &len);
      if (rc == ERROR_BROKEN_PIPE)
        break;
      if (rc != 0)
        {
          debug ("Error code %lu\n", rc);
          s_unload_all ();
          exit (2);
        }
      if (len == 0)
        break;
      if (len != sizeof (request))
        debug ("Invalid record length: %lu\n", len);
      else
        switch (req.req_code)
          {
          case _EMXLOAD_LOAD:
          case _EMXLOAD_UNLOAD:
          case _EMXLOAD_UNLOAD_WAIT:
            s_load_unload (&req);
            break;
          case _EMXLOAD_STOP:
          case _EMXLOAD_STOP_WAIT:
            s_unload_all ();
            exit (0);
          case _EMXLOAD_LIST:
            s_list ();
            break;
          default:
            debug ("Unknown request code: %d\n", req.req_code);
            break;
          }
    }
}


/* Handle connections to the pipe.  This function never returns. */

static void connections (void)
{
  ULONG rc;

  for (;;)
    {
      rc = DosConnectNPipe (hp);
      debug ("Connected\n");
      read_pipe ();
      rc = DosDisConnectNPipe (hp);
      debug ("Disconnected\n");
    }
}


/* This is an additional thread.  It handles connections to the named
   pipe. */

static void thread (void *arg)
{
  connections ();
}


/* This is the main thread of the server. */

static void server (void)
{
  ULONG rc;

  /* Set our priority to foreground server.  This decreases the time
     clients have to wait on a heavily loaded system.  Moreover, we
     may have inherited idle-time priority from our parent... */

  DosSetPriority (PRTYS_PROCESS, PRTYC_FOREGROUNDSERVER, 0, 0);

  /* Open the debug output file if the `-debug FNAME' option is used
     or the EMXLOAD_DEBUG_FILE environment variable is set. */

  if (debug_fname == NULL)
    debug_fname = getenv ("EMXLOAD_DEBUG_FILE");
  if (debug_fname != NULL)
    debug_file = fopen (debug_fname, "w");

  /* Close all the file handles as we inherit the file handles of the
     parent process.  If we didn't close the file handles, the parent
     couldn't delete, rename etc. the files.  This should be done
     before creating the pipe to avoid a timing window. */

  {
    LONG req;
    ULONG i, cur;

    req = 0; cur = 0;
    if (DosSetRelMaxFH (&req, &cur) == 0)
      for (i = 0; i < cur; ++i)
        if (!(debug_file != NULL && i == fileno (debug_file)))
          DosClose ((HFILE)i);
  }

  /* Change to the root directory of all non-removable drivers and of
     the current drive (in case it's removable).  This is for allowing
     the user to rename or delete the directories which were the
     current directories when emxload was started. */

  {
    ULONG drive, drive_map, parmlen, datalen, rc;
    CHAR parm[2], data[1];
    static CHAR dir[] = "A:\\";

    DosError (FERR_DISABLEHARDERR | FERR_ENABLEEXCEPTION);
    if (DosQueryCurrentDisk (&drive, &drive_map) == 0)
      {
        for (drive = 0; drive < 26; ++drive)
          if (drive_map & (1 << drive))
            {
              parm[0] = 0; parm[1] = drive; data[0] = 0;
              parmlen = 2; datalen = 0;
              if (DosDevIOCtl ((HFILE)-1, 8, 0x20, parm, 2, &parmlen,
                               data, 1, &datalen) == 0
                  && datalen == 1 && data[0] == 1)
                {
                  dir[0] = (CHAR)('A' + drive);
                  rc = DosSetCurrentDir (dir);
                  debug ("DosSetCurrentDir(%s) -> %lu\n", dir, rc);
                }
            }
      }
    DosSetCurrentDir ("\\");
    DosError (FERR_ENABLEHARDERR | FERR_ENABLEEXCEPTION);
  }

  /* Create the named pipe. */

  rc = DosCreateNPipe (pipe_name, &hp, NP_NOINHERIT|NP_ACCESS_DUPLEX,
                       NP_WAIT|NP_TYPE_MESSAGE|NP_READMODE_MESSAGE|1,
                       0x1000, 0x1000, 0);
  if (rc == ERROR_PIPE_BUSY)
    {
      debug ("An emxload server is already running\n");
      exit (1);
    }
  if (rc != 0)
    {
      debug ("Error code %lu\n", rc);
      exit (2);
    }

  /* Create the semaphores. */

  rc = DosCreateMutexSem (NULL, &hmtxTable, 0, FALSE);
  rc = DosCreateEventSem (NULL, &hevReady, 0, FALSE);

  /* Start the second thread. */

  if (_beginthread (thread, NULL, 0x8000, NULL) == -1)
    {
      debug ("Cannot start thread\n");
      exit (2);
    }

  auto_unload ();
}


/* ------------------------------ CLIENT ------------------------------ */

/* This flag is set by the -u and -uw command line options.  All
   programs given on the command line will be unloaded instead of
   loaded. */
static int unload_flag = FALSE;

/* This flag is set by the -uw command line option.  Wait for
   completion. */
static int unload_wait = FALSE;

/* The number of seconds to keep the programs in memory.  May be
   _EMXLOAD_INDEFINITE. */
static int seconds;

/* Prototypes. */

static void usage (void);


/* Handle one request for loading or unloading (client side). */

static void c_load_unload (const char *name)
{
  if (unload_flag)
    {
      if (_emxload_unload (name, unload_wait) != 0)
        fprintf (stderr, "emxload: cannot unload %s\n", name);
    }
  else
    {
      if (_emxload_prog (name, seconds) != 0)
        fprintf (stderr, "emxload: cannot load %s\n", name);
    }
}


/* Parse the command line for the client. */

static void client (int argc, char **argv)
{
  int i, mul;
  int load_gcc, load_gpp, load_gobjc, load_gnat, load_omf;
  char *p, *q;

  /* Set the default values. */

  seconds = 10 * 60;
  load_gcc = FALSE; load_gpp = FALSE; load_gobjc = FALSE; load_gnat = FALSE;
  load_omf = FALSE;
  i = 1;

  /* Parse the options. */

  while (i < argc && argv[i][0] == '-')
    {
      p = argv[i++];
      if (strcmp (p, "-e") == 0)
        seconds = _EMXLOAD_INDEFINITE;
      else if (strcmp (p, "-u") == 0)
        unload_flag = TRUE;
      else if (strcmp (p, "-uw") == 0)
        {
          unload_flag = TRUE;
          unload_wait = TRUE;
        }
      else if (strncmp (p, "-m", 2) == 0 || strncmp (p, "-s", 2) == 0)
        {
          mul = (p[1] == 'm' ? 60 : 1);
          p += 2;
          if (*p == 0)
            {
              if (i >= argc)
                usage ();
              p = argv[i++];
            }
          errno = 0;
          seconds = (int)strtol (p, &q, 10);
          if (seconds < 1 || errno != 0 || q == p || *q != 0)
            usage ();
          seconds *= mul;
        }
      else if (strcmp (p, "-g++") == 0)
        load_gpp = TRUE;
      else if (strcmp (p, "-gcc") == 0)
        load_gcc = TRUE;
      else if (strcmp (p, "-gobjc") == 0)
        load_gobjc = TRUE;
      else if (strcmp (p, "-gnat") == 0)
        load_gnat = TRUE;
      else if (strcmp (p, "-omf") == 0)
        load_omf = TRUE;
      else
        usage ();
    }

  /* At least program must be given. */

  if (i >= argc && !(load_gcc || load_gpp || load_gobjc || load_gnat
                     || load_omf))
    usage ();

  /* Connect to the server and keep the connection open. */

  if (!unload_flag && _emxload_connect () != 0)
    {
      fputs ("emxload: cannot start emxload server\n", stderr);
      exit (2);
    }

  /* Load or unload the programs. */

  if (load_gcc || load_gpp || load_gobjc || load_gnat)
    {
      c_load_unload ("gcc");
      c_load_unload ("as");
      c_load_unload ("ld");
      c_load_unload ("emxbind");
    }
  if (load_gcc || load_gpp || load_gobjc)
    c_load_unload ("cpp");
  if (load_gcc)
    c_load_unload ("cc1");
  if (load_gpp)
    c_load_unload ("cc1plus");
  if (load_gobjc)
    c_load_unload ("cc1obj");
  if (load_gnat)
    {
      c_load_unload ("gnat1");
      c_load_unload ("gnatbind");
    }
  if (load_omf)
    {
      c_load_unload ("emxomf");
      c_load_unload ("emxomfld");
      c_load_unload ("link386");
    }
  while (i < argc)
    c_load_unload (argv[i++]);
}


/* List preloaded programs (client side). */

static void c_list (void)
{
  char name[260], timeout[20], *p;
  int seconds, header;

  if (_emxload_list_start () != 0)
    return;

  header = FALSE;
  while (_emxload_list_get (name, sizeof (name), &seconds) == 0)
    {
      if (!header)
        {
          puts ("Time-out � Program");
          puts ("컴컴컴컴컵컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴");
          header = TRUE;
        }
      if (seconds == _EMXLOAD_INDEFINITE)
        strcpy (timeout, "NONE");
      else
        sprintf (timeout, "%d'%.2d", seconds / 60, seconds % 60);
      for (p = name; *p != 0; ++p)
        if (*p == '/')
          *p = '\\';
      printf ("%8s � %s\n", timeout, name);
    }
}

/* ------------------------------- MAIN ------------------------------- */

/* How to call this program. */

static void usage (void)
{
  fputs ("emxload " VERSION INNOTEK_VERSION " -- "
         "Copyright (c) 1993-1996 by Eberhard Mattes\n\n"
         "Usage: emxload [-m <limit>] [-s <limit>] [-e] [-u[w]]\n"
         "               [-gcc] [-g++] [-gobjc] [-gnat] [-omf] <program>...\n"
         "       emxload -l\n"
         "       emxload -q\n"
         "       emxload -qw\n\n"
         "Options:\n\n"
         "-m <limit>   Unload programs after <limit> minutes (default: 10)\n"
         "-s <limit>   Unload programs after <limit> seconds (default: 600)\n"
         "-e           No limit\n"
         "-u           Unload programs (overrides -m, -s and -e)\n"
         "-uw          Like -u, but wait until completed\n"
         "-gcc         Load gcc, cpp, cc1, as, ld, emxbind\n"
         "-g++         Load gcc, cpp, cc1plus, as, ld, emxbind\n"
         "-gobjc       Load gcc, cpp, cc1obj, as, ld, emxbind\n"
         "-gnat        Load gcc, gnat1, as, ld, gnatbind, emxbind\n"
         "-omf         Load emxomf, emxomfld, link386\n"
         "-l           List preloaded programs\n"
         "-q           Stop server\n"
         "-qw          Like -w, but wait until completed\n", stderr);
  exit (1);
}


/* Main line. */

int main (int argc, char **argv)
{
  if (argc == 2 && strcmp (argv[1], "-server") == 0)
    server ();
  else if (argc == 3 && strcmp (argv[1], "-debug") == 0)
    {
      debug_fname = argv[2];
      server ();
    }
  else if (argc == 2 && strcmp (argv[1], "-l") == 0)
    c_list ();
  else if (argc == 2 && strcmp (argv[1], "-q") == 0)
    _emxload_stop (FALSE);
  else if (argc == 2 && strcmp (argv[1], "-qw") == 0)
    _emxload_stop (TRUE);
  else
    client (argc, argv);
  return 0;
}
