/* this file is used as template for toupperlower-2. */
#ifndef _USE_CTYPE_INLINE_
# define _DONT_USE_CTYPE_INLINE_
#endif
#ifndef TESTCASENAME
# define TESTCASENAME    "toupperlower-1"
#endif

#include <stdio.h>
#include <locale.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>

static int tst(const char *pszLocale)
{
    static const char s_szLower[] = "abcdefghijklmnopqrstuvwxyz";
    static const char s_szUpper[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    unsigned cErrors = 0;
    unsigned i;

    if (pszLocale)
    {
        const char *psz = setlocale(LC_ALL, pszLocale);
        if (!psz)
        {
            printf(TESTCASENAME ": FAILURE: setlocale(LC_ALL, %s) -> %s errno=%d\n", pszLocale, psz, errno);
            return 1;
        }
    }

#define CHECK(expr) \
    do { \
        if (!(expr)) \
        { \
            printf(TESTCASENAME ": FAILURE: %s (i=%d '%c' locale=%s)\n", #expr, i, s_szLower[i], pszLocale); \
            cErrors++; \
        } \
    } while (0)

    /* simple upper/lower test */
    for (i = 0; i < sizeof(s_szLower) / sizeof(s_szLower[0]) - 1; i++)
    {
        CHECK(tolower(s_szLower[i]) == s_szLower[i]);
        CHECK(toupper(s_szUpper[i]) == s_szUpper[i]);
        CHECK(tolower(s_szUpper[i]) == s_szLower[i]);
        CHECK(toupper(s_szLower[i]) == s_szUpper[i]);
        CHECK(!isupper(s_szLower[i]));
        CHECK(isupper(s_szUpper[i]));
        CHECK(!islower(s_szUpper[i]));
        CHECK(islower(s_szLower[i]));
    }

#define CHECK2(expr, rcExpected) \
    do { \
        const int rc = expr; \
        if (rc != (rcExpected)) \
        { \
            printf(TESTCASENAME ": FAILURE: %s (rc=%d rcExpected=%d locale=%s)\n", #expr, rc, rcExpected, pszLocale); \
            cErrors++; \
        } \
    } while (0)

    /* some high values. */
    CHECK2(toupper(255), 255);
    CHECK2(tolower(255), 255);

#define CHECK3(expr) \
    do { \
        if (!(expr)) \
        { \
            printf(TESTCASENAME ": FAILURE: %s (i=%d locale=%s)\n", #expr, i, pszLocale); \
            cErrors++; \
        } \
    } while (0)

    /* match tolower/toupper with isupper/islower. */
    for (i = -128; i < 255; i++)
    {
        CHECK3(!isupper(i) || tolower(i) != i);
        CHECK3(!islower(i) || toupper(i) != i);
        CHECK3(toupper(i) == i || islower(i));
        CHECK3(tolower(i) == i || isupper(i));
        CHECK3(tolower(toupper(i)) == i);
        CHECK3(toupper(tolower(i)) == i);
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

