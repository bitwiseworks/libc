/* getpass2.c (emx+gcc) -- Copyright (c) 1993-1994 by Kai Uwe Rommel */
/*                         Copyright (c) 1994-1995 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <unistd.h>
#include <pwd.h>

char *_getpass2 (const char *prompt, int kbd)
{
  static char pbuf[_PASSWORD_LEN+1];
  int c, i;

  fputs (prompt, stderr);
  fflush (stderr);
  i = 0;
  for (;;)
    {
      c = (kbd ? getch () : fgetc (stdin));
      if (c == '\r' || c == '\n')
        break;
      if (c == '\b' || c == 127)
        {
          if (i > 0) --i;
        }
      else if (c == 0x15 || c == 0x1b)
        i = 0;
      else if (i < sizeof (pbuf)-1)
        pbuf[i++] = (char)c;
    }
  pbuf[i] = 0;
  fputs ("\r\n", stderr);
  fflush (stderr);
  return pbuf;
}
