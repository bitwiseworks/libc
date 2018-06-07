/* files.h -- Header file for modules involved in file handle management
   Copyright (c) 1994-1995 by Eberhard Mattes

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


/* Requires INCL_DOSDEVIOCTL! */

#define ASYNC_SETEXTBAUDRATE		0x0043
#define ASYNC_GETEXTBAUDRATE		0x0063

typedef struct
{
  ULONG ulCurrentRate;
  BYTE  bCurrentFract;
  ULONG ulMinimumRate;
  BYTE  bMinimumFract;
  ULONG ulMaximumRate;
  BYTE  bMaximumFract;
} GETEXTBAUDRATE;
typedef GETEXTBAUDRATE *PGETEXTBAUDRATE;

typedef struct
{
  ULONG ulRate;
  BYTE  bFract;
} SETEXTBAUDRATE;
typedef SETEXTBAUDRATE *PSETEXTBAUDRATE;


#define SET_INVALID_FILE(handle) (handle_file[handle] = handle_count)
#define IS_VALID_FILE(handle) (handle_file[handle] < handle_count)
#define GET_FILE(handle) (&files[handle_file[handle]])


typedef struct
{
  ULONG flags;
  ULONG ref_count;

  /* termio interface */

  ULONG c_iflag;
  ULONG c_oflag;
  ULONG c_cflag;
  ULONG c_lflag;
  UCHAR c_cc[11];                /* NCC == 8, NCCS == 11 */

  /* termio implementation */

  char *tio_buf;
  unsigned tio_buf_count;
  unsigned tio_buf_size;
  unsigned tio_buf_idx;
  char tio_escape;
  ULONG tio_input_handle;
  ULONG tio_output_handle;

  /* Data which depends on the file type. */

  union
    {
      struct
        {
          GETEXTBAUDRATE speed;
          LINECONTROL lctl;
          DCBINFO dcb;
        } async;
      struct
        {
          int handle;
          int fd_flags;         /* FD_CLOEXEC */
        } socket;
      struct
        {
          ULONG id;             /* ID returned by XF86SUP_DRVID */
          HEV sem;              /* Semaphore handle */
          BYTE sem_open;        /* Semaphore handle valid */
        } xf86sup;              /* xf86sup device */
    } x;
} my_file;


extern ULONG handle_count;
extern ULONG *handle_flags;
extern ULONG *handle_file;
extern my_file *files;
extern long ino_number;
extern HMTX files_access;


/* Duplicate S_IREAD, S_IWRITE, and S_IEXEC bits into all three groups
   of the permission mask. */

#define MAKEPERM(x) (((x) >> 6) * 0111)

#define IS_SOCKET(h) ((h) < handle_count && (handle_flags[h] & HF_SOCKET))


/* `handle_flags' array is protected by a Mutex semaphore.  These
   macros are used for requesting and releasing that semaphore. */

#define LOCK_FILES   request_mutex (files_access)
#define UNLOCK_FILES DosReleaseMutexSem (files_access)

void alloc_file_description (ULONG handle);
void close_handle (ULONG handle);
void set_handle_flags (ULONG handle, ULONG flags);
int find_unused_handles (ULONG *dst, int count);
