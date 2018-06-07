/*
    Locale support implementation through OS/2 Unicode API.
    Copyright (c) 2003 InnoTek Systemberatung GmbH

    For conditions of distribution and use, see the file COPYING.

    Compare two strings, interpreting every character as appropiate
    to the LC_COLLATE category of the locale. The result of the comparison
    is always in alphabetical order of selected language.
*/

#define INCL_FSMACROS
#include <os2emx.h>
#include "libc-alias.h"
#include <InnoTekLIBC/locale.h>
#include <alloca.h>
#include <string.h>

int _STD(strcoll) (const char *s1, const char *s2)
{
  int i, d;
  unsigned char c1 = *s1;
  unsigned char c2 = *s2;

  if (__libc_gLocaleCollate.mbcs)
  {
    /* MBCS case. We compare the strings as usual, but as soon as we
       encounter a MBCS character we translate the rests of the strings
       to Unicode and continue comparation in Unicode. */
    for (;;)
    {
      if (IS_MBCS_PREFIX (&__libc_gLocaleCollate, c1)
       || IS_MBCS_PREFIX (&__libc_gLocaleCollate, c2))
      {
        /* Allright, we found a MBCS character. */
        UniChar *ucs [2];
        const char *sbcs [2] = { s1, s2 };
        FS_VAR_SAVE_LOAD();

        /* I can't imagine X bytes to convert into more
           than X unicode characters... */
        for (i = 0; i < 2; i++)
        {
          void *inbuf = (void *)sbcs [i];
          size_t sl = strlen (sbcs [i]) + 1;
          size_t nonid, in_left = sl, out_left = sl;
          UniChar *outbuf = ucs [i] = alloca (sl * sizeof (UniChar));

          if (UniUconvToUcs (__libc_gLocaleCollate.uobj, &inbuf, &in_left, &outbuf, &out_left, &nonid))
          {
            /* Oops, something bad happened (invalid character code). Suppose
               the string that caused the fault is "less" than the other */
            FS_RESTORE();
            return i * 2 - 1;
          }
        }

        /* Okay, now we have two Unicode strings. Compare them. */
        d = UniStrcoll (__libc_gLocaleCollate.lobj, ucs [0], ucs [1]);
        FS_RESTORE();
        return d;
      }

      if (c1 != c2)
      {
        d = __libc_gLocaleCollate.auchWeight [c1] - __libc_gLocaleCollate.auchWeight [c2];
        if (d || !c1 || !c2)
          return d;
      }
      else if (!c1)
        return 0;
      c1 = *++s1;
      c2 = *++s2;
    } /* endfor */
  }
  else
    /* SBCS case (faster). */
    for (;;)
    {
      if (c1 != c2)
      {
        d = __libc_gLocaleCollate.auchWeight [c1] - __libc_gLocaleCollate.auchWeight [c2];
        if (d || !c1 || !c2)
          return d;
      }
      else if (!c1)
        return 0;
      c1 = *++s1;
      c2 = *++s2;
    } /* endfor */

  /* This point never achieved */
}
