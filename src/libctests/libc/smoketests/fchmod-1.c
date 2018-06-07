#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/fcntl.h>

int cErrors = 0;

static void check_mode(int fh, mode_t Mode)
{
    struct stat st;
    int rc = fstat(fh, &st);
    if (!rc)
    {
        if ((st.st_mode & (0777 | S_IFMT)) != Mode)
        {
            printf("Invalid mode 0%o, expected 0%o\n", st.st_mode & 0777, Mode);
            cErrors++;
        }
    }
    else
    {
        printf("fstat(%d,) -> %d errno=%d (%m)\n", fh, rc, errno);
        cErrors++;
    }
}

int main()
{
    umask(0);
    unlink("fchmod-1.tst");

    int rc = -1;
    int fh = open("fchmod-1.tst", O_RDWR | O_CREAT, 0766);
    if (fh >= 0)
    {
        check_mode(fh, 0766 | S_IFREG);
        rc = fchmod(fh, 0744);
        if (!rc)
        {
            check_mode(fh, 0744 | S_IFREG);
            rc = futimes(fh, NULL);
            check_mode(fh, 0744 | S_IFREG);
        }
        else
            cErrors++;
    }
    else
        cErrors++;
    printf("fchmod-1: rdwr: fh=%d rc=%d errno=%d (%m)\n", fh, rc, errno);
    close(fh);

    rc = chmod("fchmod-1.tst", 0744);
    if (rc)
    {
        printf("fchmod-1: chmod: rc=%d errno=%d (%m)\n", rc, errno);
        cErrors++;
    }

    errno = 0;
    rc = -1;
    fh = open("fchmod-1.tst", O_RDONLY);
    if (fh >= 0)
    {
        check_mode(fh, 0744 | S_IFREG);
        rc = chmod("fchmod-1.tst", 0700);
        if (!rc)
            check_mode(fh, 0700 | S_IFREG);
        else
        {
            printf("fchmod-1: rdonly chmod: fh=%d rc=%d errno=%d (%m) - expected failure, ignored\n", fh, rc, errno);
            /*cErrors++; - ignored */
        }
        rc = fchmod(fh, 0777);
        if (!rc)
            check_mode(fh, 0777 | S_IFREG);
        else
        {
            printf("fchmod-1: rdonly: fh=%d rc=%d errno=%d (%m) - expected failure, ignored\n", fh, rc, errno);
            /*cErrors++; - ignored */
        }
    }
    else
    {
        printf("fchmod-1: rdonly: open failed errno=%d (%m)\n", errno);
        cErrors++;
    }
    close(fh);

    unlink("fchmod-1.tst");

    if (!cErrors)
        printf("fchmod-1: SUCCESS\n");
    else
        printf("fchmod-1: FAILURE - %d errors\n", cErrors);

    return !!cErrors;
}


