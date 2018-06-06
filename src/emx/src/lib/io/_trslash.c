/* _trslash.c (emx+gcc) -- Copyright (c) 1996 by Eberhard Mattes */

#include "libc-alias.h"
#include <emx/io.h>
#include <InnoTekLIBC/locale.h>

/* Detect trailing slash or backslash for stat(), access(), and
   _tmpidxnam().  If MODE is 0, the trailing (back)slash may be
   preceded by an arbitrary character, including none.  Moreover, a
   trailing colon is treated like a trailing slash.  If MODE is 1, the
   trailing (back)slash must be preceded by a character which is not a
   (back)slash or a colon.  Note that the string pointed to by NAME
   need not be null-terminated! */

/* Support only SBCS and DBCS for now. */

int _trslash (const char *name, size_t len, int mode)
{
  size_t i, mbcl;

  switch (mode)
    {
    case 0:
      if (len < 1 || (name[len-1] != '\\' && name[len-1] != '/'
                      && name[len-1] != ':'))
        return 0;
      if (len == 1 && (name[0] == '\\' || name[0] == '/'))
        return 1;
      break;
    case 1:
      if (len < 2 || (name[len-1] != '\\' && name[len-1] != '/'))
        return 0;
      break;
    default:
      return 0;
    }

  /* The name seems to have a trailing (back)slash.  There are two
     conditions which must _not_ hold:

     (1) the (back)slash is the 2nd byte of a DBCS character
     (2) the (back)slash is preceded by a SBCS (back)slash or a colon

     Condition (2) does not matter if MODE is 0.

          |         |     |     | return for MODE
     Case | name(*) | (1) | (2) |   0   |   1
     -----+---------+-----+-----+-------+-------
     (a)  | ....//  | no  | yes |   1   |   0
     (b)  | ....S/  | no  | no  |   1   |   1
     (c)  | ...L2/  | no  | no  |   1   |   1
     (d)  | ....L/  | yes | -   |   0   |   0

     (*) S = SBCS character but not a (back)slash or colon
         L = lead byte
         2 = 2nd byte of DBCS char */

  i = 0;
  for (;;)
    {
      if (CHK_MBCS_PREFIX (&__libc_GLocaleCtype, name[i], mbcl))
        {
          if (i == len - 1 - mbcl)
            {
              /* Case (c) -- a DBCS character precedes the trailing
                 (back)slash. */
              return 1;
            }
          else if (i == len - mbcl)
            {
              /* Case (d) -- the last byte is the 2nd byte of a DBCS
                 character and therefore is not a SBCS (back)slash. */
              return 0;
            }
          else
            i += mbcl;
        }
      else if (i == len - 2)
        {
          /* We hit the character preceding the trailing (back)slash;
             this is either case (a) or (b).  Check for condition (2),
             condition (1) does not apply. */
          return (mode == 0
                  || (name[i] != '\\' && name[i] != '/' && name[i] != ':'));
        }
      else
        ++i;
    }
}
