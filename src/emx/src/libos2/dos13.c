/* dos13.c */

#define INCL_DOSSIGNALS
#include <os2.h>

USHORT _THUNK_FUNCTION (Dos16HoldSignal) ();

USHORT APIENTRY DosHoldSignal (USHORT fusDisable)
{
  return ((USHORT)
          (_THUNK_PASCAL_PROLOG (2);
           _THUNK_PASCAL_SHORT (fusDisable);
           _THUNK_PASCAL_CALL (Dos16HoldSignal)));
}


