/* emxloadl.c (emx+gcc) -- Copyright (c) 1993-1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdlib.h>
#include <string.h>
#include <os2emx.h>             /* Define FALSE */
#include <emx/emxload.h>
#include <sys/emxload.h>

int _emxload_list_start (void)
{
  if (_emxload_do_connect (FALSE) != 0)
    return -1;
  if (_emxload_do_request (_EMXLOAD_LIST, "", 0) != 0)
    {
      _emxload_do_disconnect (FALSE);
      return -1;
    }
  return 0;
}


int _emxload_list_get (char *buf, size_t buf_size, int *pseconds)
{
  answer ans;

  if (buf_size == 0)
    return -1;
  if (_emxload_do_receive (&ans) != 0)
    return -1;
  if (ans.ans_code != 0)
    {
      _emxload_do_disconnect (FALSE);
      return 1;
    }
  _strncpy (buf, ans.name, buf_size);
  *pseconds = ans.seconds;
  return 0;
}
