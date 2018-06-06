#include <wctype.h>
#include <wchar.h>
#include <stdio.h>
#include <locale.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>


static int testSingleChar1(const char *pszMBCS, size_t cbMBCS, const wchar_t *pwcsFacit, size_t cwcFacit, const char *pszTest)
{
    int         i;
    int         rc = 0;
    mbstate_t   mbs;
    mbstate_t   mbsLen1;
    size_t      cbLeft = cbMBCS;
    memset(&mbs, 0, sizeof(mbs));
    memset(&mbsLen1, 0, sizeof(mbsLen1));

    for (i = 0; i < cwcFacit; i++)
    {
        size_t  cbLen1;
        size_t  cbLen2;

        /* convert */
        wchar_t wc = 0xfffe;
        size_t cb = mbrtowc(&wc, pszMBCS, cbLeft, &mbs);
        if ((int)cb < 0)
        {
            printf("error: Pos %d on test %s. mbrtowc -> %d errno=%d\n", i, pszTest, cb, errno);
            return rc + 1;
        }
        if (cb == 0 && i + 1 != cwcFacit)
        {
            printf("error: Pos %d on test %s. Early end!\n", i, pszTest);
            return rc + 1;
        }

        /* check */
        if (wc != *pwcsFacit)
        {
            printf("error: Pos %d on test %s. 0x%04x != 0x%04x\n", i, pszTest, wc, *pwcsFacit);
            rc++;
        }

        /* check length */
        cbLen1 = mbrlen(pszMBCS, cbLeft, NULL);
        if (cb != cbLen1)
        {
            printf("error: Pos %d on test %s. cb=%d mbrlen=%d (1)\n", i, pszTest, cb, cbLen1);
            rc++;
        }
        cbLen2 = mbrlen(pszMBCS, cbLeft, &mbsLen1);
        if (cb != cbLen2)
        {
            printf("error: Pos %d on test %s. cb=%d mbrlen=%d (2)\n", i, pszTest, cb, cbLen2);
            rc++;
        }

        /* advance */
        pwcsFacit++;
        pszMBCS += cb;
        cbLeft -= cb;
    }

    return rc;
}


static int testSingleChar2(const char *pszMBCS, size_t cbMBCS, const wchar_t *pwcsFacit, size_t cwcFacit, const char *pszTest)
{
    int         i;
    int         rc = 0;
    mbstate_t   mbs;
    size_t      cbLeft = cbMBCS;
    memset(&mbs, 0, sizeof(mbs));

    for (i = 0; i < cwcFacit; i++)
    {
        /* convert */
        char    ach[MB_LEN_MAX];
        size_t cb = wcrtomb(ach, *pwcsFacit, &mbs);
        if ((int)cb <= 0)
        {
            printf("error: Pos %d on test %s. wcrtomb -> %d errno=%d\n", i, pszTest, cb, errno);
            return rc + 1;
        }

        /* check */
        if (memcmp(ach, pszMBCS, cb))
        {
            printf("error: Pos %d on test %s. Mismatch! wc=0x%04x\n", i, pszTest, *pwcsFacit);
            rc++;
        }

        /* advance */
        pwcsFacit++;
        pszMBCS += cb;
        cbLeft -= cb;
    }

    return rc;
}


