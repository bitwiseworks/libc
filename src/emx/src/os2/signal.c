/* signal.c -- Manage signals and exceptions
   Copyright (c) 1994-1998 by Eberhard Mattes

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


#define INCL_DOSEXCEPTIONS
#define INCL_DOSPROCESS
#define INCL_DOSSEMAPHORES
#define INCL_DOSDATETIME
#define INCL_DOSMISC
#define INCL_DOSERRORS
#include <os2emx.h>
#include "wrapper.h"
#include <sys/signal.h>
#include <sys/uflags.h>
#include <sys/errno.h>
#include <emx/syscalls.h>
#include "emxdll.h"

#define SIGF_CORE               0x01

/* TODO: SIGSTOP */
#define SET_BLOCKED(td,set) ((td)->sig_blocked = (set) & ~_SIGMASK (SIGKILL))


/* This structure describes a signal. */

struct signal_descr
{
  const char *name;             /* Name of the signal */
  BYTE dfl_action;              /* Action for SIG_DFL */
  BYTE fun_action;              /* Action taken after function */
  BYTE flags;                   /* Flags (SIGF_CORE) */
};


/* Inhibit core dumps. */

BYTE nocore_flag;

/* Don't display harderror popups.  This is done by terminating the
   process instead of returning XCPT_CONTINUE_SEARCH from the
   exception handler. */

BYTE no_popup;

/* Event semaphore for temporarily blocked signal. */

HEV signal_sem;

/* This flag is set when a temporarily blocked signal is generated. */

char sig_flag;

/* When exec() is called in a forked process, we pass on signals to
   the child process until it terminates.  This variable holds the
   process ID of the child process.  If the value is zero, there is no
   such child process. */

ULONG fork_exec_pid;

/* Critical sections for signal processing are protected by this Mutex
   semaphore. */

static HMTX sig_access;


/* 16-bit signal handler. */

extern _far16ptr old_sig16_handler;
extern void (*sig16_handler)(void);

#define ST_TERM   0             /* Terminate process (SIGINT for instance) */
#define ST_NEXT   1             /* Pass on to next exception handler */
#define ST_IGNORE 2             /* Ignore exception and continue thread */


/* This array contains TRUE for all valid signal numbers. */

char const signal_valid[NSIG] =
{
  FALSE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE,  /* 0..9 */
  TRUE,  TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, FALSE, /* 10..19 */
  FALSE, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, TRUE   /* 20..28 */
};

/* This array contains constant information about the signals: The
   name of the signal, what to do for SIG_DFL and after calling the
   signal handler, respectively, and whether to dump core or not. */

static struct signal_descr const signal_info[NSIG] =
{
  {"SIG0",     ST_TERM,   ST_TERM,   0},
  {"SIGHUP",   ST_TERM,   ST_IGNORE, 0},
  {"SIGINT",   ST_TERM,   ST_IGNORE, 0},
  {"SIGQUIT",  ST_TERM,   ST_IGNORE, SIGF_CORE},
  {"SIGILL",   ST_NEXT,   ST_NEXT,   SIGF_CORE},
  {"SIGTRAP",  ST_NEXT,   ST_NEXT,   SIGF_CORE},
  {"SIGABRT",  ST_TERM,   ST_TERM,   SIGF_CORE},
  {"SIGEMT",   ST_TERM,   ST_TERM,   SIGF_CORE},
  {"SIGFPE",   ST_NEXT,   ST_NEXT,   SIGF_CORE},
  {"SIGKILL",  ST_TERM,   ST_TERM,   0},
  {"SIGBUS",   ST_TERM,   ST_TERM,   SIGF_CORE},
  {"SIGSEGV",  ST_NEXT,   ST_NEXT,   SIGF_CORE},
  {"SIGSYS",   ST_TERM,   ST_TERM,   SIGF_CORE},
  {"SIGPIPE",  ST_TERM,   ST_IGNORE, 0},
  {"SIGALRM",  ST_TERM,   ST_IGNORE, 0},
  {"SIGTERM",  ST_NEXT,   ST_IGNORE, 0},
  {"SIGUSR1",  ST_IGNORE, ST_IGNORE, 0},
  {"SIGUSR2",  ST_IGNORE, ST_IGNORE, 0},
  {"SIGCLD",   ST_IGNORE, ST_IGNORE, 0},
  {"SIG19",    ST_TERM,   ST_TERM,   0},
  {"SIG20",    ST_TERM,   ST_TERM,   0},
  {"SIGBREAK", ST_TERM,   ST_IGNORE, 0},
  {"SIG22",    ST_TERM,   ST_TERM,   0},
  {"SIG23",    ST_TERM,   ST_TERM,   0},
  {"SIG24",    ST_TERM,   ST_TERM,   0},
  {"SIG25",    ST_TERM,   ST_TERM,   0},
  {"SIG26",    ST_TERM,   ST_TERM,   0},
  {"SIG27",    ST_TERM,   ST_TERM,   0},
  {"SIGWINCH", ST_IGNORE, ST_IGNORE, 0}
};


