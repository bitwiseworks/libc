/* vio60.c */

#define INCL_AVIO
#include <os2.h>

USHORT _THUNK_FUNCTION (Vio16CreateLogFont) ();

USHORT VioCreateLogFont (PFATTRS pfatattrs, LONG llcid, PSTR8 pName,
                         HVPS hvps)
{
  return ((USHORT)
          (_THUNK_PROLOG (4+4+4+2);
           _THUNK_FLAT (pfatattrs);
           _THUNK_LONG (llcid);
           _THUNK_FLAT (pName);
           _THUNK_SHORT (hvps);
           _THUNK_CALL (Vio16CreateLogFont)));
}
