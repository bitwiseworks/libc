/* process.c -- Manage processes
   Copyright (c) 1993-2000 by Eberhard Mattes

This file is part of emx.

emx is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

emx is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with emx; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.

As special exception, emx.dll can be distributed without source code
unless it has been changed.  If you modify emx.dll, this exception
no longer applies and you must remove this paragraph from all source
files for emx.dll.  */


#define INCL_DOSPROCESS
#define INCL_DOSEXCEPTIONS
#define INCL_DOSSEMAPHORES
#define INCL_DOSSESMGR
#define INCL_DOSMODULEMGR
#define INCL_DOSQUEUES
#define INCL_DOSERRORS
#define INCL_DOSDEVIOCTL        /* For files.h */
#include <os2emx.h>
#include <os2thunk.h>
#include <emx/syscalls.h>
#include <sys/signal.h>
#include <sys/errno.h>
#include <sys/wait.h>
#include <sys/process.h>
#include <sys/ptrace.h>
#include <sys/uflags.h>
#include "emxdll.h"
#include "files.h"
#include "clib.h"

/* Maximum sizes of various buffers and tables. */

#define MAX_THREADS     1024    /* Number of threads */
#define MAX_PROCESSES   256     /* Number of child processes */
#define FORK_REGMEM_MAX 256     /* Number of DLLs supported by fork() */
#define FORK_REGDLL_MAX 256     /* Number of DLLs supported by fork() */

/* Values for the `status' field of `struct process'. */

#define PS_FREE         0       /* Empty slot */
#define PS_PROCESS      1       /* Child is a child process */
#define PS_SESSION      2       /* Child is a session */
#define PS_ZOMBIE       3       /* Process ended, return code available */
#define PS_ATTACH       4       /* Attached debuggee */

/* Bits for the `flags' field of `struct process'. */

#define PF_SYNC         0x0001  /* Synchronous child process */
#define PF_DEBUG        0x0002  /* Child process is being debugged */
#define PF_WAIT         0x0004  /* wait() status available */


/* `process_table' is protected by a Mutex semaphore.  These macros
   are used for requesting and releasing that semaphore. */

#define LOCK_PROCESS   request_mutex (process_access)
#define UNLOCK_PROCESS DosReleaseMutexSem (process_access)


/* This structure describes a child process. */

struct process
{
  ULONG status;                 /* Process status: PS_FREE etc. */
  ULONG pid;                    /* Process ID */
  ULONG sid;                    /* Session ID (for PS_SESSION) */
  ULONG wait_tid;               /* Thread ID for wait() */
  ULONG wait_ret;               /* Return value for wait() */
  ULONG flags;                  /* Process flags: PF_SYNC */
};

/* This structure describes one registered memory area for fork(),
   such as a DLL data segment. */

struct fork_regmem
{
  ULONG start;
  ULONG end;
  HMODULE hmod;
};

/* This structure describes one registered DLL for fork(). */

struct fork_regdll
{
  HMODULE hmod;
};

/* This table keeps track of child processes. */

static struct process process_table[MAX_PROCESSES];

/* For each thread, a pointer to its thread data block is stored in
   this table indexed by thread IDs. */

thread_data *threads[MAX_THREADS];

/* `process_table' is protected by this Mutex semaphore. */

static HMTX process_access;

/* These event semaphores are posted when a process has been started
   or has ended, respectively. */

static HEV process_birth;
static HEV process_death;

/* These variables are true if cwait_thread() or qwait_thread(),
   respectively, have been started. */

static BYTE cwait_started;
static BYTE qwait_started;

/* These variables hold the thread IDs for cwait_thread() and
   qwait_thread(), respectively. */

static ULONG cwait_tid;
static ULONG qwait_tid;

/* The handle and name of the termination queue read by
   qwait_thread(). */

static HQUEUE termq_handle;
static char termq_name[64];
static char termq_created;

/* Number of zombies. */

int zombie_count;

/* Number of registered memory areas for fork().  This variable must
   be implicitely initialized to zero. */

static int fork_regmem_count;

/* This variable is made non-zero when `fork_regmem' overflows.  This
   variable must be implicitely initialized to zero. */

static char fork_regmem_overflow;

/* Table of registered memory areas for fork().  The first
   `fork_regmem_count' entries are used.  TODO: Use linked list. */

static struct fork_regmem fork_regmem[FORK_REGMEM_MAX];

/* Number of registered DLLs for fork().  This variable must be
   implicitely initialized to zero. */

static int fork_regdll_count;

/* This variable is made non-zero when `fork_regdll' overflows.  This
   variable must be implicitely initialized to zero. */

static char fork_regdll_overflow;

/* Table of DLLs for fork().  The first `fork_regdll_count' entries
   are used.  TODO: Use linked list. */

static struct fork_regdll fork_regdll[FORK_REGDLL_MAX];

/* This variable is non-zero in the parent process during fork()'s
   DosExecPgm call; it's used by ptrace() to decide whether
   PTNPROC_FORK should be set or not. */

BYTE ptrace_forking;

/* If `ptrace_forking' is non-zero, the following variable will
   contain the address at which the two processes will continue/start
   after the fork.  This is the return address of the system call. */

ULONG ptrace_fork_addr;


/* Initialize the process table etc. */

void init_process (void)
{
  ULONG i;

  /* Create the semaphores. */

  create_mutex_sem (&process_access);
  create_event_sem (&process_death, DC_SEM_SHARED);
  create_event_sem (&process_birth, DC_SEM_SHARED);

  /* There are no child processes. */

  for (i = 0; i < MAX_PROCESSES; ++i)
    process_table[i].status = PS_FREE;
  zombie_count = 0;
}


/* Find free process table slot.  Return a pointer to a free process
   table entry.  If no free slot is found, return NULL.  It is assumed
   that exclusive access to the process table has been gained. */

static struct process *proc_new (void)
{
  ULONG i;

  for (i = 0; i < MAX_PROCESSES; ++i)
    if (process_table[i].status == PS_FREE)
      return &process_table[i];
  return NULL;
}


/* Find process table slot by process ID.  Return a pointer to the
   process table entry.  If there's no such process in the process
   table, return NULL.  It is assumed that exclusive access to the
   process table has been gained. */

static struct process *proc_find (ULONG pid)
{
  ULONG i;

  for (i = 0; i < MAX_PROCESSES; ++i)
    if (process_table[i].pid == pid && process_table[i].status != PS_FREE)
      return &process_table[i];
  return NULL;
}


/* Return true if process PID is in the process table. */

int proc_check (int pid)
{
  struct process *proc;

  LOCK_PROCESS;
  proc = proc_find (pid);
  UNLOCK_PROCESS;
  return proc != NULL;
}


