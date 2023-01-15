// Mask errors regarding unsafe string functions
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#include <windows.h>

#define USCH unsigned char

// More than required
/*
USCH ExifHeader[] =
{
    0xFF, 0xE1,                    // 00..01  APP1 Marker
    0x00, 0xF4,                    // 02..03  Distance to next (verified at runtime)

    // EXIF Header
    0x45, 0x78, 0x69, 0x66, 0x00,  // 00..04  "EXIF" (nul)
    0x00,                          // 05..05  pad

    // TIFF Header 
    // Offsets below are relative to here
    0x4D, 0x4D,                    // 00..01  MM Big endian
    0x00, 0x2A,                    // 02..03  Size (always 002A)

    0x00, 0x00, 0x00, 0x08,        // 04..07  Offset of first IFD

    // IFD0
    0x00, 0x06,                    // 08..09  Cnt IFDs

    0x01, 0x1A,                    // 0A..15  Tag XResolution
    0x00, 0x05,                    //         Fmt unsigned rational
    0x00, 0x00, 0x00, 0x01,        //         Number of components
    0x00, 0x00, 0x00, 0x56,        //         Offset of storage     <-------------------

    0x01, 0x1B,                    // 16..21  Tag YResolution
    0x00, 0x05,                    //         Fmt unsigned rational
    0x00, 0x00, 0x00, 0x01,        //         Number of components
    0x00, 0x00, 0x00, 0x5E,        //         Offset of storage     <-------------------

    0x01, 0x28,                    // 22..2D  Tag ResolutionUnit
    0x00, 0x03,                    //         Fmt unsigned short
    0x00, 0x00, 0x00, 0x01,        //         Number of components
    0x00, 0x02, 0x00, 0x00,        //         Data. 2 = per inch

    0x01, 0x32,                    // 2E..39  Tag DateTimeModified
    0x00, 0x02,                    //         Fmt ascii characters
    0x00, 0x00, 0x00, 0x14,        //         Number of components
    0x00, 0x00, 0x00, 0x66,        //         Offset of storage     <-------------------

    0x02, 0x13,                    // 3A..45  Tag YCbCrPositioning
    0x00, 0x03,                    //         Fmt unsigned short
    0x00, 0x00, 0x00, 0x01,        //         Number of components
    0x00, 0x01, 0x00, 0x00,        //         Data. 1 = center

    0x87, 0x69,                    // 46..51  Tag ExifOffset
    0x00, 0x04,                    //         Fmt unsigned long
    0x00, 0x00, 0x00, 0x01,        //         Number of elements
    0x00, 0x00, 0x00, 0x7A,        //         Offset of storage  <-------------------

    0x00, 0x00, 0x00, 0x00,        // 52..55  Offset of next IFD

    // DATA for IFD0
    // TaxXResolution (0x60 / 0x1) = 96 decimal     // 56..5D
    0x00, 0x00, 0x00, 0x60,
    0x00, 0x00, 0x00, 0x01,

    // TaxYResolution (0x60 / 0x1) = 96 decimal     // 5E..65
    0x00, 0x00, 0x00, 0x60,
    0x00, 0x00, 0x00, 0x01,

    // DateTimeLastModified                         // 66..79 (x14 bytes)
    // YYYY:MM:DD HH:MM:SS\0 (Fill in at runtime)
    // Placeholder data is "1970" - search and replace at runtime
    0x31, 0x39, 0x37, 0x30, 0x3A, 0x30, 0x31, 0x3A,
    0x30, 0x31, 0x20, 0x30, 0x30, 0x3A, 0x30, 0x30, 
    0x3A, 0x30, 0x30, 0x00,

    // IFD1 - Digicam information
    0x00, 0x06,                    // 7A..7B  Cnt IFDs

    0x90, 0x00,                    // 7C..87  Tag ExifVersion
    0x00, 0x07,                    //         Fmt undefined
    0x00, 0x00, 0x00, 0x04,        //         Number of elements
    0x30, 0x32, 0x33, 0x31,        //         Data

    0x90, 0x03,                    // 88..93  Tag DateTimeOriginal
    0x00, 0x02,                    //         Fmt ascii characters
    0x00, 0x00, 0x00, 0x14,        //         Number of elements
    0x00, 0x00, 0x00, 0xC8,        //         Offset of storage  <-------------------

    0x90, 0x04,                    // 94..9F  Tag DateTimeDigitized
    0x00, 0x02,                    //         Fmt ascii strings
    0x00, 0x00, 0x00, 0x14,        //         Number of elements
    0x00, 0x00, 0x00, 0xDC,        //         Offset of storage   <-------------------

    0x91, 0x01,                    // A0..AB  Tag ComponentConfiguration
    0x00, 0x07,                    //         Fmt undefined
    0x00, 0x00, 0x00, 0x04,        //         Number of elements
    0x01, 0x02, 0x03, 0x00,        //         Data

    0xA0, 0x00,                    // AC..B7  Tag FlashPixVersion
    0x00, 0x07,                    //         Fmt undefined
    0x00, 0x00, 0x00, 0x04,        //         Number of elements
    0x30, 0x31, 0x30, 0x30,        //         Data 0100

    0xA0, 0x01,                    // B8..C3  Tag ColorSpace
    0x00, 0x03,                    //         Fmt unsigned short
    0x00, 0x00, 0x00, 0x01,        //         Number of elements
    0xFF, 0xFF, 0x00, 0x00,        //         Offset of storage

    0x00, 0x00, 0x00, 0x00,        // C4..C7  Offset to next IFD

    // DATA for IFD1
    // ***************

    // DateTimeOriginal            // C8..DB
    // YYYY:MM:DDHH:MM:SS\0\0 (Fill in at runtime)
    0x31, 0x39, 0x37, 0x30, 0x3A, 0x30, 0x31, 0x3A,
    0x30, 0x31, 0x20, 0x30, 0x30, 0x3A, 0x30, 0x30,
    0x3A, 0x30, 0x30, 0x00,

    // DateTimeDigitized           // DC..EF
    // YYYY:MM:DDHH:MM:SS\0\0 (Fill in at runtime)
    0x31, 0x39, 0x37, 0x30, 0x3A, 0x30, 0x31, 0x3A,
    0x30, 0x31, 0x20, 0x30, 0x30, 0x3A, 0x30, 0x30,
    0x3A, 0x30, 0x30, 0x00,
};
*/


