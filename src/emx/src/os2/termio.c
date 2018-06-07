/* termio.c -- General terminal interface
   Copyright (c) 1993-1998 by Eberhard Mattes

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
#define INCL_DOSDEVICES
#define INCL_DOSDEVIOCTL
#define INCL_DOSSEMAPHORES
#define INCL_DOSDATETIME
#define INCL_DOSERRORS
#define INCL_KBD
#include <os2emx.h>
#include "emxdll.h"
#include "files.h"
#include "clib.h"
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/termio.h>         /* SysV */
#include <sys/ioctl.h>          /* _TCSANOW, _TCSADRAIN, and _TCSAFLUSH */
#include <termios.h>            /* POSIX.1 */
#include <sys/signal.h>

#define NULL ((void *)0)

#define KBD_SLEEP       50


/* Semaphores. */

HEV kbd_sem_cont;
HEV kbd_sem_stop;
HEV kbd_sem_time;
HMUX kbd_sem_mux;
HMTX kbd_sem_access;
HEV kbd_sem_new;

char kbd_started;

static char kbd_buf[1024];
static unsigned kbd_ptr_in;
static unsigned kbd_ptr_out;
static char kbd_kill_flag;
static char kbd_stop_flag;
static char kbd_ext_flag;
static KBDKEYINFO kbd_inp;
static TID kbd_tid;

static char kbd_idelete;
static int kbd_cc_intr;
static int kbd_cc_quit;


static void kbd_thread (ULONG arg);


/* Start the keyboard thread. */

static void start_keyboard (void)
{
  ULONG rc;
  SEMRECORD semrec[3];

  create_event_sem (&kbd_sem_new, DC_SEM_SHARED);
  create_event_sem (&kbd_sem_cont, 0);
  create_event_sem (&kbd_sem_stop, 0);
  create_event_sem (&kbd_sem_time, DC_SEM_SHARED);

  semrec[0].hsemCur = (HSEM)kbd_sem_new;
  semrec[0].ulUser  = 0;
  semrec[1].hsemCur = (HSEM)kbd_sem_time;
  semrec[1].ulUser  = 1;
  semrec[2].hsemCur = (HSEM)signal_sem;
  semrec[2].ulUser  = 2;
  create_muxwait_sem (&kbd_sem_mux, 3, semrec, DCMW_WAIT_ANY);
  create_mutex_sem (&kbd_sem_access);

  kbd_ptr_in = kbd_ptr_out = 0;

  rc = DosCreateThread (&kbd_tid, kbd_thread, 0,
                        CREATE_READY | STACK_COMMITTED, 0x4000);
  if (rc != 0) error (rc, "DosCreateThread");
  kbd_started = TRUE;
}


/* Set default values for a termio structure.

   Note that the default values for c_iflag, c_lflag and c_cc differ
   from Unix. */

void init_termio (ULONG handle)
{
  my_file *d;

  if (!IS_VALID_FILE (handle)) return;
  d = GET_FILE (handle);
  d->c_iflag = BRKINT | ICRNL | IXON | IXANY;
  d->c_oflag = 0;
  d->c_cflag = B9600 | CS8 | CREAD | HUPCL;
  d->c_lflag = ISIG | ICANON | ECHO | ECHOE | ECHOK | IDEFAULT;
  d->c_cc[VINTR]  = 0x03;       /* Ctrl-C */
  d->c_cc[VQUIT]  = 0x1c;       /* Ctrl-\ */
  d->c_cc[VERASE] = 0x08;       /* Ctrl-H */
  d->c_cc[VKILL]  = 0x15;       /* Ctrl-U */
  d->c_cc[VEOF]   = 0x04;       /* Ctrl-D */
  d->c_cc[VEOL]   = 0;          /* Disabled */
  d->c_cc[VMIN]   = 6;
  d->c_cc[VTIME]  = 1;
  d->c_cc[VSUSP]  = 0;          /* Disabled */
  d->c_cc[VSTOP]  = 0x13;       /* Ctrl-S */
  d->c_cc[VSTART] = 0x11;       /* Ctrl-Q */
  d->tio_escape   = 0;
  d->tio_buf_count = 0;
}


/* Return the number of characters in the keyboard buffer. */

ULONG kbd_avail (void)
{
  ULONG result;

  request_mutex (kbd_sem_access);
  if (kbd_ptr_in >= kbd_ptr_out)
    result = kbd_ptr_in - kbd_ptr_out;
  else
    result = sizeof (kbd_buf) - (kbd_ptr_out - kbd_ptr_in);
  DosReleaseMutexSem (kbd_sem_access);
  return result;
}


/* Put character C into the keyboard buffer.  Return true if
   successful. */

