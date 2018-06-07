/* vio57.c */

#define INCL_AVIO
#include <os2.h>

USHORT _THUNK_FUNCTION (Vio16DeleteSetId) ();

USHORT VioDeleteSetId (LONG llcid, HVPS hvps)
{
  return ((USHORT)
          (_THUNK_PROLOG (4+2);
           _THUNK_LONG (llcid);
           _THUNK_SHORT (hvps);
           _THUNK_CALL (Vio16DeleteSetId)));
}
