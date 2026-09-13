/*
* Exercise /@unixroot file creation and realpath() before and after chroot().
*
* Attempts to create /@unixroot/test.txt with or without chroot("D:/Temp") for
* appending (leaves intact if it exists). "D:/Temp" must be already present.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static int test_unixroot(void)
{
    const char *path = "/@unixroot/test.txt";
    FILE *file;
    char *resolved;
    int rc = 0;

    file = fopen(path, "a");
    if (file == NULL)
    {
        perror("fopen(/@unixroot/test.txt)");
        rc = 1;
    }
    else if (fclose(file) != 0)
    {
        perror("fclose(/@unixroot/test.txt)");
        rc = 1;
    }
    else
    printf("Created/opened %s\n", path);

    /* Try realpath even if creation failed, so both results are visible. */
    fflush(stdout);
    resolved = realpath(path, NULL);
    if (resolved == NULL)
    {
        perror("realpath(/@unixroot/test.txt)");
        rc = 1;
    }
    else
    {
        printf("realpath(%s) = %s\n", path, resolved);
        free(resolved);
    }

    return rc;
}

int main(void)
{
    const char *unixroot = getenv("UNIXROOT");
    int rc;

    printf("UNIXROOT=%s\n", unixroot != NULL ? unixroot : "(unset)");
    printf("Before chroot:\n");
    fflush(stdout);
    rc = test_unixroot();

    printf("Calling chroot(\"D:/Temp\")\n");
    fflush(stdout);
    if (chroot("D:/Temp") != 0)
    {
        perror("chroot(D:/Temp)");
        return 1;
    }

    printf("After chroot:\n");
    fflush(stdout);
    rc |= test_unixroot();
    return rc;
}
