/* $Id: stripomf.c 828 2003-10-10 23:38:11Z bird $
 *
 * OMF Debug Info stripper.
 *
 * Copyright (c) 2003 InnoTek Systemberatung GmbH
 * Author: knut st. osmundsen <bird@anduin.net>
 *
 *
 * This file is part of GCC.
 *
 * GCC is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * GCC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GCC; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __EMX__
#include <io.h>
#endif
#include <sys/omflib.h>
#include "defs.h"

/*******************************************************************************
*   Defined Constants And Macros                                               *
*******************************************************************************/
#ifndef min
#define min(a,b)    ((a) <= (b) ? (a) : (b))
#endif


/*******************************************************************************
*   Global Variables                                                           *
*******************************************************************************/
const char *pszPgm;



/**
 * Finds the base name in the filename.
 * @returns Pointer within pszFile.
 * @param   pszFile     Filename.
 */
static const char * mybasename(const char *pszFile)
{
    const char *psz;

    psz = strrchr(pszFile, '\\');
    if (!psz || strchr(psz, '/'))
        psz = strrchr(pszFile, '/');
    if (!psz || strchr(psz, ':'))
        psz = strrchr(pszFile, ':');
    if (psz)
        psz++;
    else
        psz = pszFile;
    return psz;
}


/**
 * Main function.
 * @returns 0 on success.
 * @returns 8 on error.
 * @param   pszFile     Name of file to strip.
 */