/* A child has been started; notify cwait_thread(). */

void child_started (void)
{
  DosPostEventSem (process_birth);
}


/* Child process ended, post the `process_death' semaphore and raise
   SIGCLD. */

static void proc_death (int thread1)
{
  struct signal_entry *p;

  DosPostEventSem (process_death);
  p = threads[1]->sig_table + SIGCLD;
  if (p->handler != SIG_DFL && p->handler != SIG_IGN)
    {
      generate_signal (threads[1], SIGCLD);
      if (get_tid () == 1)
        deliver_pending_signals (threads[1], NULL, NULL);
    }
}


/* Add a PS_ATTACH entry to the process table.  Such an entry is used
   when attaching to a debuggee which is not a child process.  Return
   errno. */

ULONG proc_add_attach (ULONG pid)
{
  struct process *proc;

  LOCK_PROCESS;
  proc = proc_new ();
  if (proc != NULL)
    {
      proc->status = PS_ATTACH;
      proc->pid = pid;
      proc->sid = 0;            /* TODO */
      proc->flags = PF_DEBUG;
    }
  UNLOCK_PROCESS;
  return (proc == NULL ? EAGAIN : 0);
}


void proc_remove_attach (ULONG pid)
{
  struct process *proc;

  LOCK_PROCESS;
  proc = proc_find (pid);
  if (proc->status == PS_ATTACH)
    proc->status = PS_FREE;
  UNLOCK_PROCESS;
}


/* A child process has ended.  PID is the process ID of the process.
   If PID is zero, use the session ID SID instead for identifying the
   process.  RET is the return value for wait().  Update the process
   table and call proc_death(). */

static void bury (ULONG pid, ULONG sid, ULONG ret)
{
  struct process *proc;
  int i;

  if (pid == 0)
    {
      /* Find the process ID for the head process of session SID. */

      LOCK_PROCESS;
      for (i = 0; i < MAX_PROCESSES; ++i)
        if (process_table[i].sid == sid
            && process_table[i].status == PS_SESSION)
          {
            pid = process_table[i].pid;
            break;
          }
      UNLOCK_PROCESS;
    }
  if (pid != 0)
    {
      /* Turn the process into a zombie and call proc_death(). */

      LOCK_PROCESS;
      proc = proc_find (pid);
      if (proc == NULL)
        UNLOCK_PROCESS;
      else
        {
          proc->wait_ret = ret;
          proc->status = PS_ZOMBIE;
          ++zombie_count;
          UNLOCK_PROCESS;
          proc_death (FALSE);
        }
    }
}


/* Exception handler for cwait_thread().  On termination of the
   thread, cwait_tid will be set to zero. */

static ULONG cwait_exception (EXCEPTIONREPORTRECORD *report,
                              EXCEPTIONREGISTRATIONRECORD *registration,
                              CONTEXTRECORD *context,
                              void *dummy)
{
  if (report->fHandlerFlags & (EH_EXIT_UNWIND|EH_UNWINDING))
    return XCPT_CONTINUE_SEARCH;
  switch (report->ExceptionNum)
    {
    case XCPT_ASYNC_PROCESS_TERMINATE: /* This is the correct one */
    case XCPT_PROCESS_TERMINATE:       /* OS/2 bug */
      cwait_tid = 0;
      break;
    }
  return XCPT_CONTINUE_SEARCH;
}


/* This thread handles termination of child processes started with
   DosExec. */

static void cwait_thread (ULONG arg)
{
  RESULTCODES ret_codes;
  ULONG rc, pid, count, ret, tmp;
  EXCEPTIONREGISTRATIONRECORD registration;

  registration.prev_structure = NULL;
  registration.ExceptionHandler = cwait_exception;
  DosSetExceptionHandler (&registration);

  arg = 0;                      /* keep the compiler happy */
  for (;;)
    {
      DosResetEventSem (process_birth, &count);

      /* Originally, DCWA_PROCESSTREE was used instead of DCW_PROCESS
         to avoid reporting the death of a forked process that exec's
         another process.  Now, exec() in a process that has been
         created by fork() stays alive until the exec'd process
         terminates, so DCWA_PROCESSTREE is no longer required,
         avoiding some anomalies. */

      rc = DosWaitChild (DCWA_PROCESS, DCWW_WAIT, &ret_codes, &pid, 0);
      if (rc == ERROR_WAIT_NO_CHILDREN)
        DosWaitEventSem (process_birth, 5000);
      else
        {
          ret = ret_codes.codeResult;
          if (ret > 0xff)
            ret = 0xff;
          switch (ret_codes.codeTerminate)
            {
            case TC_EXIT:
              tmp = ret << 8;
              break;
            case TC_HARDERROR:
            case TC_KILLPROCESS:
              tmp = SIGTERM;
              break;
            default:
              tmp = SIGSEGV;
              break;
            }
          bury (pid, 0, tmp);
        }
    }
}


/* Start cwait_thread() if not already done. */

void start_cwait_thread (void)
{
  ULONG rc;

  if (!cwait_started)
    {
      cwait_started = TRUE;
      rc = DosCreateThread (&cwait_tid, cwait_thread, 0,
                            CREATE_READY | STACK_COMMITTED, 0x4000);
    }
}


/* Exception handler for qwait_thread().  On termination of the
   thread, qwait_tid will be set to zero. */

static ULONG qwait_exception (EXCEPTIONREPORTRECORD *report,
                              EXCEPTIONREGISTRATIONRECORD *registration,
                              CONTEXTRECORD *context,
                              void *dummy)
{
  if (report->fHandlerFlags & (EH_EXIT_UNWIND|EH_UNWINDING))
    return XCPT_CONTINUE_SEARCH;
  switch (report->ExceptionNum)
    {
    case XCPT_ASYNC_PROCESS_TERMINATE: /* This is the correct one */
    case XCPT_PROCESS_TERMINATE:       /* OS/2 bug */
      qwait_tid = 0;
      break;
    }
  return XCPT_CONTINUE_SEARCH;
}


/* This thread handles termination of child processes started with
   DosStartSession. */

static void qwait_thread (ULONG arg)
{
  ULONG rc, len, sid, ret, tmp;
  REQUESTDATA req;
  BYTE priority;
  ULONG *data;
  EXCEPTIONREGISTRATIONRECORD registration;

  registration.prev_structure = NULL;
  registration.ExceptionHandler = qwait_exception;
  DosSetExceptionHandler (&registration);

  arg = 0; data = 0;            /* keep the compiler happy */
  for (;;)
    {
      rc = DosReadQueue (termq_handle, &req, &len, (void **)&data, 0,
                         DCWW_WAIT, &priority, 0);
      if (rc == ERROR_QUE_INVALID_HANDLE)
        DosExit (EXIT_THREAD, 0);
      if (rc != 0)
        error (rc, "DosReadQueue");
      else if (req.ulData == 0)
        {
          /* Child session ended. */

          sid = *data & 0xffff;
          ret = (*data >> 16) & 0xffff;
          DosFreeMem (data);
          if (ret > 0xff)
            ret = 0xff;
          tmp = ret << 8;
          bury (0, sid, tmp);
        }
      else
        {
          /* Child session created (SSF_TRACEOPT_TRACEALL only). */

          DosFreeMem (data);
        }
    }
}


