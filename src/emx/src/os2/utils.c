/* utils.c -- Miscellaneous utility functions
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


#include <stdarg.h>
#define INCL_DOSSEMAPHORES
#define INCL_DOSMODULEMGR
#define INCL_DOSMISC
#define INCL_DOSNLS
#define INCL_DOSERRORS
#include <os2emx.h>
#include <sys/nls.h>
#include "emxdll.h"
#include "clib.h"

#define NULL ((void *)0)

#define MAX_EVENT_SEM   16
#define MAX_MUTEX_SEM   6
#define MAX_MUXWAIT_SEM 2

/* Mutex semaphore for protecting data in the COMMON32 segment. */

static char const common_mutex_name[] = "/sem32/emx/common.mtx";
HMTX common_mutex;
static BYTE common_open;

/* Table of event semaphores. */

static ULONG event_sem_count;
static HEV event_sem_table[MAX_EVENT_SEM];

/* Table of Mutex semaphores. */

static ULONG mutex_sem_count;
static HMTX mutex_sem_table[MAX_MUTEX_SEM];

/* Table of MuxWait semaphores. */

static ULONG muxwait_sem_count;
static HMUX muxwait_sem_table[MAX_MUXWAIT_SEM];

/* Table identifying DBCS lead bytes.  Note that our local copy of
   c.lib no longer contains _nls_ctype_tab to avoid linking in
   _dbcs_init().  This could also have been achieved by initializing
   this variable; however, initializing any variable would add
   non-shared pages to emx.dll, slowing down loading when it's already
   loaded. */

unsigned char _nls_ctype_tab[256];


/* Convert a hexadecimal number consisting of 8 digits, for
   init_fork() and init2_signal().  Return FALSE on failure.  The
   converted number is stored to *DST.  Only lower-case letters are
   supported. */

int conv_hex8 (const char *s, ULONG *dst)
{
  int i;
  ULONG n;
  unsigned char c;

  n = 0;
  for (i = 0; i < 8; ++i)
    {
      c = (unsigned char)*s++;
      if (c >= '0' && c <= '9')
        c -= '0';
      else if (c >= 'a' && c <= 'f')
        c -= 'a' - 10;
      else
        return FALSE;
      n = (n << 4) | c;
    }
  *dst = n;
  return TRUE;
}


/* Convert the unsigned number X to a hexadecimal string of DIGITS
   digits at DST. */

static void hexn (char *dst, ULONG x, int digits)
{
  int i;

  for (i = digits - 1; i >= 0; --i)
    {
      dst[i] = "0123456789abcdef"[x & 0x0f];
      x >>= 4;
    }
}


/* Convert the unsigned number X to a decimal string at DST.  Return the
   number of characters stored to DST. */

static int udecn (char *dst, unsigned x)
{
  int i, j;
  char digits[12];

  i = 0;
  do
    {
      digits[i++] = (char)(x % 10 + '0');
      x /= 10;
    } while (x != 0);
  for (j = 1; j <= i; ++j)
    *dst++ = digits[i-j];
  return i;
}


/* Convert the signed number X to a decimal string at DST.  Return the
   number of characters stored to DST. */

static int decn (char *dst, int x)
{
  if (x >= 0)
    return udecn (dst, (unsigned)x);
  else
    {
      *dst = '-';
      return udecn (dst+1, (unsigned)(-x)) + 1;
    }
}


/* Display a string on the error output. */

void otext (const char *msg)
{
  ULONG written;

  DosWrite (errout_handle, msg, strlen (msg), &written);
}


/* Handle an unexpected OS/2 error code by beeping and displaying a
   message which shows the function name (a string pointed to by MSG)
   and the return code (RC). */

void error (ULONG rc, const char *msg)
{
  oprintf ("%s: 0x%.8x ", msg, (unsigned)rc);
  DosBeep (698, 500);
}


/* Create an unnamed Mutex semaphore and store the handle to *PSEM.
   Return the OS/2 error code.

   Note: This function is not reentrant! */

ULONG create_mutex_sem (HMTX *psem)
{
  ULONG rc;

  /* This should not happen unless someone adds create_mutex_sem()
     calls without updating MAX_MUTEX_SEM.  Note that do_lock_common()
     also increments mutex_sem_count. */

  if (mutex_sem_count >= MAX_MUTEX_SEM)
    {
      oprintf ("Too many Mutex semaphores\r\n");
      return ERROR_TOO_MANY_SEMAPHORES;
    }

  rc = DosCreateMutexSem (NULL, psem, 0, 0);
  if (rc == 0)
    mutex_sem_table[mutex_sem_count++] = *psem;
  else
    error (rc, "DosCreateMutexSem");
  return rc;
}


/* Request the Mutex semaphore SEM. */

