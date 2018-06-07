/* dos34.c */

#define INCL_DOSMODULEMGR
#include <os2.h>

USHORT _THUNK_FUNCTION (Dos16GetProcAddr) ();

USHORT APIENTRY DosGetProcAddr(HMODULE hmte, PCSZ pszNameOrOrdinal, PVOID pvfpfn)
{
    if ((unsigned long)pszNameOrOrdinal < 10000)
        return ((USHORT)
                (_THUNK_PROLOG (2+4+4);
                 _THUNK_SHORT (hmte);
                 _THUNK_LONG ((LONG)pszNameOrOrdinal);
                 _THUNK_FLAT (pvfpfn);
                 _THUNK_CALL (Dos16GetProcAddr)));
    else
        return ((USHORT)
                (_THUNK_PROLOG (2+4+4);
                 _THUNK_SHORT (hmte);
                 _THUNK_FLAT ((void *)pszNameOrOrdinal);
                 _THUNK_FLAT (pvfpfn);
                 _THUNK_CALL (Dos16GetProcAddr)));
}

