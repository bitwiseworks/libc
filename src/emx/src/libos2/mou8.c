/* mou8.c */

#define INCL_MOU
#include <os2.h>

USHORT _THUNK_FUNCTION (Mou16GetNumButtons) ();

USHORT MouGetNumButtons (PUSHORT pcButtons, HMOU hmou)
{
  return ((USHORT)
          (_THUNK_PROLOG (4+2);
           _THUNK_FLAT (pcButtons);
           _THUNK_SHORT (hmou);
           _THUNK_CALL (Mou16GetNumButtons)));
}
