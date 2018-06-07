/* emxloado.c (emx+gcc) -- Copyright (c) 1993-1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdlib.h>
#include <sys/emxload.h>
#include <emx/emxload.h>

int _emxload_stop (int wait_flag)
{
  int req_code;

  req_code = (wait_flag ? _EMXLOAD_STOP_WAIT : _EMXLOAD_STOP);
  return _emxload_request (req_code, NULL, 0);
}
