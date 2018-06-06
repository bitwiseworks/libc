/* remzeros.c (emx+gcc) -- Copyright (c) 1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <string.h>
#include <emx/float.h>

/* Remove trailing zeros from DIGITS for the "%g" format.  Keep at
   least KEEP digits at the start of DIGITS. */

void __remove_zeros (char *digits, int keep)
{
  int i;

  i = strlen (digits) - 1;
  while (i >= keep && digits[i] == '0')
    --i;
  digits[i+1] = 0;
}
