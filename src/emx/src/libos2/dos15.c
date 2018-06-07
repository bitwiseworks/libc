/* dos15.c */

#define INCL_DOSSIGNALS
#include <os2.h>

USHORT _THUNK_FUNCTION (Dos16FlagProcess) ();

USHORT APIENTRY DosFlagProcess(PID pid, USHORT fScope, USHORT usFlagNum, USHORT usFlagArg)
{
  return ((USHORT)
          (_THUNK_PROLOG (2+2+2+2);
           _THUNK_SHORT (pid);
           _THUNK_SHORT (fScope);
           _THUNK_SHORT (usFlagNum);
           _THUNK_SHORT (usFlagArg);
           _THUNK_CALL (Dos16FlagProcess)));
}