int kbd_put (UCHAR c)
{
  int result;
  unsigned i;

  request_mutex (kbd_sem_access);
  i = (kbd_ptr_in + 1) % sizeof (kbd_buf);
  if (i == kbd_ptr_out)
    {
      /* Buffer full. Clear the buffer (like Unix) and return FALSE. */

      kbd_ptr_in = kbd_ptr_out = 0;
      result = FALSE;
    }
  else
    {
      kbd_buf[kbd_ptr_in] = c;
      kbd_ptr_in = i;
      DosPostEventSem (kbd_sem_new);
      result = TRUE;
    }
  DosReleaseMutexSem (kbd_sem_access);
  return result;
}


static UCHAR kbd_get (void)
{
  UCHAR c;

  if (kbd_ext_flag)
    {
      kbd_ext_flag = FALSE;
      return kbd_inp.chScan;
    }

  do
    {
      KbdCharIn (&kbd_inp, IO_NOWAIT, 0);
      if (kbd_kill_flag)
        DosExit (EXIT_THREAD, 0);
      if (kbd_inp.fbStatus == 0)
        {
          if (kbd_stop_flag)
            {
              reset_event_sem (kbd_sem_cont);
              DosPostEventSem (kbd_sem_stop);
              DosWaitEventSem (kbd_sem_cont, SEM_INDEFINITE_WAIT);
            }
          else
            DosSleep (KBD_SLEEP);
        }
    } while (!(kbd_inp.fbStatus & KBDTRF_FINAL_CHAR_IN));

  c = kbd_inp.chChar;

  if (kbd_idelete && kbd_inp.chScan == 14)
    {
      /* Swap BS and DEL. */

      if (c == 0x08)
        return 0x7f;
      else if (c == 0x7f)
        return 0x08;
    }

  /* Return extended codes for Alt-Space and Ctrl-Space. */

  if (c == ' ' && kbd_inp.chScan == 57)
    {
      if (kbd_inp.fsState & KBDSTF_ALT)
        {
          kbd_inp.chScan = 57;  /* Alt-Space */
          kbd_ext_flag = TRUE;
          return 0;
        }
      if (kbd_inp.fsState & KBDSTF_CONTROL)
        {
          kbd_inp.chScan = 2;   /* Ctrl-Space */
          kbd_ext_flag = TRUE;
          return 0;
        }
    }

  if (c == 0xe0)
    {
      if (!(kbd_inp.fbStatus & KBDTRF_EXTENDED_CODE))
        return c;

      if (kbd_inp.fsState & (KBDSTF_RIGHTSHIFT | KBDSTF_LEFTSHIFT))
        {
          if (kbd_inp.chScan == 82) /* Ins */
            kbd_inp.chScan = 4; /* Shift-Ins */
          else if (kbd_inp.chScan == 83) /* Del */
            kbd_inp.chScan = 5; /* Shift-Del */
        }
      kbd_ext_flag = TRUE;
      return 0;
    }

  if (c == 0)
    kbd_ext_flag = TRUE;
  return c;
}


/* Stuff COUNT characters at SRC into the keyboard buffer.  The first
   character of SRC will be read first, later.  Return true if
   successful. */

static int kbd_stuff (const char *src, unsigned count)
{
  int result;
  unsigned i;

  request_mutex (kbd_sem_access);
  if (count == 0)
    result = TRUE;
  else
    {
      i = kbd_ptr_out;
      while (count != 0)
        {
          if (i == 0)
            i = sizeof (kbd_buf) - 1;
          else
            --i;
          if (i == kbd_ptr_in)
            break;
          kbd_ptr_out = i;
          kbd_buf[i] = src[--count];
        }
      result = (count == 0);
    }
  DosReleaseMutexSem (kbd_sem_access);
  return result;
}


/* Flush the keyboard buffer. */

static void kbd_flush (my_file *d)
{
  if (kbd_started)
    request_mutex (kbd_sem_access);
  KbdFlushBuffer (0);
  kbd_ptr_in = kbd_ptr_out = 0;
  if (d != NULL)
    d->tio_buf_count = 0;
  if (kbd_started)
    DosReleaseMutexSem (kbd_sem_access);
}


/* Switch the keyboard to binary mode. */

static void kbd_bin_mode (void)
{
  static KBDINFO ki;            /* Must not cross a 64K boundary */

  ki.cb           = sizeof (ki);
  ki.fsMask       = KEYBOARD_ECHO_OFF | KEYBOARD_BINARY_MODE;
  ki.chTurnAround = (USHORT)'\r';
  ki.fsInterim    = 0;
  ki.fsState      = 0;
  KbdSetStatus (&ki, 0);
}


void kbd_stop (void)
{
  static KBDINFO ki;            /* Must not cross a 64K boundary */

  if (kbd_started && !kbd_stop_flag)
    {
      reset_event_sem (kbd_sem_stop);
      kbd_stop_flag = TRUE;
      DosWaitEventSem (kbd_sem_stop, SEM_INDEFINITE_WAIT);

      ki.cb           = sizeof (ki);
      ki.fsMask       = KEYBOARD_ECHO_ON | KEYBOARD_ASCII_MODE;
      ki.chTurnAround = (USHORT)'\r';
      ki.fsInterim    = 0;
      ki.fsState      = 0;
      KbdSetStatus (&ki, 0);
    }
}


