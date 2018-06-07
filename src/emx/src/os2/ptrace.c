/* ptrace.c -- Implement ptrace()
   Copyright (c) 1994-1996 by Eberhard Mattes

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
#define INCL_DOSSESMGR
#define INCL_DOSEXCEPTIONS
#define INCL_DOSMODULEMGR
#define INCL_DOSSEMAPHORES
#define INCL_DOSERRORS
#include <os2emx.h>
#include "clib.h"
#include <sys/signal.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include "reg.h"
#include <sys/errno.h>
#include <sys/uflags.h>
#include "emxdll.h"

#define DR_SERVER_HEV_CLIENT    0x0001
#define DR_SERVER_HEV_SERVER    0x0002
#define DR_CLIENT_HEV_CLIENT    0x0004
#define DR_CLIENT_HEV_SERVER    0x0008
#define DR_LIST                 0x0010

#define SHAREMEM_FMT    "/sharemem/emx/ptrace/%.8x.atc"

#define REQ_ATTACH      17
#define REQ_DOSDEBUG    18
#define REQ_DETACH      19

#define USEROFF(F) offsetof (struct user, F)
#define DBGOFF(F) offsetof (uDB_t, F)
#define CTXOFF(F) offsetof (CONTEXTRECORD, F)

#define PTRACE_REG_COUNT sizeof (ptrace_regs) / sizeof (ptrace_regs[0])

struct reg_table
{
  ULONG reg;
  ULONG user_offset;
  ULONG dbg_offset;
  ULONG ctx_offset;
  BYTE len;
};

struct co_regs
{
  USHORT cw;
  USHORT unused1;
  USHORT sw;
  USHORT unused2;
  USHORT tw;
  USHORT unused3;
  ULONG fip;
  USHORT fcs;
  USHORT fop;
  ULONG foo;
  USHORT fos;
  USHORT unused4;
  BYTE fst[8][10];
};

enum dbg_type
{
  DT_LOCAL,
  DT_SERVER,
  DT_CLIENT
};

struct debuggee
{
  struct debuggee *next;        /* Next debuggee */
  enum dbg_type type;           /* Type of debuggee */
  ULONG resources;              /* Acquired resources: DR_*. */
  ULONG pid;                    /* Process ID */
  ULONG sid;                    /* Session ID */
  ULONG syscall_addr;           /* Address of CALL _emx_syscall, or 0 */
  ULONG layout_addr;            /* Address of layout table, or 0 */
  ULONG signo;                  /* Signal number, or 0 */
  ULONG context;                /* Address of exception context record, or 0 */
  ULONG last_cmd;               /* Last command */
  ULONG regs[19];               /* Current registers */
  ULONG exc_regs[19];           /* Registers for exception handler */
  uDB_t dbgbuf;                 /* Buffer for DosDebug, includes registers */
  struct co_regs dbgco;         /* Floating point status */

  ULONG ptn;                    /* Notification for debugger */
  ULONG ptn_hmte;               /* For PTN_MODULE_LOAD and PTN_MODULE_FREE */
  ULONG ptn_tid;                /* For PTN_THREAD_NEW and PTN_THREAD_END */
  ULONG ptn_pid;                /* For PTN_PROC_NEW */
  ULONG ptn_flags;              /* For PTN_PROC_NEW */
  ULONG ptn_fork_addr;          /* For PTN_PROC_NEW */
  char ptn_name[260];           /* For PTN_PROC_NEW */

  ULONG n_cmd;                  /* run() interrupted by notification: CMD */
  ULONG n_tbreak;               /* Ditto, TBREAK */
  ULONG n_signo;                /* Ditto, SIGNO */
  ULONG n_tid;                  /* Ditto, TID */

  ULONG regs_tid;               /* Registers retrieved for this thread */
  ULONG server_request;         /* Request for DosDebug server */
  ULONG server_rc;              /* Return code of DosDebug on server */
  ULONG server_arg;             /* Argument passed to server */
  HEV hev_server;               /* Data ready for DosDebug server */
  HEV hev_client;               /* Data ready for DosDebug client */

  BYTE regs_changed;            /* Registers changed */
  BYTE fpregs_ok;               /* Floating point registers retrieved */
  BYTE auto_switch;             /* Automatic session switching enabled */
  BYTE brk_flag;                /* Break point hit */
  BYTE more;                    /* Step/run again */
  BYTE ptn_flag;                /* Notification sent to debugger */
};


/* We keep a linked list of processes being debugged. */

static struct debuggee *dbg_head;

/* Here, we cache the most recently access debuggee. */

static struct debuggee *dbg_cache;

/* Table for converting between user struct offsets, uDB_t offsets,
   and CONTEXTRECORD offsets.  It also contains the lengths of the
   members. */

static struct reg_table const ptrace_regs[] =
{
  {R_GS,   USEROFF (u_regs[R_GS]),   DBGOFF (GS),     CTXOFF (ctx_SegGs),  2},
  {R_FS,   USEROFF (u_regs[R_FS]),   DBGOFF (FS),     CTXOFF (ctx_SegFs),  2},
  {R_ES,   USEROFF (u_regs[R_ES]),   DBGOFF (ES),     CTXOFF (ctx_SegEs),  2},
  {R_DS,   USEROFF (u_regs[R_DS]),   DBGOFF (DS),     CTXOFF (ctx_SegDs),  2},
  {R_EDI,  USEROFF (u_regs[R_EDI]),  DBGOFF (EDI),    CTXOFF (ctx_RegEdi), 4},
  {R_ESI,  USEROFF (u_regs[R_ESI]),  DBGOFF (ESI),    CTXOFF (ctx_RegEsi), 4},
  {R_EBP,  USEROFF (u_regs[R_EBP]),  DBGOFF (EBP),    CTXOFF (ctx_RegEbp), 4},
  {R_ESP,  USEROFF (u_regs[R_ESP]),  DBGOFF (ESP),    CTXOFF (ctx_RegEsp), 4},
  {R_EBX,  USEROFF (u_regs[R_EBX]),  DBGOFF (EBX),    CTXOFF (ctx_RegEbx), 4},
  {R_EDX,  USEROFF (u_regs[R_EDX]),  DBGOFF (EDX),    CTXOFF (ctx_RegEdx), 4},
  {R_ECX,  USEROFF (u_regs[R_ECX]),  DBGOFF (ECX),    CTXOFF (ctx_RegEcx), 4},
  {R_EAX,  USEROFF (u_regs[R_EAX]),  DBGOFF (EAX),    CTXOFF (ctx_RegEax), 4},
  {R_EIP,  USEROFF (u_regs[R_EIP]),  DBGOFF (EIP),    CTXOFF (ctx_RegEip), 4},
  {R_CS,   USEROFF (u_regs[R_CS]),   DBGOFF (CS),     CTXOFF (ctx_SegCs),  2},
  {R_EFL,  USEROFF (u_regs[R_EFL]),  DBGOFF (EFlags), CTXOFF (ctx_EFlags), 4},
  {R_UESP, USEROFF (u_regs[R_UESP]), DBGOFF (ESP),    CTXOFF (ctx_RegEsp), 4},
  {R_SS,   USEROFF (u_regs[R_SS]),   DBGOFF (SS),     CTXOFF (ctx_SegSs),  2}
};


