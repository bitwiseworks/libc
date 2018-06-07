/* dos16.c */

#define INCL_DOSSIGNALS
#include <os2.h>

USHORT _THUNK_FUNCTION (Dos16SetSigHandler) ();

USHORT APIENTRY DosSetSigHandler (PFNSIGHANDLER pfnSigHandler, PFNSIGHANDLER *pfpfnPrev, PUSHORT pfAction, USHORT fusAction, USHORT usSigNum)
{
  return ((USHORT)
          (_THUNK_PROLOG (4+4+4+2+2);
           _THUNK_PASCAL_FLAT (pfnSigHandler);
           _THUNK_PASCAL_FLAT (pfpfnPrev);
           _THUNK_PASCAL_FLAT (pfAction);
           _THUNK_PASCAL_SHORT (fusAction);
           _THUNK_PASCAL_SHORT (usSigNum);
           _THUNK_PASCAL_CALL (Dos16SetSigHandler)));
}


