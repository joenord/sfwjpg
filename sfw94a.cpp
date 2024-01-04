/*
* Copyright(c) 2024  Joseph Nord
*
*This program is free software; you can redistribute itand /or
*modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
*This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
* GNU General Public License for more details.
*
* The GNU General Public License is available at
*
* http://www.fsf.org/copyleft/gpl.html
*
*Alternatively, you can write to the
*
* Free Software Foundation, Inc.
* 59 Temple Place - Suite 330
* Boston, MA  02111 - 1307, USA
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#include <windows.h>    // Directory enumeration - wildcard support
#include <gdiplus.h>    // Image rotate and unmirror
#pragma comment( lib, "gdiplus.lib" )
using namespace Gdiplus;

#include "util.h"
#include "sfw94a.h"
#include "exifheader.h"

// Function straight from MSDN example
// https://msdn.microsoft.com/en-us/library/windows/desktop/ms533843%28v=vs.85%29.aspx
int GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
    UINT  num = 0;          // number of image encoders
    UINT  size = 0;         // size of the image encoder array in bytes

    ImageCodecInfo* pImageCodecInfo = NULL;

    GetImageEncodersSize(&num, &size);
    if (size == 0)
        return -1;  // Failure

    pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
    if (pImageCodecInfo == NULL)
        return -1;  // Failure

    GetImageEncoders(num, size, pImageCodecInfo);

    for (UINT j = 0; j < num; ++j)
    {
        if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0)
        {
            *pClsid = pImageCodecInfo[j].Clsid;
            free(pImageCodecInfo);
            return j;  // Success
        }
    }

    free(pImageCodecInfo);
    return -1;  // Failure
}


// Rotate image 180 degrees and unmirror left and right
// Implementation utilizes Windows APIs
int fix_mirroring(char* filename)
{
    int len = 0;
    LPWSTR filenamew = NULL;

    // convert to WIDE char required for image functions
    len = strlen(filename);
    filenamew = new WCHAR[len + 1];
    for (int i = 0; i < len; i++)
    {
        filenamew[i] = filename[i];
    }
    filenamew[len] = '\0';

    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    // Load the image
    Gdiplus::Image* image = Gdiplus::Image::FromFile(filenamew);

    // Rotate image 180 degrees and fix X axis mirroring
    image->RotateFlip(Rotate180FlipX);

    // Save the image back into the same file it came from
    CLSID jpgClsid;
    GetEncoderClsid(L"image/jpeg", &jpgClsid);
    image->Save(filenamew, &jpgClsid);

    delete image;
    image = NULL;
    Gdiplus::GdiplusShutdown(gdiplusToken);
    delete[] filenamew;

    return (0);
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
        while ((ch == ' ' || ch == '/') && ch != '\0')
        {
            ans++;
            ch = *ans;
        }

        if (ch == '\0')
            ans = NULL;
    }

    return ans;
}


// Given SFW file in memory, find the date that the roll of film was 
// developed and return it to the caller in EXIF usable form.
// 
// Caller must free the returned string using delete[].
// 
// Input parameter is memory pointer to the SFW file in memory.
// The string holding date film was developed is at offset 0xE0 into the file.
//
//  000000E0  37333138 33373136 - 30302030 30203034  *7318371600 00 04*
//  000000F0  2F20322F 31393936 - 00000000 3A3B0000  */ 2 / 1996....:; ..*
//
// The string consists of 3 items.  
// 1) The roll number 
// 2) A token of "00" of unknown purpose
// 3) Date that the film was developed.
// 
// The roll number usually has 7 characters.
// The date is of a vairable format, following USA date formatting with inconsistent
// formatting of the separators and spaces depending upon the date of film developed.
// The date is USA formatting. There are extra spaces around the slash separators and the month. 
// Single digit months may or may not include leading zeros but if it is missing, will include a
// bonus space in its place. Parsing requires some care.

char* get_date_from_sfw(USCH* sfwstart, USCH* sfwend)
{
    USCH ch = ' ';
    size_t cnt = 0;
    size_t maxcnt = 0;
    char* pszRet = NULL;
    char* pszSFW = NULL;
    char* pszLocal = NULL;
    int rc = 0;

    char* token = NULL;
    int month = -1;
    int day = -1;
    int year = -1;

    // Copy the SFW file string into a local variable
    pszSFW = (char *)sfwstart + 0xE0;
    if (pszSFW >= (char *)sfwend)
    {
        return NULL;
    }

    maxcnt = sfwend - sfwstart + 1;
    cnt = 0;
    while (*pszSFW && cnt < maxcnt)
    {
        cnt++;
        pszSFW++;
    }
    cnt++;  // room for the nul

    pszLocal = new char[cnt];
    pszLocal[0] = '\0';

    pszSFW = (char*)sfwstart + 0xE0;
    strncpy_s(pszLocal, cnt, pszSFW, cnt-1);
    pszLocal[cnt - 1] = '\0';

    // Easy case
    // ROLLNUMBER  DATE_DEVELOPED
    // 05294175 00 09/24/1998
    //
    // Looks like SFW had a bug fix between 1996 and 1998 where only 1 digit was used 
    // for month and the leading zero was replaced with a bonus second space.
    // The spaces in general don't need to be there, but are present - have to skip.
    //
    // 7318371600 00 04/ 2 / 1996
    // no leading digit ^ and extra spaces

    token = pszLocal;
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
        // SFW data has only the date. Use noon for time.
        // 
        // SPACE in the below format string is absolutely required
        // else dates will not be recognized by Windows
        // char szDate[0x14] = "1970:01:01 00:00:00";

        cnt = NUM_ELEMENTS("1970:01:01 00:00:00");  // 20 chars including the nul
        pszRet = new char[cnt];

        sprintf_s(pszRet, cnt, "%04d:%02d:%02d 12:00:00", year, month, day);
    }

    delete[] pszLocal;
    pszLocal = NULL;

    return(pszRet);
}