void kbd_restart (void)
{
  if (kbd_started)
    {
      kbd_bin_mode ();
      if (kbd_stop_flag)
        {
          kbd_stop_flag = FALSE;
          DosPostEventSem (kbd_sem_cont);
        }
    }
}


/* Exception handler for kbd_thread().  On termination of the thread,
   kbd_tid will be set to zero. */

static ULONG kbd_exception (EXCEPTIONREPORTRECORD *report,
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
      kbd_tid = 0;
      break;
    }
  return XCPT_CONTINUE_SEARCH;
}


static void kbd_thread (ULONG arg)
{
  UCHAR c;
  EXCEPTIONREGISTRATIONRECORD registration;

  registration.prev_structure = NULL;
  registration.ExceptionHandler = kbd_exception;
  DosSetExceptionHandler (&registration);

  kbd_bin_mode ();
  for (;;)
    {
      c = kbd_get ();
      if (c == 0)
        {
          /* Handle scan codes: either store both characters or none. */

          c = kbd_get ();
          if (kbd_put (0))
            kbd_put (c);
          continue;
        }

      /* Raise signal if ISIG is set and the character matches VINTR
         or VQUIT. */

      if (kbd_cc_intr != 0 && c == kbd_cc_intr)
        {
          generate_signal (threads[1], SIGINT);
          continue;
        }
      else if (kbd_cc_quit != 0 && c == kbd_cc_quit)
        {
          generate_signal (threads[1], SIGQUIT);
          continue;
        }

      /* Put the character into the keyboard buffer. */

      kbd_put (c);
    }
}


void term_termio (void)
{
  kbd_kill_flag = TRUE;
  if (kbd_tid != 0)
    DosKillThread (kbd_tid);
}


#define CHAR_SIG        0x0100
#define CHAR_NO         0x0101
#define CHAR_TIME       0x0102


/* Read a character from the keyboard.

   If no character is available and WAITP is false, return CHAR_NO.
   If WAITP is true, wait until a character is available or a signal
   occurs.  Return CHAR_SIG if interrupted by a signal.  Return
   CHAR_TIME if timed out.  Otherwise, return the character. */

static unsigned read_keyboard (int waitp)
{
  ULONG rc, user;
  unsigned c;
  thread_data *td;
  
  td = get_thread ();
  for (;;)
    {
      if (sig_flag)
        return CHAR_SIG;
      reset_event_sem (kbd_sem_new);
      request_mutex (kbd_sem_access);
      if (kbd_ptr_out == kbd_ptr_in)
        c = CHAR_NO;
      else
        {
          c = kbd_buf[kbd_ptr_out];
          kbd_ptr_out = (kbd_ptr_out + 1) % sizeof (kbd_buf);
        }
      DosReleaseMutexSem (kbd_sem_access);
      if (c != CHAR_NO || !waitp)
        return c;
      rc = DosWaitMuxWaitSem (kbd_sem_mux, SEM_INDEFINITE_WAIT, &user);
      if (rc == ERROR_INTERRUPT)
        return CHAR_SIG;
      if (rc != 0)
        error (rc, "DosWaitMuxWaitSem");
      if (user == 1)
        return CHAR_TIME;
    }
}


/* Read a character from an asynchronous serial device.

   If no character is available and WAITP is false, return CHAR_NO.
   If WAITP is true, wait until a character is available or a signal
   occurs.  Return CHAR_SIG if interrupted by a signal.  Return
   CHAR_TIME if timed out.  Otherwise, return the character. */

static unsigned read_async (ULONG handle, int waitp)
{
  ULONG rc, nread;
  UCHAR c;
  thread_data *td;

  td = get_thread ();
  if (sig_flag)
    return CHAR_SIG;
  /* TODO: waitp */
  /* TODO: time out */
  rc = DosRead (handle, &c, 1, &nread);
  if (rc == ERROR_INTERRUPT)
    return CHAR_SIG;
  if (rc != 0)
    return CHAR_NO;
  return c;
}


ULONG async_avail (ULONG handle)
{
  ULONG rc, data_length;
  RXQUEUE data;

  data_length = sizeof (data);
  rc = DosDevIOCtl (handle, IOCTL_ASYNC, ASYNC_GETINQUECOUNT,
                    NULL, 0, NULL,
                    &data, data_length, &data_length);
  if (rc != 0)
    return 0;
  return data.cch;
}