/* Prototypes for forward references. */

static ULONG start_debug_descendant (struct debuggee *parent);
static ULONG start_debug_descendant_2 (struct debuggee *parent,
    struct debuggee *child);
static ULONG do_attach (ULONG pid);
static ULONG do_detach (struct debuggee *d, ULONG addr);
static ULONG call_server (struct debuggee *d, ULONG request);
static ULONG free_debuggee (struct debuggee *d, ULONG return_value);


static const struct reg_table *user_addr (ULONG addr)
{
  int i;

  for (i = 0; i < PTRACE_REG_COUNT; ++i)
    if (ptrace_regs[i].user_offset == addr)
      return &ptrace_regs[i];
  return NULL;
}


static ULONG debug (struct debuggee *d, ULONG cmd)
{
  ULONG rc;

  d->dbgbuf.Cmd = cmd;
  d->dbgbuf.Pid = d->pid;
  d->last_cmd = cmd;
  if (d->type == DT_CLIENT)
    {
      rc = call_server (d, REQ_DOSDEBUG);
      if (rc == 0)
        rc = d->server_rc;
    }
  else
    rc = DosDebug (&d->dbgbuf);
  return (rc == 0 ? 0 : set_error (rc));
}


static ULONG debug_read_8 (struct debuggee *d, ULONG addr, BYTE *dst)
{
  ULONG rc;

  d->dbgbuf.Addr = addr;
  rc = debug (d, DBG_C_ReadMem);
  if (rc != 0 || d->dbgbuf.Cmd != DBG_N_Success) return EIO;
  *dst = (BYTE)d->dbgbuf.Value;
  return 0;
}


static ULONG debug_read_16 (struct debuggee *d, ULONG addr, USHORT *dst)
{
  ULONG rc;

  d->dbgbuf.Addr = addr;
  rc = debug (d, DBG_C_ReadMem);
  if (rc != 0 || d->dbgbuf.Cmd != DBG_N_Success) return EIO;
  *dst = (USHORT)d->dbgbuf.Value;
  return 0;
}


static ULONG debug_read_32 (struct debuggee *d, ULONG addr, ULONG *dst)
{
  ULONG rc, w;

  d->dbgbuf.Addr = addr;
  rc = debug (d, DBG_C_ReadMem);
  if (rc != 0 || d->dbgbuf.Cmd != DBG_N_Success) return EIO;
  w = d->dbgbuf.Value & 0xffff;
  d->dbgbuf.Addr = addr + 2;
  rc = debug (d, DBG_C_ReadMem);
  if (rc != 0 || d->dbgbuf.Cmd != DBG_N_Success) return EIO;
  w |= (d->dbgbuf.Value & 0xffff) << 16;
  *dst = w;
  return 0;
}


static void do_auto_switch (struct debuggee *d)
{
  DosSelectSession (d->sid);
  d->auto_switch = FALSE;
}


static int debug_poke16 (struct debuggee *d, ULONG addr, ULONG value)
{
  ULONG rc;

  d->dbgbuf.Addr  = addr;
  d->dbgbuf.Value = value;
  rc = debug (d, DBG_C_WriteMem);
  if (rc == 0 && d->dbgbuf.Cmd != DBG_N_Success)
    rc = EIO;
  return rc;
}


/* Write the registers.  TID is the thread ID (which must be
   non-zero).  Return errno. */

static ULONG write_reg (struct debuggee *d, ULONG tid)
{
  ULONG rc, i;

  if (d->regs_tid != tid)
    return EIO;

  for (i = 0; i < PTRACE_REG_COUNT; ++i)
    memcpy ((char *)&d->dbgbuf + ptrace_regs[i].dbg_offset,
            &d->regs[ptrace_regs[i].reg], ptrace_regs[i].len);

  d->dbgbuf.Tid = tid;
  rc = debug (d, DBG_C_WriteReg);
  if (rc == 0 && d->dbgbuf.Cmd != DBG_N_Success)
    rc = EIO;
  if (rc == 0)
    d->regs_changed = FALSE;
  return rc;
}


/* Read the registers.  TID is the thread ID (0 means the active
   thread).  Return errno. */

static ULONG read_reg (struct debuggee *d, ULONG tid)
{
  ULONG rc, i;
  USHORT us;

  if (d->regs_tid != 0 && d->regs_tid == tid)
    return 0;

  /* Read registers from a new or a different thread.  Write back the
     registers to the previous thread if required. */

  if (d->regs_changed)
    write_reg (d, d->regs_tid);

  memset (d->regs, 0, sizeof (d->regs));
  memset (d->exc_regs, 0, sizeof (d->exc_regs));

  d->dbgbuf.Tid = tid;
  rc = debug (d, DBG_C_ReadReg);
  if (rc == 0 && d->dbgbuf.Cmd != DBG_N_Success)
    rc = EIO;
  if (rc != 0) return rc;
  for (i = 0; i < PTRACE_REG_COUNT; ++i)
    memcpy (&d->regs[ptrace_regs[i].reg],
            (char *)&d->dbgbuf + ptrace_regs[i].dbg_offset,
            ptrace_regs[i].len);

  if (d->context != 0)
    {
      /* After an exception, get the registers from the exception
         context record. */

      memcpy (d->exc_regs, d->regs, sizeof (d->exc_regs));
      for (i = 0; i < PTRACE_REG_COUNT; ++i)
        {
          if (ptrace_regs[i].len == 4)
            rc = debug_read_32 (d, d->context + ptrace_regs[i].ctx_offset,
                                &d->regs[ptrace_regs[i].reg]);
          else
            {
              rc = debug_read_16 (d, d->context + ptrace_regs[i].ctx_offset,
                                  &us);
              d->regs[ptrace_regs[i].reg] = us;
            }
          if (rc != 0) return rc;
        }
    }
  d->regs_tid = d->dbgbuf.Tid;
  return rc;
}


/* Set a watchpoint.  Return errno. */

static ULONG set_watch (struct debuggee *d, ULONG addr, ULONG len, ULONG type)
{
  ULONG rc;

  d->dbgbuf.Addr  = addr;
  d->dbgbuf.Len   = len;
  d->dbgbuf.Index = 0;          /* Reserved */
  d->dbgbuf.Value = type | DBG_W_Local;
  rc = debug (d, DBG_C_SetWatch);
  /* TODO: Check notification code? */
  return rc;
}