/* Prototypes. */

static ULONG kill_emx (int pid, int signo, int tree);


/* Display a message when terminating a process due to signal
   SIGNO. */

static void signal_message (int signo)
{
  if (signo == SIGABRT)
    otext ("\r\nAbnormal program termination\r\n");
  else
    oprintf ("\r\nProcess terminated by %s\r\n", signal_info[signo].name);
}


static void sig_lock (void)
{
  ULONG count;

  DosEnterMustComplete (&count);
  request_mutex (sig_access);
}


static void sig_unlock (void)
{
  ULONG count;

  DosReleaseMutexSem (sig_access);
  DosExitMustComplete (&count);
}


/* Acknowledge the signal to OS/2. */

static void acknowledge (int signo)
{
  switch (signo)
    {
    case SIGINT:
      DosAcknowledgeSignalException (XCPT_SIGNAL_INTR);
      break;
    case SIGTERM:
      DosAcknowledgeSignalException (XCPT_SIGNAL_KILLPROC);
      break;
    case SIGBREAK:
      DosAcknowledgeSignalException (XCPT_SIGNAL_BREAK);
      break;
    }
}


static void acknowledge_mult (ULONG set)
{
  if (set & _SIGMASK (SIGINT))
    acknowledge (SIGINT);
  if (set & _SIGMASK (SIGBREAK))
    acknowledge (SIGBREAK);
  if (set & _SIGMASK (SIGKILL))
    acknowledge (SIGKILL);
}


/* Generate a signal. */

void generate_signal (thread_data *td, int signo)
{
  struct signal_entry *p;

  sig_lock ();
  p = td->sig_table + signo;
  if (p->handler != SIG_IGN)
    {
      td->sig_pending |= _SIGMASK (signo);
      if ((td->sig_blocked & ~td->sig_prev_blocked & _SIGMASK (signo))
          && p->handler != SIG_DFL)
        {
          /* Wake up __select() */

          sig_flag = TRUE;
          DosPostEventSem (signal_sem);
        }
      if (td == threads[1] && get_tid () != 1)
        {
          /* Interrupt thread 1 and let it examine its set of pending
             signals. */

          DosFlagProcess (my_pid, FLGP_PID, PFLG_A, 0);
        }
    }
  sig_unlock ();
}


static void regen_sigcld (int lock)
{
  if (lock) sig_lock ();
  if (zombie_count > 0)
    threads[1]->sig_pending |= _SIGMASK (SIGCLD);
  if (lock) sig_unlock ();
}


/* Terminate the process.  Optionally do a core dump, taking the
   registers from the syscall stack frame or the exception context
   record.  Actually, the process is not terminated by this function
   if CONTEXT_RECORD is non-NULL and no_popup is false. */

static void sig_terminate (int signo, syscall_frame *frame,
                           CONTEXTRECORD *context_record)
{
  signal_message (signo);

  if (signo == SIGKILL
      || (!nocore_flag && (signal_info[signo].flags & SIGF_CORE)))
    {
      if (frame != NULL)
        {
          core_regs_i (frame);
          core_dump ();
        }
      else if (context_record != NULL)
        {
          core_regs_e (context_record);
          core_dump ();
        }
    }
  if (context_record == NULL || no_popup)
    quit (3);
}


/* Call a signal handler.  This function's stack frame contains four
   magic words, a pointer to the syscall frame, and a pointer to the
   context record.  This allows backtracing over invocations of signal
   handlers. */

static void call_handler2 (ULONG magic1, ULONG magic2, ULONG magic3,
                           ULONG magic4, syscall_frame *frame,
                           CONTEXTRECORD *context_record,
                           void (*handler)(int), int signo)
{
  handler (signo);
}