ULONG async_writable (ULONG handle)
{
  ULONG rc, data_length;
  RXQUEUE data;

  data_length = sizeof (data);
  rc = DosDevIOCtl (handle, IOCTL_ASYNC, ASYNC_GETOUTQUECOUNT,
                    NULL, 0, NULL,
                    &data, data_length, &data_length);
  if (rc != 0 || data.cch > data.cb)
    return 0;
  return data.cb - data.cch;
}


ULONG termio_avail (ULONG handle)
{

  if (handle_flags[handle] & HF_CON)
    {
      if (GET_FILE (handle)->c_lflag & IDEFAULT)
        return 0;
      else
        return kbd_avail ();
    }
  else if (handle_flags[handle] & HF_ASYNC)
    return async_avail (handle);
  else
    return 0;
}


static void tr_out (my_file *d, UCHAR c)
{
  ULONG cbwritten;

  DosWrite (d->tio_output_handle, &c, 1, &cbwritten);
}


/* Get and preprocess a character for termio input.

   If no character is available and WAITP is false, return CHAR_NO.
   If WAITP is true, wait until a character is available or a signal
   occurs.  Return CHAR_SIG if interrupted by a signal.  Return
   CHAR_TIME if timed out.  Otherwise, return the character. */

static unsigned tr_get (my_file *d, int waitp)
{
  unsigned c;

  do
    {
      if (d->flags & HF_ASYNC)
        c = read_async (d->tio_input_handle, waitp);
      else
        c = read_keyboard (waitp);
      if (c & ~0xff)
        return c;
      if (d->c_iflag & ISTRIP)
        c &= 0x7f;
    } while ((d->c_iflag & IGNCR) && c == '\r');

  if ((d->c_iflag & INLCR) && c == '\n')
    c = '\r';
  else if ((d->c_iflag & ICRNL) && c == '\r')
    c = '\n';

  if ((d->c_iflag & IUCLC) && (c >= 'A' && c <= 'Z'))
    c += 'a' - 'A';

  return c;
}


static void tr_canon_echo (my_file *d, UCHAR c)
{
  if (d->c_lflag & ECHO)
    {
      if (c == '\n')
        {
          tr_out (d, '\r');
          tr_out (d, '\n');
        }
      else
        tr_out (d, c);
    }
}


static void tr_back (my_file *d)
{
  ULONG cbwritten;

  --d->tio_buf_count;
  DosWrite (d->tio_output_handle, "\b \b", 3, &cbwritten);
}


/* Note that this function must not flush the buffer on overflow.
   Code calling tr_canon_copy() depends on this assumption. */

static void tr_put (my_file *d, UCHAR c)
{
  if (d->tio_buf_count < d->tio_buf_size)
    d->tio_buf[d->tio_buf_count++] = c;
}


static void tr_canon_put (my_file *d, UCHAR c)
{
  d->tio_escape = FALSE;
  tr_put (d, c);
  tr_canon_echo (d, c);
}


static void tr_canon_esc_put (my_file *d, UCHAR c)
{
  tr_out (d, '\b');
  tr_canon_put (d, c);
}


static int tr_canon_copy (my_file *d, char *dst, unsigned count, int *errnop)
{
  unsigned n;

  n = (count < d->tio_buf_count ? count : d->tio_buf_count);
  memcpy (dst, d->tio_buf + d->tio_buf_idx, n);
  d->tio_buf_count -= n; d->tio_buf_idx += n;
  *errnop = 0;
  return n;
}


static int tr_canon_read (my_file *d, char *dst, unsigned count, int *errnop)
{
  unsigned c;

  /* If there's (a remainder of) a line in the buffer, return up to
     COUNT bytes from the buffer. */

  if (d->tio_buf_count != 0)
    return tr_canon_copy (d, dst, count, errnop);

  /* Read another line.  If the end of a line (or EOF) is read, call
     tr_canon_copy() to copy (part of) the line to DST. */

  d->tio_buf_idx = 0;
  d->tio_escape = FALSE;
  for (;;)
    {
      c = tr_get (d, TRUE);
      if (c == CHAR_SIG)
        {
          /* Interrupted by a signal. */

          termio_flush (d->tio_input_handle);
          *errnop = EINTR;
          return -1;
        }
      else if (c == 0)
        {
          /* We got the 0 character which precedes a scan code.
             Ignore both characters. */

          c = tr_get (d, TRUE);
          if (c == CHAR_SIG)
            {
              /* Interrupted by a signal. */

              kbd_flush (d);
              *errnop = EINTR;
              return -1;
            }
        }
      else if (c == '\n' || c == d->c_cc[VEOL])
        {
          /* End of line.  Return the buffered line, which contains at
             least the end-of-line character. */
          /* TODO: escape? */

          tr_put (d, c);
          tr_canon_echo (d, c);
          return tr_canon_copy (d, dst, count, errnop);
        }
      else if (c == d->c_cc[VEOF])
        {
          /* End of file.  Return the buffered line. */

          if (d->tio_escape)
            tr_canon_esc_put (d, c);
          else if (d->tio_buf_count == 0)
            {
              *errnop = 0;
              return 0;
            }
          else
            return tr_canon_copy (d, dst, count, errnop);
        }
      else if (c == d->c_cc[VERASE])
        {
          /* Erase character.  Delete one character. */

          if (d->tio_escape)
            tr_canon_esc_put (d, c);
          else if (d->tio_buf_count != 0)
            tr_back (d);
        }
      else if (c == d->c_cc[VKILL])
        {
          /* Kill character.  Delete all characters typed on this line. */

          if (d->tio_escape)
            tr_canon_esc_put (d, c);
          else
            {
              d->tio_buf_count = 0;
              if (d->c_lflag & ECHOK)
                {
                  tr_out (d, '\r');
                  tr_out (d, '\n');
                }
            }
        }
      else if (d->tio_escape)
        {
          d->tio_escape = FALSE;
          tr_put (d, '\\');
          tr_put (d, c);
          tr_canon_echo (d, c);
        }
      else if (c == '\\')
        {
          d->tio_escape = TRUE;
          tr_canon_echo (d, c);
        }
      else
        tr_canon_put (d, c);
    }
}


