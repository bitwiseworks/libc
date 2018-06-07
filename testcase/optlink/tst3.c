#include <stdio.h>
#include <stdarg.h>
#include <string.h>

extern int _Optlink asmsprintf(char *, const char*,...);

long _Optlink OptSimple(int arg1, int arg2, int arg3, int arg4, int arg5)
{
    long  ret;
    if (arg1 != 0x11000011)
    {
        fprintf(stderr, "tst3: OptSimple: bad arg1 (%#x)\n", arg1);
        return -1;
    }
    if (arg2 != 0x22000022)
    {
        fprintf(stderr, "tst3: OptSimple: bad arg2 (%#x)\n", arg2);
        return -2;
    }
    if (arg3 != 0x33000033)
    {
        fprintf(stderr, "tst3: OptSimple: bad arg2 (%#x)\n", arg3);
        return -3;
    }
    if (arg4 != 0x44000044)
    {
        fprintf(stderr, "tst3: OptSimple: bad arg2 (%#x)\n", arg4);
        return -4;
    }
    if (arg5 != 0x55000055)
    {
        fprintf(stderr, "tst3: OptSimple: bad arg2 (%#x)\n", arg5);
        return -5;
    }

    return 0;
}

int _Optlink OptPrintf(const char *pszFormat, ...)
{
    va_list arg;
    char szMsg[128];

    va_start(arg, pszFormat);
    vsprintf(szMsg, pszFormat, arg);
    va_end(arg);
    if (strcmp(szMsg, "hello world\n"))
    {
        fprintf(stderr, "tst3: OptPrintf: bad result string '%s'", szMsg);
        return -1;
    }
    return 0;
}


int main(int argc, const char * const *argv)
{
    int     rc = 0;
    char    szBuffer[128];

    if (OptSimple(0x11000011, 0x22000022, 0x33000033, 0x44000044, 0x55000055))
    {
        printf("tst3: error: OptSimple failed\n");
        rc++;
    }

    if (OptPrintf("%s%c%s%s", "hello", ' ', "world", "\n"))
    {
        printf("tst3: error: OptPrintf failed\n");
        rc++;
    }

    asmsprintf(szBuffer, "%s%c%s%s", "hello", ' ', "world", "\n");
    if (strcmp(szBuffer, "hello world\n"))
    {
        printf("tst3: error: OptPrintf failed\n");
        rc++;
    }


    if (!rc)
        printf("tst3: Success\n");
    else
        printf("tst3: %d testcases failed\n");

    return rc;
}
