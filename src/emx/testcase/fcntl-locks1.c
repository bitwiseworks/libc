#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/fcntl.h>

int main(int argc, char **argv)
{
    int             rc;
    int             rcRet = 0;
    int             fd;
    struct flock    lock;

    fd = open(argv[0], O_RDONLY);
    if (fd < 0)
    {
        printf("error: failed to open '%s', errno=%d\n", argv[0], errno);
        return 1;
    }

    lock.l_type     = F_RDLCK;
    lock.l_whence   = SEEK_SET;
    lock.l_start    = 0;
    lock.l_len      = 0;
    rc = fcntl(fd, F_SETLK, &lock);
    if (rc)
    {
        rcRet++;
        printf("error: lock rc=%d, errno=%d\n", rc, errno);
    }
    close(fd);

    if (rcRet)
        printf("testcase failed\n");
    else
        printf("testcase succeeded\n");
    return rcRet;
}