static int tr_raw_read (my_file *d, char *dst, unsigned count,
                        int *errnop)
{
  int result, timer_flag, vmin, vtime;
  unsigned c;
  ULONG rc;
  HTIMER timer;

  vmin = d->c_cc[VMIN];
  vtime = d->c_cc[VTIME];

  /* If O_NDELAY is set, rule out all cases where we have to wait. */

  if ((d->flags & HF_NDELAY) && (vmin != 0 || vtime != 0))
    {
      ULONG avail;

      avail = termio_avail (d->tio_input_handle);
      if (avail < count && avail < (vmin != 0 ? vmin : 1))
        {
          *errnop = EAGAIN;
          return -1;
        }
    }

  result = 0;
  timer_flag = FALSE;
  if (d->flags & HF_CON)
    reset_event_sem (kbd_sem_time);

  /* Start the timer if VMIN == 0 and VTIME > 0. */

  if (vmin == 0 && vtime != 0 && (d->flags & HF_CON))
    {
      rc = DosAsyncTimer ((1000 / 10) * vtime, (HSEM)kbd_sem_time, &timer);
      if (rc != 0) error (rc, "DosAsyncTimer");
      timer_flag = TRUE;
      vmin = 1;
    }

  /* Read the character until the buffer is empty, enough characters
     have been read, a signal occurs, or a time out occurs. */

  *errnop = 0;
  while (count != 0)
    {
      c = tr_get (d, result < vmin);
      if (c == CHAR_NO || c == CHAR_TIME)
        break;

      if (c == CHAR_SIG)
        {
          /* Interrupted by a signal.  Do not flush keyboard buffer.
             Rather copy the keys back to the keyboard buffer.  If
             there isn't enough room, flush the keyboard buffer. */

          if (result != 0 && !kbd_stuff (dst - result, result))
            kbd_flush (d);

          *errnop = EINTR; result = -1;
          break;
        }

      /* TODO: Handle 0, scan code pairs. */

      *dst++ = (char)c;
      ++result; --count;
      tr_canon_echo (d, c);

      /* Restart timer for VMIN > 0 and VTIME > 0. */

      if (count != 0 && d->c_cc[VMIN] != 0 && vtime != 0
          && (d->flags & HF_CON))
        {
          if (timer_flag)
            {
              DosStopTimer (timer);
              timer_flag = FALSE;
            }
          reset_event_sem (kbd_sem_time);
          rc = DosAsyncTimer ((1000 / 10) * vtime, (HSEM)kbd_sem_time, &timer);
          if (rc != 0) error (rc, "DosAsyncTimer");
          timer_flag = TRUE;
        }
    }
  if (timer_flag)
    {
      DosStopTimer (timer);
      /* Reset the semaphore, in case ICANON is activated. */
      reset_event_sem (kbd_sem_time);
    }
  return result;
}


/* Unix-like read() for the terminal.  DST is the destination address,
   COUNT is the number of bytes to read.  Return the number of bytes
   read or, on error, -1.  Store the error number to *ERRNOP.

   Currently this function is implemented for the keyboard only. */

int termio_read (ULONG handle, char *dst, unsigned count, int *errnop)
{
  int result;
  my_file *d;

  sig_block_start ();

  /* Update the file description's flag bits with the flag bits of the
     handle. */

  d = GET_FILE (handle);
  d->flags = handle_flags[handle];
  d->tio_input_handle = handle;
  d->tio_output_handle = (d->flags & HF_CON ? 1 : handle);

  if (count == 0)
    {
      *errnop = 0;
      result = 0;
    }
  else if (d->c_lflag & ICANON)
    result = tr_canon_read (d, dst, count, errnop);
  else
    result = tr_raw_read (d, dst, count, errnop);
  sig_block_end ();
  return result;
}


