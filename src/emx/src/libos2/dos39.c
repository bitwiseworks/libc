/* dos39.c */

#define INCL_DOSMEMMGR
#include <os2.h>

USHORT _THUNK_FUNCTION (Dos16FreeSeg) ();

USHORT APIENTRY DosFreeSeg(SEL sel)
{
  return ((USHORT)
          (_THUNK_PROLOG (2);
           _THUNK_SHORT (sel);
           _THUNK_CALL (Dos16FreeSeg)));
}

