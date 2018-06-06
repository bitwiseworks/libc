/*
    Locale support implementation through OS/2 Unicode API.
    Copyright (c) 2003 InnoTek Systemberatung GmbH

    For conditions of distribution and use, see the file COPYING.

    Compare two memory areas case-insensitively.
*/

#define INCL_FSMACROS
#include <os2emx.h>
#include "libc-alias.h"
#include <InnoTekLIBC/locale.h>
#include <stddef.h>
#include <string.h>

int _STD(memicmp) (const void *m1, const void *m2, size_t len)
{
  unsigned char c1;
  unsigned char c2;
  const unsigned char *s1 = (const unsigned char *)m1;
  const unsigned char *s2 = (const unsigned char *)m2;

  if (!len)
    return 0;

  c1 = *s1; c2 = *s2;

  if (__libc_GLocaleCtype.mbcs)
  {
    FS_VAR();
    FS_SAVE_LOAD();
    /* MBCS case. One additional memory lookup per character. */
    for (;;)
    {
      int d;

      if (IS_MBCS_PREFIX (&__libc_GLocaleCtype, c1)
       || IS_MBCS_PREFIX (&__libc_GLocaleCtype, c2))
      {
        /* Translate unknown characters to Unicode, and compare them...
           We perhaps should convert them back to MBCS, but anyway it is
           not defined how memicmp should work on multi-byte characters... */
        UniChar uc1, uc2;
        int c1l, c2l;
        if (!(c1l = __libc_ucs2To (__libc_GLocaleCtype.uobj, s1, len, &uc1)))
          uc1 = c1, c1l = 1;
        else
          uc1 = UniTransLower (__libc_GLocaleCtype.lobj, uc1);
        if (!(c2l = __libc_ucs2To (__libc_GLocaleCtype.uobj, s2, len, &uc2)))
          uc2 = c2, c2l = 1;
        else
          uc2 = UniTransLower (__libc_GLocaleCtype.lobj, uc2);
        d = uc1 - uc2;
        c1l--; c2l--;
        s1 += c1l; s2 += c2l;
        len -= c1l;
      }
      else
        d = __libc_GLocaleCtype.auchLower [c1] - __libc_GLocaleCtype.auchLower [c2];
      if (d)
        {
          FS_RESTORE();
          return d;
        }
      if (!--len)
        {
          FS_RESTORE();
          return 0;
        }
      c1 = *++s1;
      c2 = *++s2;
    } /* endfor */
    FS_RESTORE();
  }
  else
    /* SBCS case (faster). */
    for (;;)
    {
      int d = __libc_GLocaleCtype.auchLower [c1] - __libc_GLocaleCtype.auchLower [c2];
      if (d)
        return d;
      if (!--len)
        return 0;
      c1 = *++s1;
      c2 = *++s2;
    } /* endfor */

  /* This point never reached */
}
