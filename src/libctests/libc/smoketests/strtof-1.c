#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>

int main(void)
{
    char *pszEnd = NULL;
    float th = strtof("-30", &pszEnd);
    if (th != -30.0)
    {
        printf("strtof-1: FAILURE (strtof -> %f)\n", th);
        return 1;
    }

    th = 0.0;
    sscanf("-30", "%f", &th);
    if (th != -30.0)
    {
        printf("strtof-1: FAILURE (sscanf -> %f)\n", th);
        return 1;
    }

    printf("strtof-1: SUCCESS\n");
    return 0;
}