static ULONG terminate (struct debuggee *d)
{
  ULONG rc;

  d->regs_tid = 0; d->fpregs_ok = FALSE;
  rc = debug (d, DBG_C_Term);
  if (rc != 0)
    return rc;
  return free_debuggee (d, 0);
}


/* TODO: Have two register sets, see read_reg(). */

static ULONG get_fpstate (struct debuggee *d)
{
  ULONG rc, i, flags;

  if (d->context != 0)
    {
      /* After an exception, get the floating point state from the
         exception context record. */

      d->fpregs_ok = FALSE;
      memset (&d->dbgco, 0, sizeof (d->dbgco));

      rc = debug_read_32 (d, d->context + CTXOFF (ContextFlags), &flags);
      if (rc != 0) return rc;

      if (!(flags & CONTEXT_FLOATING_POINT))
        return 0;

      for (i = 0; i < sizeof (struct co_regs); i += 2)
        {
          rc = debug_read_16 (d, d->context + CTXOFF (ctx_env) + i,
                              (USHORT *)((char *)&d->dbgco + i));
          if (rc != 0) return rc;
        }
      d->fpregs_ok = TRUE;
      return 0;
    }
  d->dbgbuf.Tid    = 0;            /* Thread: active thread */
  d->dbgbuf.Value  = DBG_CO_387;   /* Coprocessor type */
  d->dbgbuf.Buffer = (ULONG)&d->dbgco;
  d->dbgbuf.Len    = sizeof (d->dbgco);
  d->dbgbuf.Index  = 0;            /* Reserved */
  rc = debug (d, DBG_C_ReadCoRegs);
  if (rc == 0 && d->dbgbuf.Cmd != DBG_N_Success)
    rc = EIO;
  d->fpregs_ok = (rc == 0);
  return rc;
}


/* Return true if EIP points to a CALL instruction.  Moreover, true is
   returned if an error occurs (to be on the safe side for
   automatically switching to the child session). */

static int callp (struct debuggee *d)
{
  ULONG eip, rc;
  BYTE b;

  /* Do not call read_reg() as that would set `d->regs_tid'. */

  d->dbgbuf.Tid = 0;
  rc = debug (d, DBG_C_ReadReg);
  if (rc != 0 || d->dbgbuf.Cmd != DBG_N_Success)
    return TRUE;
  eip = d->dbgbuf.EIP;

  /* Skip prefixes. */

  do
    {
      rc = debug_read_8 (d, eip++, &b);
      if (rc != 0) return TRUE;
    } while ((b & 0xe7) == 0x26 /* Segment override prefix */
             || b == 0x64       /* FS prefix */
             || b == 0x65       /* GS prefix */
             || b == 0x66       /* Operand size prefix */
             || b == 0x67);     /* Address size prefix */

  /* Check for `CALL immediate' instruction. */

  if (b == 0xe8 || b == 0x9a)   /* CALL near label, CALL far label*/
    return TRUE;

  /* Check for `CALL indirect' instruction. */

  if (b == 0xff)
    {
      rc = debug_read_8 (d, eip++, &b);
      if (rc != 0) return TRUE;
      /* Note: This was off by one bit in child.asm! */
      if ((b & 0x38) == 0x10)   /* CALL near reg/mem */
        return TRUE;
      if ((b & 0x38) == 0x18)   /* CALL far reg/mem */
        return TRUE;
    }
  return FALSE;
}


#define N_STOP                  (-1)
#define N_CONTINUE_STOP         (-2)
#define N_CONTINUE_SEARCH       (-3)
#define N_CONTINUE_EXECUTION    (-4)
#define N_RESUME                (-5)
#define N_NOTIFY                (-6)


static void set_wait_cur_thread (struct debuggee *d, ULONG wait_ret)
{
  ULONG rc, tid;

  if ((uflags & _UF_PTRACE_MODEL) != _UF_PTRACE_MULTITHREAD)
    tid = 0;
  else
    {
      d->dbgbuf.Tid = 0;        /* Current thread */
      rc = debug (d, DBG_C_ReadReg);
      if (rc != 0 || d->dbgbuf.Cmd != DBG_N_Success)
        tid = 0;
      else
        tid = d->dbgbuf.Tid;
    }
  debug_set_wait (d->pid, tid, wait_ret, FALSE);
}


static int debug_signal (struct debuggee *d, int signo, int context_flag)
{
  d->more = FALSE;
  set_wait_cur_thread (d, 0x7f | (signo << 8));
  if (context_flag)
    {
      d->context = d->dbgbuf.Len; /* Address of context buffer */
      d->signo = signo;
    }
  return 0;
}


static int n_exception (struct debuggee *d)
{
  int rc;
  ULONG report, info, w;

  switch (d->dbgbuf.Value)
    {
    case DBG_X_PRE_FIRST_CHANCE: /* pre first chance */
    case DBG_X_STACK_INVALID:   /* invalid stack */
      /* The exception number is in d->dbgbuf.Buffer. */
      if (d->dbgbuf.Buffer == XCPT_BREAKPOINT)
        {
          d->brk_flag = TRUE; d->more = FALSE;
          set_wait_cur_thread (d, 0x7f | (SIGTRAP << 8));
          return N_CONTINUE_STOP;
        }
      else if (d->dbgbuf.Buffer == XCPT_SINGLE_STEP)
        {
          d->more = FALSE;
          set_wait_cur_thread (d, 0x7f | (SIGTRAP << 8));
          return N_CONTINUE_STOP;
        }
      break;

    case DBG_X_FIRST_CHANCE:    /* first chance */
      report = d->dbgbuf.Buffer; /* Address of report buffer */
      rc = debug_read_32 (d, report + offsetof (EXCEPTIONREPORTRECORD,
                                                  ExceptionNum), &w);
      if (rc == 0)
        switch (w)
          {
          case XCPT_GUARD_PAGE_VIOLATION:
          case XCPT_PROCESS_TERMINATE:
          case XCPT_ASYNC_PROCESS_TERMINATE:
            return N_CONTINUE_SEARCH;

          case XCPT_ACCESS_VIOLATION:
          case XCPT_DATATYPE_MISALIGNMENT:
            return debug_signal (d, SIGSEGV, TRUE);

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
            return debug_signal (d, SIGFPE, TRUE);

          case XCPT_ILLEGAL_INSTRUCTION:
          case XCPT_INVALID_LOCK_SEQUENCE:
          case XCPT_PRIVILEGED_INSTRUCTION:
            return debug_signal (d, SIGILL, TRUE);

          case XCPT_SIGNAL:
            info = offsetof (EXCEPTIONREPORTRECORD, ExceptionInfo) + 0;
            rc = debug_read_32 (d, report + info, &w);
            if (rc == 0)
              switch (w)
                {
                case XCPT_SIGNAL_INTR:
                  return debug_signal (d, SIGINT, TRUE);

                case XCPT_SIGNAL_BREAK:
                  return debug_signal (d, SIGBREAK, TRUE);

                case XCPT_SIGNAL_KILLPROC:
                  return debug_signal (d, SIGTERM, TRUE);
                }
            break;

          default:
            return debug_signal (d, SIGSEGV, TRUE);
          }
      break;

    case DBG_X_LAST_CHANCE:     /* last chance */

      return N_CONTINUE_SEARCH;
    }

  return debug_signal (d, SIGSEGV, FALSE);
}