static int testString1(const char *pszMBCS, size_t cbMBCS, const wchar_t *pwcsFacit, size_t cwcFacit, const char *pszTest)
{
    int         rc = 0;
    mbstate_t   mbs;
    mbstate_t   mbsLen1;
    size_t      cb;
    size_t      cbLen1;
    size_t      cbLen2;
    wchar_t    *pwcsOut = alloca((cwcFacit + 4) * sizeof(wchar_t));
    const char *pszIn = pszMBCS;
    memset(&mbs, 0, sizeof(mbs));
    memset(&mbsLen1, 0, sizeof(mbsLen1));

    /* convert it as a full string. */
    pwcsOut[cwcFacit + 1] = 0xfffe;
    pwcsOut[cwcFacit + 0] = 0x3432;
    pwcsOut[cwcFacit + 1] = 0x3031;
    cb = mbsrtowcs(pwcsOut, &pszIn, cbMBCS, &mbs);
    if ((int)cb <= 0)
    {
        printf("error: String conversion (to wc) failed, testcase %s. cb=%d errno=%d\n", pszTest, cb, errno);
        return 1;
    }
    if (cb + 1 != cwcFacit)
    {
        printf("error: String conversion (to wc) testcase %s, length mismatch cb=%d cwcFacit=%d\n", pszTest, cb, cwcFacit);
        return 1;
    }
    if (pszIn)
    {
        printf("error: String conversion (to wc) testcase %s, incorrect stop pointer. pszIn=%p expected=NULL\n", pszTest, (void *)pszIn);
        return 1;
    }
    if (memcmp(pwcsOut, pwcsFacit, cwcFacit * sizeof(wchar_t)))
    {
        printf("error: String conversion (to wc) testcase %s, failed. Incorrect output!\n", pszTest);
        return 1;
    }
    if (    pwcsOut[cwcFacit + 0] != 0x3432
        ||  pwcsOut[cwcFacit + 1] != 0x3031)
    {
        printf("error: String conversion (to wc) testcase %s, failed. Wrote to much.\n", pszTest);
        return 1;
    }
    if (pwcsOut[cwcFacit - 1])
    {
        printf("error: String conversion (to wc) testcase %s, failed. not terminated.\n", pszTest);
        return 1;
    }

    /* lengths */
    pszIn = pszMBCS;
    cbLen1 = mbsrtowcs(NULL, &pszIn, cbMBCS, NULL);
    if (cbLen1 != cb)
    {
        printf("error: String length 1 mismatched for testcase %s. cbLen1=%d cb=%d\n", pszTest, cbLen1, cb);
        rc++;
    }

    pszIn = pszMBCS;
    cbLen2 = mbsrtowcs(NULL, &pszIn, cbMBCS, &mbsLen1);
    if (cbLen2 != cb)
    {
        printf("error: String length 2 mismatched for testcase %s. cbLen2=%d cb=%d\n", pszTest, cbLen2, cb);
        rc++;
    }

    return rc;
}

static int testString2(const char *pszMBCS, size_t cbMBCS, const wchar_t *pwcsFacit, size_t cwcFacit, const char *pszTest)
{
    int         rc = 0;
    mbstate_t   mbs;
    mbstate_t   mbsLen1;
    size_t      cb;
    size_t      cbLen1;
    size_t      cbLen2;
    const wchar_t *pwcsIn = pwcsFacit;
    char       *pszOut = (char *)alloca(cbMBCS + 4);
    memset(&mbs, 0, sizeof(mbs));
    memset(&mbsLen1, 0, sizeof(mbsLen1));

    /* convert it as a full string. */
    pszOut[cbMBCS - 1] = 0x7e;
    pszOut[cbMBCS + 0] = 0x34;
    pszOut[cbMBCS + 1] = 0x32;
    cb = wcsrtombs(pszOut, &pwcsIn, cbMBCS, &mbs);
    if ((int)cb <= 0)
    {
        printf("error: String conversion (to mb) failed, testcase %s. cb=%d errno=%d\n", pszTest, cb, errno);
        return 1;
    }
    if (cb + 1 != cbMBCS)
    {
        printf("error: String conversion (to mb) testcase %s, length mismatch cb=%d cbMBCS=%d\n", pszTest, cb, cbMBCS);
        return 1;
    }
    if (pwcsIn)
    {
        printf("error: String conversion (to mb) testcase %s, incorrect stop pointer. pwcsIn=%p expected=NULL\n", pszTest, (void *)pwcsIn);
        return 1;
    }
    if (memcmp(pszOut, pszMBCS, cbMBCS))
    {
        printf("error: String conversion (to mb) testcase %s, failed. Incorrect output!\n", pszTest);
        return 1;
    }
    if (    pszOut[cbMBCS + 0] != 0x34
        ||  pszOut[cbMBCS + 1] != 0x32)
    {
        printf("error: String conversion (to mb) testcase %s, failed. Wrote to much.\n", pszTest);
        return 1;
    }
    if (pszOut[cbMBCS - 1])
    {
        printf("error: String conversion (to mb) testcase %s, failed. Not terminated.\n", pszTest);
        return 1;
    }

    /* lengths */
    pwcsIn = pwcsFacit;
    cbLen1 = wcsrtombs(NULL, &pwcsIn, cbMBCS, NULL);
    if (cbLen1 != cb)
    {
        printf("error: String length 1 mismatched for testcase %s. cbLen1=%d cb=%d\n", pszTest, cbLen1, cb);
        rc++;
    }

    pwcsIn = pwcsFacit;
    cbLen2 = wcsrtombs(NULL, &pwcsIn, cbMBCS, &mbsLen1);
    if (cbLen2 != cb)
    {
        printf("error: String length 2 mismatched for testcase %s. cbLen2=%d cb=%d\n", pszTest, cbLen2, cb);
        rc++;
    }

    return rc;
}