void termio_flush (ULONG handle)
{
  my_file *d;

  if (IS_VALID_FILE (handle))
    d = GET_FILE (handle);
  else
    d = NULL;
  if (handle_flags[handle] & HF_CON)
    kbd_flush (d);
  else
    {
      /* TODO: ASYNC */
    }
}


static void async_set (ULONG handle, ULONG new_cflag, ULONG new_iflag,
                       const UCHAR *new_cc)
{
  my_file *d;
  ULONG rc, parm_length;
  SETEXTBAUDRATE speed;
  LINECONTROL lctl;
  DCBINFO dcb;

  d = GET_FILE (handle);
  if (new_cflag != d->c_cflag)
    {
      lctl = d->x.async.lctl;
      if ((new_cflag & CBAUD) != (d->c_cflag & CBAUD))
        {
          speed.ulRate = d->x.async.speed.ulCurrentRate;
          switch (new_cflag & CBAUD)
            {
            case B0:
              /* TODO: Hang up */
              break;
            case B50:
              speed.ulRate = 50; break;
            case B75:
              speed.ulRate = 75; break;
            case B110:
              speed.ulRate = 110; break;
            case B134:
              speed.ulRate = 134; break;
            case B150:
              speed.ulRate = 150; break;
            case B200:
              speed.ulRate = 200; break;
            case B300:
              speed.ulRate = 300; break;
            case B600:
              speed.ulRate = 600; break;
            case B1200:
              speed.ulRate = 1200; break;
            case B1800:
              speed.ulRate = 1800; break;
            case B2400:
              speed.ulRate = 2400; break;
            case B4800:
              speed.ulRate = 4800; break;
            case B9600:
              speed.ulRate = 9600; break;
            case B19200:
              speed.ulRate = 19200; break;
            case B38400:
              speed.ulRate = 38400; break;
            }
          if (speed.ulRate != d->x.async.speed.ulCurrentRate)
            {
              /* Set the bit rate. */

              parm_length = sizeof (speed);
              rc = DosDevIOCtl (handle, IOCTL_ASYNC, ASYNC_SETEXTBAUDRATE,
                                &speed, parm_length, &parm_length,
                                NULL, 0, NULL);
              d->x.async.speed.ulCurrentRate = speed.ulRate;
            }
        }

      /* Character size */

      switch (new_cflag & CSIZE)
        {
        case CS5:
          lctl.bDataBits = 5; break;
        case CS6:
          lctl.bDataBits = 6; break;
        case CS7:
          lctl.bDataBits = 7; break;
        case CS8:
          lctl.bDataBits = 8; break;
        }

      /* Parity none/odd/even */

      if ((new_cflag & (PARENB|PARODD)) != (d->c_cflag & (PARENB|PARODD)))
        {
          if (!(new_cflag & PARENB))
            lctl.bParity = 0;   /* No parity */
          else if (new_cflag & PARODD)
            lctl.bParity = 1;   /* Odd parity */
          else
            lctl.bParity = 2;   /* Even parity */
        }

      /* Stop bits */

      if ((new_cflag & CSTOPB) != (d->c_cflag & CSTOPB))
        {
          if (new_cflag & CSTOPB)
            lctl.bStopBits = 2; /* 2 stop bits */
          else
            lctl.bStopBits = 0; /* 1 stop bit */
        }

      /* Stop bits vs. character size */

      if (lctl.bDataBits == 5 && lctl.bStopBits == 2)
        lctl.bStopBits = 1;
      else if (lctl.bDataBits != 5 && lctl.bStopBits == 1)
        lctl.bStopBits = 2;

      if (memcmp (&lctl, &d->x.async.lctl.bDataBits, sizeof (lctl)) != 0)
        {
          /* Set the line characteristics. */

          parm_length = sizeof (lctl);
          rc = DosDevIOCtl (handle, IOCTL_ASYNC, ASYNC_SETLINECTRL,
                            &lctl, parm_length, &parm_length,
                            NULL, 0, NULL);
          d->x.async.lctl = lctl;
        }
    }

  /* Note: There are no VSTOP and VSTART entries in `struct termio'! */

  if (new_iflag != d->c_iflag
      || new_cc[VMIN] != d->c_cc[VMIN]
      || new_cc[VTIME] != d->c_cc[VTIME])
    {
      dcb = d->x.async.dcb;
      if ((new_iflag & IXON) != (d->c_iflag & IXON))
        {
          if (new_iflag & IXON)
            dcb.fbFlowReplace |= 1;
          else
            dcb.fbFlowReplace &= ~1;
        }
      if ((new_iflag & IXOFF) != (d->c_iflag & IXOFF))
        {
          if (new_iflag & IXOFF)
            dcb.fbFlowReplace |= 2;
          else
            dcb.fbFlowReplace &= ~2;
        }
      if (new_cc[VMIN] != d->c_cc[VMIN]
          || new_cc[VTIME] != d->c_cc[VTIME])
        {
          if (new_cc[VMIN] == 0)
            {
              if (new_cc[VTIME] == 0)
                dcb.fbTimeout = (dcb.fbTimeout & ~6) | 6;
              else
                {
                  dcb.fbTimeout = (dcb.fbTimeout & ~6) | 4;
                  dcb.usReadTimeout = new_cc[VTIME] * 10 - 1;
                }
            }
          else
            {
              dcb.fbTimeout = (dcb.fbTimeout & ~6) | 2;
              dcb.usReadTimeout = new_cc[VTIME] * 10 - 1;
            }
        }
      if (memcmp (&dcb, &d->x.async.dcb, sizeof (dcb)) != 0)
        {
          /* Set the DCB */

          parm_length = sizeof (dcb);
          rc = DosDevIOCtl (handle, IOCTL_ASYNC, ASYNC_SETDCBINFO,
                            &dcb, parm_length, &parm_length,
                            NULL, 0, NULL);
          d->x.async.dcb = dcb;
        }
    }
}


