/* os2bad.c (emx+gcc) */

#include "libc-alias.h"
#include <stdlib.h>
#include <emx/syscalls.h>

static char msg[] = "\r\nOS/2 API call not fixed up\r\n";

void _os2_bad (void);

void _os2_bad (void)
{
  __write (2, msg, sizeof (msg) - 1);
  _exit (2);
}
