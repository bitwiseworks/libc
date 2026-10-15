/*
 * sys/socket.h must be usable in strict POSIX mode without BSD type aliases.
 */
#define _POSIX_SOURCE 1
#define _POSIX_C_SOURCE 200809L

#include <sys/socket.h>

int main(void)
{
    struct cmsghdr header;
    unsigned char *data = CMSG_DATA(&header);

    (void)data;
    return 0;
}
