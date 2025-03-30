// UNIX Source downloaded from http://www.lipman.org/software/sfw/
// 2002-Aug Joe Nord     Ported to Windows 32 bit command line execution
// 2011-Oct Joe Nord     Added wildcard support for Windows command line
// 2016-Jan Joe Nord     Correct left-right mirroring and rotate image 180 degrees
// 2016-Apr Joe Nord     Implement support for 93A format.  Reference:
//                       http://jonesrh.info/sfw/sfw_sfw93a_details.html
// 2016-Sep Joe Nord     Implement support for Overlake Photo Express .PIC files
// 2023-Jan Joe Nord     Create EXIF Header and set file CreateDate to match the
//                       date information stored in SFW file (date of develop)
// 2024-Jan Joe Nord     Write date information only for 94A format files and recode header write

/*
 * sfwjpg.c
 *
 * Copyright (c) 1997-1999  Everett Lipman
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * The GNU General Public License is available at
 *
 *    http://www.fsf.org/copyleft/gpl.html
 *
 * Alternatively, you can write to the
 *
 *    Free Software Foundation, Inc.
 *    59 Temple Place - Suite 330
 *    Boston, MA  02111-1307, USA
 *
 *
 * Revision History:
 *
 *     22 Sep 1997  EAL Completed first version.
 *      9 Jan 1999  EAL Changed to forward scanning for the end of
 *                      JFIF data, as suggested by Bo Lindbergh.
 *      9 May 1999  EAL Unknown markers now result in warning, not
 *                      failure.  Unknown markers will be converted
 *                      to COM (0xfe) marker.  Added code to deal with
 *                      SFW DHT marker (0xa4).  Added output options
 *                      suggested by John Oppenheimer.
 */

// Mask errors regarding unsafe string functions
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#include <windows.h>

#include "util.h"
#include "sfw94a.h"

typedef enum
{
    SIF_UNKNOWN,
    SIF_93A,         // Seattle Filmworks original format
    SIF_94A,         // Seattle Filmworks tag obfuscated format SFW
    SIF_95B,         // Seattle Filmworks PhotoWorks format PWP
    SIF_PIC          // Overlake Photo Express PIC
} SOURCEIMAGEFORMAT;

#define USCH unsigned char

#define HUFFSIZE 420

USCH hufftbl[HUFFSIZE] = {
    0xFF,0xC4,0x01,0xA2,0x00,0x00,0x01,0x05,0x01,0x01,0x01,0x01,0x01,0x01,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,
    0x0B,0x01,0x00,0x03,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x00,0x00,0x00,
    0x00,0x00,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x10,0x00,
    0x02,0x01,0x03,0x03,0x02,0x04,0x03,0x05,0x05,0x04,0x04,0x00,0x00,0x01,0x7D,0x01,
    0x02,0x03,0x00,0x04,0x11,0x05,0x12,0x21,0x31,0x41,0x06,0x13,0x51,0x61,0x07,0x22,
    0x71,0x14,0x32,0x81,0x91,0xA1,0x08,0x23,0x42,0xB1,0xC1,0x15,0x52,0xD1,0xF0,0x24,
    0x33,0x62,0x72,0x82,0x09,0x0A,0x16,0x17,0x18,0x19,0x1A,0x25,0x26,0x27,0x28,0x29,
    0x2A,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4A,
    0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5A,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6A,
    0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7A,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8A,
    0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9A,0xA2,0xA3,0xA4,0xA5,0xA6,0xA7,0xA8,
    0xA9,0xAA,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,0xBA,0xC2,0xC3,0xC4,0xC5,0xC6,
    0xC7,0xC8,0xC9,0xCA,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,0xE1,0xE2,0xE3,
    0xE4,0xE5,0xE6,0xE7,0xE8,0xE9,0xEA,0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,0xF9,
    0xFA,0x11,0x00,0x02,0x01,0x02,0x04,0x04,0x03,0x04,0x07,0x05,0x04,0x04,0x00,0x01,
    0x02,0x77,0x00,0x01,0x02,0x03,0x11,0x04,0x05,0x21,0x31,0x06,0x12,0x41,0x51,0x07,
    0x61,0x71,0x13,0x22,0x32,0x81,0x08,0x14,0x42,0x91,0xA1,0xB1,0xC1,0x09,0x23,0x33,
    0x52,0xF0,0x15,0x62,0x72,0xD1,0x0A,0x16,0x24,0x34,0xE1,0x25,0xF1,0x17,0x18,0x19,
    0x1A,0x26,0x27,0x28,0x29,0x2A,0x35,0x36,0x37,0x38,0x39,0x3A,0x43,0x44,0x45,0x46,
    0x47,0x48,0x49,0x4A,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5A,0x63,0x64,0x65,0x66,
    0x67,0x68,0x69,0x6A,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7A,0x82,0x83,0x84,0x85,
    0x86,0x87,0x88,0x89,0x8A,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9A,0xA2,0xA3,
    0xA4,0xA5,0xA6,0xA7,0xA8,0xA9,0xAA,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,0xBA,
    0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,
    0xD9,0xDA,0xE2,0xE3,0xE4,0xE5,0xE6,0xE7,0xE8,0xE9,0xEA,0xF2,0xF3,0xF4,0xF5,0xF6,
    0xF7,0xF8,0xF9,0xFA
};