/* Start qwait_thread() if not already done. */

static void start_qwait_thread (void)
{
  ULONG qn, rc;

  if (!qwait_started)
    {
      qwait_started = TRUE;
      LOCK_COMMON;
      qn = ++queue_number;
      UNLOCK_COMMON;
      sprintf (termq_name, "/queues/emx/termq/%.8x.tqe", (unsigned)qn);
#if 0
      prt ("Queue name: ");
      prt (termq_name);
      prt ("\r\n");
#endif
      rc = DosCreateQueue (&termq_handle, QUE_FIFO | QUE_CONVERT_ADDRESS,
                           termq_name);
      if (rc == 0)
        termq_created = TRUE;
      else
        error (rc, "DosCreateQueue");
      rc = DosCreateThread (&qwait_tid, qwait_thread, 0,
                            CREATE_READY | STACK_COMMITTED, 0x4000);
      if (rc != 0) error (rc, "DosCreateThread");
    }
}


/* This function is called on termination of emx.dll. */

void term_process (void)
{
  ULONG i;

  if (termq_created)
    {
      termq_created = FALSE;
      DosCloseQueue (termq_handle);
    }

  /* Closing termq_handle should terminate qwait_thread().  Give it a
     chance to terminate. */

  for (i = 0; qwait_tid != 0 && i < 10; ++i)
    DosSleep (10);

  /* If qwait_thread() is still alive, kill it. */

  if (qwait_tid != 0)
    DosKillThread (qwait_tid);

  /* Kill cwait_thread() if it is alive. */

  if (cwait_tid != 0)
    DosKillThread (cwait_tid);
}


/* Set the process status of the debuggee PID to WAIT_RET, to be
   returned by wait().  Also set the thread ID to be returned by
   wait() to TID.  If END is true, turn the process into a zombie. */

void debug_set_wait (ULONG pid, ULONG tid, ULONG wait_ret, ULONG end)
{
  ULONG i;
  struct process *p;
  static struct process *cache;

  LOCK_PROCESS;
  if (!(cache != NULL
        && cache->pid == pid
        && (cache->status == PS_PROCESS || cache->status == PS_SESSION
            || cache->status == PS_ATTACH)))
    {
      p = process_table; cache = NULL;
      for (i = 0; i < MAX_PROCESSES; ++i, ++p)
        if (p->pid == pid
            && (p->status == PS_PROCESS || p->status == PS_SESSION
                || p->status == PS_ATTACH))
          {
            cache = p;
            break;
          }
    }
  if (cache != NULL)
    {
      cache->wait_ret = wait_ret;
      cache->wait_tid = tid;
      if (end)
        {
          cache->status = PS_ZOMBIE;
          ++zombie_count;
        }
      else
        cache->flags |= PF_WAIT;
    }
  UNLOCK_PROCESS;
  if (cache != NULL && end)
    proc_death (TRUE);

}


/* Wait for termination of process *PPID.  Update the PID (perhaps
   including the TID) pointed to by PPID.  Store the termination
   status (with the return code in bits 8..15, for wait()) to
   *RET_CODE.  Return 0 on success.  Otherwise, return the errno code
   (for instance EINTR when interrupted, unless IGNORE_INT is true).
   If NOHANG is true, wait_for() returns EAGAIN in case it would
   block. */

static ULONG wait_for (ULONG *ppid, ULONG *ret_code, ULONG nohang,
                       ULONG ignore_int)
{
  ULONG rc, count, i, children;
  struct process *p;

  for (;;)
    {
      DosResetEventSem (process_death, &count);

      /* Note that LOCK_PROCESS ignores interrupts. */

      do
        {
          rc = DosRequestMutexSem (process_access, -1);
        } while (rc == ERROR_SEM_OWNER_DIED
                 || (rc == ERROR_INTERRUPT && ignore_int));
      if (rc != 0) return set_error (rc);

      p = process_table; children = 0;
      for (i = 0; i < MAX_PROCESSES; ++i, ++p)
        switch (p->status)
          {
          case PS_ZOMBIE:
            if (p->pid == *ppid)
              {
                if (!(p->flags & PF_SYNC))
                  p->status = PS_FREE;
                --zombie_count;
                *ret_code = p->wait_ret;
                UNLOCK_PROCESS;
                return 0;
              }
            break;
          case PS_FREE:
            break;
          default:
            if (p->pid == *ppid)
              {
                if (p->flags & PF_WAIT)
                  {
                    p->flags &= ~PF_WAIT;
                    *ret_code = p->wait_ret;
                    *ppid = PTRACE_PIDTID (p->pid, p->wait_tid);
                    UNLOCK_PROCESS;
                    return 0;
                  }
                ++children;
              }
            break;
          }
      UNLOCK_PROCESS;
      if (children == 0)
        return ECHILD;
      if (nohang)
        return EAGAIN;
      rc = DosWaitEventSem (process_death, SEM_INDEFINITE_WAIT);
      if (rc != 0 && !(rc == ERROR_INTERRUPT && ignore_int))
        return set_error (rc);
    }
}


/* Wait for termination of any child process.  Store the termination
   status (with the return code in bits 8..15, for wait()) to
   *RET_CODE.  Return 0 on success.  Otherwise, return the errno code
   (for instance EINTR when interrupted).  If NOHANG is true,
   wait_any() returns EAGAIN in case it would block. */

static ULONG wait_any (ULONG *ppid, ULONG *ret_code, ULONG nohang)
{
  ULONG rc, count, i, children;
  struct process *p;

  for (;;)
    {
      DosResetEventSem (process_death, &count);

      /* Note that LOCK_PROCESS ignores interrupts. */

      do
        {
          rc = DosRequestMutexSem (process_access, -1);
        } while (rc == ERROR_SEM_OWNER_DIED);
      if (rc != 0) return set_error (rc);

      p = process_table; children = 0;
      for (i = 0; i < MAX_PROCESSES; ++i, ++p)
        switch (p->status)
          {
          case PS_ZOMBIE:
            if (!(p->flags & PF_SYNC))
              p->status = PS_FREE;
            --zombie_count;
            *ppid = p->pid;
            *ret_code = p->wait_ret;
            UNLOCK_PROCESS;
            return 0;
          case PS_FREE:
            break;
          default:
            if (p->flags & PF_WAIT)
              {
                p->flags &= ~PF_WAIT;
                *ppid = PTRACE_PIDTID (p->pid, p->wait_tid);
                *ret_code = p->wait_ret;
                UNLOCK_PROCESS;
                return 0;
              }
            ++children;
            break;
          }
      UNLOCK_PROCESS;
      if (children == 0)
        return ECHILD;
      if (nohang)
        return EAGAIN;
      rc = DosWaitEventSem (process_death, SEM_INDEFINITE_WAIT);
      if (rc != 0) return set_error (rc);
    }
}


