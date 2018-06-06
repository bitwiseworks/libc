/* uname.c (emx+gcc) -- Copyright (c) 1993 by Kai Uwe Rommel */
/*                      Copyright (c) 1993-1995 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdlib.h>
#include <string.h>
#include <sys/utsname.h>

#define SET(dst,src) _strncpy (name->dst, (src), sizeof (name->dst))
#define CHR(x)       (char)((x)+'0')

int _STD(uname) (struct utsname *name)
{
  char *host;

  host = getenv ("HOSTNAME");
  if (host == NULL)
    host = "standalone";

  SET (sysname, ("OS/2"));
  SET (nodename, host);
  SET (release, "1");
  name->version[0] = (CHR (_osmajor / 10));
  name->version[1] = '.';
  name->version[2] = CHR (_osminor / 10);
  name->version[3] = CHR (_osminor % 10);
  name->version[4] = 0;
  SET (machine, "i386");
  return 0;
}
