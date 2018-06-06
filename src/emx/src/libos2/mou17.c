/* mou17.c */

#define INCL_MOU
#include <os2.h>

USHORT _THUNK_FUNCTION (Mou16Open) ();

USHORT MouOpen (PCSZ pszDvrName, PHMOU phmou)
{
  return ((USHORT)
          (_THUNK_PROLOG (4+4);
           _THUNK_FLAT ((PSZ)pszDvrName);
           _THUNK_FLAT (phmou);
           _THUNK_CALL (Mou16Open)));
}
