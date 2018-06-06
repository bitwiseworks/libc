/* vio6.c */

#define INCL_VIO
#include <os2.h>

USHORT _THUNK_FUNCTION (Vio16DeRegister) ();

USHORT VioDeRegister (VOID)
{
  return ((USHORT)
          (_THUNK_PROLOG (0);
           _THUNK_CALL (Vio16DeRegister)));
}