/* This function implements the __wait() system call. */

ULONG do_wait (ULONG *ptermstatus, ULONG *errnop)
{
  ULONG rc, pid;

  rc = wait_any (&pid, ptermstatus, FALSE);
  *errnop = rc;
  return (rc == 0 ? pid : -1);
}


/* This function implements the __waitpid() system call. */

ULONG do_waitpid (ULONG pid, ULONG opt, ULONG *ptermstatus, ULONG *errnop)
{
  ULONG rc;

  if (pid == (ULONG)-1)
    rc = wait_any (&pid, ptermstatus, opt & WNOHANG);
  else
    rc = wait_for (&pid, ptermstatus, opt & WNOHANG, FALSE);
  if (rc == EAGAIN)
    {
      *errnop = 0;
      return 0;
    }
  *errnop = rc;
  return (rc == 0 ? pid : -1);
}

#define ADDCHR(c) do { \
  if (arg_buf != NULL) \
    { \
      if (size + 1 > arg_buf_size) return -E2BIG; /* Oops */ \
      arg_buf[size] = (char)c; \
    } \
  size += 1; \
  } while (0)

#define ADDCHRS(c,n) do { \
  if (arg_buf != NULL) \
    { \
      if (size + (n) > arg_buf_size) return -E2BIG; /* Oops */ \
      memset (arg_buf + size, c, n); \
    } \
  size += n; \
  } while (0)

#define ADDSTR(s,n) do { \
  if (arg_buf != NULL) \
    { \
      if (size + (n) > arg_buf_size) return -E2BIG; /* Oops */ \
      memcpy (arg_buf + size, s, n); \
    } \
  size += n; \
  } while (0)

/* Build the command line for a child process.  Return the number of
   bytes required/used for the buffer or -errno.  Well, there's not
   much point in allocating the buffer dynamically as OS/2 limits the
   size to 64KB anyway. */

static LONG spawn_args (char *arg_buf, ULONG arg_buf_size,
                        const char *arg_ptr,
                        ULONG argc, ULONG mode32, UCHAR session,
                        const char *prog_name)
{
  size_t len, size;
  const char *src, *s, *base;
  int i, quote, bs, method;

  base = (const char *)_getname (prog_name);
  method = 0;
  if (stricmp (base, "cmd.exe") == 0 || stricmp (base, "4os2.exe") == 0)
    method = 1;
  src = arg_ptr;
  size = 0;
  if (argc > 0)
    {
      ++src;                /* skip flags byte */
      len = strlen (src) + 1;
      if (!session)
        ADDSTR (src, len);
      src += len;
    }
  for (i = 1; i < argc; ++i)
    {
      if (i > 1)
        ADDCHR (' ');
      ++src;                    /* skip flags byte */
      quote = FALSE;
      if (*src == 0)
        quote = TRUE;
      else if (opt_quote || (mode32 & P_QUOTE))
        {
          if (src[0] == '@' && src[1] != 0)
            quote = TRUE;
          else
            for (s = src; *s != 0; ++s)
              if (*s == '?' || *s == '*')
                {
                  quote = TRUE;
                  break;
                }
        }
      if (!quote)
        for (s = src; *s != 0; ++s)
          if (*s == ' ' || *s == '\t' || (*s == '"' && method == 1))
            {
              quote = TRUE;
              break;
            }
      if (quote)
        ADDCHR ('"');
      bs = 0;
      while (*src != 0)
        {
          if (*src == '"' && method == 0)
            {
              ADDCHRS ('\\', bs+1);
              bs = 0;
            }
          else if (*src == '\\' && method == 0)
            ++bs;
          else
            bs = 0;
          ADDCHR (*src);
          ++src;
        }
      if (quote)
        {
          ADDCHRS ('\\', bs);
          bs = 0;
          ADDCHR ('"');
        }
      ++src;
    }
  ADDCHR (0);
  if (!session && argc > 0 && (mode32 & P_TILDE))
    {
      ADDCHR ('~');
      src = arg_ptr;
      for (i = 0; i < argc; ++i)
        {
          ++src;                    /* skip flags byte */
          if (*src == 0 || *src == '~')
            ADDCHR ('~');
          len = strlen (src) + 1;
          ADDSTR (src, len);
          src += len;
        }
    }
  ADDCHR (0);
  return size;
}


/* Build environment for child process: Set the `_emx_sig' environment
   variable to let the child process inherit signal settings.  Store a
   pointer to the new environment, allocated with private_alloc(), to
   *DST.  SRC points to the original environment to be passed to the
   child process.  SRC_SIZE is the size of that environment. */

static ULONG spawn_env (char **dst, const char *src, ULONG src_size)
{
  void *pv;
  char *env;
  ULONG rc, size;
  char temp[40];
  unsigned sigset;
  int len, i;
  thread_data *td;

  /* Do some sanity checks to avoid crashing. */

  if (src_size < 1)
    return EINVAL;
  if (src[src_size-1] != 0)
    return EINVAL;
  if (src_size >= 2 && src[src_size-2] != 0)
    return EINVAL;

  /* Build the string to be added to the environment.  Let the child
     process inherit the signal settings of the *current* thread. */

  td = get_thread ();
  sigset = 0;
  for (i = 1; i < NSIG; ++i)
    if (td->sig_table[i].handler == SIG_IGN)
      sigset |= (1 << i);
  len = sprintf (temp, "_emx_sig=%.8x:%.8x", (unsigned)my_pid, sigset);

  /* Allocate memory for the new environment. */

  size = src_size + len + 1;
  rc = private_alloc (&pv, size);
  if (rc != 0) return set_error (rc);
  *dst = env = (char *)pv;

  /* Copy the environment, dropping `_emx_sig' if present. */

  while (*src != 0)
    {
      if (*src == '_' && strncmp (src, temp, 9) == 0)
        while (*src++ != 0)
          ;
      else
        {
          while (*src != 0)
            *env++ = *src++;
          *env++ = *src++;
        }
    }

  /* Add the new variable. */

  memcpy (env, temp, len + 1);
  env[len+1] = 0;

  return 0;
}