/* Possible notification for the debugger. */

static int notify (struct debuggee *d, int ptn)
{
  if ((uflags & _UF_PTRACE_MODEL) == _UF_PTRACE_STANDARD)
    return N_CONTINUE_STOP;
  set_wait_cur_thread (d, 0x7f | (SIGPTRACENOTIFY << 8));
  d->ptn = ptn;
  d->ptn_flag = TRUE;
  return N_NOTIFY;
}


/* Handle the current notification and return N_STOP, N_CONTINUE_STOP,
   N_CONTINUE_SEARCH, N_RESUME, or errno (non-negative). */

static int notification (struct debuggee *d)
{
  int rc;

  switch (d->dbgbuf.Cmd)
    {
    case DBG_N_Success:

      /* The request was completed successfully.  Run or step again
         unless `d->more' has been set to FALSE. */

      if (d->more)
        return N_RESUME;

      /* Running or stepping the debuggee is completed.  Read the
         registers.  Skip over INT3 if a breakpoint was hit. */

      rc = read_reg (d, 0);
      if (rc == 0 && d->brk_flag)
        {
          /* Skip the INT3 instruction for compatibility with emx.exe
             and GDB. */

          d->regs[R_EIP] += 1;  /* Skip INT3 */
          d->regs_changed = TRUE;
        }
      return rc;                /* ptrace() successful (usually) */

    case DBG_N_Error:

      /* An error occured. */

#if 0
      oprintf ("DosDebug (%d) error: %d\r\n",
               (int)d->last_cmd, (int)d->dbgbuf.Value);
#endif
      return set_error (d->dbgbuf.Value);

    case DBG_N_ProcTerm:

      /* Process terminated.  Let wait() return the termination
         code. */

      debug_set_wait (d->pid, 0, (d->dbgbuf.Value & 0xff) << 8, TRUE);
      d->more = FALSE;
      rc = terminate (d);
      return rc;                /* ptrace() successful (usually) */

    case DBG_N_Exception:

      /* An exception occured. */

      return n_exception (d);

    case DBG_N_ModuleLoad:
      d->ptn_hmte = d->dbgbuf.Value;
      return notify (d, PTN_MODULE_LOAD);

    case DBG_N_ModuleFree:
      d->ptn_hmte = d->dbgbuf.Value;
      return notify (d, PTN_MODULE_FREE);

    case DBG_N_ThreadCreate:
      d->ptn_tid = d->dbgbuf.Tid;
      return notify (d, PTN_THREAD_NEW);

    case DBG_N_ThreadTerm:
      rc = notify (d, PTN_THREAD_END);
      if (rc == N_NOTIFY)
        {
          d->dbgbuf.Tid = 0;
          if (debug (d, DBG_C_ReadReg) != 0 || d->dbgbuf.Cmd != DBG_N_Success)
            rc = EINVAL;
          else
            d->ptn_tid = d->dbgbuf.Tid;
        }
      return rc;

    case DBG_N_NewProc:
      rc = notify (d, PTN_PROC_NEW);
      if (rc == N_NOTIFY)
        {
          d->ptn_pid = d->dbgbuf.Value;
          d->ptn_name[0] = 0;
          d->ptn_fork_addr = 0;
          d->ptn_flags = 0;
          rc = start_debug_descendant (d);
        }
      return rc;

    case DBG_N_Watchpoint:

      /* Watchpoint hit.  Watchpoints are currently used only for
         skipping over the call to emx_syscall when single-stepping.
         Stop the debuggee and let wait() indicate SIGTRAP. */

      d->more = FALSE;
      set_wait_cur_thread (d, 0x7f | (SIGTRAP << 8));
      return N_STOP;

    case DBG_N_AliasFree:
    default:

      /* These notifications are not expected to occur. */

      oprintf ("Unexpected DosDebug notification: %d\r\n",
               (int)d->dbgbuf.Cmd);
      quit (255);
    }
}


/* Perform a DBG_C_Go or DBG_C_SStep command.  Set a breakpoint at
   address TBREAK, if non-zero.  Send signal SIGNO, if non-zero. */

static ULONG run (struct debuggee *d, ULONG cmd, ULONG tbreak, ULONG signo,
                  ULONG tid)
{
  ULONG rc;
  int next;

  d->ptn_flag = FALSE;

  /* For now, we support raising only that signal by which the
     debuggee has been stopped. */

  if (signo != 0 && signo != d->signo)
    return EINVAL;

  /* Automatically switch sessions now for DBG_C_Go.  (For
     DBG_C_SStep, that will be done later, depending on the thread
     context.) */

  if (d->auto_switch && cmd == DBG_C_Go)
    do_auto_switch (d);

  /* Set a breakpoint at address TBREAK, if non-zero.  This is used
     for avoiding stepping into emx.dll (which would confuse GDB). */

  if (tbreak != 0)
    {
      rc = set_watch (d, tbreak, 1, DBG_W_Execute);
      if (rc != 0) return rc;
    }

  /* If the debuggee has been stopped by an exception, special
     processing is required now.  Note that DBG_C_Continue has not yet
     been performed in that case (still in first chance
     processing). */

  if (d->signo != 0)
    {
      if (signo == d->signo)
        {
          /* Restore the exception handler context, and call user
             exception handlers. */

          memcpy (d->regs, d->exc_regs, sizeof (d->regs));
          next = N_CONTINUE_SEARCH;
        }
      else
        {
          /* Restore the exception context (retrieved from the
             exception context record), and do not call user exception
             handlers. */

          next = N_CONTINUE_EXECUTION;
        }
      d->regs_changed = TRUE;   /* Write back registers */

      /* Special processing of exception done (except for the action
         indicated by `next'. */

      d->context = 0;
      d->signo = 0;
    }
  else
    next = N_RESUME;

  /* If any registers have been changed, write them to the
     debuggee. */

  if (d->regs_changed)
    {
      rc = write_reg (d, d->regs_tid);
      if (rc != 0) return rc;
    }

  d->more = TRUE; d->brk_flag = FALSE;
  d->regs_tid = 0; d->fpregs_ok = FALSE;

  for (;;)
    {
      switch (next)
        {
        case N_RESUME:
          if (d->auto_switch && cmd == DBG_C_SStep && callp (d))
            do_auto_switch (d);
          d->dbgbuf.Tid = tid;
          rc = debug (d, cmd);
          if (rc != 0) return rc;
          break;

        case N_STOP:
          rc = debug (d, DBG_C_Stop);
          if (rc != 0) return rc;
          break;

        case N_CONTINUE_STOP:
        case N_CONTINUE_SEARCH:
        case N_CONTINUE_EXECUTION:

          d->dbgbuf.Tid = 0;
          rc = debug (d, DBG_C_ReadReg);
          if (rc != 0 || d->dbgbuf.Cmd != DBG_N_Success)
            d->dbgbuf.Tid = 1;

          if (next == N_CONTINUE_SEARCH)
            d->dbgbuf.Value = XCPT_CONTINUE_SEARCH;
          else if (next == N_CONTINUE_EXECUTION)
            d->dbgbuf.Value = XCPT_CONTINUE_EXECUTION;
          else
            d->dbgbuf.Value = XCPT_CONTINUE_STOP;
          rc = debug (d, DBG_C_Continue);
          if (rc != 0) return rc;
          break;

        case N_NOTIFY:
          d->n_cmd = cmd;
          d->n_tbreak = tbreak;
          d->n_signo = signo;
          d->n_tid = tid;
          return 0;

        default:
          /* errno value */
          return (ULONG)next;
        }
      next = notification (d);
    }
}


