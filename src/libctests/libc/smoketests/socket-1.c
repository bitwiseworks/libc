#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>

int main()
{
    char szBuf[256];
    const char *psz;
    const struct in_addr AddrIn = {0x0100007f};
    struct in_addr AddrOut;

    psz = inet_ntop(AF_INET, &AddrIn, szBuf, sizeof(szBuf));
    printf("1: %s\n", psz);

    int rc = inet_pton(AF_INET, "127.0.0.1", &AddrOut);
    printf("2: %d %08x\n", rc, *(unsigned *)(void *)&AddrOut);

    printf("3: h_errno=%d\n", h_errno);
    return 0;
}
