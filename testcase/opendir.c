#include <dirent.h>
#include <stdio.h>
#include <errno.h>

int ls(const char *pszDir)
{
    struct dirent * pdent;
    DIR *pDir = opendir(pszDir);
    if (!pDir)
    {
        printf("opendir failed on '%s'. errno=%d\n", pszDir, errno);
        return -1;
    }

    printf("Directory: %s\n", pszDir);
    while ((pdent = readdir(pDir)) != NULL)
    {
        printf("\t%ld\t%s\n", (long)pdent->d_size, pdent->d_name);
    }
    closedir(pDir);
    return 0;
}


int main()
{
    return ls(".");
}