/* Read a 32-bit word from the layout table of the module, for
   find_module_layout().  Note that this macro uses local variables of
   find_module_layout(). */

#define READ_LAYOUT(member,dst) \
  (debug_read_32 (d, addr_layout + offsetof (layout_table, member), dst) == 0)


/* Set the object addresses and sizes in the structure pointed to by
   PMODULE, if possible. */

static void find_module_layout (struct debuggee *d, struct ptn_module *pmodule)
{
  ULONG addr_1, addr_2, addr_layout, flags;
  ULONG text_base, text_end, data_base, data_end, bss_base, bss_end;
  BYTE b;

  /* Get the address of the 1st object, which is supposed to be the
     .text object. */

  d->dbgbuf.Value = 1;
  d->dbgbuf.MTE = d->ptn_hmte;
  if (debug (d, DBG_C_NumToAddr) != 0)
    return;
  addr_1 = d->dbgbuf.Addr;

  /* Get the address of the 2nd object, which is supposed to be the
     .data object. */

  d->dbgbuf.Value = 2; /* 2nd object */
  d->dbgbuf.MTE = d->ptn_hmte;
  if (debug (d, DBG_C_NumToAddr) != 0)
    return;
  addr_2 = d->dbgbuf.Addr;

  /* Check that the .text object starts with the emx startup code.
     See also spawn_debug(), crt0.s, and dll0.s. */

  if (debug_read_8 (d, addr_1 + 0, &b) != 0
      || b != 0x68
      || debug_read_32 (d, addr_1 + 1, &addr_layout) != 0
      || addr_layout != addr_2
      || debug_read_8 (d, addr_1 + 5, &b) != 0
      || b != 0xe8
      || debug_read_8 (d, addr_1 + 10, &b) != 0
      || b != 0xeb
      || debug_read_8 (d, addr_1 + 12, &b) != 0
      || b != 0xe8)
    return;

  /* The module seems to be an emx executable or DLL.  Read and check
     the layout table. */

  if (!READ_LAYOUT (flags, &flags)
      || (flags >> 24) < 2
      || !READ_LAYOUT (text_base, &text_base)
      || !READ_LAYOUT (text_end, &text_end)
      || text_base > text_end
      || !READ_LAYOUT (data_base, &data_base)
      || !READ_LAYOUT (data_end, &data_end)
      || data_base > data_end
      || !READ_LAYOUT (bss_base, &bss_base)
      || !READ_LAYOUT (bss_end, &bss_end)
      || bss_base > bss_end)
    return;

  /* Store the object addresses and sizes to the structure provided by
     the caller of ptrace(). */

  pmodule->text_start = text_base;
  pmodule->text_size = text_end - text_base;
  pmodule->data_start = data_base;
  pmodule->data_size = data_end - data_base;
  pmodule->bss_start = bss_base;
  pmodule->bss_size = bss_end - bss_base;

  /* Set PTNMOD_AOUT unless L_FLAG_LINK386 is set. */

  if (!(flags & L_FLAG_LINK386))
    pmodule->flags |= PTNMOD_AOUT;
}


/* Implement PTRACE_NOTIFICATION. */

static int get_notification (struct debuggee *d, void *data, size_t size,
                             ULONG *errnop)
{
  struct ptn_thread *pthread;
  struct ptn_module *pmodule;
  struct ptn_proc *pproc;
  ULONG rc, flags;

  if (!d->ptn_flag)
    {
      *errnop = EINVAL;
      return -1;
    }
  switch (d->ptn)
    {
    case PTN_THREAD_NEW:
    case PTN_THREAD_END:
      if (data != NULL && size >= sizeof (struct ptn_thread))
        {
          pthread = data;
          pthread->tid = d->ptn_tid;
        }
      break;

    case PTN_MODULE_LOAD:
    case PTN_MODULE_FREE:
      if (data != NULL && size >= sizeof (struct ptn_module))
        {
          pmodule = data;
          pmodule->hmte = d->ptn_hmte;
          rc = DosQueryModuleName (d->ptn_hmte, sizeof (pmodule->name),
                                   pmodule->name);
          if (rc != 0)
            pmodule->name[0] = 0;
          pmodule->text_start = 0; pmodule->text_size = 0;
          pmodule->data_start = 0; pmodule->data_size = 0;
          pmodule->bss_start = 0; pmodule->bss_size = 0;
          pmodule->flags = 0;
          if (d->ptn == PTN_MODULE_LOAD)
            find_module_layout (d, pmodule);
          if (pmodule->name[0] != 0)
            {
              rc = DosQueryAppType (pmodule->name, &flags);
              if (rc == 0 && (flags & FAPPTYP_DLL))
                pmodule->flags |= PTNMOD_DLL;
            }
        }
      break;

    case PTN_PROC_NEW:
      if (data != NULL && size >= sizeof (struct ptn_proc))
        {
          pproc = data;
          pproc->pid = d->ptn_pid;
          pproc->fork_addr = d->ptn_fork_addr;
          pproc->flags = d->ptn_flags;
          memcpy (pproc->name, d->ptn_name, sizeof (pproc->name));
        }
      break;
    }
  *errnop = 0;
  return d->ptn;
}


