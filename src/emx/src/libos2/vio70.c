/* vio70.c */

#define INCL_VIO
#include <os2.h>

USHORT _THUNK_FUNCTION (Vio16GlobalReg) ();

USHORT VioGlobalReg (PCSZ pszModName, PCSZ pszEntryName,
                     ULONG ulFunMask1, ULONG ulFunMask2, USHORT usReturn)
{
  return ((USHORT)
          (_THUNK_PROLOG (4+4+4+4+2);
           _THUNK_FLAT ((PSZ)pszModName);
           _THUNK_FLAT ((PSZ)pszEntryName);
           _THUNK_LONG (ulFunMask1);
           _THUNK_LONG (ulFunMask2);
           _THUNK_SHORT (usReturn);
           _THUNK_CALL (Vio16GlobalReg)));
}
