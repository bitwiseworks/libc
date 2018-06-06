#include <stdio.h>
#include <string.h>
#include <uconv.h>

static int ToUcs(UconvObject uobj, const unsigned char *pach, size_t cch)
{
    void *pvIn = (void *)pach;
    size_t  cchInLeft = cch;
    UniChar ucsOut[1] = {0};
    UniChar *pucsOut = ucsOut;
    size_t  cbOutLeft = 1;
    size_t  cSubst = 0;

    int rc = UniUconvToUcs(uobj, &pvIn, &cchInLeft, &pucsOut, &cbOutLeft, &cSubst);
    if (!rc)
    {
        if (!cchInLeft)
        {
            switch (cch)
            {
                case 1: printf("%02x          - 0x%04x %6d\n", pach[0], ucsOut[0], ucsOut[0]);  break;
                case 2: printf("%02x %02x       - 0x%04x %6d\n", pach[0], pach[1], ucsOut[0], ucsOut[0]);  break;
                case 3: printf("%02x %02x %02x    - 0x%04x %6d\n", pach[0], pach[1], pach[2], ucsOut[0], ucsOut[0]);  break;
                case 4: printf("%02x %02x %02x %02x - 0x%04x %6d\n", pach[0], pach[1], pach[2], pach[3], ucsOut[0], ucsOut[0]);  break;
                default:
                    printf("huh! cch=%d", cch);
                    break;
            }
            return ucsOut[0];
        }
        else
        {
            switch (cch)
            {
                case 1: printf("%02x          - more than one!\n", pach[0]);  break;
                case 2: printf("%02x %02x       - more than one!\n", pach[0], pach[1]);  break;
                case 3: printf("%02x %02x %02x    - more than one!\n", pach[0], pach[1], pach[2]);  break;
                case 4: printf("%02x %02x %02x %02x - more than one!\n", pach[0], pach[1], pach[2], pach[3]);  break;
                default:
                    printf("huh! cch=%d", cch);
                    break;
            }
        }
    }
    return -1;
}