/* Handle PTRACE_THAW and PTRACE_FREEEZ. */

static int thaw_freeze (struct debuggee *d, ULONG cmd, ULONG tid,
                        ULONG *errnop)
{
  ULONG rc;

  if ((uflags & _UF_PTRACE_MODEL) != _UF_PTRACE_MULTITHREAD || tid == 0)
    {
      *errnop = EINVAL;
      return -1;
    }

  d->dbgbuf.Tid = tid;
  rc = debug (d, cmd);
  *errnop = rc;
  return rc == 0 ? 0 : -1;
}


int do_ptrace (ULONG request, ULONG pid, ULONG addr, ULONG data, ULONG *errnop)
{
  ULONG rc, w, tid;
  const struct reg_table *rp;
  struct debuggee *d;

  switch (request)
    {
    case PTRACE_ATTACH:
      rc = do_attach (pid);
      *errnop = rc;
      return (rc == 0 ? 0 : -1);
    }

  if ((uflags & _UF_PTRACE_MODEL) == _UF_PTRACE_MULTITHREAD)
    {
      tid = PTRACE_GETTID (pid);
      pid = PTRACE_GETPID (pid);
    }
  else
    tid = 1;

  if (pid == 0)
    {
      *errnop = ESRCH;
      return -1;
    }

  if (dbg_cache != NULL && dbg_cache->pid == pid)
    d = dbg_cache;
  else
    {
      for (d = dbg_head; d != NULL; d = d->next)
        if (d->pid == pid)
          break;
      if (d == NULL)
        {
          *errnop = ESRCH;
          return -1;
        }
      dbg_cache = d;
    }
  switch (request)
    {
    case PTRACE_EXIT:
      d->regs_tid = 0; d->fpregs_ok = FALSE;
      rc = terminate (d);
      *errnop = rc;
      if (rc != 0) return -1;
      debug_set_wait (pid, 0, 0, TRUE);
      return 0;

    case PTRACE_PEEKTEXT:
    case PTRACE_PEEKDATA:
      rc = debug_read_32 (d, addr, &w);
      *errnop = rc;
      if (rc != 0) return -1;
      return w;

    case PTRACE_POKETEXT:
    case PTRACE_POKEDATA:
      rc = debug_poke16 (d, addr, (ULONG)data & 0xffff);
      if (rc == 0)
        rc = debug_poke16 (d, addr +2, (ULONG)data >> 16);
      *errnop = rc;
      return (rc == 0 ? 0 : -1);

    case PTRACE_PEEKUSER:
      if (addr == USEROFF (u_ar0))
        w = USEROFF (u_regs) + KERNEL_U_ADDR;
      else if (addr == USEROFF (u_fpvalid))
        {
          if (!d->fpregs_ok)
            get_fpstate (d);
          w = (d->fpregs_ok ? 0xff : 0);
        }
      else if (addr == USEROFF (u_fpstate.status))
        w = 0;                  /* ... */
      else if (addr >= USEROFF (u_fpstate.state)
               && addr <= USEROFF (u_fpstate.state) + sizeof (d->dbgco) - 4)
        {
          if (!d->fpregs_ok)
            {
              rc = get_fpstate (d);
              if (rc != 0)
                {
                  *errnop = rc;
                  return -1;
                }
            }
          memcpy (&w, (char *)&d->dbgco + addr - USEROFF (u_fpstate.state), 4);
        }
      else if ((rp = user_addr (addr)) != NULL)
        {
          rc = read_reg (d, tid);
          if (rc != 0)
            {
              *errnop = rc;
              return -1;
            }
          w = 0;
          memcpy (&w, &d->regs[rp->reg], rp->len);
        }
      else
        {
          *errnop = EINVAL;
          return -1;
        }
      *errnop = 0;
      return w;

    case PTRACE_POKEUSER:
      rp = user_addr (addr);
      if (rp == NULL)
        rc = EIO;
      else
        rc = read_reg (d, tid);
      if (rc == 0)
        {
          d->regs[rp->reg] = 0;
          memcpy (&d->regs[rp->reg], &data, rp->len);
          d->regs_changed = TRUE;
        }
      *errnop = rc;
      return (rc == 0 ? 0 : -1);

    case PTRACE_RESUME:
      rc = run (d, DBG_C_Go, 0, data, tid);
      *errnop = rc;
      return (rc == 0 ? 0 : -1);

    case PTRACE_STEP:
      /* Avoid stepping into emx_syscall. */
      if (d->syscall_addr != 0 && d->syscall_addr == d->regs[R_EIP])
        rc = run (d, DBG_C_Go, d->syscall_addr + 5, data, tid);
      else
        rc = run (d, DBG_C_SStep, 0, data, tid);
      *errnop = rc;
      return (rc == 0 ? 0 : -1);

    case PTRACE_SESSION:
      if (!debug_same_sess)
        switch (data)
          {
          case 0:               /* Switch to debugger */
            DosSelectSession (0);
            d->auto_switch = FALSE;
            break;

          case 1:               /* Switch to child */
            DosSelectSession (d->sid);
            d->auto_switch = FALSE;
            break;

          case 2:               /* Automatic switch to child */
            d->auto_switch = TRUE;
            break;

          default:
            /* Succeed for undefined values. */
            break;
          }
      *errnop = 0;
      return 0;

    case PTRACE_NOTIFICATION:
      return get_notification (d, (void *)addr, (size_t)data, errnop);

    case PTRACE_CONT:
      if (!d->ptn_flag)
        {
          *errnop = EINVAL;
          return -1;
        }
      rc = run (d, d->n_cmd, d->n_tbreak, d->n_signo, d->n_tid);
      *errnop = rc;
      return (rc == 0 ? 0 : -1);

    case PTRACE_THAW:
      return thaw_freeze (d, DBG_C_Resume, tid, errnop);

    case PTRACE_FREEZE:
      return thaw_freeze (d, DBG_C_Freeze, tid, errnop);

    case PTRACE_DETACH:
      rc = do_detach (d, addr);
      *errnop = rc;
      return (rc == 0 ? 0 : -1);

    case PTRACE_TRACEME:
    default:
      *errnop = EINVAL;
      return -1;
    }
}


static ULONG free_debuggee (struct debuggee *d, ULONG return_value)
{
  struct debuggee **pd;

  if (d->resources & (DR_SERVER_HEV_SERVER | DR_CLIENT_HEV_SERVER))
    DosCloseEventSem (d->hev_server);
  if (d->resources & (DR_SERVER_HEV_CLIENT | DR_CLIENT_HEV_CLIENT))
    DosCloseEventSem (d->hev_client);

  if (d->resources & DR_LIST)
    {
      for (pd = &dbg_head; *pd != NULL; pd = &(*pd)->next)
        if (*pd == d)
          {
            *pd = d->next;
            break;
          }
      if (dbg_cache == d)
        dbg_cache = NULL;
    }

  DosFreeMem (d);
  return return_value;
}


