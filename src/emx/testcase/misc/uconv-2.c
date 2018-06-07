#include <stdio.h>
#include <string.h>
#include <uconv.h>

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

        /*
         * Test what NULL of pvOut yields.
         */
        UniChar ucsIn[2] = {0x31, 0x32};
        UniChar *pucsIn = &ucsIn[0];
        size_t  cucInLeft = 1;
        char    achOut[8] = {0};
        void   *pvOut = NULL;//&achOut[0];
        size_t  cbOutLeft = 1;//8;
        size_t  cSubst = 1;

        rc = UniUconvFromUcs(uobj, &pucsIn, &cucInLeft, &pvOut, &cbOutLeft, &cSubst);
        printf("UniUconvFromUcs -> %d\n", rc);
    }

    return 0;
}