/* Call a signal handler.  This function puts magic words into the
   stack, see call_handler2(). */

static void call_handler1 (void (*handler)(int), int signo,
                           syscall_frame *frame, CONTEXTRECORD *context_record)
{
  call_handler2 (_SIG_MAGIC1, _SIG_MAGIC2, _SIG_MAGIC3, _SIG_MAGIC4,
                 frame, context_record, handler, signo);
}



/* Deliver a signal.  FRAME points to the system call stack frame,
   CONTEXT_RECORD points to the exception context record.  This
   function must be called with sig_lock() in effect; sig_unlock() is
   called by this function.

   Return ST_TERM, ST_IGNORE, or ST_NEXT. */

static int deliver_signal (thread_data *td, int signo,
                           syscall_frame *frame,
                           CONTEXTRECORD *context_record)
{
  struct signal_entry *p;
  void (*handler)(int signo);
  ULONG mask, old_blocked;

  mask = _SIGMASK (signo);
  p = td->sig_table + signo;

  td->sig_pending &= ~mask;
  handler = p->handler;

  if (handler == SIG_IGN)
    {
      /* Ignore the signal. */

      sig_unlock ();
    }
  else if (handler == SIG_DFL)
    {
      /* If the default handler SIG_DFL is installed, check the signal
         information table to learn whether to ignore the signal or to
         terminate the process. */

      sig_unlock ();
      if (signal_info[signo].dfl_action != ST_IGNORE)
        {
          sig_terminate (signo, frame, context_record);
          return signal_info[signo].dfl_action;
        }
    }
  else
    {
      /* Call the user-defined signal handler. */

      if (p->sa_flags & SA_SYSV)
        {
          p->handler = SIG_DFL;
          sig_unlock ();
          call_handler1 (handler, signo, frame, context_record);
        }
      else if (p->sa_flags & SA_ACK)
        {
          td->sig_blocked |= mask;
          sig_unlock ();
          call_handler1 (handler, signo, frame, context_record);
        }
      else
        {
          old_blocked = td->sig_blocked;
          SET_BLOCKED (td, td->sig_blocked | mask | p->sa_mask);
          sig_unlock ();
          call_handler1 (handler, signo, frame, context_record);
          td->sig_blocked = old_blocked;
        }
      if (signo == SIGCLD && td == threads[1])
        regen_sigcld (TRUE);

      /* Check the signal information table to learn whether to
         continue the program or to terminate the process.  This is
         only done when called from an exception handler. */

      if (context_record != NULL && signal_info[signo].fun_action != ST_IGNORE)
        {
          sig_terminate (signo, frame, context_record);
          return signal_info[signo].fun_action;
        }
    }
  return ST_IGNORE;
}


/* Deliver all pending signals. */

int deliver_pending_signals (thread_data *td, syscall_frame *frame,
                              CONTEXTRECORD *context_record)
{
  int signo, action;

  sig_lock ();
  while ((td->sig_pending & ~td->sig_blocked) != 0)
    {
      for (signo = 1; signo < NSIG; ++signo)
        if (td->sig_pending & ~td->sig_blocked & _SIGMASK (signo))
          {
            action = deliver_signal (td, signo, frame, context_record);
            if (action != ST_IGNORE)
              return action;
            sig_lock ();
          }
    }
  sig_unlock ();
  return ST_IGNORE;
}


/* This function is called from the 16-bit signal handler to generate
   signal SIGNO (if SIGNO is non-zero) and to deliver pending
   signals. */

void raise_from_16 (int signo)
{
  if (fork_exec_pid != 0)
    kill_emx (fork_exec_pid, signo, FALSE);
  else
    {
      if (signo != 0)
        generate_signal (threads[1], signo);
      deliver_pending_signals (threads[1], NULL, NULL);
    }
}


/* Unblock temporarily blocked signals, raise pending signals. */

void sig_block_end (void)
{
  thread_data *td;

  td = get_thread ();
  sig_lock ();
  td->sig_blocked = td->sig_prev_blocked;
  td->sig_prev_blocked = 0;
  sig_unlock ();
  deliver_pending_signals (td, NULL, NULL);
}


/* Temporarily block signals (for the duration of a system call).
   From now on, no signals will be raised.  Instead, generated signals
   will be made pending. */

