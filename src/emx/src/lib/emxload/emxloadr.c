/* emxloadr.c (emx+gcc) -- Copyright (c) 1993-1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdlib.h>
#include <string.h>
#include <sys/emxload.h>
#include <emx/emxload.h>
#define INCL_DOSNMPIPES
#define INCL_DOSPROCESS
#define INCL_DOSERRORS
#include <os2emx.h>


static char const pipe_name[] = _EMXLOAD_PIPENAME;
static HPIPE hpServer = 0;
static int connect_level = 0;

int _emxload_do_connect (int start)
{
  int i, ok;
  ULONG rc, action;

  if (connect_level <= 0)
    {
      ok = FALSE;
      for (i = 0; i < 30; ++i)
        {
          rc = DosOpen (pipe_name, &hpServer, &action, 0, 0,
                        OPEN_ACTION_FAIL_IF_NEW | OPEN_ACTION_OPEN_IF_EXISTS,
                        OPEN_FLAGS_NOINHERIT | OPEN_SHARE_DENYREADWRITE
                        | OPEN_ACCESS_READWRITE, NULL);
          if (rc == 0)
            {
              ok = TRUE;
              break;
            }
          else if (rc == ERROR_PIPE_BUSY)
            rc = DosWaitNPipe (pipe_name, 1000L);
          if (rc == ERROR_FILE_NOT_FOUND || rc == ERROR_PATH_NOT_FOUND)
            {
              if (!start)
                return 0;
              if (i == 0)
                {
                  CHAR objbuf[64];
                  RESULTCODES res;

                  /* Note: emxload will inherit the non-private file
                     handles.  Therefore, emxload should close all the
                     handles. */

                  rc = DosExecPgm (objbuf, sizeof (objbuf), EXEC_BACKGROUND,
                                   "emxload\0-server\0", NULL, &res,
                                   "emxload.exe");
                  if (rc != 0)
                    return -1;
                }
              else
                {
                  /* Perhaps we should check here whether the server
                     process is alive or not. */

                  DosSleep (1000L);
                }
            }
          else if (rc != 0 && rc != ERROR_PIPE_BUSY)
            return -1;
        }
      if (!ok)
        return -1;
    }
  ++connect_level;
  return 0;
}


int _emxload_do_disconnect (int force)
{
  if (connect_level <= 0)
    return -1;
  --connect_level;
  if (connect_level <= 0 || force)
    {
      connect_level = 0;
      if (DosClose (hpServer) != 0)
        return -1;
    }
  return 0;
}


int _emxload_do_request (int req_code, const char *name, int seconds)
{
  ULONG rc, cb;
  int len;
  request req;

  if (connect_level <= 0)
    return -1;
  len = (name == NULL ? 0 : strlen (name));
  if (name == NULL)
    req.name[0] = 0;
  else if (len >= sizeof (req.name))
    return -1;
  else
    memcpy(req.name, name, len + 1);
  req.req_code = req_code;
  req.seconds = seconds;
  rc = DosWrite (hpServer, &req, sizeof (req), &cb);
  if (rc != 0)
    {
      rc = DosClose (hpServer);
      connect_level = 0;
      return -1;
    }
  return 0;
}


int _emxload_do_receive (answer *ans)
{
  ULONG rc, cb;

  rc = DosRead (hpServer, ans, sizeof (answer), &cb);
  if (rc != 0 || cb != sizeof (answer))
    {
      rc = DosClose (hpServer);
      connect_level = 0;
      return -1;
    }
  return 0;
}


int _emxload_do_wait (void)
{
  answer dummy;

  return _emxload_do_receive (&dummy);
}


int _emxload_request (int req_code, const char *name, int seconds)
{
  int result;

  switch (req_code)
    {
    case _EMXLOAD_LOAD:
      if (_emxload_do_connect (TRUE) != 0)
        return -1;
      result = _emxload_do_request (req_code, name, seconds);
      if (_emxload_do_disconnect (FALSE) != 0)
        result = -1;
      return result;

    case _EMXLOAD_UNLOAD:
    case _EMXLOAD_UNLOAD_WAIT:
      if (_emxload_do_connect (FALSE) != 0)
        return 0;
      result = _emxload_do_request (req_code, name, seconds);
      if (req_code == _EMXLOAD_UNLOAD_WAIT && _emxload_do_wait () != 0)
        result = -1;
      if (_emxload_do_disconnect (FALSE) != 0)
        result = -1;
      return result;

    case _EMXLOAD_CONNECT:
      return _emxload_do_connect (TRUE);

    case _EMXLOAD_STOP:
    case _EMXLOAD_STOP_WAIT:
      if (_emxload_do_connect (FALSE) != 0)
        return -1;
      result = _emxload_do_request (req_code, NULL, 0);
      if (req_code == _EMXLOAD_STOP_WAIT && connect_level > 0)
        _emxload_do_wait ();    /* Will always fail */
      _emxload_do_disconnect (TRUE); /* DosClose may fail */
      return result;

    case _EMXLOAD_DISCONNECT:
      return _emxload_do_disconnect (TRUE);

    default:
      return -1;
    }
}
