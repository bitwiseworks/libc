#include <stdio.h>
#include <unistd.h>
#include <errno.h>

int main(int argc, char **argv)
{
#if 1
    char szLink[4096] = {0};
    int rc = readlink(argv[0], szLink, sizeof(szLink));
    if (errno != EINVAL)
    {
        printf("readlink-1: FAILURE - errno %d\n", errno);
        return 1;
    }
    printf("readlink-1: SUCCESS\n");
    return 0;
#else
    int i;
    for (i = 1; i < argc; i++)
    {
        char szLink[4096] = {0};
        int rc = readlink(argv[i], szLink, sizeof(szLink));
        printf("rc=%d errno=%d '%s' '%s'\n", rc, errno, argv[i], szLink);
    }
    return 0;
#endif
}