USCH markertbl[256];


// Overlake Photo Express .PIC files when converted to JPG need a DQT table added (FF DB)

USCH overlakephotoDQTtable[0x86] =
{
    0xFF,0xDB,  // DQT marker
    0x00, 0x84, // Size less 2
    0x00, 0x07, 0x04, 0x05, 0x06, 0x05, 0x04, 0x07, 0x06, 0x05, 0x06, 0x07, 0x07, 0x07, 0x08, 0x0A,
    0x11, 0x0B, 0x0A, 0x09, 0x09, 0x0A, 0x15, 0x0F, 0x10, 0x0C, 0x11, 0x19, 0x16, 0x1A, 0x1A, 0x18,
    0x16, 0x18, 0x18, 0x1C, 0x1F, 0x28, 0x22, 0x1C, 0x1D, 0x26, 0x1E, 0x18, 0x18, 0x23, 0x2F, 0x23,
    0x26, 0x29, 0x2A, 0x2D, 0x2D, 0x2D, 0x1B, 0x21, 0x31, 0x34, 0x31, 0x2B, 0x34, 0x28, 0x2C, 0x2D,
    0x2B, 0x01, 0x07, 0x07, 0x07, 0x0A, 0x09, 0x0A, 0x14, 0x0B, 0x0B, 0x14, 0x2B, 0x1C, 0x18, 0x1C,
    0x2B, 0x2B, 0x2B, 0x2B, 0x2B, 0x2B, 0x2B, 0x2B, 0x2B, 0x2B, 0x2B, 0x2B, 0x2B, 0x2B, 0x2B, 0x2B,
    0x2B, 0x2B, 0x2B, 0x2B, 0x2B, 0x2B, 0x2B, 0x2B, 0x2B, 0x2B, 0x2B, 0x2B, 0x2B, 0x2B, 0x2B, 0x2B,
    0x2B, 0x2B, 0x2B, 0x2B, 0x2B, 0x2B, 0x2B, 0x2B, 0x2B, 0x2B, 0x2B, 0x2B, 0x2B, 0x2B, 0x2B, 0x2B,
    0x2B, 0x2B
};

int sfw_to_jfif(USCH *sfwstart, USCH *sfwend, SOURCEIMAGEFORMAT sourceimageformat, char *outfilename);
USCH *forward_scan(USCH *start, USCH *stop, USCH *goal, int length);
int fix_marker(USCH* marker, SOURCEIMAGEFORMAT sourceimageformat);
long read_skip_length(USCH *marker);

SOURCEIMAGEFORMAT getformat(USCH *filebuf, size_t len)
{
    SOURCEIMAGEFORMAT ret = SIF_UNKNOWN;
    char *pbuff = (char *)filebuf;

    if (len > 5)
    {
        if (strncmp(pbuff, "SFW93", 5) == 0)
        {
            ret = SIF_93A;
        }
        else if (strncmp(pbuff, "SFW94", 5) == 0)
        {
            ret = SIF_94A;
        }
        else if (strncmp(pbuff, "SFW95", 5) == 0)
        {
            ret = SIF_95B;
        }
        else if (strncmp(pbuff, "BM", 2) == 0)
        {
            // First 16 bytes of all sample Overlake Photo Files are same
            // 424D0000 00000000 - 00005204 00004400  *BM........R...D.*
            ret = SIF_PIC;
        }
    }

    return ret;
}


