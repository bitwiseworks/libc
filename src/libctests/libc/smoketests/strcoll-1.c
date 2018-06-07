#include <stdio.h>
#include <locale.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>

#define TESTCASENAME    "strcoll-1"


static int tst(const char *pszLocale)
{
    char sz1[16];
    char sz2[16];
    unsigned i, j;
    int rc;
    int cErrors = 0;

    if (pszLocale)
    {
        const char *psz = setlocale(LC_ALL, pszLocale);
        if (!psz)
        {
            printf(TESTCASENAME ": FAILURE: setlocale(LC_ALL, %s) -> %s errno=%d\n", pszLocale, psz, errno);
            return 1;
        }
    }

    /* very basic test. */
    for (i = 0; i < 256; i++)
    {
        sz1[0] = sz2[0] = i;
        sz1[1] = sz2[1] = '\0';
        rc = strcoll(sz1, sz2);
        if (rc)
        {
            printf(TESTCASENAME ": FAILURE: %d != %d; rc=%d (locale=%s)\n", i, i, rc, pszLocale);
            return 1;
        }
    }

    /*  basic test. */
    for (i = 'A'; i <= 'Z' && cErrors < 32; i++)
    {
        sz2[0] = i;
        sz2[1] = sz1[1] = '\0';

        /* smaller */
        for (j = 'A'; j < i && cErrors < 32; j++)
        {
            sz1[0] = j;
            rc = strcoll(sz1, sz2);
            if (rc >= 0)
            {
                printf(TESTCASENAME ": FAILURE: %s >= %s; rc=%d (locale=%s)\n", sz1, sz2, rc, pszLocale);
                cErrors++;
            }
        }

        /* larger */
        for (j = i + 1; j <= 'Z' && cErrors < 32; j++)
        {
            sz1[0] = j;
            rc = strcoll(sz1, sz2);
            if (rc <= 0)
            {
                printf(TESTCASENAME ": FAILURE: %s <= %s; rc=%d (locale=%s)\n", sz1, sz2, rc, pszLocale);
                cErrors++;
            }
        }

    }

    for (i = 'a'; i <= 'z' && cErrors < 32; i++)
    {
        sz2[0] = i;
        sz2[1] = sz1[1] = '\0';

        /* smaller */
        for (j = 'a'; j < i && cErrors < 32; j++)
        {
            sz1[0] = j;
            rc = strcoll(sz1, sz2);
            if (rc >= 0)
            {
                printf(TESTCASENAME ": FAILURE: %s >= %s; rc=%d (locale=%s)\n", sz1, sz2, rc, pszLocale);
                cErrors++;
            }
        }

        /* larger */
        for (j = i + 1; j <= 'z' && cErrors < 32; j++)
        {
            sz1[0] = j;
            rc = strcoll(sz1, sz2);
            if (rc <= 0)
            {
                printf(TESTCASENAME ": FAILURE: %s <= %s; rc=%d (locale=%s)\n", sz1, sz2, rc, pszLocale);
                cErrors++;
            }
        }

    }

    return cErrors;
}

int main()
{
    int rc = 0;

    rc += tst(NULL);
    rc += tst("POSIX");
    rc += tst("C");
#ifdef OTHER
    rc += tst("en_US.IBM850");
#else
    rc += tst("en_US.ISO8859-1");
#endif
    rc += tst("C");
    rc += tst("en_US");
    rc += tst("de_DE");
    rc += tst("no_NO");

    /* .. done .. */
    if (!rc)
        printf(TESTCASENAME ": testcase executed successfully.\n");
    else
        printf(TESTCASENAME ": testcase failed. %d errors\n", rc);
    return !!rc;
}

