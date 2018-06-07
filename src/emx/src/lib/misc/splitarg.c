/* splitarg.c (emx+gcc) -- Copyright (c) 1993-1995 by Eberhard Mattes */

#include "libc-alias.h"
#include <stdlib.h>
#include <errno.h>

#define WHITE(c) ((c) == ' ' || (c) == '\t' || (c) == '\n')

char ** _splitargs (char *string, int *count)
{
  int argc, arga, src, dst, bs, quote;
  char *q, **argv;

  argv = NULL; argc = 0; arga = 0; dst = 0; src = 0;
  while (WHITE (string[src]))
    ++src;
  do 
    {
      if (argc >= arga)
        {
          arga += 16;
          argv = realloc (argv, arga * sizeof (*argv));
          if (argv == NULL)
            {
              errno = ENOMEM;
              return NULL;
            }
        }
      if (string[src] == 0)
        q = NULL;
      else
        {
          q = string+dst; bs = 0; quote = 0;
          for (;;)
            {
              if (string[src] == '"')
                {
                  while (bs >= 2)
                    {
                      string[dst++] = '\\';
                      bs -= 2;
                    }
                  if (bs & 1)
                    string[dst++] = '"';
                  else
                    quote = !quote;
                  bs = 0;
                }
              else if (string[src] == '\\')
                ++bs;
              else
                {
                  while (bs != 0)
                    {
                      string[dst++] = '\\';
                      --bs;
                    }
                  if (string[src] == 0 || (WHITE (string[src]) && !quote))
                    break;
                  string[dst++] = string[src];
                }
              ++src;
            }
          while (WHITE (string[src]))
            ++src;
          string[dst++] = 0;
        }
      argv[argc++] = q;
    } while (q != NULL);
  if (count != NULL)
    *count = argc - 1;
  return argv;
}