void sig_block_start (void)
{
  thread_data *td;

  td = get_thread ();
  sig_lock ();
  sig_flag = FALSE;
  reset_event_sem (signal_sem); /* select() waits for this semaphore */
  td->sig_prev_blocked = td->sig_blocked;
  SET_BLOCKED (td, ~0);
  sig_unlock ();
}


/* Send a signal to another process (or process tree).  PID is the
   process ID of the target process and SIGNO is the signal number.
   Send signal to the process tree starting at PID if tree is true. */

static ULONG kill_emx (int pid, int signo, int tree)
{
  return DosFlagProcess (pid, (tree ? FLGP_SUBTREE : FLGP_PID), PFLG_A, signo);
}


void do_raise (int signo, syscall_frame *frame)
{
  thread_data *td;

  td = get_thread ();
  generate_signal (td, signo);
  deliver_pending_signals (td, frame, NULL);
}


/* This function implements the __kill() system call.  SIGNO is the
   signal number (or 0 to check whether the target process is alive).
   PID is the process ID of the target process.  Store the error
   number (errno) to *ERRNOP.  Return 0 on success, -1 on failure. */

int do_kill (int signo, int pid, syscall_frame *frame, int *errnop)
{
  ULONG rc;
  int org_pid;

  /* Check whether process PID is alive if SIGNO is 0. */

  if (signo == 0)
    {
      if (proc_check (pid))
        {
          /* The process is alive. */

          *errnop = 0;
          return 0;
        }

      /* The process does not exist. */

      *errnop = ESRCH;
      return -1;
    }

  /* Fail if SIGNO is not a valid signal number. */

  if (signo < 0 || signo >= NSIG || !signal_valid[signo])
    {
      *errnop = EINVAL;
      return -1;
    }

  /* Map `kill(getpid(),signo)' to `raise(signo)' to enable core
     dumps.  Do this only for the main thread as kill() is defined
     (for emx) to send the signal to the main thread. */

  if (pid == my_pid && get_tid () == 1)
    {
      do_raise (signo, frame);
      *errnop = 0;
      return 0;
    }

  /* Save the original process or group ID and turn a group ID (PID is
     negative) into a process ID. */

  org_pid = pid;
  if (pid < -1)                 /* Process group? */
    pid = -pid;                 /* Yes: compute PID from group ID */

  /* SIGKILL is treated specially: Call DosKillProcess. */

  if (signo == SIGKILL)
    {
      /* When sending SIGKILL to a process group, first try to kill
         the process tree.  If this fails, kill only one process.
         When sending SIGKILL to a single process, kill only that
         process. */

      if (org_pid < -1)
        {
          rc = DosKillProcess (DKP_PROCESSTREE, pid);
          if (rc != 0)
            rc = DosKillProcess (DKP_PROCESS, pid);
        }
      else
        rc = DosKillProcess (DKP_PROCESS, pid);

      /* If the process has already been killed but has not yet
         reacted to the signal, the return code will be
         ERROR_SIGNAL_PENDING.  Ignore this error. */

      if (rc == 0 || rc == ERROR_SIGNAL_PENDING)
        {
          *errnop = 0;
          return 0;
        }
      *errnop = set_error (rc);
      return -1;
    }

  /* We can send SIGINT and SIGBREAK to child processes. */

  if ((signo == SIGINT || signo == SIGBREAK) && pid != my_pid)
    {
      rc = DosSendSignalException (pid, ((signo == SIGINT)
                                         ? XCPT_SIGNAL_INTR
                                         : XCPT_SIGNAL_BREAK));
      if (rc == 0)
        {
          *errnop = 0;
          return 0;
        }
      /* Seems not to be a child process.  Use DosFlagProcess. */
    }

  /* Send the signal via a 16-bit signal (DosFlagProcess).  This works
     only for emx programs. */

  if (org_pid < -1)
    {
      rc = kill_emx (pid, signo, TRUE);
      if (rc != 0)
        rc = kill_emx (pid, signo, FALSE);
    }
  else
    rc = kill_emx (pid, signo, FALSE);
  if (rc == 0)
    {
      *errnop = 0;
      return 0;
    }
  *errnop = set_error (rc);
  return -1;
}


