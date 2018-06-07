/* vio61.c */

#define INCL_AVIO
#include <os2.h>

USHORT _THUNK_FUNCTION (Vio16DestroyPS) ();

USHORT VioDestroyPS (HVPS hvps)
{
  return ((USHORT)
          (_THUNK_PROLOG (2);
           _THUNK_SHORT (hvps);
           _THUNK_CALL (Vio16DestroyPS)));
}