// Returned filename string is allocated by function
// Caller must release the memory using delete[]
char *GetOutputFileName(char* infilename, size_t cnt)
{
    char* outfilename = NULL;
    char* pdot = NULL;
    size_t namelen = 0;

    namelen = strlen(infilename);
    outfilename = new char[namelen + 1 + 4 + 4];  // nul, -nnn, .ext

    strcpy(outfilename, infilename);

    pdot = strrchr(outfilename, '.');
    if (!pdot)
    {
        // no file extension
        pdot = outfilename + namelen;
    }

    if (_stricmp(pdot, ".pwp") == 0)
    {
        sprintf(pdot, "-%02d.jpg", cnt);
    }
    else if (_stricmp(pdot, ".sfw") == 0 || // 94A files have sfw extention on input
        _stricmp(pdot, ".pic") == 0)        // Overlake Photo Express
    {
        // Output file same name as input file, with jpg extension
        strcpy(pdot, ".jpg");
    }
    else
    {
        // Probably 93A format where input filenames have this format
        //    rollnumber.#nn    where nn is the frame number
        // Since rollnumber is same for all shots, and only extension changes,
        // keep the #nn and append jpg extension
        strcat(outfilename, ".jpg");
    }
    return outfilename;
}

// Produce ONE output file based upon the given "file" input.
// For SFW and normal files, there is a 1:1 relation to file on disk to disk images.
// PWP files have multiple image "files" inside the PWP file (like ZIP).
// This function produces ONE output based upon the input and position data in the file.
int ConvertWorker (USCH* sfwstart, USCH* sfwend, SOURCEIMAGEFORMAT sif, char* outfilename)
{
    int retval = 0;
    char* pszDateFilmDeveloped = NULL;

    // Read out the date the film was developed (pull from SFW file in memory)
    if (sif == SIF_94A)
    {
        // This variable is allocated and we are responsible for delete[]
        pszDateFilmDeveloped = get_date_from_sfw(sfwstart, sfwend);
        //printf("DateDeveloped:%s", pszDateFilmDeveloped);
    }

    retval = sfw_to_jfif(sfwstart, sfwend, sif, outfilename);

    if (retval == 0)
    {
        // 20-Jan-2016 For SFW94A unmirror the output and rotate 180 degrees
        if (sif == SIF_94A)
        {
            retval = fix_mirroring(outfilename);
            if (retval != 0)
            {
                fprintf(stderr, "Un-mirror failed: %s.\n\n", outfilename);
            }
            else
            {
                if (pszDateFilmDeveloped)
                {
                    retval = add_develop_date_to_jpg(outfilename, pszDateFilmDeveloped);
                    if (retval != 0)
                    {
                        fprintf(stderr, "Add date information failed: %s.\n\n", outfilename);
                    }
                }
            }
        }
    }

    if (pszDateFilmDeveloped)
    {
        delete[] pszDateFilmDeveloped;
        pszDateFilmDeveloped = NULL;
    }

    return (retval);
}


