/* herror.c (emx+gcc) -- Copyright (c) 1994 by Eberhard Mattes */

#ifndef TCPV40HDRS
#error TCPV40HDRS only
#endif

#include "libc-alias.h"
#include <stdio.h>
#include <sys/types.h>
#include <netdb.h>
#define __LIBC_LOG_GROUP __LIBC_LOG_GRP_SOCKET
#include <InnoTekLIBC/logstrict.h>

extern int h_errno;

void TCPCALL herror(const char *string)
{
  LIBCLOG_ENTER("string=%p={%25s}\n", string, string);
  int e = h_errno;
  const char *msg;

  if (string != NULL && *string != 0)
    {
      fputs (string, stderr);
      fputs (": ", stderr);
    }
  switch (e)
    {
    case 0:
      msg = "Error 0";
      break;
    case HOST_NOT_FOUND:
      msg = "Host not found";
      break;
    case TRY_AGAIN:
      msg = "Host name lookup failure";
      break;
    case NO_RECOVERY:
      msg = "Unknown server error";
      break;
    case NO_DATA:
      msg = "No address associated with name";
      break;
    default:
      fprintf (stderr, "Unknown error %d\n", e);
      LIBCLOG_RETURN_VOID();
    }
  fputs (msg, stderr);
  fputc ('\n', stderr);
  LIBCLOG_RETURN_VOID();
}
