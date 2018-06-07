/*
    Locale support implementation through OS/2 Unicode API.
    Copyright (c) 2003 InnoTek Systemberatung GmbH

    For conditions of distribution and use, see the file COPYING.

    Uppercase an ASCIIZ string.
*/

#define INCL_FSMACROS
#include <os2emx.h>
#include "libc-alias.h"
#include <InnoTekLIBC/locale.h>
#include <string.h>

static int __uni_strupr (UniChar *ucs, void *arg)
{
  UniChar *c;
  (void)arg;
  for (c = ucs; *c; c++)
    *c = UniTransUpper (__libc_GLocaleCtype.lobj, *c);
  return 1;
}

char *_STD(strupr) (char *string)
{
  unsigned char c;
  unsigned char *s = (unsigned char *)string;

  if (__libc_GLocaleCtype.mbcs)
  {
    while ((c = *s))
    {
      if (IS_MBCS_PREFIX (&__libc_GLocaleCtype, c))
      {
        /* Allright, we encountered a MBCS character. Convert everything
           until the end to Unicode, do the work in Unicode and then
           convert back to MBCS. */
        FS_VAR();
        FS_SAVE_LOAD();
        __libc_ucs2Do (__libc_GLocaleCtype.uobj, (char *)s, s, __uni_strupr);
        FS_RESTORE();
        break;
      }
      *s++ = __libc_GLocaleCtype.auchUpper [c];
    }
  }
  else
    while ((c = *s))
      *s++ = __libc_GLocaleCtype.auchUpper [c];

  return string;
}