static int test(const unsigned char *pszMBCS, size_t cbMBCS, const wchar_t *pwcsFacit, size_t cwcFacit, const char *pszTest)
{
    int rc = testSingleChar1((const char *)pszMBCS, cbMBCS, pwcsFacit, cwcFacit, pszTest);
    rc += testSingleChar2((const char *)pszMBCS, cbMBCS, pwcsFacit, cwcFacit, pszTest);
    rc += testString1((const char *)pszMBCS, cbMBCS, pwcsFacit, cwcFacit, pszTest);
    rc += testString2((const char *)pszMBCS, cbMBCS, pwcsFacit, cwcFacit, pszTest);
    return rc;
}


int main(void)
{
    /* -- ASSUMES wchar is unicode. */
    static const wchar_t        wcsTestUCS2[14]   = { 0x00E6, 0x00F8, 0x00E5, 0x0a,
                                                      0x00C6, 0x00D8, 0x00C5, 0x0a,
                                                      0x00E4, 0x00C4, 0x00F6, 0x00D6, 0x0a, 0};
    static const unsigned char  szTest850[14]     = { 0x91, 0x9b, 0x86, 0x0a,
                                                      0x92, 0x9d, 0x8f, 0x0a,
                                                      0x84, 0x8e, 0x94, 0x99, 0x0a, 0 }; /* "‘›†\n’\n„Ž”™\n" */
    static const unsigned char  szTest8859_1[14]  = { 0xe6, 0xf8, 0xe5, 0x0a,
                                                      0xc6, 0xd8, 0xc5, 0x0a,
                                                      0xe4, 0xc4, 0xf6, 0xd6, 0x0a, 0 }; /* "æøå\nÆØÅ\näÄöÖ\n" */
    static const unsigned char  szTestUTF_8[]     = { 0xc3, 0xa6, 0xc3, 0xb8, 0xc3, 0xa5, 0x0a,
                                                      0xc3, 0x86, 0xc3, 0x98, 0xc3, 0x85, 0x0a,
                                                      0xc3, 0xa4, 0xc3, 0x84, 0xc3, 0xb6, 0xc3, 0x96, 0x0a, 0 };
    int rc = 0;
    printf("mbstuff-1: TESTING\n");


    /*
     * The tests.
     */
    if (    setlocale(LC_CTYPE, "no_NO.IBM-850")
        ||  setlocale(LC_CTYPE, "no_NO.IBM850")
        ||  setlocale(LC_CTYPE, "no_NO.CP850")
        ||  setlocale(LC_CTYPE, "de_DE.CP850"))
    {
        rc += test(&szTest850[0], sizeof(szTest850), &wcsTestUCS2[0], sizeof(wcsTestUCS2) / sizeof(wcsTestUCS2[0]), "IBM-850");
    }
    else
        printf("warning! Couldn't set LC_CTYPE to any IBM-850 base locale. :/\n");

    if (    setlocale(LC_CTYPE, "en_US.ISO8859-1")
        ||  setlocale(LC_CTYPE, "en_US.ISO-8859-1")
        ||  setlocale(LC_CTYPE, "en_US.IBM-819")
        ||  setlocale(LC_CTYPE, "en_US.IBM-819"))
    {
        rc += test(&szTest8859_1[0], sizeof(szTest8859_1), &wcsTestUCS2[0], sizeof(wcsTestUCS2) / sizeof(wcsTestUCS2[0]), "ISO8859-1");
    }
    else
        printf("warning! Couldn't set LC_CTYPE to any ISO-8859-1 based locale. :/\n");

    if (    setlocale(LC_CTYPE, "en_US.UTF-8")
        ||  setlocale(LC_CTYPE, "en_US.UTF8")
        ||  setlocale(LC_CTYPE, "en_US.IBM1208")
        ||  setlocale(LC_CTYPE, "en_US.IBM-1208")
        ||  setlocale(LC_CTYPE, "ja_JP.UTF-8")
        ||  setlocale(LC_CTYPE, "ja_JP.UTF8"))
    {
        rc += test(&szTestUTF_8[0], sizeof(szTestUTF_8), &wcsTestUCS2[0], sizeof(wcsTestUCS2) / sizeof(wcsTestUCS2[0]), "UTF-8");
    }
    else
        printf("warning! Couldn't set LC_CTYPE to any UTF-8 based locale. :/\n");

    /* test summary. */
    if (!rc)
        printf("mbstuff-1: SUCCESS\n");
    else
        printf("mbstuff-1: FAILURE. %d errors\n", rc);
    return !!rc;
}