/* When calling exec() in a fork()ed process, we do not exit after
   starting the process as our parent (who forked us) cannot retrieve
   the return code of our child process.  Wait for termination of the
   child process, passing on signals.  After termination of the child
   process, return its return code. */

static void spawn_fork_exec (ULONG pid)
{
  ULONG wait_ret, i;

#if 0
  if (get_tid () != 1)
    {
      /* TODO: transfer control to the main thread and call
         spawn_fork_exec() there */
    }
#endif

  stop_alarm ();

  /* TODO: stop all user threads */

  fork_exec_pid = pid;

  /* Close all handles but socket handles.  We can't close socket
     handles as that would create a time window. */

  for (i = 0; i < handle_count; ++i)
    if ((files[i].flags & (HF_OPEN|HF_SOCKET)) == HF_OPEN)
      close_handle (i);

  wait_for (&pid, &wait_ret, FALSE, TRUE);
  quit (wait_ret >> 8);
}


/* This function implements the __spawnve() system call. */

ULONG do_spawn (struct _new_proc *np, ULONG *result)
{
  STARTDATA sd;
  struct process *proc;
  ULONG rc, pid, sid;
  RESULTCODES ret_codes;
  ULONG exec_type;
  ULONG mode32;
  UCHAR mode8;
  long arg_buf_size;
  char fname[512];
  char obj_buf[40];
  BYTE session;
  PVOID arg_buf;
  char *env_ptr = NULL;

  /* Kludge alert: Unfortunately, the `mode' member was made only 16
     bits wide originally.  Bit 15 is used to indicate that the
     `mode2' member is used, which holds the upper 16 bits of MODE.
     This allows using emx 0.8 applications with emx 0.9 as emx
     doesn't look at `mode2' unless bit 15 of `mode' is set.  Of
     course, this fails if an (old) application program sets bit 15 of
     MODE. */

  mode32 = np->mode;
  if (np->mode & 0x8000)
    mode32 |= np->mode2 << 16;

  mode8 = np->mode & 0xff;
  switch (mode8)
    {
    case P_NOWAIT:
    case P_WAIT:
    case P_OVERLAY:
      session = FALSE;
      exec_type = EXEC_ASYNCRESULT;
      break;
    case P_DETACH:
      session = FALSE;
      exec_type = EXEC_BACKGROUND;
      break;
    case P_DEBUG:
      if ((mode32 & P_DEBUGDESC)
          && (uflags & _UF_PTRACE_MODEL) == _UF_PTRACE_STANDARD)
        return EINVAL;
      exec_type = (mode32 & P_DEBUGDESC) ? EXEC_ASYNCRESULTDB : EXEC_TRACE;
      session = !(debug_same_sess || (mode32 & P_NOSESSION));
      if (session & (mode32 & P_UNRELATED))
        return EINVAL;
      break;
    case P_SESSION:
    case P_PM:
      exec_type = 0;
      session = TRUE;
      break;
    default:
      return EINVAL;
    }
  truncate_name (fname, (char *)np->fname_off);
  _defext (fname, "exe");

  /* Compute the size of the argument buffer. */

  arg_buf_size = spawn_args (NULL, 0, (const char *)np->arg_off,
                             np->arg_count, mode32, session, fname);
  if (arg_buf_size < 0)
    return -arg_buf_size;       /* errno */
  if (arg_buf_size > 65536)
    return E2BIG;

  /* The buffer pointed to by the ArgPointer argument of DosExecPgm
     must not cross a 64KByte boundary.  Note that objects are aligned
     on a 64KByte boundary. */

  rc = DosAllocMem (&arg_buf, (ULONG)arg_buf_size,
                    PAG_READ | PAG_WRITE | PAG_COMMIT);
  if (rc != 0) return ENOMEM;

  /* Build the argument buffer. */

  arg_buf_size = spawn_args (arg_buf, (ULONG)arg_buf_size,
                             (const char *)np->arg_off,
                             np->arg_count, mode32, session, fname);
  if (arg_buf_size < 0)
    {
      DosFreeMem (arg_buf);
      return -arg_buf_size;     /* errno */
    }

  rc = spawn_env (&env_ptr, (const char *)np->env_off, np->env_size);
  if (rc != 0)
    {
      DosFreeMem (arg_buf);
      return rc;
    }
  LOCK_PROCESS;
  proc = proc_new ();
  if (proc == 0)
    {
      UNLOCK_PROCESS;
      DosFreeMem (arg_buf);
      private_free (env_ptr);
      return EAGAIN;
    }
  if (session)
    {
      if (!(mode32 & P_UNRELATED))
        start_qwait_thread ();
      sd.Length = 50;
      sd.Related = ((mode32 & P_UNRELATED)
                    ? SSF_RELATED_INDEPENDENT : SSF_RELATED_CHILD);
      sd.PgmTitle = 0;
      sd.PgmName = fname;
      sd.PgmInputs = arg_buf;
      sd.TermQ = termq_name;
      sd.Environment = env_ptr;
      sd.InheritOpt = SSF_INHERTOPT_PARENT;
      sd.IconFile = 0;
      sd.PgmHandle = 0;
      sd.InitXPos = 0;
      sd.InitYPos = 0;
      sd.InitXSize = 0;
      sd.InitYSize = 0;
      sd.Reserved = 0;
      sd.ObjectBuffer = 0;
      sd.ObjectBuffLen = 0;
      switch (mode32 & 0x0f00)
        {
        case P_FULLSCREEN:
          sd.SessionType = SSF_TYPE_FULLSCREEN;
          break;
        case P_WINDOWED:
          sd.SessionType = SSF_TYPE_WINDOWABLEVIO;
          break;
        default:
          if (mode8 == P_PM)
            sd.SessionType = SSF_TYPE_PM;
          else
            sd.SessionType = SSF_TYPE_DEFAULT;
          break;
        }
      switch (mode32 & 0xf00)
        {
        case P_MINIMIZE:
          sd.PgmControl = SSF_CONTROL_MINIMIZE;
          break;
        case P_MAXIMIZE:
          sd.PgmControl = SSF_CONTROL_MAXIMIZE;
          break;
        default:
          sd.PgmControl = 0;
          break;
        }
      if (mode32 & P_NOCLOSE)
        sd.PgmControl |= SSF_CONTROL_NOAUTOCLOSE;
      if (mode8 != P_DEBUG)
        sd.TraceOpt = SSF_TRACEOPT_NONE;
      else if (mode32 & P_DEBUGDESC)
        sd.TraceOpt = SSF_TRACEOPT_TRACEALL;
      else
        sd.TraceOpt = SSF_TRACEOPT_TRACE;
      sd.FgBg = (mode32 & P_BACKGROUND ? SSF_FGBG_BACK : SSF_FGBG_FORE);
      rc = DosStartSession (&sd, &sid, &pid);
      DosFreeMem (arg_buf);
      private_free (env_ptr);
      if (rc != 0 && rc != ERROR_SMG_START_IN_BACKGROUND)
        {
          UNLOCK_PROCESS;
          return set_error (rc);
        }
      if (!(mode32 & P_UNRELATED))
        {
          proc->status = PS_SESSION;
          proc->pid = pid;
          proc->sid = sid;
          proc->wait_ret = 0;
          proc->flags = 0;
          if (mode8 == P_DEBUG)
            {
              proc->flags |= PF_DEBUG | PF_WAIT;
              proc->wait_ret = 0x7f + (SIGTRAP << 8);
              proc->wait_tid = 1;
            }
        }
      UNLOCK_PROCESS;
      if (!(mode32 & P_UNRELATED))
        child_started ();
      if (mode8 == P_DEBUG)
        {
          rc = spawn_debug (pid, sid);
          if (rc != 0) return rc;
        }
      *result = pid;
      return 0;
    }
  else
    {
      kbd_stop ();
      start_cwait_thread ();
      rc = DosExecPgm (obj_buf, sizeof (obj_buf),
                       exec_type, arg_buf, env_ptr,
                       &ret_codes, fname);
      DosFreeMem (arg_buf);
      private_free (env_ptr);
      if (rc != 0)
        {
          UNLOCK_PROCESS;
          kbd_restart ();
          return set_error (rc);
        }
      UNLOCK_COMMON;
      pid = ret_codes.codeTerminate;
      proc->status = PS_PROCESS;
      proc->pid = pid;
      proc->sid = 0;
      proc->wait_ret = 0;
      proc->flags = 0;
      if (mode8 == P_WAIT)
        proc->flags |= PF_SYNC;
      if (mode8 == P_DEBUG)
        {
          proc->flags |= PF_DEBUG | PF_WAIT;
          proc->wait_ret = 0x7f + (SIGTRAP << 8);
          proc->wait_tid = 1;
        }
      UNLOCK_PROCESS;
      child_started ();
      switch (mode8)
        {
        case P_DEBUG:
          rc = spawn_debug (pid, 0);
          kbd_restart ();
          if (rc != 0) return rc;
          *result = pid;
          return 0;
        case P_OVERLAY:
          if (fork_flag)
            spawn_fork_exec (pid);
          quit (0);
        case P_NOWAIT:
        case P_DETACH:
          *result = pid;
          kbd_restart ();
          return 0;
        default:
          rc = wait_for (&pid, result, FALSE, TRUE);
          *result = (*result >> 8) & 0xff;
          proc->status = PS_FREE;
          kbd_restart ();
          return rc;
        }
    }
}


