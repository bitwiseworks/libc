/* emx/emxload.h (emx+gcc) */

#ifndef _EMX_EMXLOAD_H
#define _EMX_EMXLOAD_H

#if defined (__cplusplus)
extern "C" {
#endif

#define _EMXLOAD_LOAD           1
#define _EMXLOAD_UNLOAD         2
#define _EMXLOAD_STOP           3
#define _EMXLOAD_CONNECT        4
#define _EMXLOAD_DISCONNECT     5
#define _EMXLOAD_UNLOAD_WAIT    6
#define _EMXLOAD_STOP_WAIT      7
#define _EMXLOAD_LIST           8

#define _EMXLOAD_PIPENAME       "/pipe/emx/emxload"

typedef struct
{
  int req_code;
  int seconds;
  char name[260];
} request;

typedef struct
{
  int ans_code;
  int seconds;
  char name[260];
} answer;

int _emxload_request (int req_code, const char *name, int seconds);
int _emxload_do_connect (int start);
int _emxload_do_disconnect (int force);
int _emxload_do_request (int req_code, const char *name, int seconds);
int _emxload_do_receive (answer *ans);
int _emxload_do_wait (void);

#if defined (__cplusplus)
}
#endif

#endif /* not _EMX_EMXLOAD_H */
