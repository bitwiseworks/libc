/* sys/emxload.h (emx+gcc) */

#ifndef _SYS_EMXLOAD_H
#define _SYS_EMXLOAD_H

#if defined (__cplusplus)
extern "C" {
#endif

#if !defined (_SIZE_T)
#define _SIZE_T
typedef unsigned long size_t;
#endif

#define _EMXLOAD_INDEFINITE     (-1)

int _emxload_env (const char *envname);
int _emxload_this (int seconds);
int _emxload_prog (const char *name, int seconds);
int _emxload_unload (const char *name, int wait_flag);

int _emxload_list_start (void);
int _emxload_list_get (char *buf, size_t buf_size, int *pseconds);

int _emxload_connect (void);
int _emxload_disconnect (void);
int _emxload_stop (int wait_flag);

#if defined (__cplusplus)
}
#endif

#endif /* not _SYS_EMXLOAD_H */
