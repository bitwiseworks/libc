/* mou26.c */

#define INCL_MOU
#include <os2.h>

USHORT _THUNK_FUNCTION (Mou16DrawPtr) ();

USHORT MouDrawPtr (HMOU hmou)
{
  return ((USHORT)
          (_THUNK_PROLOG (2);
           _THUNK_SHORT (hmou);
           _THUNK_CALL (Mou16DrawPtr)));
}