/* Copy the name of the EXE file to BUF.  Copy at most BUFSIZE bytes,
   including the terminating null character. */

int execname (char *buf, ULONG bufsize)
{
  size_t len;

  if (bufsize == 0)
    return -1;

  if (exe_name[0] == 0)
    {
      *buf = 0;
      return -1;
    }
  len = strlen (exe_name);
  if (len >= bufsize)
    {
      *buf = 0;
      return -1;
    }
  memcpy (buf, exe_name, len + 1);
  return 0;
}


/* Create and initialize thread data block for a new thread.  TID is
   the thread ID.  Return 0 on success, -1 on failure.  Store the
   error number to ERRNOP unless ERRNOP is NULL. */

ULONG new_thread (ULONG tid, int *errnop)
{
  thread_data *td;
  ULONG rc;
  int i;

  if (tid >= MAX_THREADS)
    {
      if (errnop != NULL) *errnop = EAGAIN;
      return -1;
    }

  /* Silently ignore multiple initializations of the same thread. */

  if (threads[tid] != NULL)
    return 0;

  rc = private_alloc ((void **)&td, sizeof (thread_data));
  if (rc != 0)
    {
      if (errnop != NULL) *errnop = set_error (rc);
      return -1;
    }
  threads[tid] = td;
  td->last_sys_errno = 0;       /* No error occured yet */
  td->prev_sys_errno = 0;       /* No error occured yet */
  td->sig_blocked = 0;          /* No signals are blocked */
  td->sig_prev_blocked = 0;     /* Ditto */
  td->sig_pending = 0;          /* No signals are pending */
  for (i = 0; i < NSIG; ++i)
    {
      td->sig_table[i].handler = SIG_DFL;
      td->sig_table[i].sa_mask = 0;
      td->sig_table[i].sa_flags = SA_ACK;
    }
  td->find_handle = HDIR_CREATE; /* Handle not open */
  td->find_next = NULL;
  td->find_count = 0;
  if (errnop != NULL) *errnop = 0;
  return 0;
}


/* Deallocate the thread data block of thread TID.  Return 0 on
   success, -1 on failure.  Store the error number to *ERRNOP. */

ULONG end_thread (ULONG tid, int *errnop)
{
  thread_data *td;
  ULONG rc;

  if (tid >= MAX_THREADS || threads[tid] == NULL)
    {
      *errnop = EINVAL;
      return -1;
    }
  td = threads[tid];
  if (td->find_handle != HDIR_CREATE)
    DosFindClose (td->find_handle);
  rc = private_free (td);
  threads[tid] = NULL;
  if (rc != 0)
    {
      *errnop = set_error (rc);
      return -1;
    }
  *errnop = 0;
  return 0;
}


/* Check if the direct child process PID is alive.  Return FALSE if
   the child process is not alive. */

static int alive (ULONG pid)
{
  ULONG rc;

  rc = DosSetPriority (PRTYS_PROCESS, PRTYC_NOCHANGE, 0, pid);
  return (rc == 0 ? TRUE : FALSE);
}


/* Register a memory area for fork().  This is used for copying DLL
   data segments. */

void fork_register_mem (ULONG start, ULONG end, HMODULE hmod)
{
  if (fork_regmem_count >= FORK_REGMEM_MAX)
    fork_regmem_overflow = TRUE;
  else
    {
      fork_regmem[fork_regmem_count].start = start;
      fork_regmem[fork_regmem_count].end = end;
      fork_regmem[fork_regmem_count].hmod = hmod;
      fork_regmem_count += 1;
    }
}


/* Register a DLL for fork().  This is used for loading DLLs in the
   child process. */

