/* pmviop30.c */

#define INCL_AVIO
#include <os2.h>

USHORT _THUNK_FUNCTION (Win16DefAVioWindowProc) ();

MRESULT WinDefAVioWindowProc (HWND hwnd, USHORT msg, ULONG mp1, ULONG mp2)
{
  return ((MRESULT)
          (_THUNK_PROLOG (4+2+4+4);
           _THUNK_LONG ((ULONG)hwnd);
           _THUNK_SHORT (msg);
           _THUNK_LONG (mp1);
           _THUNK_LONG (mp2);
           _THUNK_CALL (Win16DefAVioWindowProc)));
}
