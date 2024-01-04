#pragma once

int ExifSetDates(char* pszDate);
void ExifGetHeader(USCH** pExif, size_t* psize);

#pragma pack (1)
typedef struct                      // 16 bytes
{
    char szJFIF[5];                 // "JFIF" and nul = 5 bytes
    BYTE bMajVersion;
    BYTE bMinorVersion;
    BYTE bPixelDensityUnits;        // 00 = none, 01 = pixels per inch, 02 = pixels per centimeter
    USHORT usHorizontalDensity;
    USHORT usVerticalDensity;
    USHORT usThumbnailHorizontal;   // Can be 0
    USHORT usThumbnailVertical;     // Can be not present
    // Followed by 3*(horizontal*vertical) bytes of not compressed RGB data
} JFIFHEADER;
typedef JFIFHEADER* PJFIFHEADER;