//
// Caller is responsible for calling free() on the returned memory
//
void GetSourceFile(char* filename, USCH *pBuf, size_t *pSize)
{
    struct stat filestat;
    FILE* infile;   
    errno_t err = 0;
    int iRC = 0;
    USCH* ret = NULL;

    iRC = _stat(filename, (struct _stat*)&filestat);
    if (iRC == -1)
    {
        fprintf(stderr, "Error getting status for file '%s'", filename);
        return;
    }

    // Tell caller the size of the buffer they need to allocate to hold the data
    *pSize = (size_t)filestat.st_size;

    // Caller has to call us twice, once to get the size and a second
    // time to actually fetch the file contents
    if (pBuf == NULL)
    {
        return;
    }

    err = fopen_s(&infile, filename, "rb");
    if (err != 0)
    {
        fprintf(stderr, "Error opening file '%s'", filename);
        free(ret);
        return;
    }

    iRC = fread(pBuf, filestat.st_size, 1, infile);
    if (iRC == 0)
    {
        fprintf(stderr, "Error reading file '%s'", filename);
        free(ret);
        return;
    }
    fclose(infile);
}


int add_develop_date_to_jpg(char* filename, char* exifdate)
{
    int rc = 0;

    int len = 0;
    USCH *pSrc = NULL;
    int ofs = 0;
    USCH* pBuf = NULL;
    USCH* pExif = NULL;
    size_t exifSize = 0;
    size_t srcSize = 0;
    errno_t err = 0;
    size_t cnt = 0;
    FILE* outfile;

    // Windows has a promising API to set EXIF data in JPG files, Image::SetPropertyItem.
    // Unfortunately, this API does not work on files which already exist (have contents).
    // The return code is success, but no change to the file is actually performed.
    // It is likely that this is because expanding EXIF data is hard and it is easier to 
    // write it in place as a series of write actions to a new file.
    // In concept we could create a new image per Windows API, set the property and then copy the JPG 
    // contents into the new file - BUT, there are zero, nada, none, no PUBLIC API on Windows to 
    // "create" an "image" that does not depend upon the image on disk or memory already existing.
    // 
    // Solution: Brute force - create the image EXIF data manually and populate the JPG image directly.
    //
    // Read the JPG into memory which is lacking the Exif data for date of file create.  
    // Write an appropriate EXIF header to the start of the file, then copy all the other JFIF 
    // blocks into the output file.

    // Update the Exif data structure with date provided
    rc = ExifSetDates(exifdate);
    if (rc != 0)
    {
        // Msg already displayed
        return (rc);
    }

    // Get the Exif header which incorporates the provided date
    ExifGetHeader(&pExif, &exifSize);

    // Read the existing JPG file into RAM
    GetSourceFile(filename, NULL, &srcSize);
    pSrc = (USCH*)malloc(srcSize);
    if (pSrc == NULL)
    {
        fprintf(stderr, "Error allocating memory for filebuf");
        return (1);
    }
    GetSourceFile(filename, pSrc, &srcSize);

    // Open the same name file for output (overwrite it)
    err = fopen_s(&outfile, filename, "wb");
    if (err != 0)
    {
        fprintf(stderr, "Error opening output file '%s'", filename);
        return(2);
    }

    // First fields are the 
    // SOI (FFDO) + the 
    // JFIF Header which has a bit of a variable size
    // Write these to the output file

    cnt = (2 +          // 0xFFD8 (SOI)
        *(pSrc + 5) +   // 0xFFE0 plus the rest of the JFIF Header
        2);

    cnt = fwrite(pSrc, sizeof(*pSrc), cnt, outfile);
    if (cnt == 0)
    {
        fprintf(stderr, "Write SOI failed.");
        return(3);
    }
    ofs += cnt;

    // Finally - add the Exif header which includes the date information
    cnt = fwrite(pExif, exifSize, 1, outfile);
    if (cnt == 0)
    {
        fprintf(stderr, "Write Exif header failed.");
        return(3);
    }

    // Copy the remaining JFIF blocks directly from input file to output file.
    // There can be many of these, but no matter how many, it reduces to just
    // copying all the remaining data in the input buffer to the output.
    cnt = fwrite(pSrc + ofs, srcSize - ofs, 1, outfile);
    if (cnt == 0)
    {
        fprintf(stderr, "Write remaining blocks failed.");
        return(3);
    }
    fclose(outfile);

    if (pSrc)
    {
        free(pSrc);
        pSrc = NULL;
    }

    return 0;

}