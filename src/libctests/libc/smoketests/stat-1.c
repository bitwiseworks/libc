#include <unistd.h>
#include <sys/stat.h>
#include <stdio.h>
#include <errno.h>


int main(int argc, char **argv)
{
    int             cErrors = 0;
    int             rc;
    struct stat     s;
    printf("stat-1: TESTING\n");


    /*
     * Current dir.
     */
    rc = stat(".", &s);
    if (rc != 0)
    {
        printf("stat(%s) -> %d errno=%d\n", ".", rc, errno);
        cErrors++;
    }

    /*
     * Parent dir.
     */
    rc = stat("..", &s);
    if (rc != 0)
    {
        printf("stat(%s) -> %d errno=%d\n", "..", rc, errno);
        cErrors++;
    }


    /*
     * Executable.
     */
    rc = stat(argv[0], &s);
    if (rc != 0)
    {
        printf("stat(%s) -> %d errno=%d\n", argv[0], rc, errno);
        cErrors++;
    }

    /*
     * Summary.
     */
    if (cErrors)
        printf("stat-1: FAILURE %d failures\n", cErrors);
    else
        printf("stat-1: SUCCESS\n");
    return cErrors;
}