int ReadSfwConvertToJpg (char *infilename)
{
    int retval = 0;
    size_t sretval;
    char mesg[256];
    struct stat filestat;
    char *outfilename = NULL;
    USCH *filebuf;
    FILE *infile;
    SOURCEIMAGEFORMAT sourceimageformat = SIF_UNKNOWN;

    /*** read entire source file ***********************************************/

    retval = _stat(infilename, (struct _stat*)&filestat);
    if (retval == -1)
    {
        sprintf(mesg, "Error getting status for file '%s'", infilename);
        fprintf(stderr, "\n");
        perror(mesg);
        fprintf(stderr, "\n");
        return(1);
    }

    filebuf = (USCH*)malloc((size_t)filestat.st_size);
    if (filebuf == NULL)
    {
        fprintf(stderr, "\n");
        perror("Error allocating memory for filebuf");
        fprintf(stderr, "\n");
        return(1);
    }

    infile = fopen(infilename, "rb");
    if (infile == NULL)
    {
        sprintf(mesg, "Error opening file '%s'", infilename);
        fprintf(stderr, "\n");
        perror(mesg);
        fprintf(stderr, "\n");
        return(1);
    }

    sretval = fread(filebuf, filestat.st_size, 1, infile);
    if (sretval == 0)
    {
        sprintf(mesg, "Error reading file '%s'", infilename);
        fprintf(stderr, "\n");
        perror(mesg);
        fprintf(stderr, "\n");
        return(1);
    }
    fclose(infile);

    // Determine the input file format (from the input file on disk)
    sourceimageformat = getformat(filebuf, filestat.st_size);

    if (sourceimageformat == SIF_95B)
    {
        // PhotoWorks files (.PWP) have multiple SFW94A images per file.
        // Loop through all of the images.
        USCH scanbuf[6] = { 0x53, 0x46, 0x57, 0x39, 0x34, 0x41 };   // SFW94A
        SOURCEIMAGEFORMAT sif = SIF_UNKNOWN;
        USCH* pCur = filebuf;
        USCH* pNext = NULL;
        size_t BufSize = filestat.st_size;
        size_t CurSize = 0;
        size_t cnt = 0;

        // Find the first SFW94A in the buffer
        pCur = forward_scan(filebuf, filebuf + BufSize - 1, scanbuf, NUM_ELEMENTS(scanbuf));

        while (pCur)
        {
            cnt++;

            pNext = forward_scan(pCur + 1, pCur + BufSize - 2, scanbuf, NUM_ELEMENTS(scanbuf));
            if (pNext)
            {
                CurSize = pNext - pCur;
            }
            else
            {
                CurSize = BufSize;
            }
                
            sif = getformat(pCur, BufSize);
            outfilename = GetOutputFileName(infilename, cnt);
            retval = ConvertWorker(pCur, pCur + BufSize - 1, sif, outfilename);

            delete[] outfilename;
            outfilename = NULL;

            BufSize -= CurSize;
            pCur = pNext;
        }
    }
    else
    {
        // One image per file - process the single image
        outfilename = GetOutputFileName(infilename, 1);
        retval = ConvertWorker (filebuf, filebuf + filestat.st_size - 1, sourceimageformat, outfilename);

        delete[] outfilename;
        outfilename = NULL;
    }

    free((void *)filebuf);
    filebuf = NULL;

    if (retval != 0)
    {
        fprintf(stderr, "Conversion to %s failed.\n\n",outfilename);
    }

    return(retval);
}


// Given C:\path\*.jpg", return "C:\path\"
void GetDir(char *Spec, char *pDir)
{
    size_t Len;
    char *pSlash = NULL;

    *pDir = '\0';

    Len = strlen(Spec);
    if (Len == 2 && Spec[1] == ':')
    {
        strcpy(pDir, Spec);
    }
    else
    {
        pSlash = strrchr (Spec, '\\');
        if (pSlash)
        {
            int iCnt = pSlash - Spec + 1;
            strncpy(pDir, Spec, pSlash - Spec + 1);
            pDir[iCnt] = '\0';
        }
        else
        {
            // No path - use current directory
            strcpy (pDir, ".\\");
        }
    }
}


int main(int argc, char *argv[])
{
    int retval = 0;
    char szDir[MAX_PATH];
    char szTemp[MAX_PATH];
    char *pSearch = NULL;

    WIN32_FIND_DATA FindData;
    BOOL fFound = FALSE;
    HANDLE hf = NULL;

    if ((argc == 1) || (argc > 2))
    {
        fprintf(stderr,"\nusage: sfwjpg filename.sfw\n\n");
        fprintf(stderr,"where filename.sfw is the .sfw file which\n");
        fprintf(stderr,"will be converted to .jpg (JFIF) format.\n");
        fprintf(stderr,"Wildcards are supported on input filename.\n");
        return(1);
    }

    GetDir(argv[1], szDir);

    memset (&FindData, '\0', sizeof(FindData));
    pSearch = argv[1];
    hf = FindFirstFile (pSearch, &FindData);

    fFound = (hf != INVALID_HANDLE_VALUE);
    if (!fFound)
    {
        fprintf(stderr,"File not found %s\n", argv[1]);
        fFound = FALSE;
    }

    while (fFound) // Push on even for errors
    {
        strcpy_s(szTemp, MAX_PATH, szDir);
        strcat_s(szTemp, MAX_PATH, FindData.cFileName);

        if ((FindData.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM)) == 0)
        {
            retval = ReadSfwConvertToJpg (szTemp);
        }

        fFound = FindNextFile (hf, &FindData);
    }

    FindClose (hf);

    return(retval);
}

