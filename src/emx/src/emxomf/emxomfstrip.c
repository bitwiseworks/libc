/* $Id: emxomfstrip.c 3940 2014-11-10 20:16:06Z bird $ */
/** @file
 * emxstrip - Simple LX debug info stripping tool.
 *
 * @copyright   Copyright (C) 2014 knut st. osmundsen <bird-klibc-spam-xiv@anduin.net>
 * @licenses    MIT, BSD2, BSD3, BSD4, LGPLv2.1, LGPLv3, LGPLvFuture.
 */


/*******************************************************************************
* Header Files                                                                 *
*******************************************************************************/
#include "defs.h"
#include <errno.h>
#include <getopt.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/omflib.h>
#include <sys/emxload.h>
#if defined(_MSC_VER) || defined(__WATCOMC__)
# include <io.h>
# define ftruncate _chsize
#endif



/*******************************************************************************
*   Structures and Typedefs                                                    *
*******************************************************************************/
typedef struct EMXOMFSTRIPOPTS
{
    const char *pszInFile;
    const char *pszDbgFile;
    const char *pszOutFile;
    int         cVerbose;
} EMXOMFSTRIPOPTS;


static int error(const char *pszMsg, ...)
{
    va_list va;
    va_start(va, pszMsg);
    fprintf(stderr, "emxomfstrip: error: ");
    vfprintf(stderr, pszMsg, va);
    va_end(va);
    return 1;
}


static int copyFileBytes(FILE *pDst, const char *pszDstName, FILE *pSrc, const char *pszSrcName,
                         unsigned long offSrc, unsigned long cb)
{
    if (fseek(pDst, 0, SEEK_SET) != 0)
        return error("Seek error: %s\n", strerror(errno));
    if (fseek(pSrc, offSrc, SEEK_SET) != 0)
        return error("Seek error: %s\n", strerror(errno));
    while (cb > 0)
    {
        byte     abBuf[0x8000];
        unsigned cbThis = cb < sizeof(abBuf) ? (unsigned)cb : sizeof(abBuf);
        if (fread(abBuf, cbThis, 1, pSrc) != 1)
            return error("fread (@%#lx) failed on '%s': %s\n", offSrc, pszSrcName, strerror(errno));
        if (fwrite(abBuf, cbThis, 1, pDst) != 1)
            return error("fwrite failed on '%s': %s\n", pszDstName, strerror(errno));
        cb -= cbThis;
        offSrc += cbThis;
    }
    return 0;
}


static int isNbSignature(byte const *pbMagic)
{
    return pbMagic[0] == 'N'
        && pbMagic[1] == 'B'
        && pbMagic[2] >= '0' && pbMagic[2] <= '1'
        && pbMagic[3] >= '0' && pbMagic[3] <= '9';
}


static int isLxDebugInfoSignature(byte const *pbMagic)
{
    return isNbSignature(pbMagic);
}


/**
 * Strips a file.
 *
 * @returns 0 on success, 1 on failure, -1 if no debug info.
 * @param   pOpts           Options.
 * @param   pInFile         The input file.
 * @param   pOutFile        The output file. Can be the same as @a pInFile, can
 *                          also be NULL (no stripping).
 * @param   pDbgFile        Where to write the raw debug.
 */
