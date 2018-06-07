/* mon4.c */

#define INCL_DOSMONITORS
#include <os2.h>

USHORT _THUNK_FUNCTION (Dos16MonOpen) ();

USHORT DosMonOpen (PSZ pszDevName, PHMONITOR phmon)
{
  return ((USHORT)
          (_THUNK_PROLOG (4+4);
           _THUNK_FLAT (pszDevName);
           _THUNK_FLAT (phmon);
           _THUNK_CALL (Dos16MonOpen)));
}