/* Set a signal handler or acknowledge a signal.  This is the main
   code for the __signal() system call.  SIGNO is the signal number.
   HANDLER is SIG_DFL, SIG_IGN, SIG_ACK or the address of the signal
   handler to be used.  FRAME points to the syscall stack frame in
   case we have to dump core.  Return the previous (or current for
   SIG_ACK) signal handler or SIG_ERR on error. */

sigfun *do_signal (ULONG signo, sigfun *handler, syscall_frame *frame)
{
  if (handler == SIG_ACK)
    {
      thread_data *td;
      struct signal_entry *p;
      void (*result)(int signo);

      /* Acknowledge (unblock) the signal. */

      if (signo < 1 || signo >= NSIG || !signal_valid[signo])
        return SIG_ERR;

      td = get_thread ();
      p = td->sig_table + signo;

      sig_lock ();
      if (!(p->sa_flags & SA_ACK))
        {
          sig_unlock ();
          return SIG_ERR;
        }
      acknowledge (signo);
      td->sig_blocked &= ~_SIGMASK (signo);
      result = p->handler;
      sig_unlock ();
      deliver_pending_signals (td, frame, NULL);
      return handler;
    }
  else
    {
      struct sigaction isa, osa;

      isa.sa_handler = handler;
      isa.sa_mask = 0;
      switch (uflags & _UF_SIG_MODEL)
        {
        case _UF_SIG_SYSV:
          isa.sa_flags = SA_SYSV;
          break;
        case _UF_SIG_EMX:
          isa.sa_flags = SA_ACK;
          break;
        default:
          isa.sa_flags = 0;
          break;
        }
      if (do_sigaction (signo, &isa, &osa) != 0)
        return SIG_ERR;
      else
        return osa.sa_handler;
    }
}


/* This function implements the __sigaction() system call. */

int do_sigaction (int signo, const struct sigaction *iact,
                  struct sigaction *oact)
{
  thread_data *td;
  struct signal_entry *p;
  struct sigaction output;

  /* Check the signal number. */

  if (signo < 1 || signo >= NSIG || !signal_valid[signo])
    return EINVAL;

  /* Fail if the process tries to set the action for SIGKILL.

     POSIX.1, 3.3.1.3: "The system shall not allow the action for the
     signals SIGKILL or SIGSTOP to be set to SIG_IGN."

     POSIX.1, 3.3.1.3: "The system shall not allow a process to catch
     the signals SIGKILL and SIGSTOP."

     POSIX.1, 3.3.4.2: "It is unspecified whether an attempt to set
     the action for a signal that cannot be caught or ignored to
     SIG_DFL is ignored or causes an error to be returned with errno
     set to [EINVAL]." */

  if (signo == SIGKILL && iact != NULL)
    return EINVAL;

  /* Get a pointer to the thread data block and a pointer to the
     signal table entry for SIGNO of the current thread. */

  td = get_thread ();
  p = td->sig_table + signo;

  /* Save the previous action. */

  sig_lock ();
  output.sa_handler = p->handler;
  output.sa_mask = p->sa_mask;
  output.sa_flags = p->sa_flags;

  /* Install new action if IACT is not NULL. */

  if (iact != NULL)
    {
      p->handler = iact->sa_handler;
      p->sa_mask = iact->sa_mask;
      p->sa_flags = iact->sa_flags;

      if ((p->handler == SIG_DFL && signal_info[signo].dfl_action != ST_IGNORE)
          || p->handler == SIG_IGN)
        {
          /* POSIX.1, 3.3.1.3: "Setting a signal action to SIG_DFL for
             a signal that is pending, and whose default action is to
             ignore the signal (for example, SIGCHLD), shall cause the
             pending signal to be discarded, whether or not it is
             blocked."

             POSIX.1, 3.3.1.3: "Setting a signal action to SIG_IGN for
             a signal that is pending shall cause the pending signal
             to be discarded, whether or not it is blocked." */

          td->sig_pending &= ~_SIGMASK (signo);
        }

      if ((p->sa_flags & SA_SYSV) && signo == SIGCLD)
        regen_sigcld (FALSE);
    }

  /* Store previous action to *OACT if OACT is not NULL.  Do this
     after reading *IACT because IACT may point to the same set as
     OACT. */

  if (oact != NULL)
    *oact = output;

  sig_unlock ();
  return 0;
}


int do_sigpending (sigset_t *set)
{
  thread_data *td;

  td = get_thread ();
  sig_lock ();
  *set = td->sig_blocked & td->sig_pending;
  sig_unlock ();
  return 0;
}


