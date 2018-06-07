#define INCL_BASE
#include <os2.h>
#include <stdio.h>



int main(int argc, char **argv)
{
    int rc;
    if (argc != 3)
    {
        printf("syntax error\n");
        return 1;
    }
    rc = DosCopy(argv[1], argv[2], DCPY_EXISTING);
    if (!rc)
        printf("Successfully copied %s to %s\n", argv[1], argv[2]);
    else
        printf("Failed with rc=%d copying %s to %s\n", rc, argv[1], argv[2]);
    return rc;
}
