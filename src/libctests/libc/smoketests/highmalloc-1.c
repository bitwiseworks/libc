#include <stdlib.h>
#include <stdio.h>
#define INCL_BASE
#include <os2.h>



int main()
{
    ULONG ul;
    if (DosQuerySysInfo(QSV_VIRTUALADDRESSLIMIT, QSV_VIRTUALADDRESSLIMIT, &ul, sizeof(ul)))
        ul = 512;
    if (ul > 768)
    {
        printf("highmalloc: %d MB of virtual memory\n", ul);
        static void *apv[1000];
        int         i;
        void *pv1 = malloc(34*1024*1024);
        void *pv2 = malloc(34*1024*1024);
        void *pv3 = malloc(34*1024*1024);

        printf("pv1=%p pv2=%p pv3=%p\n", pv1, pv2, pv3);
        free(pv1);
        _heapmin();
        free(pv2);
        _heapmin();
        free(pv3);
        _heapmin();

        for (i = 0; i < 1000; i++)
            apv[i] = malloc(2048 * i);

        for (i = 0; i < 1000; i++)
        {
            free(apv[i]);
            _heapmin();
        }
    }
    else
        printf("skipping: no high memory support.\n");
    return 0;
}
