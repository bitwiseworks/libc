/* emxloadc.c (emx+gcc) -- Copyright (c) 1993-1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdlib.h>
#include <sys/emxload.h>
#include <emx/emxload.h>

int _emxload_connect (void)
{
  return _emxload_request (_EMXLOAD_CONNECT, NULL, 0);
}
