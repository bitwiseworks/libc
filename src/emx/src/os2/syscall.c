/* syscall.c -- emx system calls
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
#define INCL_DOSMISC
#define INCL_DOSNLS
#define INCL_DOSPROCESS
#define INCL_VIO
#include <os2emx.h>
#include <sys/signal.h>
#include <sys/errno.h>
#include "emxdll.h"
#include "tcpip.h"
#include "version.h"
#include <sys/nls.h>
#include <sys/timeb.h>
#include <sys/time.h>
#include <sys/ulimit.h>

static void bad_spec (syscall_frame *f);
static ULONG do_syserrno (void);
static ULONG do_setsyserrno (ULONG errcode);
static void do_memupr (char *dst, ULONG size);
static int do_ctype (unsigned char *dst, int *errnop);
static void do_scrsize (int *dst);
static ULONG do_read_kbd (ULONG flags);
static unsigned do_sleep (unsigned sec);


void sys_call (syscall_frame *f)
{
  int err_no;

  if (debug_flags & DEBUG_SYSCALL)
    oprintf ("%u: syscall 7f%.2x\r\n", my_pid, (unsigned)f->b.al);

  f->e.eflags &= ~FLAG_C;

  switch (f->b.al)
    {
    case 0x00:

      /* __sbrk() */

      f->e.eax = do_sbrk (f->e.edx);
      break;

    case 0x01:

      /* __brk() */

      f->e.eax = do_brk (f->e.edx);
      break;

    case 0x02:

      /* __ulimit()

         In:   ECX      Command code (must be UL_GMEMLIM or UL_OBJREST)
               EDX      New limit (currently ignored)

         Out:  EAX      Return value:
                          greatest possible break value for UL_GMEMLIM
                          remaining bytes in current heap object for UL_OBJREST
               ECX      errno (0 if successful)

         Get greatest possible break value or remaining number of
         bytes in current heap object. */

      switch (f->e.ecx)
        {
        case UL_GMEMLIM:
          /* Note: As this is the limit for brk(), not for sbrk(), we
             don't return 512M. */
          f->e.eax = top_heap_obj != NULL ? top_heap_obj->end : 0;
          f->e.ecx = 0;
          break;
        case UL_OBJREST:
          f->e.eax = (top_heap_obj != NULL
                      ? top_heap_obj->end - top_heap_obj->brk : 0);
          f->e.ecx = 0;
          break;
        default:
          f->e.eax = (ULONG)(-1);
          f->e.ecx = EINVAL;
          break;
        }
      break;

    case 0x03:

      /* __vmstat() */

      if (f->e.ecx >= 4)
        ((ULONG *)f->e.ebx)[0] = 0;
      if (f->e.ecx >= 8)
        ((ULONG *)f->e.ebx)[1] = 0;
      break;

    case 0x04:

      /* __umask1() */

      f->e.eax = umask_bits1;
      umask_bits1 &= ~f->e.edx;
      break;

    case 0x05:

      /* __getpid() */

      f->e.eax = my_pid;
      break;

    case 0x06:

      /* __spawnve() */

      err_no = do_spawn ((struct _new_proc *)f->e.edx, &f->e.eax);
      if (err_no != 0)
        {
          f->e.eax = err_no;
          f->e.eflags |= FLAG_C;
        }
      break;

    case 0x08:

      /* __ptrace()

         In:   EBX      Request code (see /emx/include/sys/ptrace.h)
               EDI      Process ID
               EDX      Address
               ECX      Data

         Out:  EAX      Result
               ECX      errno (0 if successful)

         Debugging support */

      f->e.eax = do_ptrace (f->e.ebx, f->e.edi, f->e.edx, f->e.ecx, &f->e.ecx);
      break;

    case 0x09:

      /* __wait()

         In:   --

         Out:  EAX     Process ID of child process (-1 if no children)
               ECX     errno
               EDX     Termination status

         Wait for child process.  There are two cases:

         (1)  when debugging, return the value stored by
             __ptrace(). Waiting for other processes is not possible

         (2)  examine process table; sit on `process_death'. */

      f->e.eax = do_wait (&f->e.edx, &f->e.ecx);
      break;

    case 0x0a:

      /* __version()

         In:   --

         Out:  EAX      Version number
                          Bits 0..7:    version letter ('a'..'z')
                          Bits 8..15:   minor version number ('0'..'9')
                          Bits 16..23:  0x2e ('.')
                          Bits 24..31:  major version number ('0'..'9')
               EBX      Environment:
                          Bit 0:        VCPI
                          Bit 1:        XMS
                          Bit 2:        VDISK.SYS 3.3
                          Bit 3:        DESQview
                          Bit 4:        287
                          Bit 5:        387
                          Bit 6:        486      (not implemented)
                          Bit 7:        DPMI 0.9 (not implemented)
                          Bit 8:        DPMI 1.0 (not implemented)
                          Bit 9:        OS/2 2.0
                          Bit 10:       -t option given
                          Bits 6..31:   0 (reserved for future expansion)
               ECX      Revision index
               EDX      0 (reserved for future expansion)

         Return emx version. */

      f->e.eax = (VERSION[3] | (VERSION[2] << 8)
                  | (VERSION[1] << 16) | (VERSION[0] << 24));
      f->e.ebx = 0x0a20;        /* OS/2 2.0, 387, -ac */
      if (opt_trunc)
        f->e.ebx |= 0x400;      /* Set -t bit */
      f->e.ecx = REV_INDEX;
      f->e.edx = 0;
      break;

    case 0x0b:

      /* __memavail() */

      f->e.eax = 0;
      break;

    case 0x0c:

      /* __signal()

         In:   ECX      Signal number
               EDX      Address of signal handler or SIG_ACK, SIG_DFL, SIG_IGN

         Out:  EAX      Previous value (success) or SIG_ERR (error)

         Set signal handler. */

      f->e.eax = (ULONG)do_signal (f->e.ecx, (sigfun *)f->e.edx, f);
      break;

    case 0x0d:

      /* __kill()

         In:   ECX      Signal number
               EDX      Process ID

         Out:  EAX      0 if successful, -1 otherwise
               ECX      errno

         Send a signal to a process.  If PID is the process ID of this
         process, send the signal to thread 1.  Under OS/2, a process
         cannot send a signal to itself using DosSendSignalException:
         DosRaiseException must be used instead. */

      f->e.eax = do_kill (f->e.ecx, f->e.edx, f, &err_no);
      f->e.ecx = err_no;
      break;

    case 0x0e:

      /* __raise()

         In:   ECX      Signal number

         Out:  EAX      0 if successful, a non-zero value otherwise
               ECX      errno (0 if successful)

         Raise a signal (in current process). */

      if (f->e.ecx >= NSIG || !signal_valid [f->e.ecx])
        {
          f->e.eax = (ULONG)(-1);
          f->e.ecx = EINVAL;
        }
      else
        {
          do_raise (f->e.ecx, f);
          f->e.eax = 0;
          f->e.ecx = 0;
        }
      break;

    case 0x0f:

      /* __uflags()

         In:   ECX      Mask (only bits set to 1 in ECX will be changed)
               EDX      New values of the bits selected by ECX

         Out:  EAX      Previous set of flags
               
         Set user flags. */

      f->e.eax = uflags;
      uflags = (uflags & ~f->e.ecx) | (f->e.edx & f->e.ecx);
      break;

    case 0x10:

      /* __unwind()

         Unwind signal handlers for longjmp() -- old version. */

      unwind (exc_reg_ptr, NULL);
      break;

    case 0x11:

      /* __core() */

      if (layout_flags & L_FLAG_LINK386)
        {
          f->e.eax = EACCES;
          f->e.eflags |= FLAG_C;
        }
      else
        {
          core_regs_i (f);
          f->e.eax = core_main (f->e.ebx);
          if (f->e.eax != 0)
            f->e.eflags |= FLAG_C;
        }
      break;

    case 0x12:

      /* __portaccess() */

      f->e.eax = 0;
      break;

    case 0x13:

      /* __memaccess () */

      f->e.eax = EACCES;
      f->e.eflags |= FLAG_C;
      break;

    case 0x14:

      /* __ioctl2()

         In:   EBX      Handle
               ECX      Request code
               EDX      Argument

         Out:  EAX      Return value
               ECX      errno

         Unix-like termio ioctl(). */

      f->e.eax = do_ioctl2 (f->e.ebx, f->e.ecx, f->e.edx, &err_no);
      f->e.ecx = err_no;
      break;

    case 0x15:

      /* __alarm()

         In:   EDX      Seconds

         Out:  EAX      Time remaining

         Set alarm clock. */

      f->e.eax = set_alarm (f->e.edx);
      break;

    case 0x17:

      /* __sleep()

         In:   EDX      Seconds

         Out:  EAX      Remaining time if interrupted by signal, or 0

         Suspend process. */

      f->e.eax = do_sleep (f->e.edx);
      break;

    case 0x18:

      /* __chsize()

         In:   EBX      File handle
               EDX      File size

         Out:  CY       Error
               EAX      errno (CY)

         Set file size. */

      f->e.eax = do_chsize (f->e.ebx, f->e.edx);
      if (f->e.eax != 0)
        f->e.eflags |= FLAG_C;
      break;

    case 0x19:

      /* __fcntl()

        In:   EBX       File handle
              ECX       Request code
              EDX       Argument

        Out:  EAX       Return value
              ECX       errno (0 if successful)

        Unix-like file control. */

      f->e.eax = do_fcntl (f->e.ebx, f->e.ecx, f->e.edx, &err_no);
      f->e.ecx = err_no;
      break;

    case 0x1a:

      /* __pipe()

        In:   ECX       Size of pipe
              EDX       Pointer to storage for two handles (two DWORDs)

        Out:  EAX       0 (success) or -1 (failure)
              ECX       errno (0 if successful)

        Create unnamed pipe. */

      f->e.ecx = do_pipe ((int *)f->e.edx, f->e.ecx);
      if (f->e.ecx == 0)
        f->e.eax = 0;
      else
        f->e.eax = (ULONG)(-1);
      break;

    case 0x1b:

      /* __fsync()

         In:   EBX      File handle

         Out:  EAX      0 (success) or -1 (failure)
               ECX      errno (0 if successful)

         Update file system. */

      f->e.eax = do_fsync (f->e.ebx, &err_no);
      f->e.ecx = err_no;
      break;

    case 0x1c:

      /* __fork()

         Out:  EAX      Process ID or 0 (in new process) or -1 (failure)
               ECX      errno (0 if successful)

         Duplicate process. */

      f->e.eax = do_fork (f, &f->e.ecx);
      break;

    case 0x1d:

      /* __scrsize()

         In:   EDX      Pointer to structure

         Out:  --

         Get number of rows and columns. */

      do_scrsize ((int *)f->e.edx);
      break;

    case 0x1e:

      /* __select()

        In:   EDX       Pointer to structure

        Out:  EAX       0 (timeout), > 0 (ready) or -1 (failure)
              ECX       errno (0 if successful)

        Synchronous I/O multiplexing. */

      f->e.eax = do_select ((struct _select *)f->e.edx, &err_no);
      f->e.ecx = err_no;
      break;

    case 0x1f:

      /* __syserrno()

        Out:  EAX       Error number
       
        Return OS/2 error number for last syscall. */

      f->e.eax = do_syserrno ();
      break;

    case 0x20:

      /* __stat()

         In:  EDX       Path name
              EDI       Pointer to structure

         Out: EAX       0 (ok), -1 (error)
              ECX       errno (0 if successful)

         Get information about a path name. */

      f->e.eax = do_stat ((const char *)f->e.edx, (struct stat *)f->e.edi,
                          &err_no);
      f->e.ecx = err_no;
      break;

    case 0x21:

      /* __fstat()

        In:  EBX        File handle
             EDI        Pointer to structure

        Out: EAX        0 (ok), -1 (error)
             ECX        errno (0 if successful)

        Get information about an open file. */

      f->e.eax = do_fstat (f->e.ebx, (struct stat *)f->e.edi, &err_no);
      f->e.ecx = err_no;
      break;

    case 0x23:

      /* __filesys()

        In:  EDX        Pointer to drive name
             EDI        Pointer to output buffer
             ECX        Size of output buffer

        Out: EAX        0 (ok) or -1 (error)
             ECX        errno (0 if successful)

        Get name of file-system driver. */

      f->e.eax = do_filesys ((char *)f->e.edi, f->e.ecx,
                             (const char *)f->e.edx, &err_no);
      f->e.ecx = err_no;
      break;

    case 0x24:

      /* __utimes()

         In:  EDX       Pointer to path name
              ESI       Pointer to array of structures

         Out: EAX       0 (ok) or -1 (error)
              ECX       errno (0 if successful)

         Set access and modification time of a file. */

      f->e.eax = do_utimes ((const char *)f->e.edx,
                            (const struct timeval *)f->e.esi, &err_no);
      f->e.ecx = err_no;
      break;

    case 0x25:

      /* __ftruncate()

         In:   EBX      File handle
               EDX      File size

         Out:  EAX      0 (ok), -1 (error)
               ECX      errno (0 if successful)

         Truncate a file. */

      f->e.ecx = do_ftruncate (f->e.ebx, f->e.edx);
      if (f->e.ecx == 0)
        f->e.eax = 0;
      else
        f->e.eax = (ULONG)(-1);
      break;

    case 0x26:

      /* __clock()

         Out:  EAX      Timer ticks of CPU time used, low-order 32 bits
               EDX      High-order 32 bits

         Return CPU time used by this process. */

      {
        unsigned long long tmp;

        tmp = get_clock (FALSE);
        f->e.eax = (ULONG)tmp;
        f->e.edx = (ULONG)(tmp >> 32);
      }
      break;

    case 0x27:

      /* __ftime()

         In:   EDX      Pointer to structure

         Get current time. */

      do_ftime ((struct timeb *)f->e.edx);
      break;

    case 0x28:

      /* __umask()

         In:   EDX      New file permission mask

         Out:  EAX      Previous file permission mask

         Set file permission mask. */

      f->e.eax = umask_bits;
      umask_bits = f->e.edx;
      break;

    case 0x29:

      /* __getppid()

         Out: EAX       Parent process ID

         Get parent process ID. */

      f->e.eax = init_pib_ptr->pib_ulppid;
      break;

    case 0x2a:

      /* __nls_memupr()

         In:  EDX       Pointer to buffer
              ECX       Size of buffer

         Convert buffer to upper case. */

      do_memupr ((char *)f->e.edx, f->e.ecx);
      break;

    case 0x2b:

      /* __open()

         In:   EDX      Pointer to file name
               ECX      Flags
               EBX      Size

         Out:  EAX      Handle or -1 (failure)
               ECX      errno (0 if successful)

         Open a file. */

      f->e.eax = sys_open ((const char *)f->e.edx, f->e.ecx, f->e.ebx,
                           &err_no);
      f->e.ecx = err_no;
      break;

    case 0x2c:

      /* __newthread()

         In:   EDX      Thread ID

         Out:  EAX      0 (success), -1 (failure)
               ECX      errno (0 if successful)

         Notify emx of a thread, create thread data block. */

      f->e.eax = new_thread (f->e.edx, &err_no);
      f->e.ecx = err_no;
      break;

    case 0x2d:

      /* __endthread()

         In:   EDX      Thread ID

         Out:  EAX      0 (success), -1 (failure)
               ECX      errno (0 if successful)

         Notify emx of the end of a thread, deallocate thread data
         block. */

      f->e.eax = end_thread (f->e.edx, &err_no);
      f->e.ecx = err_no;
      break;

    case 0x2e:

      /* __waitpid()

         In:   EDX      Process ID
               ECX      Options

         Out:  EAX      Process ID of child process (-1 if no children)
               ECX      errno
               EDX      Termination status

         Wait for child process. */

      f->e.eax = do_waitpid (f->e.edx, f->e.ecx, &f->e.edx, &f->e.ecx);
      break;

    case 0x2f:

      /* __read_kbd()

         In:   EDX      Flags (bit 0: echo, bit 1: wait, bit 2:sig)

         Out:  EAX      Character (or -1)

         Raw keyboard input. */

      f->e.eax = do_read_kbd (f->e.edx);
      break;

    case 0x30:

      /* __sleep2()

         In:   EDX      Milliseconds

         Out:  EAX      0

         Suspend process. */

      DosSleep (f->e.edx);
      f->e.eax = 0;
      break;

    case 0x31:

      /* __unwind2()

         Unwind signal handlers for longjmp(). */

      unwind ((EXCEPTIONREGISTRATIONRECORD *)f->e.edx, NULL);
      break;

    case 0x32:

      /* __pause()

         Wait for signal. */

      do_pause ();
      break;

    case 0x33:

      /* __execname()

         In:   EDX      Buffer
               ECX      Buffer size

         Out:  EAX      0 if successful, -1 otherwise

         Get the name of the executable file. */

      f->e.eax = execname ((char *)f->e.edx, f->e.ecx);
      break;

    case 0x34:

      /* __initthread()

         In:   EDX      Pointer to EXCEPTIONREGISTRATIONRECORD

         Out:  EAX      0 if successful, -1 otherwise

         Install exception handler in new thread. */

      f->e.eax = initthread ((EXCEPTIONREGISTRATIONRECORD *)f->e.edx);
      break;

    case 0x35:

      /* __sigaction */

      f->e.ecx = do_sigaction (f->e.ecx, (const struct sigaction *)f->e.edx,
                               (struct sigaction *)f->e.ebx);
      if (f->e.ecx == 0)
        f->e.eax = 0;
      else
        f->e.eax = (ULONG)(-1);
      break;

    case 0x36:

      /* __sigpending */

      f->e.ecx = do_sigpending ((sigset_t *)f->e.edx);
      if (f->e.ecx == 0)
        f->e.eax = 0;
      else
        f->e.eax = (ULONG)(-1);
      break;

    case 0x37:

      /* __sigprocmask */

      f->e.ecx = do_sigprocmask (f->e.ecx, (const sigset_t *)f->e.edx,
                                 (sigset_t *)f->e.ebx, f);
      if (f->e.ecx == 0)
        f->e.eax = 0;
      else
        f->e.eax = (ULONG)(-1);
      break;

    case 0x38:

      /* __sigsuspend */

      f->e.ecx = do_sigsuspend ((const sigset_t *)f->e.edx);
      if (f->e.ecx == 0)
        f->e.eax = 0;
      else
        f->e.eax = (ULONG)(-1);
      break;

    case 0x39:

      /* __imphandle */

      f->e.eax = do_imphandle (f->e.edx, &err_no);
      f->e.ecx = err_no;
      break;

    case 0x3b:

      /* __getsockhandle */

      f->e.eax = tcpip_getsockhandle (f->e.ebx, &err_no);
      f->e.ecx = err_no;
      break;

    case 0x3c:

      /* __socket */

      f->e.eax = tcpip_socket (f->e.ecx, f->e.edx, f->e.ebx, &err_no);
      f->e.ecx = err_no;
      break;

    case 0x3d:

      /* __bind */

      f->e.ecx = tcpip_bind (f->e.ebx, (void *)f->e.edx, f->e.ecx);
      f->e.eax = (ULONG)(f->e.ecx == 0 ? 0 : -1);
      break;

    case 0x3e:

      /* __listen */

      f->e.ecx = tcpip_listen (f->e.ebx, f->e.edx);
      f->e.eax = (ULONG)(f->e.ecx == 0 ? 0 : -1);
      break;

    case 0x3f:

      /* __recv */

      f->e.eax = tcpip_recv (f->e.ebx, (void *)f->e.edx, f->e.ecx, f->e.esi,
                             &err_no);
      f->e.ecx = err_no;
      break;

    case 0x40:

      /* __send */

      f->e.eax = tcpip_send (f->e.ebx, (const void *)f->e.edx, f->e.ecx,
                             f->e.esi, &err_no);
      f->e.ecx = err_no;
      break;

    case 0x41:

      /* __accept */

      f->e.eax = tcpip_accept (f->e.ebx, (void *)f->e.edx, (int *)f->e.ecx,
                               &err_no);
      f->e.ecx = err_no;
      break;

    case 0x42:

      /* __connect */

      f->e.ecx = tcpip_connect (f->e.ebx, (void *)f->e.edx, f->e.ecx);
      f->e.eax = (ULONG)(f->e.ecx == 0 ? 0 : -1);
      break;

    case 0x43:

      /* __getsockopt */

      f->e.ecx = tcpip_getsockopt (f->e.ebx, f->e.edx, f->e.ecx,
                                   (void *)f->e.esi, (int *)f->e.edi);
      f->e.eax = (ULONG)(f->e.ecx == 0 ? 0 : -1);
      break;

    case 0x44:

      /* __setsockopt */

      f->e.ecx = tcpip_setsockopt (f->e.ebx, f->e.edx, f->e.ecx,
                                   (const void *)f->e.esi, f->e.edi);
      f->e.eax = (ULONG)(f->e.ecx == 0 ? 0 : -1);
      break;

    case 0x45:

      /* __getsockname */

      f->e.ecx = tcpip_getsockname (f->e.ebx, (void *)f->e.edx,
                                    (int *)f->e.ecx);
      f->e.eax = (ULONG)(f->e.ecx == 0 ? 0 : -1);
      break;

    case 0x46:

      /* __getpeername */

      f->e.ecx = tcpip_getpeername (f->e.ebx, (void *)f->e.edx,
                                    (int *)f->e.ecx);
      f->e.eax = (ULONG)(f->e.ecx == 0 ? 0 : -1);
      break;

    case 0x47:

      /* __gethostbyname */

      f->e.ecx = tcpip_gethostbyname ((const char *)f->e.edx,
                                      (void **)f->e.ebx);
      f->e.eax = (ULONG)(f->e.ecx == 0 ? 0 : -1);
      break;

    case 0x48:

      /* __gethostbyaddr */

      f->e.ecx = tcpip_gethostbyaddr ((const char *)f->e.edx, f->e.ecx,
                                      f->e.esi, (void **)f->e.ebx);
      f->e.eax = (ULONG)(f->e.ecx == 0 ? 0 : -1);
      break;

    case 0x49:

      /* __getservbyname */

      f->e.eax = tcpip_getservbyname ((const char *)f->e.edx,
                                      (const char *)f->e.ecx,
                                      (void **)f->e.ebx);
      f->e.ecx = 0;
      break;

    case 0x4a:

      /* __getservbyport */

      f->e.eax = tcpip_getservbyport (f->e.edx, (const char *)f->e.ecx,
                                      (void **)f->e.ebx);
      f->e.ecx = 0;
      break;

    case 0x4b:

      /* __getprotobyname */

      f->e.eax = tcpip_getprotobyname ((const char *)f->e.edx,
                                       (void **)f->e.ebx);
      f->e.ecx = 0;
      break;

    case 0x4c:

      /* __getprotobynumber */

      f->e.eax = tcpip_getprotobynumber (f->e.edx, (void **)f->e.ebx);
      f->e.ecx = 0;
      break;

    case 0x4d:

      /* __getnetbyname */

      f->e.eax = tcpip_getnetbyname ((const char *)f->e.edx,
                                     (void **)f->e.ebx);
      f->e.ecx = 0;
      break;

    case 0x4e:

      /* __getnetbyaddr */

      f->e.eax = tcpip_getnetbyaddr (f->e.edx, (void **)f->e.ebx);
      f->e.ecx = 0;
      break;

    case 0x4f:

      /* __gethostname */

      f->e.ecx = tcpip_gethostname ((char *)f->e.edx, f->e.ecx);
      f->e.eax = (ULONG)(f->e.ecx == 0 ? 0 : -1);
      break;

    case 0x50:

      /* __gethostid */

      f->e.ecx = tcpip_gethostid ((int *)f->e.ebx);
      f->e.eax = (ULONG)(f->e.ecx == 0 ? 0 : -1);
      break;

    case 0x51:

      /* __shutdown */

      f->e.ecx = tcpip_shutdown (f->e.ebx, f->e.edx);
      f->e.eax = (ULONG)(f->e.ecx == 0 ? 0 : -1);
      break;

    case 0x52:

      /* __recvfrom */

      f->e.eax = tcpip_recvfrom ((const struct _recvfrom *)f->e.edx, &err_no);
      f->e.ecx = err_no;
      break;

    case 0x53:

      /* __sendto */

      f->e.eax = tcpip_sendto ((const struct _sendto *)f->e.edx, &err_no);
      f->e.ecx = err_no;
      break;

    case 0x54:

      /* __impsockhandle */

      f->e.eax = tcpip_impsockhandle (f->e.edx, f->e.ecx, &err_no);
      f->e.ecx = err_no;
      break;

    case 0x57:

      /* __ttyname()

         In:  EDX       File handle
              EDI       Pointer to output buffer
              ECX       Size of output buffer

        Out:  EAX       0 (ok) or -1 (error)
              ECX       errno (0 if successful)

        Get name of device. */

      f->e.eax = do_ttyname ((char *)f->e.edi, f->e.ecx, f->e.edx, &err_no);
      f->e.ecx = err_no;
      break;

    case 0x58:

      /* __settime()

         In:  EDX       Pointer to struct timeval

         Out: EAX       0 (ok) or -1 (error)
              ECX       errno (0 if successful)

         Set system time and time zone. */

      f->e.eax = do_settime ((const struct timeval *)f->e.edx, &err_no);
      f->e.ecx = err_no;
      break;

    case 0x59:

      /* __profil()

         In:  EDX       Pointer to struct _profil

         Out: EAX       0 (ok) or -1 (error)
              ECX       errno (0 if successful)

         Sampling profiler. */

      f->e.eax = do_profil ((const struct _profil *)f->e.edx, &err_no);
      f->e.ecx = err_no;
      break;

    case 0x5a:

      /* __nls_ctype()

         In:  EDX       Pointer to buffer

         Mark DBCS lead bytes. */

      f->e.eax = do_ctype ((unsigned char *)f->e.edx, &err_no);
      f->e.ecx = err_no;
      break;

    case 0x5b:

      /* __setsyserrno()

        In:   EDX       New error number
        Out:  EAX       Old error number
       
        Return old OS/2 error number for last syscall. */

      f->e.eax = do_setsyserrno (f->e.edx);
      break;

    default:
      if (f->b.al > 0x39)
        {
          f->e.eax = (ULONG)(-1);
          f->e.ecx = ENOSYS;
        }
      else
        bad_spec (f);
      break;
    }
}