int do_sigprocmask (int how, const sigset_t *iset, sigset_t *oset,
                    syscall_frame *frame)
{
  thread_data *td;
  ULONG temp, output;

  td = get_thread ();
  sig_lock ();
  output = td->sig_blocked;
  if (iset != NULL)
    {
      switch (how)
        {
        case SIG_BLOCK:
          temp = td->sig_blocked | *iset;
          break;
        case SIG_UNBLOCK:
          temp = td->sig_blocked & ~*iset;
          break;
        case SIG_SETMASK:
          temp = *iset;
          break;
        default:
          sig_unlock ();
          return EINVAL;
        }
      acknowledge_mult (td->sig_blocked & ~temp);
      SET_BLOCKED (td, temp);

    }

  /* Set *OSET after reading *ISET, because ISET may point to the same
     set as OSET. */

  if (oset != NULL)
    *oset = output;

  /* POSIX.1, 3.3.5.2: "If there are any pending unblocked signals
     after the call to the sigprocmask() function, at least one of
     those signals shall be delivered before the sigprocmask()
     function returns." */

  sig_unlock ();
  deliver_pending_signals (td, frame, NULL);
  return 0;
}


/* This function implements the __pause() system call.  Wait for the
   occurance of a signal.  Signals set to SIG_IGN should not make this
   function return, but they do (for now). */

void do_pause (void)
{
  ULONG rc;

  do
    {
      rc = DosSleep ((ULONG)(-1));
    } while (rc == 0);
}


int do_sigsuspend (const sigset_t *mask)
{
  ULONG old_blocked;
  thread_data *td;

  td = get_thread ();
  sig_lock ();
  old_blocked = td->sig_blocked;
  acknowledge_mult (~*mask & td->sig_blocked);
  SET_BLOCKED (td, *mask);
  sig_unlock ();
  do_pause ();
  td->sig_blocked = old_blocked;
  return EINTR;
}


/* Raise signal SIGNO from the exception handler. */

ULONG signal_exception (int signo, CONTEXTRECORD *context)
{
  thread_data *td;
  struct signal_entry *p;
  int action;

  if (fork_exec_pid != 0)
    {
      kill_emx (fork_exec_pid, signo, FALSE);
      acknowledge (signo);
      return XCPT_CONTINUE_EXECUTION;
    }

  /* Get a pointer to the thread data block and a pointer to the
     signal table entry for SIGNO of the current thread. */

  td = get_thread ();
  p = td->sig_table + signo;

  if (p->handler != SIG_IGN)
    {
      generate_signal (td, signo);
      action = deliver_pending_signals (td, NULL, context);
      switch (action)
        {
        case ST_TERM:
          quit (3);
        case ST_NEXT:
          acknowledge (signo);
          return XCPT_CONTINUE_SEARCH;
        default:
          break;
        }
    }
  acknowledge (signo);
  return XCPT_CONTINUE_EXECUTION;
}


static ULONG trap (EXCEPTIONREPORTRECORD *report,
                   CONTEXTRECORD *context,
                   int signo)
{
  struct signal_entry *p;
  thread_data *td;

  /* Don't try to do anything if the stack is invalid or in a nested
     exception. */

  if (report->fHandlerFlags & (EH_STACK_INVALID|EH_NESTED_CALL))
    return XCPT_CONTINUE_SEARCH;

  /* Get a pointer to the thread data block and a pointer to the
     signal table entry for SIGNO of the current thread. */

  td = get_thread ();
  p = get_thread ()->sig_table + signo;

  if (p->handler != SIG_IGN)
    {
      generate_signal (td, signo);

      /* Deliver the signal even if its blocked. */

      sig_lock ();
      deliver_signal (td, signo, NULL, context);

      /* Deliver any other pending signals. */

      deliver_pending_signals (td, NULL, context);
    }

  /* Invoke the next exception handler. */

  return XCPT_CONTINUE_SEARCH;
}


/* Load a heap page from a dumped executable. */

static void load_heap (ULONG addr)
{
  ULONG rc, cbread, newpos;

  addr &= ~0xfff;
  rc = DosSetFilePtr (exe_fhandle, addr - heap_objs[0].base + heap_off,
                      FILE_BEGIN, &newpos);
  if (rc == 0)
    rc = DosRead (exe_fhandle, (void *)addr, 0x1000, &cbread);
  if (rc != 0)
    {
      otext ("\r\nCannot read EXE file\r\n");
      quit (255);
    }
}