/***********************************************************************/
// JFIF Tags (FF nn)
// http://dev.exiv2.org/projects/exiv2/wiki/The_Metadata_in_JPEG_files
//
// Hex          TagName Comments
// FFD8 FFE0            Start of JFIF file (4 bytes)
// FFFE                 Comment
// FFDA
// FFA0         SOF0
// FFA4         DHT
// FFC4                 Huffman table
// FFC8         SOI
// FFC9         EOI
// FFCA         SOS
// FFCB         DQT
// FFD0         APP0
// FFDA                 Data
//
//
/***********************************************************************/

int sfw_to_jfif(USCH *sfwstart, USCH *sfwend, SOURCEIMAGEFORMAT sourceimageformat, char *outfilename)
{
    int  retval, dataflag = 0, huffmanflag = 0;
    size_t sretval;
    char mesg[256];
    USCH scanbuf[256];
    USCH *bufpos, *headerstart, *headerend, *dataend;
    FILE *outfile;

/*** Initialize lookup table for SFW 94A marker conversions
 *** Just to make stuff hard to understand, the 94A SFW files change the tags
 *** away from the JPEG normal.  Table is used to convert them back.
 ***/

    memset(markertbl, '\0', sizeof(markertbl));
    markertbl[0xa0] = 0xc0;   /* SOF0 */
    markertbl[0xa4] = 0xc4;   /* DHT  */
    markertbl[0xc8] = 0xd8;   /* SOI  */
    markertbl[0xc9] = 0xd9;   /* EOI  */
    markertbl[0xca] = 0xda;   /* SOS  */
    markertbl[0xcb] = 0xdb;   /* DQT  */
    markertbl[0xd0] = 0xe0;   /* APP0 */

/*** Scan for start of JFIF data ***/

    scanbuf[0] = 0xff;  // normally found at offset 1B for SFW 94A
    scanbuf[1] = 0xd8;
    scanbuf[2] = 0xff;
    scanbuf[3] = 0xe0;

    if (sourceimageformat == SIF_94A)
    {
        scanbuf[0] = 0xff;
        scanbuf[1] = 0xc8;
        scanbuf[2] = 0xff;
        scanbuf[3] = 0xd0;
    }
    headerstart = forward_scan(sfwstart, sfwend, scanbuf, 4);

    if (headerstart == NULL)
        return(-1);

/*** fix SOI and APP0 tags ***/

    retval = fix_marker(headerstart, sourceimageformat);
    if (retval == -1) return(-1);
    retval = fix_marker(headerstart+2, sourceimageformat);
    if (retval == -1) return(-1);

/*** fix identifier and version number           ***/
/*** place string "JFIF\0\001\0" in proper place ***/

    // .PIC format reads as
    USCH b6, b7, b8, b9, ba, bb, bc;
    b6 = headerstart[6];   // 'J'
    b7 = headerstart[7];   // 'F'
    b8 = headerstart[8];   // 'I'
    b9 = headerstart[9];   // 'F' JIFF = Identifier
    ba = headerstart[10];  // 00  Format revision byte 1
    bb = headerstart[11];  // 01  Format revision byte 2
    bc = headerstart[12];  // 02  Units for resolution (note different than set 00 below)
                           //     00 = none, 01 dots per inch, 02 dots per centimeter

    // Force identifier and version number for SFW files
    if (sourceimageformat == SIF_93A || sourceimageformat == SIF_94A)
    {
        headerstart[6] = 0x4a;
        headerstart[7] = 0x46;
        headerstart[8] = 0x49;
        headerstart[9] = 0x46;
        headerstart[10] = 0x00;
        headerstart[11] = 0x01;
        headerstart[12] = 0x00;
    }

/*** set bufpos to start of next marker ***/

    bufpos = headerstart + 2;
    bufpos += read_skip_length(bufpos);

/*** fix remaining markers ***/

    while (!dataflag)
    {
        retval = fix_marker(bufpos, sourceimageformat);
        if (retval == -1)
            return(-1);
        if (retval == (int)0xc4)
            huffmanflag = 1;
        if (bufpos[1] == 0xda)
            dataflag = 1;
        else
            bufpos += read_skip_length(bufpos);
    }
    headerend = bufpos-1;

/*** scan forward for EOI marker ***/

    scanbuf[0] = 0xff;
    scanbuf[1] = 0xd9;
    if (sourceimageformat == SIF_94A)
    {
        // Search for SFW 94A obfuscated EOI tag
        scanbuf[1] = 0xc9;
    }
    dataend = forward_scan(bufpos, sfwend, scanbuf, 2);
    if (dataend == NULL)
        return(-1);

/*** fix EOI marker ***/

    retval = fix_marker(dataend, sourceimageformat);
    if (retval == -1) return(-1);
    dataend ++;

/*** open output file ***/

    if (outfilename[0] == '\0')
        outfile = stdout;
    else
        outfile = fopen(outfilename,"wb");

    if (outfile == NULL)
    {
        if (outfilename[0] == '\0') strcpy(outfilename, "standard output");
        sprintf(mesg,"Error opening file '%s'", outfilename);
        fprintf(stderr,"\n");
        perror(mesg);
        fprintf(stderr,"\n");
        return(-1);
    }

/*** write jfif file header ***/

    sretval = fwrite(headerstart, (headerend-headerstart)+1, 1, outfile);
    if (sretval == 0)
    {
        sprintf(mesg,"Error writing file '%s'", outfilename);
        fprintf(stderr,"\n");
        perror(mesg);
        fprintf(stderr,"\n");
        return(-1);
    }

/*** write DQT table for Overlake Photo Express ***/
    if (sourceimageformat == SIF_PIC)
    {
        sretval = fwrite(overlakephotoDQTtable, sizeof(overlakephotoDQTtable), 1, outfile);
        if (sretval == 0)
        {
            sprintf(mesg, "Error writing file '%s'", outfilename);
            fprintf(stderr, "\n");
            perror(mesg);
            fprintf(stderr, "\n");
            return(-1);
        }
    }


/*** write Huffman table if it is not already there ***/

    if (!huffmanflag)
    {
        sretval = fwrite(hufftbl, HUFFSIZE, 1, outfile);
        if (sretval == 0)
        {
            sprintf(mesg,"Error writing file '%s'", outfilename);
            fprintf(stderr,"\n");
            perror(mesg);
            fprintf(stderr,"\n");
            return(-1);
        }
    }

    /*** write rest of jfif file ***/

    sretval = fwrite(headerend+1, dataend-headerend, 1, outfile);
    if (sretval == 0)
    {
        sprintf(mesg,"Error writing file '%s'", outfilename);
        fprintf(stderr,"\n");
        perror(mesg);
        fprintf(stderr,"\n");
        return(-1);
    }

    fclose(outfile);

    return(0);
}
/***********************************************************************/


