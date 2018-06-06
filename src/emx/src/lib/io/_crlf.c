/* _crlf.c (emx+gcc) -- Copyright (c) 1990-1995 by Eberhard Mattes */

#include "libc-alias.h"
#include <string.h>
#include <io.h>

/* Do CR/LF -> LF conversion in the buffer BUF containing SIZE characters. */
/* Store the number of resulting characters to *NEW_SIZE. This value does  */
/* not include the final CR, if present. If the buffer ends with CR, 1 is  */
/* returned, 0 otherwise.                                                  */

int _crlf (char *buf, size_t size, size_t *new_size)
{
  size_t src, dst;
  char *p;

  p = memchr (buf, '\r', size); /* Avoid copying until CR reached */
  if (p == NULL)                /* This is the trivial case */
    {
      *new_size = size;
      return 0;
    }
  src = dst = p - buf;          /* Start copying here */
  while (src < size)
    {
      if (buf[src] == '\r')     /* CR? */
        {
          ++src;                /* Skip the CR */
          if (src >= size)      /* Is it the final char? */
            {
              *new_size = dst;  /* Yes -> don't include in new_size, */
              return 1;         /*        notify caller              */
            }
          if (buf[src] != '\n') /* CR not followed by LF? */
            --src;              /* Yes -> copy the CR */
        }
      buf[dst++] = buf[src++];  /* Copy a character */
    }
  *new_size = dst;
  return 0;
}
