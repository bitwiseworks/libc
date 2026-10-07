/*
 * Verify that a native OS/2 copy results in a distinct LIBC inode and that
 * stat and fstat report the same inode for the copy.
 */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#define SOURCE "inode-copy-1-source"
#define COPY   "inode-copy-1-copy"

int main(void)
{
    struct stat source, copy, opened;
    int fd;
    int failed = 1;

    unlink(SOURCE);
    unlink(COPY);

    fd = open(SOURCE, O_WRONLY | O_CREAT | O_EXCL, 0600);
    if (fd < 0)
    {
        perror("open source");
        return 1;
    }
    close(fd);

    if (system("cmd /c copy " SOURCE " " COPY " >nul") != 0)
    {
        fprintf(stderr, "native copy failed\n");
        goto out;
    }

    if (stat(SOURCE, &source) < 0 || stat(COPY, &copy) < 0)
    {
        perror("stat");
        goto out;
    }
    fd = open(COPY, O_RDONLY);
    if (fd < 0)
    {
        perror("open copy");
        goto out;
    }
    if (fstat(fd, &opened) < 0)
    {
        perror("fstat");
        close(fd);
        goto out;
    }
    close(fd);

    failed = source.st_dev == copy.st_dev && source.st_ino == copy.st_ino;
    if (copy.st_dev != opened.st_dev || copy.st_ino != opened.st_ino)
        failed = 1;
    printf("%s: source=%X:%LX copy=%X:%LX opened=%X:%LX\n",
           failed ? "FAILED" : "PASSED",
           source.st_dev, source.st_ino,
           copy.st_dev, copy.st_ino,
           opened.st_dev, opened.st_ino);

out:
    unlink(SOURCE);
    unlink(COPY);

    return failed;
}
