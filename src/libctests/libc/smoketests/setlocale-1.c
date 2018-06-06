#include <stdio.h>
#include <locale.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>

#define TESTCASENAME    "setlocale"


static int tst(const char *pszLocale)
{
#ifdef OTHER
    static char szTest[] = "‘›†\n’\n„Ž”™\n"; /* 850 */
#else
    static char szTest[] =   "æøå\nÆØÅ\näÄöÖ\n"; /* 8859-1 */
#endif
    char   *psz;

    psz = setlocale(LC_ALL, pszLocale);
    printf("setlocale(LC_ALL, %s) -> %s  errno=%d\n", pszLocale, psz, errno);
    for (psz = szTest; *psz; psz++)
    {
        if (*psz == '\n')
            printf("\n");
        else
        {
            printf("'%c'(%02x): %d,%d,%d%s",
                   *psz, (unsigned char)*psz, !!isalpha(*psz), !!isupper(*psz), !!islower(*psz), psz[1] ? " " : "");

        }
    }
    return 0;
}

static int tst2(void)
{
    char   *psz1;
    char   *psz2;
    int     rc = 0;

    printf(TESTCASENAME "\n");

    setlocale(LC_ALL, "en_US");
    setlocale(LC_CTYPE, "de_DE");

    psz1 = setlocale(LC_ALL, NULL);
    psz2 = setlocale(LC_CTYPE, NULL);
    if (psz1 && psz2 && strcmp(psz1, psz2))
        printf("LC_ALL != LC_CTYPE - ok (%s != %s) errno=%d\n", psz1, psz2, errno);
    else
    {
        printf("error: LC_ALL == LC_CTYPE - %s errno=%d\n", psz1, errno);
        rc++;
    }

    return rc;
}


static int tst3(void)
{
    char   *psz1;
    char   *psz2;
    int     rc = 0;

    setlocale(LC_ALL, "en_US");
    psz1 = setlocale(LC_CTYPE, "de_DE");
    psz2 = setlocale(LC_CTYPE, "de_DE");
    if (psz1 && psz2 && !strcmp(psz1, psz2))
        printf("2nd setlocale returns the same. %s errno=%d\n", psz1, errno);
    else
    {
        printf("error: 2nd setlocale returns differntly. %s != %s errno=%d\n", psz1, psz2, errno);
        rc++;
    }

    return rc;
}


int main()
{
    int rc = 0;

    rc += tst("");
    rc += tst("POSIX");
#ifdef OTHER
    rc += tst("en_US.IBM850");
#else
    rc += tst("en_US.ISO8859-1");
#endif
    rc += tst("C");
    rc += tst("en_US");
    rc += tst2();
    rc += tst3();

    /* .. done .. */
    if (!rc)
        printf(TESTCASENAME ": testcase executed successfully.\n");
    else
        printf(TESTCASENAME ": testcase failed. %d failed\n", rc);
    return !!rc;
}

