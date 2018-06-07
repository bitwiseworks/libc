#define INCL_BASE
#include <os2.h>
#include <stdio.h>

APIRET grc;

void APIENTRY thread2(ULONG ulArg)
{
    HMODULE hmod = NULLHANDLE;
    char    szObj[32];
    APIRET  rc;
    szObj[0] = '\0';
    rc = DosLoadModule(szObj, sizeof(szObj), "LIBC06.DLL", &hmod);
    if (rc)
        printf("DosLoadModule failed loading LIBC06.DLL. rc=%d szObj=%s\n", rc, szObj);
    else
        printf("Successfully loaded LIBC06.DLL\n");
    grc = rc;
}

int main()
{
    APIRET  rc;
    TID     tid;

    rc = DosCreateThread(&tid, thread2, 0, CREATE_READY | STACK_COMMITTED, 128*1024);
    if (!rc)
    {
        rc = DosWaitThread(&tid, DCWW_WAIT);
        if (!rc)
        {
            if (!grc)
            {
                printf("loadthread2: success\n");
                return 0;
            }
            rc = grc;
        }
    }
    printf("loadthread2: Failure. (rc=%d)\n", rc);
    return 1;
}