void request_mutex (HMTX sem)
{
  ULONG rc;

  do
    {
      rc = DosRequestMutexSem (sem, SEM_INDEFINITE_WAIT);
    } while (rc == ERROR_SEM_OWNER_DIED || rc == ERROR_INTERRUPT);
}


/* Create an unnamed event semaphore and store the semaphore handle to
   *PSEM.  Attribute bits (such as DC_SEM_SHARED) are given in
   ATTR.  Return the OS/2 error code.

   Note: This function is not reentrant! */

ULONG create_event_sem (HEV *psem, ULONG attr)
{
  ULONG rc;

  /* This should not happen unless someone adds create_event_sem()
     calls without updating MAX_EVENT_SEM. */

  if (event_sem_count >= MAX_EVENT_SEM)
    {
      oprintf ("Too many event semaphores\r\n");
      return ERROR_TOO_MANY_SEMAPHORES;
    }

  rc = DosCreateEventSem (NULL, psem, attr, FALSE);
  if (rc == 0)
    event_sem_table[event_sem_count++] = *psem;
  else
    error (rc, "DosCreateEventSem");
  return rc;
}


/* Close an event semaphore.

   Note: This function is not reentrant! */

void close_event_sem (HEV sem)
{
  ULONG i;

  for (i = 0; i < event_sem_count; ++i)
    if (event_sem_table[i] == sem)
      {
        event_sem_table[i] = event_sem_table[--event_sem_count];
        break;
      }
  DosCloseEventSem (sem);
}


/* Create an unnamed MuxWait semaphore and store the semaphore handle
   to *PSEM.  Return the OS/2 error code.

   Note: This function is not reentrant! */

ULONG create_muxwait_sem (HMUX *psem, ULONG count, SEMRECORD *array,
                          ULONG attr)
{
  ULONG rc;

  /* This should not happen unless someone adds create_muxwait_sem()
     calls without updating MAX_MUXWAIT_SEM. */

  if (muxwait_sem_count >= MAX_MUXWAIT_SEM)
    {
      oprintf ("Too many MuxWait semaphores\r\n");
      return ERROR_TOO_MANY_SEMAPHORES;
    }

  rc = DosCreateMuxWaitSem (NULL, psem, count, array, attr);
  if (rc == 0)
    muxwait_sem_table[muxwait_sem_count++] = *psem;
  else
    error (rc, "DosCreateMuxWaitSem");
  return rc;
}


/* Close an MuxWait semaphore.

   Note: This function is not reentrant! */

void close_muxwait_sem (HMUX sem)
{
  ULONG i;

  for (i = 0; i < muxwait_sem_count; ++i)
    if (muxwait_sem_table[i] == sem)
      {
        muxwait_sem_table[i] = muxwait_sem_table[--muxwait_sem_count];
        break;
      }
  DosCloseMuxWaitSem (sem);
}


/* Close all semaphores.  This function is called on termination of
   emx.dll. */

void term_semaphores (void)
{
  while (muxwait_sem_count != 0)
    DosCloseMuxWaitSem (muxwait_sem_table[--muxwait_sem_count]);
  while (event_sem_count != 0)
    DosCloseEventSem (event_sem_table[--event_sem_count]);
  while (mutex_sem_count != 0)
    DosCloseMutexSem (mutex_sem_table[--mutex_sem_count]);
}


/* Reset the event semaphore SEM.  It's no error if the semaphore is
   already reset.  Return the OS/2 error code. */

ULONG reset_event_sem (HEV sem)
{
  ULONG rc, post_count;

  rc = DosResetEventSem (sem, &post_count);
  if (rc == ERROR_ALREADY_RESET)
    rc = 0;
  if (rc != 0)
    error (rc, "DosResetEventSem");
  return rc;
}


/* Lock the common data area.  This creates or opens the semaphore if
   not already done. */

void do_lock_common (void)
{
  ULONG rc;

  if (!common_open)
    {
      common_open = TRUE;
      rc = DosCreateMutexSem (common_mutex_name, &common_mutex,
                              DC_SEM_SHARED, 0);
      if (rc == ERROR_DUPLICATE_NAME)
        rc = DosOpenMutexSem (common_mutex_name, &common_mutex);
      if (rc == 0 && mutex_sem_count < MAX_MUTEX_SEM)
        mutex_sem_table[mutex_sem_count++] = common_mutex;
        
    }
  request_mutex (common_mutex);
}


/* Terminate the process, returning RC. */

void quit (ULONG rc)
{
  kbd_stop ();
  DosExit (EXIT_PROCESS, rc);
}


/* Return the value of a static system variable. */

ULONG querysysinfo (ULONG index)
{
  ULONG value;

  DosQuerySysInfo (index, index, &value, sizeof (value));
  return value;
}


