#include <process.h>
#include <sys/wait.h>
#include <stdio.h>
#include <string.h>


int main(int argc, char **argv)
{
    int i;
    pid_t pid = spawnvp(0, argv[1], &argv[1]);
    printf("pid=%d\n", pid);
    pid = wait(&i);
    printf("pid=%d i=%d\n", pid, i);
    return 0;
}