void fork_register_dll (HMODULE hmod)
{
  /* If the module handle is already registered, the DLL was unloaded
     and another one loaded.  (We don't record unloading of DLLs.)
     Here, we are only interested in the module handles, no matter
     what DLL they are associated with. */

  if (fork_dll_registered (hmod))
    return;

  if (fork_regdll_count >= FORK_REGDLL_MAX)
    fork_regdll_overflow = TRUE;
  else
    {
      fork_regdll[fork_regdll_count].hmod = hmod;
      fork_regdll_count += 1;
    }
}

/* Return TRUE iff the DLL HMOD is registered. */

int fork_dll_registered (HMODULE hmod)
{
  int i;

  for (i = 0; i < fork_regdll_count; ++i)
    if (fork_regdll[i].hmod == hmod)
      return TRUE;
  return FALSE;
}


/* Remove entries for unloaded modules. */

static void fork_reg_cleanup (void)
{
  int i, j;

  if (fork_regdll_overflow || fork_regmem_overflow)
    {
      /* We cannot recover from overflow.  Just keep the deleted
         entries. */
      return;
    }

  /* Remove deleted entries by moving the remaining entries down.  We
     keep the order of the entries in an attempt to cope with
     unloading and reloading modules.  (Actually, the order might be
     irrelevant, as loading a DLL automatically causes all DLLs it
     depends on to be loaded.) */

  j = 0;
  for (i = 0; i < fork_regdll_count; ++i)
    if (fork_regdll[i].hmod != 0)
      {
        if (i != j)
          fork_regdll[j] = fork_regdll[i];
        ++j;
      }
  fork_regdll_count = j;

  j = 0;
  for (i = 0; i < fork_regmem_count; ++i)
    if (fork_regmem[i].hmod != 0)
      {
        if (i != j)
          fork_regmem[j] = fork_regmem[i];
        ++j;
      }
  fork_regmem_count = j;
}


/* Wait for acknowledge from child process. */

ULONG fork_wait (ULONG ack_sem, ULONG pid)
{
  ULONG rc;

  for (;;)
    {
      rc = DosWaitEventSem (ack_sem, 500);
      if (rc == ERROR_TIMEOUT)
        {
          if (!alive (pid))
            return ERROR_INVALID_PROCID; /* -> ESRCH */
        }
      else if (rc != ERROR_INTERRUPT)
        return rc;
    }
}


/* Send a data block to the child process. */

ULONG fork_send (ULONG req_sem, ULONG ack_sem, ULONG pid)
{
  ULONG rc, post_count;

  rc = DosResetEventSem (ack_sem, &post_count);
  if (rc != 0 && rc != ERROR_ALREADY_RESET)
    return rc;
  rc = DosPostEventSem (req_sem);
  if (rc != 0)
    return rc;
  return fork_wait (ack_sem, pid);
}


/* Make the child process load a DLL. */

ULONG fork_dll (fork_data *shmem, ULONG req_sem, ULONG ack_sem, ULONG pid,
                HMODULE hmod)
{
  ULONG rc;
  int i;

  shmem->dll.req_code = FORK_REQ_DLL;
  shmem->dll.hmod = hmod;
  rc = DosQueryModuleName (hmod, sizeof (shmem->dll.path), shmem->dll.path);
  if (rc == 0)
    {
      /* The module handle is valid.  Now check if all the registered
         memory regions are readable.  If the process has unloaded a
         DLL which is still loaded by another process the module
         handle will remain valid! */

      for (i = 0; i < fork_regmem_count; ++i)
        if (fork_regmem[i].hmod == hmod
            && !verify_memory (fork_regmem[i].start,
                               fork_regmem[i].end - fork_regmem[i].start))
          {
            rc = ERROR_INVALID_HANDLE;
            break;
          }
    }

  if (rc == ERROR_INVALID_HANDLE)
    {
      int i;

      /* We should get exactly one match. */

      for (i = 0; i < fork_regdll_count; ++i)
        if (fork_regdll[i].hmod == hmod)
          fork_regdll[i].hmod = 0;

      /* We should get 0 through 2 matches. */

      for (i = 0; i < fork_regmem_count; ++i)
        if (fork_regmem[i].hmod == hmod)
          fork_regmem[i].hmod = 0;

      return 0;
    }
  if (rc != 0)
    return rc;
  return fork_send (req_sem, ack_sem, pid);
}


/* Send a block of memory to the child process.  The memory is owned
   by DLL HMOD (if HMOD is not NULLHANDLE) or by the base process (if
   HMOD is NULLHANDLE).  This assumes that no DLL has module handle
   NULLHANDLE. */

ULONG fork_mem (fork_data *shmem, ULONG req_sem, ULONG ack_sem, ULONG pid,
                ULONG base, ULONG top, HMODULE hmod)
{
  ULONG rc, count, chunk;

  count = top - base;
  while (count != 0)
    {
      shmem->mem.req_code = FORK_REQ_MEM;
      shmem->mem.address = base;
      shmem->mem.shared = shmem->mem.buf;
      shmem->mem.hmod = hmod;
      chunk = FORK_OBJ_SIZE - (sizeof (shmem->mem) - 1);
      if (count < chunk)
        chunk = count;
      shmem->mem.count = chunk;
      memcpy (shmem->mem.buf, (void *)base, chunk);
      rc = fork_send (req_sem, ack_sem, pid);
      if (rc != 0)
        return rc;
      base += chunk; count -= chunk;
    }
  return 0;
}


/* Implementation of fork().

   The following string is passed in the 3rd argument string:

   ^mmmmmmmm pppppppp

   mmmmmmmm      Base address of shared memory object (hexadecimal)
   pppppppp      Process ID of parent process

   Note: emx_syscall (in emxdll.asm) pushes and pops all registers,
   including EBX, ESI, and EDI.  As the stack is copied to the child
   process, the child process will inherit register variables. */

#define FORK_RSC_PROC    0x01
#define FORK_RSC_MEM     0x02
#define FORK_RSC_REQ     0x04
#define FORK_RSC_ACK     0x08
#define FORK_RSC_SOCK    0x10
#define FORK_RSC_FILEIO  0x20