USCH *forward_scan(USCH *start, USCH *stop, USCH *goal, int length)
{
    USCH *i;
    int j, flag;

    for (i=start; i<stop; i++)
    {
        if (*i != *goal) continue;
        else
        {
            if (length == 1) return(i);
            else
            {
                flag = 1;
                for (j=1; j<length; j++)
                    if ( *(i+j) != *(goal+j) )
                    {
                        flag = 0;
                        break;
                    }
                if (flag) return(i);
            }
        }
    }
    //fprintf(stderr,"forward_scan() failed.\n\n");
    return(NULL);
}
/***********************************************************************/

int fix_marker(USCH *marker, SOURCEIMAGEFORMAT sourceimageformat)
{
    if (marker[0] != 0xFF)
    {
        fprintf(stderr, "fix_marker(): Marker must begin with 0xFF. 0x%02x 0x%02x\n"
                , marker[0], marker[1]);
        return(-1);
    }

    // Unmangle is only required on 94A
    if (sourceimageformat == SIF_94A)
    {
        if (markertbl[marker[1]] == 0)
        {
            fprintf(stderr, "\nWARNING: Unknown marker 0x%02x changed to comment.\n\n",
                marker[1]);
            marker[1] = 0xfe;   /* 0xfe is the comment marker */
        }
        else
            marker[1] = markertbl[marker[1]];
    }
    return((int)marker[1]);
}
/***********************************************************************/



/***********************************************************************/

long read_skip_length(USCH *marker)
{
    long msb,lsb, retval;

    msb = (long) marker[2];
    lsb = (long) marker[3];

    retval = 256*msb + lsb + 2;
    return( retval );
}
/***********************************************************************/