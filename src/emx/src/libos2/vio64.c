/* vio64.c */

#define INCL_AVIO
#include <os2.h>

USHORT _THUNK_FUNCTION (Vio16QueryFonts) ();

USHORT VioQueryFonts (PLONG plRemfonts, PFONTMETRICS afmMetrics,
                      LONG lMetricsLength, PLONG plFonts, PSZ pszFacename,
                      ULONG flOptions, HVPS hvps)
{
  return ((USHORT)
          (_THUNK_PROLOG (4+4+4+4+4+4+2);
           _THUNK_FLAT (plRemfonts);
           _THUNK_FLAT (afmMetrics);
           _THUNK_LONG (lMetricsLength);
           _THUNK_FLAT (plFonts);
           _THUNK_FLAT (pszFacename);
           _THUNK_LONG (flOptions);
           _THUNK_SHORT (hvps);
           _THUNK_CALL (Vio16QueryFonts)));
}
