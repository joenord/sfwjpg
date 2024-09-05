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
#include "exifheader.h"

// JPEG files all begin with SOI marker (FFD8)
// This is followed by a JFIF Header

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
    // Placeholder data "1970"
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


// Update the Exif header structure in memory with the given date.
// Note that the input string must be precisly 19 characters (20 bytes) and
// be in this format: "1970:01:01 00:00:00"

int ExifSetDates(char* pszDate)
{
    int rc = 0;
    int sizeset = 0;
    char* dest = NULL;
    size_t cnt = strlen(pszDate) + 1;

    // Ensure the header field for offset to next block 
    // reflects the size of the header
    sizeset = sizeof(ExifHeader) - 2;
    ExifHeader[2] = sizeset >> 8;
    ExifHeader[3] = sizeset & 0xFF;

    if (strlen(pszDate) != 19)
    {
        printf("SetExifDates: Date is invalid");
        return 87;
    }

    // Place the date string into the 3 locations in the EXIF data where date is needed.
    // Note that on first file processed these will point to "1970".
    // On subsequent files, the destination will be poluted with the date of the prior
    // file and overwritten with the file date.

    // 0x0A is offset within the TIFF header to the EXIF header
    // Other offset is within the EXIF header (date storage).
    const unsigned int ofsDateTimeModified = 0x0A + 0x26;
    const unsigned int ofsDateTimeOriginal = 0x0A + 0x64;
    const unsigned int ofsDateTimeDigitized = 0x0A + 0x78;

    dest = (char*)&ExifHeader[ofsDateTimeModified];
    memcpy(dest, pszDate, cnt);

    dest = (char*)&ExifHeader[ofsDateTimeOriginal];
    memcpy(dest, pszDate, cnt);

    dest = (char*)&ExifHeader[ofsDateTimeDigitized];
    memcpy(dest, pszDate, cnt);

    return 0;
}


void ExifGetHeader(USCH** pExif, size_t* psize)
{
    // hexdump((USCH *)&ExifHeader, sizeof(ExifHeader));
    *pExif = (USCH*)&ExifHeader;
    *psize = sizeof(ExifHeader);
}