/* This function implements a subset of ANSI vsprintf().  Supported
   format specifications are %s, %d, %u and %x, with optional
   precision.  The default precision for %x is currently 8 (no caller
   depends on this). */

int vsprintf (char *dst, const char *fmt, va_list arg_ptr)
{
  char *d;
  int prec, i;
  unsigned u;
  const char *s;

  d = dst;
  while (*fmt != 0)
    if (*fmt != '%')
      *d++ = *fmt++;
    else if (fmt[1] == '%')
      *d++ = '%', fmt += 2;
    else
      {
        ++fmt; prec = -1;
        if (*fmt == '.')
          {
            ++fmt; prec = 0;
            while (*fmt >= '0' && *fmt <= '9')
              prec = prec * 10 + (*fmt++ - '0');
          }
        switch (*fmt)
          {
          case 's':
            s = va_arg (arg_ptr, char *);
            if (prec == -1) prec = 1 << 30;
            while (*s != 0 && prec > 0)
              *d++ = *s++, --prec;
            break;
          case 'd':
            i = va_arg (arg_ptr, int);
            d += decn (d, i);
            break;
          case 'u':
            u = va_arg (arg_ptr, unsigned);
            d += udecn (d, u);
            break;
          case 'x':
            u = va_arg (arg_ptr, unsigned);
            if (prec == -1) prec = 8;
            hexn (d, u, prec);
            d += prec;
            break;
          }
        ++fmt;
      }
  *d = 0;
  return d - dst;
}


/* Simple version of sprintf().  See vsprintf() for supported format
   specifications. */

int sprintf (char *dst, const char *fmt, ...)
{
  int result;
  va_list arg_ptr;

  va_start (arg_ptr, fmt);
  result = vsprintf (dst, fmt, arg_ptr);
  va_end (arg_ptr);
  return result;
}


/* Simple version of printf() with output to error output.  See
   vsprintf() for supported format specifications. */

void oprintf (const char *fmt, ...)
{
  va_list arg_ptr;
  char buf[1024];
  int len;
  ULONG written;

  va_start (arg_ptr, fmt);
  len = vsprintf (buf, fmt, arg_ptr);
  DosWrite (errout_handle, buf, len, &written);
  va_end (arg_ptr);
}


/* Mark in _nls_ctype_tab all DBCS lead bytes with _NLS_DBCS_LEAD.
   Note that this must be compatible to the C library's
   _nls_is_dbcs_lead macro.  Return 0 or an OS/2 error code. */

ULONG get_dbcs_lead (void)
{
  ULONG rc;
  COUNTRYCODE cc;
  BYTE lead[12];
  unsigned i, j;

  memset (_nls_ctype_tab, 0, 256);
  cc.country = 0;
  cc.codepage = 0;
  rc = DosQueryDBCSEnv (sizeof (lead), &cc, (PCHAR)lead);
  for (i = 0; i <= sizeof (lead) - 2; i += 2)
    {
      if (lead[i+0] == 0 && lead[i+1] == 0)
        break;
      for (j = lead[i+0]; j <= lead[i+1]; ++j)
        _nls_ctype_tab[j] = _NLS_DBCS_LEAD;
    }
  return 0;
}


/* This version of stricmp() does not support locales. */

int stricmp (const char *string1, const char *string2)
{
  int d;

  for (;;)
    {
      d = tolower ((unsigned char)*string1)
        - tolower ((unsigned char)*string2);
      if (d != 0 || *string1 == 0 || *string2 == 0)
        return d;
      ++string1; ++string2;
    }
}


/* Load a module.  This is a wrapper for DosLoadModule, saving the
   coprocessor state.  The initialization code of SO32DLL.DLL and
   TCP32DLL.DLL set the coprocessor control word to 0x362! */

ULONG load_module (const char *mod_name, HMODULE *phmod)
{
  char state[108];
  char obj[9];
  ULONG rc;

  __asm__ ("fnsave %0" : "=m"(state));
  rc = DosLoadModule (obj, sizeof (obj), mod_name, phmod);
  __asm__ ("frstor %0" : "=m"(state));
  return rc;
}


/* Return TRUE iff the region of memory starting at START is
   readable. */

int verify_memory (ULONG start, ULONG size)
{
  ULONG rc, cb, flag;

  while (size != 0)
    {
      cb = size;
      rc = DosQueryMem ((PVOID)start, &cb, &flag);
      if (rc != 0)
        return FALSE;
      if (!(flag & (PAG_COMMIT|PAG_SHARED)))
        return FALSE;
      if (flag & PAG_FREE)
        return FALSE;           /* Should not happen */
      if (!(flag & PAG_READ))
        return FALSE;
      if (cb == 0 || cb > size)
        return FALSE;           /* Should not happen */
      start += cb; size -= cb;
    }
  return TRUE;
}