USCH ExifHeader[] =
{
    0xFF, 0xE1,                    // 00..01  APP1 Marker
    0x00, 0x94,                    // 02..03  Distance to next (verified at runtime)

    // EXIF Header
    0x45, 0x78, 0x69, 0x66, 0x00,  // 00..04  "EXIF" (nul)
    0x00,                          // 05..05  pad

    // TIFF Header 
    // Offsets below are relative to here
    0x4D, 0x4D,                    // 00..01  MM Big endian
    0x00, 0x2A,                    // 02..03  Size (always 002A)

    0x00, 0x00, 0x00, 0x08,        // 04..07  Offset of first IFD

    // IFD0
    0x00, 0x02,                    // 08..09  Cnt IFDs

    0x01, 0x32,                    // 0A..15  Tag DateTimeModified
    0x00, 0x02,                    //         Fmt ascii characters
    0x00, 0x00, 0x00, 0x14,        //         Number of components
    0x00, 0x00, 0x00, 0x26,        //         Offset of storage  <-------------------

    0x87, 0x69,                    // 16..21  Tag ExifOffset
    0x00, 0x04,                    //         Fmt unsigned long
    0x00, 0x00, 0x00, 0x01,        //         Number of elements
    0x00, 0x00, 0x00, 0x3A,        //         Offset of storage  <-------------------

    0x00, 0x00, 0x00, 0x00,        // 22..25  Offset of next IFD

    // DATA for IFD0
    // DateTimeLastModified        // 26..39 (x14 bytes)
    // YYYY:MM:DD HH:MM:SS\0 (Fill in at runtime)
    // Placeholder data is "1970" - search and replace at runtime
    0x31, 0x39, 0x37, 0x30, 0x3A, 0x30, 0x31, 0x3A,
    0x30, 0x31, 0x20, 0x30, 0x30, 0x3A, 0x30, 0x30,
    0x3A, 0x30, 0x30, 0x00,

    // IFD1 - Digicam information
    0x00, 0x03,                    // 3A..3B  Cnt IFDs

    0x90, 0x00,                    // 3C..47  Tag ExifVersion
    0x00, 0x07,                    //         Fmt undefined
    0x00, 0x00, 0x00, 0x04,        //         Number of elements
    0x30, 0x32, 0x33, 0x31,        //         Data

    0x90, 0x03,                    // 48..53  Tag DateTimeOriginal
    0x00, 0x02,                    //         Fmt ascii characters
    0x00, 0x00, 0x00, 0x14,        //         Number of elements
    0x00, 0x00, 0x00, 0x64,        //         Offset of storage  <-------------------

    0x90, 0x04,                    // 54..5F  Tag DateTimeDigitized
    0x00, 0x02,                    //         Fmt ascii strings
    0x00, 0x00, 0x00, 0x14,        //         Number of elements
    0x00, 0x00, 0x00, 0x78,        //         Offset of storage  <-------------------

    0x00, 0x00, 0x00, 0x00,        // 60..63  Offset to next IFD

    // DATA for IFD1

    // DateTimeOriginal            // 64..77
    // YYYY:MM:DDHH:MM:SS\0\0 (Fill in at runtime)
    0x31, 0x39, 0x37, 0x30, 0x3A, 0x30, 0x31, 0x3A,
    0x30, 0x31, 0x20, 0x30, 0x30, 0x3A, 0x30, 0x30,
    0x3A, 0x30, 0x30, 0x00,

    // DateTimeDigitized           // 78..8B
    // YYYY:MM:DDHH:MM:SS\0\0 (Fill in at runtime)
    0x31, 0x39, 0x37, 0x30, 0x3A, 0x30, 0x31, 0x3A,
    0x30, 0x31, 0x20, 0x30, 0x30, 0x3A, 0x30, 0x30,
    0x3A, 0x30, 0x30, 0x00,
};



