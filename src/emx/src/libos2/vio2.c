/* vio2.c */

#define INCL_VIO
#include <os2.h>

USHORT _THUNK_FUNCTION (Vio16GetPhysBuf) ();

USHORT VioGetPhysBuf (PVIOPHYSBUF pvioPhysBuf, USHORT usReserved)
{
  return ((USHORT)
          (_THUNK_PROLOG (4+2);
           _THUNK_FLAT (pvioPhysBuf);
           _THUNK_SHORT (usReserved);
           _THUNK_CALL (Vio16GetPhysBuf)));
}
