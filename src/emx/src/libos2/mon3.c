/* mon3.c */

#define INCL_DOSMONITORS
#include <os2.h>

USHORT _THUNK_FUNCTION (Dos16MonClose) ();

USHORT DosMonClose (HMONITOR hmon)
{
  return ((USHORT)
          (_THUNK_PROLOG (2);
           _THUNK_SHORT (hmon);
           _THUNK_CALL (Dos16MonClose)));
}
