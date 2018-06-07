#include <wctype.h>
#include <wchar.h>
#include <stdio.h>
#include <locale.h>
#include <errno.h>
#include <string.h>

#define TESTCASENAME    "wcscoll-1"


static int tst(const char *pszLocale)
{
    wchar_t wsz1[16];
    wchar_t wsz2[16];
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
    for (i = 0; i < 127; i++)
    {
        wsz1[0] = wsz2[0] = i;
        wsz1[1] = wsz2[1] = '\0';
        rc = wcscoll(wsz1, wsz2);
        if (rc)
        {
            printf(TESTCASENAME ": FAILURE: %d != %d; rc=%d (locale=%s)\n", i, i, rc, pszLocale);
            return 1;
        }
    }

    /*  basic test. */
    for (i = 'A'; i <= 'Z' && cErrors < 32; i++)
    {
        wsz2[0] = i;
        wsz2[1] = wsz1[1] = '\0';

        /* smaller */
        for (j = 'A'; j < i && cErrors < 32; j++)
        {
            wsz1[0] = j;
            rc = wcscoll(wsz1, wsz2);
            if (rc >= 0)
            {
                printf(TESTCASENAME ": FAILURE: %ls >= %ls; rc=%d (locale=%s)\n", wsz1, wsz2, rc, pszLocale);
                cErrors++;
            }
        }

        /* larger */
        for (j = i + 1; j <= 'Z' && cErrors < 32; j++)
        {
            wsz1[0] = j;
            rc = wcscoll(wsz1, wsz2);
            if (rc <= 0)
            {
                printf(TESTCASENAME ": FAILURE: %ls <= %ls; rc=%d (locale=%s)\n", wsz1, wsz2, rc, pszLocale);
                cErrors++;
            }
        }

    }

    for (i = 'a'; i <= 'z' && cErrors < 32; i++)
    {
        wsz2[0] = i;
        wsz2[1] = wsz1[1] = '\0';

        /* smaller */
        for (j = 'a'; j < i && cErrors < 32; j++)
        {
            wsz1[0] = j;
            rc = wcscoll(wsz1, wsz2);
            if (rc >= 0)
            {
                printf(TESTCASENAME ": FAILURE: %ls >= %ls; rc=%d (locale=%s)\n", wsz1, wsz2, rc, pszLocale);
                cErrors++;
            }
        }

        /* larger */
        for (j = i + 1; j <= 'z' && cErrors < 32; j++)
        {
            wsz1[0] = j;
            rc = wcscoll(wsz1, wsz2);
            if (rc <= 0)
            {
                printf(TESTCASENAME ": FAILURE: %ls <= %ls; rc=%d (locale=%s)\n", wsz1, wsz2, rc, pszLocale);
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