void hexdump(USCH* buf, size_t size)
{
    size_t I = 0;

    printf("Hexdump %d (0x%X) bytes.\n", size, size);

    for (I = 0; I < size; I++)
    {
        printf("%02X", buf[I]);

        if (I % 0x10 == 0xF)
        {
            printf(" (%X)\n", I+1);
        }
        else
        {
            printf(" ");
        }
    }

    if (size % 0x10 != 0)
    {
        printf("\n");
    }
}


char* nexttoken(char* pstring)
{
    char* ans = NULL;
    int found = 0;
    char ch = '\0';

    if (pstring == NULL)
    {
        ans = NULL;
    }
    else
    {
        ans = pstring;
        ch = *ans;

        // Skip to next space and treat slash as a space
        while (ch != ' ' && ch != '/' && ch != '\0')
        {
            ans++;
            ch = *ans;
        }

        // Skip to next non space and treat slash as a space
        while ((ch == ' ' || ch == '/')  && ch != '\0')
        {
            ans++;
            ch = *ans;
        }

        if (ch == '\0')
            ans = NULL;
    }

    return ans;
}


// return a pointer to the next location in the Exif header that needs a date set
char* ofsnextdate(void)
{
    char *pheader = (char*) ExifHeader;
    char* ans = NULL;

    // Search for "1970" in the header - first part of the next date string to set
    // An optmization to not have to hard code offsets - its crude but will work.
    for (int I = 0; I < sizeof(ExifHeader) - 3; I++)
    {
        if (ExifHeader[I + 0] == '1' &&
            ExifHeader[I + 1] == '9' &&
            ExifHeader[I + 2] == '7' &&
            ExifHeader[I + 3] == '0')
        {
            ans = (char *) &ExifHeader[I + 0];
            break;
        }
    }

    return ans;
}


//
// Input parameter is memory pointer to the SFW file in memory
//
// The roll number and date of develop are stored in the SFW file at offset 0xE0.
// The roll number usually has 7 characters, but sometimes has 9.
//
//  000000E0  37333138 33373136 - 30302030 30203034  *7318371600 00 04*
//  000000F0  2F20322F 31393936 - 00000000 3A3B0000  */ 2 / 1996....:; ..*
//
// This is an ascii string including a terminating nul.
// The parameters are mostly space delimited though this is not true after the month.
//
int SetExifDates(USCH* psfw, size_t bufsize)
{
    int rc = 0;
    int sizeset = 0;

    char* token = NULL;
    int month = -1;
    int day = -1;
    int year = -1;
    char szDate[0x14] = "1970:01:01 00:00:00";

    sizeset = sizeof(ExifHeader) - 2;

    // Ensure the header field for offset to next block 
    // reflects the size of the header
    ExifHeader[2] = sizeset >> 8;
    ExifHeader[3] = sizeset & 0xFF;

    // Easy case
    // 05294175 00 09/24/1998
    //
    // Looks like SFW had a bug fix between 1996 and 1998
    //
    // Hard case
    // 7318371600 00 04/ 2 / 1996
    // no leading digit ^ and extra spaces
    //
    token = (char*)psfw + 0xE0;
    // printf("%s\n", token);

    // Skip the roll number
    token = nexttoken(token);

    // Skip the next token which appears to always be 00
    token = nexttoken(token);
    month = atoi(token);

    token = nexttoken(token);
    day = atoi(token);

    token = nexttoken(token);
    year = atoi(token);

    if (month == 0 || day == 0 || year == 0)
    {
        perror("Err: SetExifDates Date conversion from SFW file failed");
        rc = -1;
    }
    else
    {
        char* dest = NULL;

        // SFW data has only the date. Use noon for time.
        // 
        // SPACE in the below format string is absolutely required
        // else dates will not be recognized by Windows
        sprintf(szDate, "%04d:%02d:%02d 12:00:00", year, month, day);

        //printf("%s\n", szDate);

        // We have the date - place the date string into the 3 locations in the EXIF data
        // Rather than hard code the offsets (which could change), search for a token.
        dest = ofsnextdate();
        if (dest)
        {
            memcpy(dest, szDate, sizeof(szDate));
        }

        dest = ofsnextdate();
        if (dest)
        {
            memcpy(dest, szDate, sizeof(szDate));
        }

        dest = ofsnextdate();
        if (dest)
        {
            memcpy(dest, szDate, sizeof(szDate));
        }

        if (dest == NULL)
        {
            rc = -1;
        }
    }

    return rc;
}


void GetExifHeader(USCH** pExif, size_t* psize)
{
    // hexdump((USCH *)&ExifHeader, sizeof(ExifHeader));
    *pExif = (USCH*)&ExifHeader;
    *psize = sizeof(ExifHeader);
}
