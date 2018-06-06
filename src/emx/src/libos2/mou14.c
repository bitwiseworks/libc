/* mou14.c */

#define INCL_MOU
#include <os2.h>

USHORT _THUNK_FUNCTION (Mou16DeRegister) ();

USHORT MouDeRegister (VOID)
{
  return ((USHORT)
          (_THUNK_PROLOG (0);
           _THUNK_CALL (Mou16DeRegister)));
}