/* This is the exception handler. */

ULONG exception (EXCEPTIONREPORTRECORD *report,
                 EXCEPTIONREGISTRATIONRECORD *registration,
                 CONTEXTRECORD *context, void *dummy)
{
  ULONG addr;

  /* Don't trust -- clear the decrement flag. */

  __asm__ ("cld");

  /* Don't do anything when unwinding exception handlers. */

  if (report->fHandlerFlags & (EH_EXIT_UNWIND|EH_UNWINDING))
    return XCPT_CONTINUE_SEARCH;

  /* The rest of this function depends on the exception number. */

  switch (report->ExceptionNum)
    {
    case XCPT_GUARD_PAGE_VIOLATION:

      /* Guard page violation.  This exception is used for loading
         heap pages from a dumped executable.  Moreover, this is used
         by old emx programs for expanding the stack.  First, get the
         address of the page. */

      addr = report->ExceptionInfo[1];

      /* Load a page from the executable if the page is in the heap
         and we have a dumped executable. */

      if (exe_heap && heap_obj_count >= 1
          && addr >= heap_objs[0].base && addr < heap_objs[0].brk)
        {
          load_heap (addr);
          return XCPT_CONTINUE_EXECUTION;
        }

      /* Looks like a guard page created by the application program.
         Pass the exception to the next exception handler. */

      return XCPT_CONTINUE_SEARCH;

    case XCPT_ACCESS_VIOLATION:
    case XCPT_DATATYPE_MISALIGNMENT:

      /* Map access violations to SIGSEGV. */

      return trap (report, context, SIGSEGV);

    case XCPT_INTEGER_DIVIDE_BY_ZERO:
    case XCPT_INTEGER_OVERFLOW:
    case XCPT_ARRAY_BOUNDS_EXCEEDED:
    case XCPT_FLOAT_DENORMAL_OPERAND:
    case XCPT_FLOAT_DIVIDE_BY_ZERO:
    case XCPT_FLOAT_INEXACT_RESULT:
    case XCPT_FLOAT_INVALID_OPERATION:
    case XCPT_FLOAT_OVERFLOW:
    case XCPT_FLOAT_STACK_CHECK:
    case XCPT_FLOAT_UNDERFLOW:

      /* Map integer division by zero and floating point exceptions to
         SIGFPE. */

      return trap (report, context, SIGFPE);

    case XCPT_ILLEGAL_INSTRUCTION:
    case XCPT_INVALID_LOCK_SEQUENCE:
    case XCPT_PRIVILEGED_INSTRUCTION:

      /* Map the various exceptions for illegal instructions to
         SIGILL. */

      return trap (report, context, SIGILL);

    case XCPT_SIGNAL:

      /* Map signal exceptions to SIGINT, SIGTERM or SIGBREAK. */

      if (report->cParameters >= 1)
        switch (report->ExceptionInfo[0])
          {
          case XCPT_SIGNAL_INTR:
            return signal_exception (SIGINT, context);
          case XCPT_SIGNAL_KILLPROC:
            return signal_exception (SIGTERM, context);
          case XCPT_SIGNAL_BREAK:
            return signal_exception (SIGBREAK, context);
          }
      return XCPT_CONTINUE_SEARCH;
    }

  /* The exception is not handled by emx.  Pass it to the next
     exception handler. */

  return XCPT_CONTINUE_SEARCH;
}


/* Install the exception handler for the main thread.  REGISTRATION
   points to the exception registration record, which must be in the
   stack (at the `bottom' of the stack). */

void install_except (EXCEPTIONREGISTRATIONRECORD *registration)
{
  ULONG rc;

  registration->prev_structure = NULL;
  registration->ExceptionHandler = exception;
  rc = DosSetExceptionHandler (registration);
  if (rc != 0)
    error (rc, "DosSetExceptionHandler");
}


/* Install the exception handler for another thread.  REGISTRATION
   points to the exception registration record, which must be in the
   stack (at the `bottom' of the stack). */

int initthread (EXCEPTIONREGISTRATIONRECORD *registration)
{
  ULONG rc;

  registration->prev_structure = NULL;
  registration->ExceptionHandler = exception;
  rc = DosSetExceptionHandler (registration);
  return 0;
}