static void termio_set_common (ULONG handle, my_file *d)
{
  if (handle_flags[handle] & HF_CON)
    {
      kbd_idelete = (d->c_iflag & IDELETE) != 0;
      if (d->c_lflag & ISIG)
        {
          kbd_cc_intr = d->c_cc[VINTR];
          kbd_cc_quit = d->c_cc[VQUIT];
        }
      else
        kbd_cc_intr = kbd_cc_quit = 0;

      if (d->c_lflag & IDEFAULT)
        {
          if (kbd_started)
            kbd_stop ();
        }
      else if (kbd_started)
        kbd_restart ();
      else
        start_keyboard ();
    }
}


void termio_set (ULONG handle, const struct termio *tio)
{
  my_file *d;

  d = GET_FILE (handle);
  if (!(tio->c_lflag & IDEFAULT) && d->tio_buf == NULL)
    {
      d->tio_buf_size = 1024;
      d->tio_buf = checked_private_alloc (d->tio_buf_size);
    }

  if (handle_flags[handle] & HF_ASYNC)
    async_set (handle, tio->c_cflag, tio->c_iflag, tio->c_cc);

  d->c_iflag = tio->c_iflag;
  d->c_oflag = tio->c_oflag;
  d->c_cflag = tio->c_cflag;
  d->c_lflag = tio->c_lflag;
  memcpy (d->c_cc, tio->c_cc, NCC);
  d->c_cc[VSUSP]  = 0;          /* Disabled */
  d->c_cc[VSTOP]  = 0x13;       /* Ctrl-S */
  d->c_cc[VSTART] = 0x11;       /* Ctrl-Q */

  termio_set_common (handle, d);
}


void termio_get (ULONG handle, struct termio *tio)
{
  const my_file *d;

  d = GET_FILE (handle);
  tio->c_iflag = d->c_iflag;
  tio->c_oflag = d->c_oflag;
  tio->c_cflag = d->c_cflag;
  tio->c_lflag = d->c_lflag;
  tio->c_line  = 0;
  memcpy (tio->c_cc, d->c_cc, NCC);
}


void termios_get (ULONG handle, struct termios *tio)
{
  const my_file *d;

  d = GET_FILE (handle);
  tio->c_iflag = d->c_iflag;
  tio->c_oflag = d->c_oflag;
  tio->c_cflag = d->c_cflag;
  tio->c_lflag = d->c_lflag & ~IDEFAULT;
  memcpy (tio->c_cc, d->c_cc, NCCS);
}


void termios_set (ULONG handle, const struct termios *tio)
{
  my_file *d;

  d = GET_FILE (handle);
  if (d->tio_buf == NULL)
    {
      d->tio_buf_size = 1024;
      d->tio_buf = checked_private_alloc (d->tio_buf_size);
    }

  if (handle_flags[handle] & HF_ASYNC)
    async_set (handle, tio->c_cflag, tio->c_iflag, tio->c_cc);

  d->c_iflag = tio->c_iflag;
  d->c_oflag = tio->c_oflag;
  d->c_cflag = tio->c_cflag;
  d->c_lflag = tio->c_lflag & ~IDEFAULT;
  memcpy (d->c_cc, tio->c_cc, NCCS);

  termio_set_common (handle, d);
}


