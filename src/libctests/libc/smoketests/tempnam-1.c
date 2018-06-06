#include <stdio.h>
#include <errno.h>
#include <string.h>

#define TESTCASENAME    "tempnam-1"


int main()
{
    int rc = 0;
    char *psz;

    errno = 0;
    psz = tempnam(NULL, NULL);
    if (psz && *psz)
        printf(TESTCASENAME ": tempnam(NULL, NULL) -> %s (errno=%d)\n", psz, errno);
    else
    {
        printf(TESTCASENAME ": FAILURE: tempnam(NULL, NULL) -> %p (%s) errno=%d\n", psz, psz, errno);
        rc++;
    }

    /* .. done .. */
    if (!rc)
        printf(TESTCASENAME ": testcase executed successfully.\n");
    else
        printf(TESTCASENAME ": testcase failed. %d errors\n", rc);
    return !!rc;
}

