#define INCL_BASE
#include <os2.h>
#include <stdio.h>


int main(int argc, char **argv)
{
    const char *pszFilename = "./tstfile";
    unsigned cbSize = 0;
    if (argc >= 2)
        cbSize = atol(argv[1]);
    if (!cbSize)
        cbSize = 1024*1024*1024;
    if (argc >= 3)
        pszFilename = argv[2];

    ULONG ulAction;
    HFILE hFile;
    int rc = DosOpen(pszFilename, &hFile, &ulAction, cbSize / 2, FILE_NORMAL,
                     OPEN_ACTION_CREATE_IF_NEW | OPEN_ACTION_REPLACE_IF_EXISTS,
                     OPEN_FLAGS_NOINHERIT | OPEN_SHARE_DENYNONE | OPEN_ACCESS_READWRITE, NULL);
    if (!rc)
    {
        static char achBuf[0xff00];
        ULONG off = 0;
        unsigned cNonZero = 0;
        printf("info: successfully create file %s with %d bytes size\n", pszFilename, cbSize);
        while (off < cbSize / 2)
        {
            char *pch;
            ULONG cbRead = 0;
            rc = DosRead(hFile, &achBuf, sizeof(achBuf), &cbRead);
            if (rc)
            {
                printf("DosRead -> %d, off=%d\n", rc);
                break;
            }
            if (!cbRead && !off)
            {
                printf("Using samba. DosOpen(,,,size/2,,) didn't set the filesize it seems...\n");
                break;
            }

            pch = achBuf;
            while (pch < &achBuf[cbRead])
            {
                if (*pch)
                {
                    printf(".");
                    cNonZero++;
                }
                pch++;
            }

            off += cbRead;
        }
        printf("info: checked %d bytes, found %d non-zero bytes\n", off, cNonZero);

        rc = DosSetFileSize(hFile, cbSize);
        if (!rc)
        {
            printf("info: expanded the file to %d bytes\n", cbSize);
            LONG lPos = 0;
            DosSetFilePtr(hFile, 0, FILE_BEGIN, &lPos);
            off = 0;
            cNonZero = 0;
            while (off < cbSize)
            {
                char *pch;
                ULONG cbRead = 0;
                rc = DosRead(hFile, &achBuf, sizeof(achBuf), &cbRead);
                if (rc)
                {
                    printf("DosRead -> %d, off=%d\n", rc);
                    break;
                }

                pch = achBuf;
                while (pch < &achBuf[cbRead])
                {
                    if (*pch)
                    {
                        printf(".");
                        cNonZero++;
                    }
                    pch++;
                }

                off += cbRead;
            }
            printf("info: checked all %d bytes, found %d non-zero bytes\n", off, cNonZero);

            /* fill the file with non-zero bits */
            memset(achBuf, 0xab, sizeof(achBuf));
            DosSetFilePtr(hFile, 0, FILE_BEGIN, &lPos);
            while (off < cbSize)
            {
                ULONG cbWritten = 0;
                rc = DosWrite(hFile, &achBuf, sizeof(achBuf), &cbWritten);
                if (rc)
                {
                    printf("DosRead -> %d, off=%d\n", rc);
                    break;
                }
                off += cbWritten;
            }
            printf("info: filled %d with 0xab\n", off);
        }

        DosClose(hFile);
        DosDelete(pszFilename);
    }
    else
        printf("error: failed to open '%s', rc=%d\n", pszFilename, rc);

    return 0;
}
