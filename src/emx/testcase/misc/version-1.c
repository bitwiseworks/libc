#include <stdio.h>

#include <features.h>
#if defined(__LIBCN_PREREQ_FULL)
#if __LIBCN_PREREQ_FULL(0, 1, 15)
#define HAVE_LIBCN_VERSION 1
#endif
#endif

#ifdef HAVE_LIBCN_VERSION
#include <libcn/version.h>
#else
#error Wrong LIBCn version
#endif

int main(void)
{
#ifdef HAVE_LIBCN_VERSION
    struct __libcn_version version = { sizeof(version) };

    if (__libcn_query_version(&version) != 0)
    {
        perror("__libcn_query_version");
        return 1;
    }

    printf("LIBC Next %u.%u.%u (struct size %u)\n",
        version.major, version.minor, version.build, version.cb);
#endif

    return 0;
}