static void bad_spec (syscall_frame *f)
{
  oprintf ("Invalid syscall function code: 7f%.2x\r\n", (unsigned)f->b.al);
  quit (255);
}


static ULONG do_syserrno (void)
{
  thread_data *td;

  td = get_thread ();
  if (td == NULL)
    return (ULONG)-1;
  return td->prev_sys_errno;
}


static ULONG do_setsyserrno (ULONG errcode)
{
  thread_data *td;

  td = get_thread ();
  if (td == NULL)
    return (ULONG)-1;
  td->last_sys_errno = errcode;
  return td->prev_sys_errno;
}

/* Convert a string to upper case. */

static void do_memupr (char *dst, ULONG size)
{
  COUNTRYCODE cc;

  cc.country = 0;
  cc.codepage = 0;
  DosMapCase (size, &cc, dst);
}


/* Mark DBCS lead bytes. */

static int do_ctype (unsigned char *dst, int *errnop)
{
  ULONG rc;
  unsigned i;

  rc = get_dbcs_lead ();
  if (rc != 0)
    {
      *errnop = set_error (rc);
      return -1;
    }
  for (i = 0; i < 256; ++i)
    if (_nls_is_dbcs_lead (i))
      dst[i] |= _NLS_DBCS_LEAD;
    else
      dst[i] &= ~_NLS_DBCS_LEAD;
  *errnop = 0;
  return 0;
}


/* Get the number of rows and columns of the screen. */

static void do_scrsize (int *dst)
{
  static VIOMODEINFO vmi;       /* Must not cross a 64K boundary */

  vmi.cb = sizeof (vmi);
  VioGetMode (&vmi, 0);
  dst[0] = vmi.col;
  dst[1] = vmi.row;
}


static ULONG do_read_kbd (ULONG flags)
{
  UCHAR c;

  if (kbd_input (&c, !(flags & 4), flags & 1, flags & 2, FALSE))
    return c;
  else
    return (ULONG)-1;
}


static unsigned do_sleep (unsigned sec)
{
  if (sec == 0)
    {
      DosSleep (0);
      return 0;
    }
  else
    {
      ULONG start, stop, elapsed;

      /* TODO: Don't get interrupted by ignored signals (SIG_IGN). */

      start = querysysinfo (QSV_TIME_LOW);
      if (DosSleep (1000 * sec) == 0)
        return 0;
      stop = querysysinfo (QSV_TIME_LOW);
      elapsed = stop - start;
      if (sec < elapsed)
        return 0;
      return sec - elapsed;
    }
}