/* Return errno. */

static ULONG new_debuggee (struct debuggee **pd, ULONG pid, ULONG sid,
                           enum dbg_type type)
{
  ULONG rc, w;
  BYTE b;
  struct debuggee *d;
  void *mem;

  if (type == DT_CLIENT || type == DT_SERVER)
    d = *pd;
  else
    {
      rc = DosAllocMem (&mem, sizeof (*d), PAG_COMMIT | PAG_READ | PAG_WRITE);
      if (rc != 0) return set_error (rc);
      d = (struct debuggee *)mem;
      d->resources = 0;
    }

  d->next = NULL;
  d->type = type;
  d->syscall_addr = 0;
  d->layout_addr = 0;
  d->fpregs_ok = FALSE;
  d->regs_tid = 0;
  d->regs_changed = FALSE;
  d->context = 0;
  d->signo = 0;
  d->ptn = PTN_NONE;
  d->ptn_flag = FALSE;

  if (type == DT_CLIENT)
    {
      rc = call_server (d, REQ_ATTACH);
      if (rc != 0)
        return free_debuggee (d, set_error (rc));
      if (d->server_rc != 0)
        return free_debuggee (d, d->server_rc);
    }

  if (type == DT_LOCAL || type == DT_SERVER)
    {
      d->pid = pid;
      d->sid = sid;

      d->dbgbuf.Tid = 0;        /* reserved */
      d->dbgbuf.Value = DBG_L_386; /* level */
      rc = debug (d, DBG_C_Connect);
      if (rc != 0)
        return free_debuggee (d, rc);
      if (d->dbgbuf.Cmd != DBG_N_Success)
        return free_debuggee (d, EINVAL);
      d->dbgbuf.Tid = 0;        /* Active thread */
      rc = debug (d, DBG_C_ReadReg);
      if (rc != 0)
        return free_debuggee (d, rc);
      if (d->dbgbuf.Cmd != DBG_N_Success)
        return free_debuggee (d, EINVAL);
    }
  if (debug_read_8 (d, ENTRY_POINT+0, &b) == 0 && b == 0x68     /* PUSH n */
      && debug_read_8 (d, ENTRY_POINT+5, &b) == 0 && b == 0xe8  /* CALL */
      && debug_read_8 (d, ENTRY_POINT+10, &b) == 0 && b == 0xeb /* JMP */
      && debug_read_8 (d, ENTRY_POINT+12, &b) == 0 && b == 0xe8 /* CALL */
      && debug_read_32 (d, ENTRY_POINT+1, &w) == 0)
    {
      d->layout_addr = w;
      d->syscall_addr = ENTRY_POINT + 12;
    }

  if (pd != NULL)
    *pd = d;

  if (type == DT_LOCAL || type == DT_CLIENT)
    {
      d->next = dbg_head;
      dbg_head = d;
      d->resources |= DR_LIST;
    }
  return 0;
}


/* Prepare a child process for debugging.  Return errno. */

ULONG spawn_debug (ULONG pid, ULONG sid)
{
  return new_debuggee (NULL, pid, sid, DT_LOCAL);
}


/* Detach from a child process, that is, let it run to the end. */

static void handle_detach (struct debuggee *d)
{
  ULONG rc;
  enum {GO, END, NOTIFICATION} state;

  /* Thaw all threads. */

  d->dbgbuf.Cmd = DBG_C_Resume;
  d->dbgbuf.Pid = d->pid;
  d->dbgbuf.Tid = 0;            /* All threads */
  rc = DosDebug (&d->dbgbuf);

  state = GO;
  do
    {
      if (state == GO)
        {
          d->dbgbuf.Cmd = DBG_C_Go;
          d->dbgbuf.Pid = d->pid;
          rc = DosDebug (&d->dbgbuf);
          if (rc != 0) break;
        }

      switch (d->dbgbuf.Cmd)
        {
        case DBG_N_Error:
          state = END;
          break;

        case DBG_N_Exception:
          d->dbgbuf.Cmd = DBG_C_ReadReg;
          d->dbgbuf.Pid = d->pid;
          d->dbgbuf.Tid = 0;
          rc = DosDebug (&d->dbgbuf);
          if (rc != 0 || d->dbgbuf.Cmd != DBG_N_Success)
            d->dbgbuf.Tid = 1;

          d->dbgbuf.Cmd = DBG_C_Continue;
          d->dbgbuf.Value = XCPT_CONTINUE_SEARCH;
          d->dbgbuf.Pid = d->pid;
          rc = DosDebug (&d->dbgbuf);
          state = rc == 0 ? NOTIFICATION : END;
          break;
          
        case DBG_N_ProcTerm:
          d->dbgbuf.Cmd = DBG_C_Go;
          d->dbgbuf.Pid = d->pid;
          rc = DosDebug (&d->dbgbuf);
          state = END;
          break;

        default:
          state = GO;
          break;
        }
    } while (state != END);

  free_debuggee (d, 0);
  DosExit (EXIT_THREAD, 0);
}



static void server_answer (struct debuggee *d)
{
  ULONG rc;

  rc = DosPostEventSem (d->hev_client);
  if (rc != 0)
    {
      oprintf ("server_answer: DosPostEventSem failed, rc=%u\r\n", rc);
      DosExit (EXIT_THREAD, 2);
    }
}


/* Handle DosDebug requests from a debugger which attached to a child
   process of one of our debuggees. */

static void debug_descendant_thread (ULONG arg)
{
  struct debuggee *d;
  ULONG rc, post_count, clients;

  /* Raise our priority. */

  rc = DosSetPriority (PRTYS_THREAD, PRTYC_FOREGROUNDSERVER, 0, 0);

  d = (struct debuggee *)arg;
  clients = 0;

  /* Handle requests. */

  for (;;)
    {
      rc = DosWaitEventSem (d->hev_server, SEM_INDEFINITE_WAIT);
      if (rc != 0)
        {
          oprintf ("debug_descendant_thread: DosWaitEventSem failed, "
                   "rc=%u\r\n", rc);
          return;
        }

      rc = DosResetEventSem (d->hev_server, &post_count);

      switch (d->server_request)
        {
        case REQ_ATTACH:
          if (clients == 0)
            {
              ++clients;
              d->server_rc = 0;
            }
          else
            d->server_rc = EACCES;
          server_answer (d);
          break;

        case REQ_DOSDEBUG:
          d->server_rc = DosDebug (&d->dbgbuf);
          server_answer (d);
          break;

        case REQ_DETACH:
          d->server_rc = 0;     /* errno*/
          server_answer (d);
          --clients;
          if (d->server_arg == 1)
            handle_detach (d);
          break;

        default:
          oprintf ("debug_descendant_thread: invalid request %u\r\n",
                   d->server_request);
          break;
        }
    }
}