int do_fork (syscall_frame *frame, ULONG *errnop)
{
  static char arg_buf[64];      /* Must not cross a 64KByte boundary */
  char *p;
  char obj_buf[32];
  RESULTCODES ret_codes;
  ULONG rc, req_sem, ack_sem, stk_low, pid, resources, *sp, ip;
  fork_data *shmem;
  struct process *pte;
  thread_data *td;
  int result, i;

  resources = 0; result = -1;
  if (heap_obj_count > 1 || fork_regmem_overflow || fork_regdll_overflow
      || (heap_obj_count != 0 && !first_heap_obj_fixed))
    {
      /* Cannot copy more than one heap object and not more than
         FORK_REGMEM_MAX DLL data segments.  Cannot load more than
         FORK_REGDLL_MAX DLLs.  Cannot copy a heap object which isn't
         an object of the EXE file (malloc() in _DLL_InitTerm()!). */

      *errnop = ENOMEM;
      goto done;
    }
  if (layout_flags & L_FLAG_LINK386)
    {
      *errnop = EACCES;
      goto done;
    }
  LOCK_PROCESS;
  resources |= FORK_RSC_PROC;
  pte = proc_new ();
  if (pte == NULL)
    {
      *errnop = EAGAIN;
      goto done;
    }
  rc = DosAllocSharedMem ((void **)&shmem, NULL, FORK_OBJ_SIZE,
                          PAG_READ|PAG_WRITE|PAG_COMMIT|OBJ_GETTABLE);
  if (rc != 0)
    goto error;
  resources |= FORK_RSC_MEM;
  p = arg_buf;
  *p++ = ' ';                   /* Program name */
  *p++ = 0;
  *p++ = ' ';                   /* Command line */
  *p++ = 0;
  sprintf (p, "^%.8x %.8x", (unsigned)shmem, (unsigned)my_pid); /* fork */
  stk_low = (ULONG)alloca (0) & ~0xfff;

  shmem->init.req_code = FORK_REQ_INIT;
  shmem->init.msize = FORK_OBJ_SIZE;
  shmem->init.brk = heap_obj_count == 0 ? 0 : heap_objs[0].brk;
  shmem->init.reg_ebp = (ULONG)frame;
  shmem->init.umask = umask_bits;
  shmem->init.umask1 = umask_bits1;
  shmem->init.uflags = uflags;
  shmem->init.stack_page = stk_low;
  shmem->init.stack_base = stack_base;

  rc = create_event_sem (&req_sem, DC_SEM_SHARED);
  if (rc != 0)
    goto error;
  shmem->init.req_sem = req_sem;
  resources |= FORK_RSC_REQ;

  rc = create_event_sem (&ack_sem, DC_SEM_SHARED);
  if (rc != 0)
    goto error;
  shmem->init.ack_sem = ack_sem;
  resources |= FORK_RSC_ACK;

  fileio_fork_parent_init ();
  resources |= FORK_RSC_FILEIO;

  start_cwait_thread ();
  xf86sup_all_enadup (TRUE);

  /* Compute the return address, for use by ptrace().  We add 4 to the
     stack pointer to compensate for the PUSHFD of emx_syscall. */

  sp = (ULONG *)(frame->e.esp + 4);
  ip = frame->e.eip;

  /* Check whether we have been called from __syscall().  That should
     always be the case, but... We check only the lower 16 bits
     because we don't know the address of __syscall() in a DLL such as
     emxlibcs.dll. */

  if ((ip & 0xffff) == 12 + 5)
    {
      ++sp;                     /* Remove CALL emx_syscall */
      ip = *sp++;               /* Get and remove EIP */

      /* IP is now in __fork().  See /emx/src/lib/emx_386/fork.s for
         details -- note that __fork() does not create a stack frame.
         Get the function which called __fork(). */

      ip = *sp++;
    }
  ptrace_fork_addr = ip;

  /* Create the child process. */

  ptrace_forking = TRUE;
  rc = DosExecPgm (obj_buf, sizeof (obj_buf), EXEC_ASYNCRESULT,
                   arg_buf, startup_env, &ret_codes, exe_name);
  ptrace_forking = FALSE;

  xf86sup_all_enadup (FALSE);
  if (rc != 0)
    goto error;

  pid = ret_codes.codeTerminate;
  pte->status = PS_PROCESS;
  pte->pid = pid;
  pte->sid = 0;
  pte->wait_ret = 0;
  pte->flags = 0;
  UNLOCK_PROCESS;
  resources &= ~FORK_RSC_PROC;

  child_started ();
  rc = fork_wait (ack_sem, pid);
  if (rc != 0)
    goto error;

  rc = fork_mem (shmem, req_sem, ack_sem, pid,
                 layout->data_base, layout->bss_end, NULLHANDLE);
  if (rc != 0)
    goto error;
  if (heap_obj_count != 0 && heap_objs[0].brk != 0)
    {
      rc = fork_mem (shmem, req_sem, ack_sem, pid, heap_objs[0].base,
                     heap_objs[0].brk, NULLHANDLE);
      if (rc != 0)
        goto error;
    }

  /* Load registered DLLs. */

  if (!(debug_flags & DEBUG_NOFORKDLL))
    for (i = 0; i < fork_regdll_count; ++i)
      if (fork_regdll[i].hmod != 0)
        {
          rc = fork_dll (shmem, req_sem, ack_sem, pid, fork_regdll[i].hmod);
          if (rc != 0)
            goto error;
        }

  /* Copy registered memory areas (DLL data segments). */

  for (i = 0; i < fork_regmem_count; ++i)
    if (fork_regmem[i].hmod != 0)
      {
        rc = fork_mem (shmem, req_sem, ack_sem, pid,
                       fork_regmem[i].start, fork_regmem[i].end,
                       fork_regmem[i].hmod);
        if (rc != 0)
          goto error;
      }

  rc = fork_mem (shmem, req_sem, ack_sem, pid, stk_low, stack_end, NULLHANDLE);
  if (rc != 0)
    goto error;

  /* Send the final request to the child process.  The structure
     contains data which will be kept by the child process in the
     shared memory object until it can be used.  Therefore, this data
     must be sent last. */

  shmem->done.req_code = FORK_REQ_DONE;
  shmem->done.size = sizeof (shmem->done);
  td = threads[1];              /* Copy signal handlers of thread 1 */
  memcpy (shmem->done.sig_actions, td->sig_table,
          sizeof (shmem->done.sig_actions));
  fileio_fork_parent_fillin (&shmem->done);
  tcpip_fork_sock (&shmem->done);
  resources |= FORK_RSC_SOCK;
  rc = fork_send (req_sem, ack_sem, pid);
  if (rc != 0)
    goto error;
  result = pid;
  *errnop = 0;
  goto done;

error:
  if (resources & FORK_RSC_SOCK)
    tcpip_undo_fork_sock (&shmem->done);
  result = -1;
  *errnop = set_error (rc);

done:
  if (resources & FORK_RSC_PROC)
    UNLOCK_PROCESS;
  if (resources & FORK_RSC_FILEIO)
    fileio_fork_parent_restore ();
  if (resources & FORK_RSC_REQ)
    close_event_sem (req_sem);
  if (resources & FORK_RSC_ACK)
    close_event_sem (ack_sem);
  if (resources & FORK_RSC_MEM)
    DosFreeMem (shmem);
  fork_reg_cleanup ();
  return result;
}
