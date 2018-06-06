#include <wctype.h>
#include <wchar.h>
#include <stdio.h>
#include <locale.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <iconv.h>

int main(int argc, char **argv)
{
    iconv_t ic;
    wchar_t wc;
    const char *pszLocale = setlocale(LC_ALL, argv[1]);
    if (!pszLocale)
    {
        printf("setlocale failed errno=%d\n", errno);
        return 1;
    }
    printf("pszLocale=%s\n", pszLocale);

    ic = iconv_open("UCS-2",
#ifdef __IBMC__
                    strchr(pszLocale, '.') + 1
#else
                    ""
#endif
                    );
    if (ic == (iconv_t)-1)
    {
        printf("iconv_open failed errno=%d\n", errno);
        return 1;
    }

    for (wc = 0; wc < 0xffff; wc++)
    {
        char ach[7] = {0};
        int rc = wctomb(ach, wc);
        if (rc > 0)
        {
            unsigned char achUni[8] = {0};
            char   *pszIn = ach;
            size_t  cbInLeft = rc;
            char   *pszOut = (char *)&achUni[0];
            size_t  cbOutLeft = sizeof(achUni);
            int rc2 = iconv(ic, &pszIn, &cbInLeft, &pszOut, &cbOutLeft);
            *pszOut = 0;

            printf("wc: 0x%04x %6d - %2d -- mb: %02x %02x %02x -- %s %s %s %s -- UCS2: %2d - 0x%04x %6d\n",
                   wc, wc, rc, ach[0], ach[1], ach[2],
                   iswupper(wc) ? "U" : "u", iswlower(wc) ? "L" : "l", iswgraph(wc) ? "G" : "g", iswalpha(wc) ? "A" : "a",
                   rc2, *(int *)achUni, *(int *)achUni);
        }
    }

    return 0;
}