/* The debuggee PARENT got a new child process.  Enable debugging of
   the child process for PTRACE_ATTACH.  Return errno or N_NOTIFY. */

static ULONG start_debug_descendant (struct debuggee *parent)
{
  char mname[64];
  struct debuggee *child;
  void *mem;
  TID tid;
  ULONG rc;

  sprintf (mname, SHAREMEM_FMT, (unsigned)parent->ptn_pid);
  rc = DosAllocSharedMem (&mem, mname, sizeof (*child),
                          PAG_COMMIT | PAG_READ | PAG_WRITE);
  if (rc != 0) return set_error (rc);
  child = (struct debuggee *)mem;
  child->resources = 0;

  rc = DosCreateEventSem (NULL, &child->hev_server, DC_SEM_SHARED, FALSE);
  if (rc == 0)
    {
      child->resources |= DR_SERVER_HEV_SERVER;
      rc = DosCreateEventSem (NULL, &child->hev_client, DC_SEM_SHARED, FALSE);
      if (rc == 0)
        {
          child->resources |= DR_SERVER_HEV_CLIENT;

          /* We don't have a session ID.  TODO: qwait_thread() may
             provide the session ID sooner or later. */

          rc = new_debuggee (&child, parent->ptn_pid, 0, DT_SERVER);

          /* More initializations. */

          if (rc == 0)
            rc = start_debug_descendant_2 (parent, child);

          /* Create a thread which will handle debugging of the new
             child process. */

          if (rc == 0)
            rc = DosCreateThread (&tid, debug_descendant_thread, (ULONG)child,
                                  CREATE_READY | STACK_COMMITTED, 0x4000);
        }
    }
  if (rc != 0)
    return free_debuggee (child, set_error (rc));

  return N_NOTIFY;
}


/* Subroutine of the above.  Return OS/2 error code. */

static ULONG start_debug_descendant_2 (struct debuggee *parent,
                                       struct debuggee *child)
{
  ULONG rc, flags, w;
  BYTE b;

  /* For now assume that the child is in the same session as its
     parent. (The condition is always true.)  That doesn't hurt
     because we can't switch sessions anyway (either we'll get error
     ERROR_SMG_SESSION_NOT_PARENT or ERROR_SMG_SESSION_NOT_FOREGRND,
     depending on whether the client or server attempts to select the
     session). */

  if (child->sid == 0)
    child->sid = parent->sid;

  /* Get module name of the child process. */

  child->dbgbuf.Tid = 0;        /* Current thread */
  rc = debug (child, DBG_C_ReadReg);
  if (rc == 0 && child->dbgbuf.Cmd == DBG_N_Success)
    {
      rc = DosQueryModuleName (child->dbgbuf.MTE, sizeof (parent->ptn_name),
                               parent->ptn_name);
      if (rc != 0)
        parent->ptn_name[0] = 0;
    }

  /* Find out whether the child process uses a.out format. */

  if (child->layout_addr != 0
      && debug_read_32 (child, (child->layout_addr
                                + offsetof (layout_table, flags)),
                        &flags) == 0
      && !(flags & L_FLAG_LINK386))
    parent->ptn_flags |= PTNPROC_AOUT;

  /* Find out whether the child process has been forked. */

  if (debug_read_8 (parent, (ULONG)&ptrace_forking, &b) == 0 && b)
    {
      parent->ptn_flags |= PTNPROC_FORK;
      if (debug_read_32 (parent, (ULONG)&ptrace_fork_addr, &w) == 0)
        parent->ptn_fork_addr = w;
    }

  return 0;
}


/* Return OS/2 error code. */

static ULONG call_server (struct debuggee *d, ULONG request)
{
  ULONG rc, count;

  rc = DosResetEventSem (d->hev_client, &count);
  if (rc != 0 && rc != ERROR_ALREADY_RESET)
    return rc;

  d->server_request = request;
  rc = DosPostEventSem (d->hev_server);
  if (rc != 0) return rc;

  rc = DosWaitEventSem (d->hev_client, SEM_INDEFINITE_WAIT);
  if (rc != 0) return rc;
  return 0;
}


/* Return errno. */

static ULONG new_client (struct debuggee **pd, ULONG pid)
{
  char mname[64];
  ULONG rc;
  void *mem;
  struct debuggee *d;

  sprintf (mname, SHAREMEM_FMT, (unsigned)pid);
  rc = DosGetNamedSharedMem (&mem, mname, PAG_READ | PAG_WRITE);
  if (rc != 0)
    return ESRCH;

  d = (struct debuggee *)mem;

  /* Assume that at most one process at a time connects to the server
     for each PID. */

  d->resources &= ~(DR_CLIENT_HEV_CLIENT | DR_CLIENT_HEV_SERVER);

  rc = DosOpenEventSem (NULL, &d->hev_server);
  if (rc == 0)
    {
      d->resources |= DR_CLIENT_HEV_SERVER;
      rc = DosOpenEventSem (NULL, &d->hev_client);
      if (rc == 0)
        d->resources |= DR_CLIENT_HEV_CLIENT;
    }
  if (rc != 0)
    return free_debuggee (d, set_error (rc));
  *pd = d;
  return 0;
}


/* Handle PTRACE_ATTACH.  Return errno. */

static ULONG do_attach (ULONG pid)
{
  struct debuggee *d;
  ULONG rc;

  rc = proc_add_attach (pid);
  if (rc != 0) return rc;

  rc = new_client (&d, pid);
  if (rc == 0)
    {
      rc = new_debuggee (&d, pid, 0, DT_CLIENT);
      if (rc == 0)
        debug_set_wait (d->pid, 0, 0x7f | (SIGTRAP << 8), FALSE);
      if (rc != 0)
        free_debuggee (d, 0);
    }
  if (rc != 0)
    proc_remove_attach (pid);
  return rc;
}


/* Handle PTRACE_DETACH.  Return errno. */

static ULONG do_detach (struct debuggee *d, ULONG addr)
{
  ULONG rc;

  if (d->type != DT_CLIENT)
    return EINVAL;
  if (addr != 0 && addr != 1)
    return EINVAL;
  d->server_arg = addr;
  rc = call_server (d, REQ_DETACH);
  if (rc != 0)
    return set_error (rc);
  else
    return free_debuggee (d, d->server_rc);
}
