#include <stdio.h>

int main()
{
    int i;
    for (i = 1; i < 20000; i++)
    {
        FILE *pFile = tmpfile();
        fclose(pFile);
        printf("%d ", i);
        fflush(stdout);
    }
    return 0;
}