int main(int argc, char **argv)
{
    int argi;
    for (argi = 1; argi < argc; argi++)
    {
        /* convert argument */
        printf("argv[%i]=%s\n", argi, argv[argi]);
        UniChar             ucs[32];
        int                 i = strlen(argv[argi]);
        ucs[i] = '\0';
        while (i-- > 0)
            ucs[i] = argv[argi][i];

        /* Create */
        UconvObject         uobj = NULL;
        int rc = UniCreateUconvObject(ucs, &uobj);
        if (rc != ULS_SUCCESS)
        {
            printf("UniCreateUconvObject error: return code = %u\n", rc);
            return 1;
        }

        /* Query the conversion object */
        uconv_attribute_t   attr = {0};
        char achFirst[256] = {0};
        char achOther[256] = {0};
        udcrange_t aRanges[32] = {{0}};
        rc = UniQueryUconvObject(uobj, &attr,
                                 sizeof(uconv_attribute_t), achFirst,
                                 achOther, aRanges);
        if (rc != ULS_SUCCESS)
        {
            printf("UniQueryUconvObject error: return code = %u\n", rc);
            return 1;
        }

        /*
         * Dump data
         */
        const char *pszESDI;
        switch (attr.esid)
        {
            #define ESDI(a) case a: pszESDI = #a; break
            ESDI(ESID_sbcs_data);
            ESDI(ESID_sbcs_pc);
            ESDI(ESID_sbcs_ebcdic);
            ESDI(ESID_sbcs_iso);
            ESDI(ESID_sbcs_windows);
            ESDI(ESID_sbcs_alt);
            ESDI(ESID_dbcs_data);
            ESDI(ESID_dbcs_pc);
            ESDI(ESID_dbcs_ebcdic);
            ESDI(ESID_mbcs_data);
            ESDI(ESID_mbcs_pc);
            ESDI(ESID_mbcs_ebcdic);
            ESDI(ESID_ucs_2);
            ESDI(ESID_ugl);
            ESDI(ESID_utf_8);
            ESDI(ESID_upf_8);
            default:
                pszESDI = "unknown";
                break;

        }
        printf("version=%#lx mb_len={%d,%d} ucs_len={%d,%d} esid=%#06x (%s)\n",
               attr.version, attr.mb_min_len, attr.mb_max_len, attr.usc_min_len, attr.usc_max_len,
               attr.esid, pszESDI);

        printf("options=%#04x", attr.options);
        switch (attr.options)
        {
            case UCONV_OPTION_SUBSTITUTE_FROM_UNICODE: printf(" UCONV_OPTION_SUBSTITUTE_FROM_UNICODE"); break;
            case UCONV_OPTION_SUBSTITUTE_TO_UNICODE: printf(" UCONV_OPTION_SUBSTITUTE_TO_UNICODE"); break;
            case UCONV_OPTION_SUBSTITUTE_BOTH: printf(" UCONV_OPTION_SUBSTITUTE_BOTH"); break;
        }
        printf("\n");
        printf("state=%#04x ", attr.state);
        printf("endian={.src=%#x(%s), .trg=%#x(%s)}\n",
               attr.endian.source,
               attr.endian.source == ENDIAN_SYSTEM ? "SYSTEM"
               : attr.endian.source == ENDIAN_BIG ? "BIG"
               : attr.endian.source == ENDIAN_LITTLE ? "LITTLE" : "??",
               attr.endian.target,
               attr.endian.target == ENDIAN_SYSTEM ? "SYSTEM"
               : attr.endian.target == ENDIAN_BIG ? "BIG"
               : attr.endian.target == ENDIAN_LITTLE ? "LITTLE" : "??");
        printf("displaymask=%#010lx", attr.displaymask);
        if (attr.displaymask == DSPMASK_DATA)
            printf(" DSPMASK_DATA");
        else
        {
            if (attr.displaymask & DSPMASK_DISPLAY)   printf(" DSPMASK_DISPLAY");
            if (attr.displaymask & DSPMASK_TAB    )   printf(" DSPMASK_TAB");
            if (attr.displaymask & DSPMASK_LF     )   printf(" DSPMASK_LF");
            if (attr.displaymask & DSPMASK_CR     )   printf(" DSPMASK_CR");
            if (attr.displaymask & DSPMASK_CRLF   )   printf(" DSPMASK_CRLF");
        }
        printf("\n");

        printf("converttype=%#010lx", attr.converttype);
        if (attr.converttype & CVTTYPE_PATH)    printf(" CVTTYPE_PATH");
        if (attr.converttype & CVTTYPE_CDRA)    printf(" CVTTYPE_CDRA");
        if (attr.converttype & CVTTYPE_CTRL7F)  printf(" CVTTYPE_CTRL7F");
        printf("\n");

        printf("subchar_len=%#06x, subuni_len=%#06x\n", attr.subchar_len, attr.subuni_len);
        printf("subchar:");
        for (i = 0; i < attr.subchar_len; i++)
            printf(" %02x", (unsigned char)attr.subchar[i]);
        printf("\n");

        printf("subuni: ");
        for (i = 0; i < attr.subuni_len; i++)
            printf(" %04x", (unsigned short)attr.subuni[i]);
        printf("\n");

        /* ranges */
        for (i = 0; i < 32; i++)
            if (aRanges[i].first || aRanges[i].last)
                printf("range %d: %#x - %#x\n", i, aRanges[i].first, aRanges[i].last);


        /* multibyte */
        int iPrev = ~0;
        char ch = 0;
        for (i = 0; i < 256; i++)
        {
            if (achFirst[i] != ch)
            {
                if (iPrev != ~0)
                    printf("%#04x-%#04x %d\n", iPrev, i - 1, ch);
                iPrev = i;
                ch = achFirst[i];
            }
        }
        printf("%#04x-%#04x %d\n", iPrev < 0 ? 0 : iPrev, i - 1, ch);

        iPrev = ~0;
        ch = 0;
        for (i = 0; i < 256; i++)
        {
            if (achOther[i] != ch)
            {
                if (iPrev != ~0)
                    printf("2nd %#04x-%#04x %d\n", iPrev, i - 1, ch);
                if (!achOther[i])
                    iPrev = ~0;
                else
                    iPrev = i;
                ch = achOther[i];
            }
        }
        if (ch)
            printf("2nd %#04x-%#04x %d\n", iPrev < 0 ? 0 : iPrev, i - 1, ch);

        /*
         * Create ranges using the above info.
         */
        attr.options = 0;
        rc = UniSetUconvObject(uobj, &attr);
        if (rc) printf("Set -> %d\n", rc);

        unsigned char ach[8];
        switch (attr.mb_max_len)
        {
            case 1:
                printf("calc range: %d-%d\n", 0, 255);
                break;

            case 2:
                for (i = 0; i < 256; i++)
                {
                    ach[0] = i;
                    int ucs = ToUcs(uobj, &ach[0], 1);
                    if (achFirst[i] != 1)
                    {
                        int j;
                        for (j = 0; j < 256; j++)
                        {
                            ach[1] = j;
                            ucs = ToUcs(uobj, &ach[0], 2);
                        }
                    }
                }
                break;

            case 3:
                for (i = 0; i < 256; i++)
                {
                    ach[0] = i;
                    int ucs = ToUcs(uobj, &ach[0], 1);
                    if (achFirst[i] != 1)
                    {
                        int j;
                        for (j = 0; j < 256; j++)
                        {
                            ach[1] = j;
                            ucs = ToUcs(uobj, &ach[0], 2);
                            int k;
                            for (k = 0; k < 256; k++)
                            {
                                ach[2] = k;
                                ucs = ToUcs(uobj, &ach[0], 3);
                            }
                        }
                    }
                }
                break;

            case 4:
                for (i = 0; i < 256; i++)
                {
                    ach[0] = i;
                    int ucs = ToUcs(uobj, &ach[0], 1);
                    if (achFirst[i] != 1)
                    {
                        int j;
                        for (j = 0; j < 256; j++)
                        {
                            ach[1] = j;
                            ucs = ToUcs(uobj, &ach[0], 2);
                            int k;
                            for (k = 0; k < 256; k++)
                            {
                                ach[2] = k;
                                ucs = ToUcs(uobj, &ach[0], 3);
                                int l;
                                for (l = 0; l < 256; l++)
                                {
                                    ach[3] = l;
                                    ucs = ToUcs(uobj, &ach[0], 4);
                                }
                            }
                        }
                    }
                }
                break;


        }


        #if 0
        /*
         * Testconvert and calc the ranges ourselves.
         */
        attr.options = 0;
        rc = UniSetUconvObject(uobj, &attr);
        if (rc) printf("Set -> %d\n", rc);

        int iLast = ~0;
        for (i = 0; i <= 0xffff; i++)
        {
            UniChar ucsIn[2] = {i, 0};
            UniChar *pucsIn = &ucsIn[0];
            size_t  cucInLeft = 1;
            char    achOut[8] = {0};
            void   *pvOut = &achOut[0];
            size_t  cbOutLeft = 8;
            size_t  cSubst = 1;

            rc = UniUconvFromUcs(uobj, &pucsIn, &cucInLeft, &pvOut, &cbOutLeft, &cSubst);
            if (rc)
            {
                if (iLast != ~0)
                    printf("%#x-%#x ", iLast, i - 1);
                iLast = ~0;
            }
            else if (iLast == ~0)
                iLast = i;
        }
        if (iLast != ~0)
            printf("%#x-%#x ", iLast, i - 1);
        #endif
    }

    return 0;
}
