#include <stdio.h>
int main()
{
    printf("throw-3: TESTING\n");
    try
    {
        printf("throw-3: throw 7\n");
        throw 7;
        printf("throw-3: FAILURE\n");
        return 1;
    }
    catch (...)
    {
        printf("throw-3: SUCCESS - exception caught\n");
        return 0;
    }
}

