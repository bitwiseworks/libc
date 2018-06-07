#define sysctl _std_sysctl
#include <string.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/time.h>
#include <stdio.h>
#include <errno.h>

int main()
{

    {
    int mib[] = {CTL_KERN, KERN_BOOTTIME};
    struct timeval j;
    size_t  cbOut = sizeof(j);
    int     rc = sysctl(mib, 2, &j, &cbOut, NULL, 0);
    printf("KERN_BOOTTIME: rc=%d errno=%d cbOut=%d - %d.%.6ld\n", rc, errno, cbOut, j.tv_sec, j.tv_usec);
    }

#define GETSTRING(a, b) \
    { \
    int mib[] = {a, b}; \
    char sz[256] = {0}; \
    size_t  cbOut = sizeof(sz); \
    int     rc = sysctl(mib, 2, sz, &cbOut, NULL, 0); \
    printf(#b ": rc=%d errno=%d cbOut=%d - %s\n", rc, errno, cbOut, sz); \
    }

#define GETINT(a, b) \
    { \
    int mib[] = {a, b}; \
    int     j; \
    size_t  cbOut = sizeof(j); \
    int     rc = sysctl(mib, 2, &j, &cbOut, NULL, 0); \
    printf(#b ": rc=%d errno=%d cbOut=%d - %d (%#x)\n", rc, errno, cbOut, j, j); \
    }

    GETSTRING(CTL_KERN, KERN_BOOTFILE)
    GETSTRING(CTL_HW, HW_MACHINE_ARCH)
    GETSTRING(CTL_HW, HW_MACHINE)
    GETSTRING(CTL_HW, HW_MODEL)
    GETINT(CTL_HW, HW_NCPU)


    return 0;
}
