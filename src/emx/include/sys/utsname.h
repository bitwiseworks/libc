/* sys/utsname.h (emx+gcc) */

#ifndef _SYS_UTSNAME_H
#define _SYS_UTSNAME_H

#if defined (__cplusplus)
extern "C" {
#endif

#define _UN_LEN         16

struct utsname
{
  char sysname[_UN_LEN];
  char nodename[_UN_LEN];
  char release[_UN_LEN];
  char version[_UN_LEN];
  char machine[_UN_LEN];
};

int uname (struct utsname *);

#if defined (__cplusplus)
}
#endif

#endif /* not SYS_UTSNAME_H */
