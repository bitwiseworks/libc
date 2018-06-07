#define sysctl _std_sysctl
#include <string.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include <errno.h>

int main()
{
    int cErrors = 0;

    {
        char sz[80] = {0};
        struct tm tm = {0};
        time_t t;
        int mib[] = {CTL_KERN, KERN_BOOTTIME};
        struct timeval j, now;
        size_t  cbOut = sizeof(j);
        errno = 0; \
        int     rc = sysctl(mib, 2, &j, &cbOut, NULL, 0);
        if (rc || errno) cErrors++;
        t= j.tv_sec;
        gmtime_r(&t, &tm);
        asctime_r(&tm, sz);
        printf("KERN_BOOTTIME: rc=%d errno=%d cbOut=%d - %lu.%.6ld (%s)\n", rc, errno, cbOut, j.tv_sec, j.tv_usec, sz);
        gettimeofday(&now, NULL);
        if (now.tv_sec < j.tv_sec)
        {
            printf("error: KERN_BOOTTIME is in the future!\n");
            cErrors++;
        }
        else if (j.tv_sec <= 0)
        {
            printf("error: KERN_BOOTTIME is before 1970!\n");
            cErrors++;
        }
    }

#define GETSTRING(a, b) \
    { \
    int mib[] = {a, b}; \
    char sz[256] = {0}; \
    size_t  cbOut = sizeof(sz); \
    errno = 0; \
    int     rc = sysctl(mib, 2, sz, &cbOut, NULL, 0); \
    if (rc || errno) cErrors++; \
    printf(#b ": rc=%d errno=%d cbOut=%d - %s\n", rc, errno, cbOut, sz); \
    }

#define GETINT(a, b, min, max) \
    { \
    int mib[] = {a, b}; \
    int     j; \
    size_t  cbOut = sizeof(j); \
    errno = 0; \
    int     rc = sysctl(mib, 2, &j, &cbOut, NULL, 0); \
    if (rc || errno) cErrors++; \
    printf(#b ": rc=%d errno=%d cbOut=%d - %d (%#x)\n", rc, errno, cbOut, j, j); \
    if (j < (min)) { printf("error:" #b ": %d is too small, min=%d.\n", j, min); cErrors++;} \
    if (j > (max)) { printf("error:" #b ": %d is too big, max=%d.\n", j, max); cErrors++;} \
    }

    GETSTRING(CTL_KERN, KERN_BOOTFILE)
    GETSTRING(CTL_HW, HW_MACHINE_ARCH)
    GETSTRING(CTL_HW, HW_MACHINE)
    GETSTRING(CTL_HW, HW_MODEL)
    GETINT(CTL_HW, HW_NCPU, 1, 127)

    if (!cErrors)
        printf("sysctl-1: SUCCESS\n");
    else
        printf("sysctl-1: FAILURE - %d errors\n", cErrors);
    return !!cErrors;
}