static int stripFile(EMXOMFSTRIPOPTS *pOpts, FILE *pInFile, FILE *pOutFile, FILE *pDbgFile)
{
    struct stat     StIn;
    unsigned long   offLX  = 0;
    unsigned long   offDbg = 0;
    unsigned long   cbDbg  = 0;
    int             rc;
    union
    {
        struct exe1p2_header mz;
        struct os2_header lx;
        byte  abBuf[32];
    } uHdr;
    union
    {
        byte  abBuf[32];
        word  awBuf[32/2];
        dword adwBuf[32/4];
    } uBuf;

    if (fstat(fileno(pInFile), &StIn) != 0)
        return error("fstat failed on '%s': %s\n", pOpts->pszInFile, strerror(errno));

    /*
     * Parse the executable header.
     */
    if (fread(&uHdr.mz, sizeof(uHdr.mz), 1, pInFile) != 1)
        return error("Failed to read the header of '%s': %s\n", pOpts->pszInFile, strerror(errno));
    if (uHdr.mz.e_magic == EXE_MAGIC_MZ)
    {
        offLX = uHdr.mz.e_lfanew;
        if (fseek(pInFile, offLX, SEEK_SET) != 0)
            return error("Failed to seek to 2nd header of '%s': %s\n", pOpts->pszInFile, strerror(errno));
        if (fread(&uHdr.lx, sizeof(uHdr.lx), 1, pInFile) != 1)
            return error("Failed to read the LX header of '%s': %s\n", pOpts->pszInFile, strerror(errno));
        if (uHdr.lx.magic != EXE_MAGIC_LX)
            return error("Header of '%s' is not recognized (%#x)\n", pOpts->pszInFile, uHdr.lx.magic);
    }
    else if (uHdr.lx.magic == EXE_MAGIC_LX)
    {
        if (fread(&uHdr.mz + 1, sizeof(uHdr.lx) - sizeof(uHdr.mz), 1, pInFile) != 1)
            return error("Failed to read the header of '%s': %s\n", pOpts->pszInFile, strerror(errno));
    }
    else if (   uHdr.abBuf[0] == THEADR
             && (uHdr.abBuf[1] > 3 || uHdr.abBuf[2]) )
        return error("%s: Stripping OMF object files is not currently supported!\n", pOpts->pszInFile);
    else if (   uHdr.abBuf[0] == LIBHDR
             && (uHdr.abBuf[1] > 3 || uHdr.abBuf[2]) )
        return error("%s: Stripping OMF library files is not currently supported!\n", pOpts->pszInFile);
    else
        return error("Header of '%s' is not recognized (%#x)\n", pOpts->pszInFile, uHdr.mz.e_magic);

    /*
     * Is there a pointer to the debug info in the header? If so, check that
     * it is at the end of the file.
     */
    if (   uHdr.lx.debug_offset != 0
        && uHdr.lx.debug_size != 0 && 0)
    {
        offDbg = uHdr.lx.debug_offset;
        cbDbg  = uHdr.lx.debug_size;
        if (   offDbg > StIn.st_size
            || cbDbg > StIn.st_size
            || (off_t)offDbg + cbDbg > StIn.st_size)
            return error("Debug info indicated by the LX header of '%s' makes no sense! (%#lx + %#lx = %#lx (file: %#x)\n",
                         pOpts->pszInFile, offDbg, cbDbg, offDbg + cbDbg, (unsigned long)StIn.st_size);

        if (   fseek(pInFile, offDbg, SEEK_SET) != 0
            || fread(&uBuf, 4, 1, pInFile) != 1)
            return error("Error seeking/reading debug info header (@%#lx) in '%s': %s\n",
                         offDbg, pOpts->pszInFile, strerror(errno));
        if (!isLxDebugInfoSignature(uBuf.abBuf))
        {
            /* Try add the LX offset. */
            if (   fseek(pInFile, offDbg + offLX, SEEK_SET) == 0
                && fread(&uBuf.abBuf[4], 4, 1, pInFile) == 0
                && isLxDebugInfoSignature(&uBuf.abBuf[4]))
                offDbg += offLX;
            else
                return error("Debug info indicated by the LX header of '%s' is not recognized! (magic: %02x %02x %02x %02x)\n",
                             pOpts->pszInFile, uBuf.abBuf[0], uBuf.abBuf[1], uBuf.abBuf[2], uBuf.abBuf[3]);
        }

        if (offDbg + cbDbg != StIn.st_size) /* (This can be relaxed if we wish.) */
            return error("Debug info indicated by the LX header is not at the end of '%s'! (%#lx + %#lx = %#lx (file: %#x)\n",
                         pOpts->pszInFile, offDbg, cbDbg, offDbg + cbDbg, (unsigned long)StIn.st_size);
    }
    /*
     * Check for debug info at the end of the file.
     */
    else
    {
        if (fseek(pInFile, -8, SEEK_END) != 0)
            return error("Failed seeking in '%s': %s\n", pOpts->pszInFile, strerror(errno));
        if (fread(&uBuf, 8, 1, pInFile) != 1)
            return error("Error reading last 8 bytes of '%s': %s\n", pOpts->pszInFile, strerror(errno));
        if (isNbSignature(uBuf.abBuf))
        {
            int iVer = (uBuf.abBuf[2] - '0') * 10 + (uBuf.abBuf[3] - '0');
            if (iVer != 4 && iVer != 2)
                fprintf(stderr, "emxomfstrip: warning: Unknown debug info signature '%4.4s' in '%s'.\n",
                        uBuf.abBuf, pOpts->pszInFile);
            cbDbg = uBuf.adwBuf[1];
            if (   cbDbg >= (unsigned long)StIn.st_size
                || cbDbg <= 16)
                return error("Bad size in NB trailer debug of '%s': %#x (file size %#lx)\n",
                             pOpts->pszInFile, cbDbg, (unsigned long)StIn.st_size);
            offDbg = (unsigned long)StIn.st_size - cbDbg;

            if (   fseek(pInFile, offDbg, SEEK_SET) != 0
                || fread(&uBuf.abBuf[4], 4, 1, pInFile) != 1)
                return error("Error seeking/reading debug info header (@%#lx) in '%s': %s\n",
                             offDbg, pOpts->pszInFile, strerror(errno));
            if (uBuf.adwBuf[0] != uBuf.adwBuf[1])
                return error("Debug header does not match trailer in '%s': %02x %02x %02x %02x  !=   %02x %02x %02x %02x\n",
                             pOpts->pszInFile,
                             uBuf.abBuf[0], uBuf.abBuf[1], uBuf.abBuf[2], uBuf.abBuf[3],
                             uBuf.abBuf[4+0], uBuf.abBuf[4+1], uBuf.abBuf[4+2], uBuf.abBuf[4+3]);
        }
    }

    /*
     * Did we find anything to strip? If not return immediately indicating this.
     */
    if (cbDbg == 0)
        return -1;

    /*
     * Do we copy the debug info into a separate file first?
     */
    rc = 0;
    if (pDbgFile)
        rc = copyFileBytes(pDbgFile, pOpts->pszDbgFile, pInFile, pOpts->pszInFile, offDbg, cbDbg);

    /*
     * If pOutFile is NULL, we're only supposed to extract the debug info.
     */
    if (rc == 0 && pOutFile != NULL)
    {
        /*
         * If there is a separate output file, copy over the bytes that we should keep.
         */
        if (pOutFile != pInFile)
            rc = copyFileBytes(pOutFile, pOpts->pszOutFile, pInFile, pOpts->pszInFile, 0, offDbg);

        /*
         * Update the LX header in the output(/input) file.
         */
        if (   rc == 0
            && (   uHdr.lx.debug_offset != 0
                || uHdr.lx.debug_size != 0))
        {
            uHdr.lx.debug_offset    = 0;
            uHdr.lx.debug_size      = 0;
            uHdr.lx.loader_checksum = 0;
            if (fseek(pOutFile, offLX, SEEK_SET) != 0)
                return error("Error seeking to LX header in output file: %s\n", strerror(errno));
            if (fwrite(&uHdr.lx, sizeof(uHdr.lx), 1, pOutFile) != 1)
                return error("Error writing LX header of output file: %s\n", strerror(errno));
        }

        /*
         * Truncate the output file if it's the same as the input.
         */
        if (rc == 0 && pOutFile == pInFile)
        {
            if (ftruncate(fileno(pOutFile), offDbg) != 0)
                return error("Error truncating '%s': %s\n", pOpts->pszInFile, strerror(errno));
        }
    }

    return rc;
}


