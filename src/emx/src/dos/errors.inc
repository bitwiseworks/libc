;
; ERRORS.INC -- Translate error codes
;
; Copyright (c) 1991-1995 by Eberhard Mattes
;
; This file is part of emx.
;
; emx is free software; you can redistribute it and/or modify it
; under the terms of the GNU General Public License as published by
; the Free Software Foundation; either version 2, or (at your option)
; any later version.
;
; emx is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with emx; see the file COPYING.  If not, write to
; the Free Software Foundation, 59 Temple Place - Suite 330,
; Boston, MA 02111-1307, USA.
;
; See emx.asm for a special exception.
;

;
; errno values
;
EPERM           =       1               ; Operation not permitted
ENOENT          =       2               ; No such file or directory
ESRCH           =       3               ; No such process
EINTR           =       4               ; Interrupted system call
EIO             =       5               ; I/O error
ENXIO           =       6               ; No such device or address
E2BIG           =       7               ; Arguments or environment too big
ENOEXEC         =       8               ; Invalid executable format
EBADF           =       9               ; Bad file number
ECHILD          =       10              ; No child processes
EAGAIN          =       11              ; No more processes
ENOMEM          =       12              ; Not enough memory
EACCES          =       13              ; Permission denied
EFAULT          =       14              ; Bad address
ENOLCK          =       15              ; No locks available
EBUSY           =       16              ; Resource busy
EEXIST          =       17              ; File exists
EXDEV           =       18              ; Cross-device link
ENODEV          =       19              ; No such device
ENOTDIR         =       20              ; Not a directory
EISDIR          =       21              ; Is a directory
EINVAL          =       22              ; Invalid argument
ENFILE          =       23              ; Too many open files in system
EMFILE          =       24              ; Too many open files
ENOTTY          =       25              ; Inappropriate ioctl
EDEADLK         =       26              ; Resource deadlock avoided
EFBIG           =       27              ; File too large
ENOSPC          =       28              ; Disk full
ESPIPE          =       29              ; Invalid seek
EROFS           =       30              ; Read-only file system
EMLINK          =       31              ; Too many links
EPIPE           =       32              ; Broken pipe
EDOM            =       33              ; Domain error
ERANGE          =       34              ; Result too large
ENOTEMPTY       =       35              ; Directory not empty
; EINPROGRESS   =       36              ;
ENOSYS          =       37              ; Function not implemented
ENAMETOOLONG    =       38              ; File name too long


              IFNDEF __ERRORS

SV_CODE         SEGMENT

                EXTRN   DOS_ERROR_TO_ERRNO:NEAR ; Translate error code to errno
                EXTRN   XLATE_ERRNO:NEAR        ; Translate errno value

SV_CODE         ENDS

              ENDIF
