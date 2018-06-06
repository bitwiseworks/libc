/* ulserrno.h,v 1.3 2004/09/14 22:27:36 bird Exp */
/** @file
 * IGCC.
 */
/*
 * Legalesy-free Unicode API interface for OS/2
 * Unicode API error codes
 *
 * Written by Andrew Zabolotny <bit@eltech.ru>
 *
 * This file is put into public domain. You are free to do
 * literally anything you wish with it: modify, print, sell,
 * rent, eat, throw out of window: in all (esp. in later)
 * cases I am not responsible for any damage it causes.
 */

#ifndef __ULSERRNO_H__
#define __ULSERRNO_H__

#define ULS_API_ERROR_BASE	0x00020400

typedef enum _uls_return_codes
{
  ULS_SUCCESS = 0,
  ULS_OTHER = ULS_API_ERROR_BASE + 1,
  ULS_ILLEGALSEQUENCE,
  ULS_MAXFILESPERPROC,
  ULS_MAXFILES,
  ULS_NOOP,
  ULS_TOOMANYKBD,
  ULS_KBDNOTFOUND,
  ULS_BADHANDLE,
  ULS_NODEAD,
  ULS_NOSCAN,
  ULS_INVALIDSCAN,
  ULS_NOTIMPLEMENTED,
  ULS_NOMEMORY,
  ULS_INVALID,
  ULS_BADOBJECT,
  ULS_NOTOKEN,
  ULS_NOMATCH,
  ULS_BUFFERFULL,
  ULS_RANGE,
  ULS_UNSUPPORTED,
  ULS_BADATTR,
  ULS_VERSION
} uls_error_t;


#endif /* __ULSERRNO_H__ */