static int usage(FILE *pOut, int rc)
{
    fprintf(pOut, "emxomfstrip [-dgsSxXvV] [-D dbginfo.file] [-o output.file] <file1> [file2...]\n");
    return rc;
}


int main(int argc, char **argv)
{
    int ch;
    int rcExit = 0;
    EMXOMFSTRIPOPTS Opts = { NULL, NULL, NULL, 0 } ;

#ifdef __EMX__
    /* Preload the executable. */
    _emxload_env("GCCLOAD");
#endif

    /* Get more arguments from EMXOMFSTRIPOPT and load reponse files and expand wildcards. */
#ifdef __EMX__
    _envargs(&argc, &argv, "EMXOMFSTRIPOPT");
    _response(&argc, &argv);
    _wildcard(&argc, &argv);
#endif

    /*
     * Parse the options.
     */
    while ((ch = getopt_long(argc, argv, "dD:go:sSxXvV", NULL, NULL)) != EOF)
    {
        switch (ch)
        {
            case 'd':
            case 'g':
            case 's':
            case 'S':
            case 'x':
            case 'X':
                break;

            case 'D':
                Opts.pszDbgFile = optarg;
                break;

            case 'o':
                Opts.pszOutFile = optarg;
                break;

            case 'v':
                Opts.cVerbose++;
                break;

            case 'V':
                printf(VERSION VERSION_DETAILS "\n");
                return 0;

            default:
              return usage(stderr, 1);
        }
    }

    if (argc - optind == 0)
        return usage(stderr, 1);
    if (   argc - optind != 1
        && (Opts.pszDbgFile || Opts.pszOutFile))
        return error("exactly one file must be specified if -D or -o is used\n");

    /*
     * Process the files.
     */
    while (   optind < argc
           && rcExit == 0)
    {
        /* Open the input and maybe output file. */
        FILE *pInFile;
        Opts.pszInFile = argv[optind];
        if (Opts.pszOutFile)
            pInFile = fopen(Opts.pszInFile, "rb");
        else
            pInFile = fopen(Opts.pszInFile, "r+b");
        if (pInFile)
        {
            /* Open the output file if it differs. Special handling of /dev/null
               for facilitaing only extracting the debug info. */
            int   fNull = 0;
            FILE *pOutFile;
            if (!Opts.pszOutFile)
                pOutFile = pInFile;
            else
            {
                if (strcmp(Opts.pszOutFile, "/dev/null"))
                    pOutFile = fopen(Opts.pszOutFile, "wb");
                else
                {
                    fNull = 1;
                    pOutFile = NULL;
                }
            }
            if (pOutFile || fNull)
            {
                /* Open the debug file. */
                FILE *pDbgFile = NULL;
                if (Opts.pszDbgFile)
                    pDbgFile = fopen(Opts.pszDbgFile, "wb");
                if (pDbgFile || !Opts.pszDbgFile)
                {
                    /*
                     * All the files have been opened, get down to business.
                     */
                    int rc = stripFile(&Opts, pInFile, pOutFile, pDbgFile);
                    if (rc != 0 && rc != -1)
                        rcExit = rc;

                    /*
                     * Close the files. The user can clean up files on
                     * failure, at least for now.
                     */
                    if (pDbgFile && fclose(pDbgFile) != 0)
                        rcExit = error("fclose failed on '%s': %s\n", Opts.pszDbgFile, strerror(errno));
                    else if (rc == -1 && Opts.pszDbgFile != NULL && unlink(Opts.pszDbgFile) != 0)
                        rcExit = error("Failed to unlink debug info file '%s': %s\n", Opts.pszDbgFile, strerror(errno));
                }
                else
                    rcExit = error("Failed opening debug info file '%s': %s\n", Opts.pszDbgFile, strerror(errno));
                if (pOutFile && pOutFile != pInFile && fclose(pOutFile) != 0)
                    rcExit = error("fclose failed on '%s': %s\n", Opts.pszOutFile, strerror(errno));
            }
            else
                rcExit = error("Failed opening output file '%s': %s\n", Opts.pszOutFile, strerror(errno));
            if (fclose(pInFile) != 0)
                rcExit = error("fclose failed on '%s': %s\n", Opts.pszInFile, strerror(errno));
        }
        else
            rcExit = error("Failed opening '%s': %s\n", Opts.pszInFile, strerror(errno));
        optind++;
    }

    return rcExit;
}