/* We want to receive signals. */

void receive_signals (void)
{
  ULONG times;

  DosSetSignalExceptionFocus (SIG_SETFOCUS, &times);
}


void init_exceptions (void)
{
  USHORT old_sig16_action;      /* Dummy (output) */

  sig_flag = FALSE;
  create_event_sem (&signal_sem, DC_SEM_SHARED);
  create_mutex_sem (&sig_access);
  install_except (exc_reg_ptr);
  init_signal16 ();
  DosSetSigHandler (_emx_32to16 (&sig16_handler), &old_sig16_handler,
                    &old_sig16_action, SIGA_ACCEPT, SIG_PFLG_A);
}


/* alarm() */

static BYTE alarm_started;
static BYTE alarm_set;
static ULONG alarm_interval;
static ULONG alarm_time;
static HTIMER alarm_timer;
static HEV alarm_sem_timer;
static TID alarm_tid;


/* Exception handler for alarm_thread().  On termination of the
   thread, alarm_tid will be set to zero. */

static ULONG alarm_exception (EXCEPTIONREPORTRECORD *report,
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
      alarm_tid = 0;
      break;
    }
  return XCPT_CONTINUE_SEARCH;
}


static void alarm_thread (ULONG arg)
{
  ULONG rc;
  EXCEPTIONREGISTRATIONRECORD registration;

  registration.prev_structure = NULL;
  registration.ExceptionHandler = alarm_exception;
  DosSetExceptionHandler (&registration);

  for (;;)
    {
      rc = DosWaitEventSem (alarm_sem_timer, -1);
      if (rc == 0)
        {
          reset_event_sem (alarm_sem_timer);
          generate_signal (threads[1], SIGALRM);
        }
      else if (rc != ERROR_INTERRUPT)
        error (rc, "DosWaitEventSem");
    }
}


void start_alarm (void)
{
  ULONG rc;

  if (create_event_sem (&alarm_sem_timer, DC_SEM_SHARED) == 0)
    {
      rc = DosCreateThread (&alarm_tid, alarm_thread, 0,
                            CREATE_READY | STACK_COMMITTED, 0x4000);
      if (rc != 0)
        error (rc, "DosCreateThread");
      else
        alarm_started = TRUE;
    }
}


ULONG set_alarm (ULONG value)
{
  ULONG rc, rest, result;

  if (!alarm_started)
    {
      start_alarm ();
      if (!alarm_started)
        return -1;
    }
  result = 0;
  if (alarm_set)
    {
      rest = querysysinfo (QSV_TIME_LOW) - alarm_time;
      if (alarm_interval >= rest)
        result = alarm_interval - rest;
      DosStopTimer (alarm_timer);
      alarm_set = FALSE;
      reset_event_sem (alarm_sem_timer);
    }
  if (value == 0)
    alarm_interval = 0;
  else
    {
      alarm_set = TRUE;
      alarm_interval = value;
      rc = DosAsyncTimer (1000 * value, (HSEM)alarm_sem_timer, &alarm_timer);
      if (rc != 0)
        error (rc, "DosAsyncTimer");
      alarm_time = querysysinfo (QSV_TIME_LOW);
    }
  return result;
}


void stop_alarm (void)
{
  if (!alarm_started)
    set_alarm (0);
}


/* Called from DLL termination routine. */

void term_signal (void)
{
  if (alarm_tid != 0)
    DosKillThread (alarm_tid);
}


/* Inherit signal settings from parent process.  These are passed in
   the `_emx_sig' environment variable, which consists of the PID of
   the parent process (to ensure that we get the signal settings of
   our parent process), a colon, and a 32-bit number.  Bit n is set in
   that number if signal n is to be ignored.  Each number is passed as
   8 hexadecimal digits. */

void init2_signal (void)
{
  PSZ val;
  ULONG ppid, sigset;
  int sig;

  if (!(debug_flags & DEBUG_NOINHSIG)
      && DosScanEnv ("_emx_sig", &val) == 0 && conv_hex8 (val, &ppid)
      && ppid == init_pib_ptr->pib_ulppid && val[8] == ':'
      && conv_hex8 (val + 9, &sigset) && val[17] == 0)
    {
      for (sig = 1; sig < NSIG; ++sig)
        if (sigset & (1 << sig))
          threads[1]->sig_table[sig].handler = SIG_IGN;
    }
}
