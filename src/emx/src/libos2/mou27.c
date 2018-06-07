/* mou27.c */

#define INCL_MOU
#include <os2.h>

USHORT _THUNK_FUNCTION (Mou16InitReal) ();

USHORT MouInitReal (PCSZ pszDriverName)
{
  return ((USHORT)
          (_THUNK_PROLOG (4);
           _THUNK_FLAT ((PSZ)pszDriverName);
           _THUNK_CALL (Mou16InitReal)));
}