static int stripomf(const char *pszFile)
{
    FILE *      pIn;
    FILE *      pTmp;
    unsigned    cb;
    int         rc = 8;

    /*
     * Open input file and output file.
     */
    pIn = fopen(pszFile, "rb+");
    if (!pIn)
    {
        fprintf(stderr, "%s: failed to open file '%s'\n", pszPgm, pszFile);
        return 8;
    }

    if (    fseek(pIn, 0, SEEK_END)
        ||  (cb = ftell(pIn)) < 0
        ||  fseek(pIn, 0, SEEK_SET)
        )
    {
        fclose(pIn);
        fprintf(stderr, "%s: failed to determin size of file '%s'\n", pszPgm, pszFile);
        return 8;
    }


    pTmp = tmpfile();
    if (pTmp)
    {
        int     off = 0;
        int     iSegDef = 0;
        int     iName = 0;
        int     fPassThruPrev = 1;
        int     iNameTypes = -1;
        int     iNameSymbols = -1;
        int     iSegTypes = -1;
        int     iSegSymbols = -1;

        while (off < cb)
        {
            #pragma pack(1)
            static struct
            {
                struct omf_rec  hdr;
                char            ach[2048];
            } Rec;
            #pragma pack()
            int     fPassThru = 1;

            /* read header */
            if (fread(&Rec.hdr, sizeof(Rec.hdr), 1, pIn) != 1)
            {
                fprintf(stderr, "%s: Failed reading from file '%s'\n", pszPgm, pszFile);
                break;
            }

            if (Rec.hdr.rec_len > sizeof(Rec.ach))
            {
                fprintf(stderr, "%s: Record at offset %d in '%s' is too long\n", pszPgm, off, pszFile);
                break;
            }
            if (Rec.hdr.rec_type == LIBHDR)
            {
                fprintf(stderr, "%s: OMF libraries are not yet supported ('%s').\n", pszPgm, pszFile);
                break;
            }

            /* read the rest of the record. */
            if (fread(Rec.ach, Rec.hdr.rec_len, 1, pIn) != 1)
            {
                fprintf(stderr, "%s: Failed reading from file '%s'\n", pszPgm, pszFile);
                break;
            }

            fPassThru = 1;
            switch (Rec.hdr.rec_type)
            {
                /*
                 * Need to clear the state.
                 */
                case THEADR:
                    iNameTypes = iNameSymbols = iSegTypes = iSegSymbols = -1;
                    iName = iSegDef = 0;
                    break;

                /*
                 * Need to find the name table indexs of $$SYMBOLS and $$TYPES.
                 */
                case LNAMES:
                {
                    int offRec = 0;
                    while (offRec < Rec.hdr.rec_len - 1)
                    {
                        int cchStr = Rec.ach[offRec++];
                        iName++;
                        if (cchStr == 9 && !strncmp(&Rec.ach[offRec], "$$SYMBOLS", 9))
                            iNameSymbols = iName;
                        else if (cchStr == 7 && !strncmp(&Rec.ach[offRec], "$$TYPES", 7))
                            iNameTypes = iName;
                        offRec += cchStr;
                    }
                    break;
                }

                /*
                 * Need to find the name table indexs of $$SYMBOLS and $$TYPES.
                 */
                case SEGDEF:
                case SEGDEF|REC32:
                {
                    int iName;
                    int offRec = (Rec.ach[0] >> 5) ? 1 : 4;
                    offRec += (Rec.hdr.rec_type & REC32) ? 4 : 2;
                    /** @todo support name indexes higher than 127 */
                    iName = Rec.ach[offRec];
                    iSegDef++;
                    if (iName == iNameSymbols)
                    {
                        iSegSymbols = iSegDef;
                    }
                    else if (iName == iNameTypes)
                    {
                        iSegTypes = iSegDef;
                    }
                    break;
                }

                /*
                 * Linenumbers - no question.
                 */
                case LINNUM:
                case LINNUM|REC32:
                    fPassThru = 0;
                    break;

                /*
                 * LEDATA if it's $$SYMBOLS or $$TYPES.
                 */
                case LEDATA:
                case LEDATA|REC32:
                {   /* @todo indexes higher than 127. */
                    int iSeg = Rec.ach[0];
                    if (iSeg == iSegTypes || iSeg == iSegSymbols)
                        fPassThru = 0;
                    break;
                }

                /*
                 * If previous record was debug info this one is fixups to it
                 * and must be removed.
                 */
                case FIXUPP:
                case FIXUPP|REC32:
                    fPassThru = fPassThruPrev;
                    break;
            }

            if (    fPassThru
                &&  fwrite(&Rec, Rec.hdr.rec_len + sizeof(Rec.hdr), 1, pTmp) != 1)
            {
                fprintf(stderr, "%s: Failed to write to tempfile.\n", pszPgm);
                break;
            }

            off += Rec.hdr.rec_len + sizeof(Rec.hdr);
            fPassThruPrev = fPassThru;
        } /* read loop */


        /*
         * Did we succeed reading?
         */
        if (off >= cb)
        {
            /*
             * Now we must copy the result from the temp file to pszFile.
             */
            int cbToCopy;
            if (    (cbToCopy = ftell(pTmp)) >= 0
                &&  !fseek(pIn, 0, SEEK_SET)
                &&  !fseek(pTmp, 0, SEEK_SET))
            {
                int     cbNewSize = cbToCopy;
                while (cbToCopy > 0)
                {
                    static char achBuffer[0x10000];
                    int     cbRead = min(sizeof(achBuffer), cbToCopy);
                    cbRead = fread(achBuffer, 1, cbRead, pTmp);
                    if (cbRead <= 0)
                    {
                        fprintf(stderr, "%s: Failed reading from tempfile\n", pszPgm);
                        break;
                    }

                    if (fwrite(achBuffer, cbRead, 1, pIn) != 1)
                    {
                        fprintf(stderr, "%s: Failed to write to '%s'\n", pszPgm, pszFile);
                        break;
                    }

                    cbToCopy -= cbRead;
                } /* tmp -> file loop */

                /* success? */
                if (cbToCopy == 0)
                {
                    fflush(pIn);
                    _chsize(fileno(pIn), cbNewSize);
                    rc = 0;
                }
            }
            else
                fprintf(stderr, "%s: Failed to seek to start.\n", pszPgm);
        }

        fclose(pTmp);
    }
    else
        fprintf(stderr, "%s: failed to open temporary file\n", pszPgm);

    fclose(pIn);
    return rc;
}


/**
 * Show program usage.
 * @returns 8 (prefered program exist code).
 */
static int usage(void)
{
    fputs("stripomf " VERSION VERSION_DETAILS
          "Copyright (c) 2003 by InnoTek Systemberatung GmbH\n" VERSION_COPYRIGHT "\n\n"
          "Usage: stripomf <input_file>\n\n",
          stderr);
    return 8;
}


int main(int argc, char **argv)
{
    int argi;

    pszPgm = mybasename(argv[0]);
    _response (&argc, &argv);
    _wildcard (&argc, &argv);

#ifdef __EMX__
    if (_isterm (1))
        setvbuf (stdout, NULL, _IOLBF, BUFSIZ);
    else
        setvbuf (stdout, NULL, _IOFBF, BUFSIZ);
#endif /* __EMX__ */


    /*
     * Arguments
     */
    for (argi = 1; argi < argc; argi++)
    {
        if (argv[argi][0] == '-')
        {
            /* option!*/
            return usage();
        }
        else
        {
            int rc = stripomf(argv[argi]);
            if (rc != 0)
                return rc;
        }
    }

    return 0;
}
