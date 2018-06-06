/* vio62.c */

#define INCL_AVIO
#include <os2.h>

USHORT _THUNK_FUNCTION (Vio16QuerySetIds) ();

USHORT VioQuerySetIds (PLONG allcids, PSTR8 pNames, PLONG alTypes, LONG lcount,
                       HVPS hvps)
{
  return ((USHORT)
          (_THUNK_PROLOG (4+4+4+4+2);
           _THUNK_FLAT (allcids);
           _THUNK_FLAT (pNames);
           _THUNK_FLAT (alTypes);
           _THUNK_LONG (lcount);
           _THUNK_SHORT (hvps);
           _THUNK_CALL (Vio16QuerySetIds)));
}