ULONG query_async (ULONG handle)
{
  ULONG rc;
  ULONG data_length;
  my_file *d;

  d = GET_FILE (handle);

  /* Retrieve the bit rate. */

  memset (&d->x.async.speed, 0, sizeof (d->x.async.speed));
  data_length = sizeof (d->x.async.speed);
  rc = DosDevIOCtl (handle, IOCTL_ASYNC, ASYNC_GETEXTBAUDRATE,
                    NULL, 0, NULL,
                    &d->x.async.speed, data_length, &data_length);
  if (rc != 0) return rc;

  /* Retrieve the line characteristics. */

  memset (&d->x.async.lctl, 0, sizeof (d->x.async.lctl));
  data_length = sizeof (d->x.async.lctl);
  rc = DosDevIOCtl (handle, IOCTL_ASYNC, ASYNC_GETLINECTRL,
                    NULL, 0, NULL,
                    &d->x.async.lctl, data_length, &data_length);
  if (rc != 0) return rc;

  /* Retrieve the DCB parameters. */

  memset (&d->x.async.dcb, 0, sizeof (d->x.async.dcb));
  data_length = sizeof (d->x.async.dcb);
  rc = DosDevIOCtl (handle, IOCTL_ASYNC, ASYNC_GETDCBINFO,
                    NULL, 0, NULL,
                    &d->x.async.dcb, data_length, &data_length);
  return rc;
}


/* Translate values for termio. */

ULONG translate_async (ULONG handle)
{
  ULONG cflag;
  my_file *d;

  d = GET_FILE (handle);
  cflag = 0;
  switch (d->x.async.speed.ulCurrentRate)
    {
    case 50: cflag |= B50; break;
    case 75: cflag |= B75; break;
    case 110: cflag |= B110; break;
    case 134: cflag |= B134; break;
    case 150: cflag |= B150; break;
    case 200: cflag |= B200; break;
    case 300: cflag |= B300; break;
    case 600: cflag |= B600; break;
    case 1200: cflag |= B1200; break;
    case 1800: cflag |= B1800; break;
    case 2400: cflag |= B2400; break;
    case 4800: cflag |= B4800; break;
    case 9600: cflag |= B9600; break;
    case 19200: cflag |= B19200; break;
    case 38400: cflag |= B38400; break;
    default: cflag |= B1200; break;
    }

  switch (d->x.async.lctl.bDataBits)
    {
    case 5: cflag |= CS5; break;
    case 6: cflag |= CS6; break;
    case 7: cflag |= CS7; break;
    case 8: cflag |= CS8; break;
    default: return ERROR_GEN_FAILURE;
    }
  switch (d->x.async.lctl.bParity)
    {
    case 1: cflag |= PARENB|PARODD; break; /* Odd parity */
    case 2: cflag |= PARENB;        break; /* Even parity */
    case 3: break;                         /* Mark parity */
    case 4: break;                         /* Space parity */
    default: break;
    }
  switch (d->x.async.lctl.bStopBits)
    {
    case 0: break;              /* 1 stop bit */
    case 1: cflag |= CSTOPB;    /* 1.5 stop bits (CS5 only) */
    case 2: cflag |= CSTOPB;    /* 2 stop bits (all but CS5) */
    default: break;
    }
  cflag |= CREAD;               /* Enable receiver */
  cflag |= HUPCL;               /* Hang up on last close */

  /* Note: IGNPAR, INPCK, and PARMRK are not supported; parity
     checking is always enabled and characters with wrong parity are
     always read. */

  /* TODO: CLOCAL, HUPCL? */

  init_termio (handle);
  d = GET_FILE (handle);
  d->c_cflag = cflag;
  d->c_lflag = IDEFAULT;
  d->c_cc[VSTOP] = d->x.async.dcb.bXOFFChar;
  d->c_cc[VSTART] = d->x.async.dcb.bXONChar;

  /* Automatic transmit flow control (XON/XOFF) */

  if (d->x.async.dcb.fbFlowReplace & 1)
    d->c_iflag |= IXON;
  else
    d->c_iflag &= ~IXON;

  /* Automatic receive flow control (XON/XOFF) */

  if (d->x.async.dcb.fbFlowReplace & 2)
    d->c_iflag |= IXOFF;
  else
    d->c_iflag &= ~IXOFF;

  /* Time out processing.  Note that dcb.usReadTimeout contains the
     number of hundredths of a second, minus 1.  That is, 0 means 0.01
     seconds. */

  switch (d->x.async.dcb.fbTimeout & 6)
    {
    case 2:                     /* Normal */
      d->c_cc[VMIN] = 255;      /* Approximation to infinity */
      d->c_cc[VTIME] = (d->x.async.dcb.usReadTimeout > 2550 - 1
                        ? 255 : (d->x.async.dcb.usReadTimeout + 1 + 9) / 10);
      break;

    case 4:                     /* Wait for something */
      d->c_cc[VMIN] = 0;
      d->c_cc[VTIME] = (d->x.async.dcb.usReadTimeout > 2550 - 1
                        ? 255 : (d->x.async.dcb.usReadTimeout + 1 + 9) / 10);
      break;

    case 6:                     /* No-Wait */
    default:                    /* Should not happen */
      d->c_cc[VMIN] = 0;
      d->c_cc[VTIME] = 0;
      break;
    }
  return 0;
}
