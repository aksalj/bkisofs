/******************************* LICENCE **************************************
* Any code in this file may be redistributed or modified under the terms of
* the GNU General Public Licence as published by the Free Software 
* Foundation; version 2 of the licence.
****************************** END LICENCE ***********************************/

/******************************************************************************
* Author:
* Andrew Smith, http://littlesvr.ca/misc/contactandrew.php
*
* Contributors:
* 
******************************************************************************/

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "bkInternal.h"
#include "bkIoWrappers.h"

void bkClose(int file)
{
#ifdef WINDOWS_BUILD
    _close(file);
#else
    close(file);
#endif
}

int bkFstat(int file, BkStatStruct* statStruct)
{
#ifdef WINDOWS_BUILD
    return _fstati64(file, statStruct);
#else
    return fstat(file, statStruct);
#endif
}

size_t bkRead(int file, void* dest, size_t numBytes)
{
#ifdef WINDOWS_BUILD
    return _read(file, dest, numBytes);
#else
    return read(file, dest, numBytes);
#endif
}

/******************************************************************************
* bkSeekSet()
* */
bk_off_t bkSeekSet(int file, bk_off_t offset, int origin)
{
#ifdef WINDOWS_BUILD
    return _lseeki64(file, offset, origin);
#else
    return lseek(file, offset, origin);
#endif
}

/******************************************************************************
* bkSeekTell()
* */
bk_off_t bkSeekTell(int file)
{
#ifdef WINDOWS_BUILD
    return _lseeki64(file, 0, SEEK_CUR);
#else
    return lseek(file, 0, SEEK_CUR);
#endif
}

int bkStat(const char* pathAndName, BkStatStruct* statStruct)
{
#ifdef WINDOWS_BUILD
    return _stati64(pathAndName, statStruct);
#else
    return stat(pathAndName, statStruct);
#endif
}

size_t bkWrite(int file, const void* src, size_t numBytes)
{
#ifdef WINDOWS_BUILD
    return _write(file, src, numBytes);
#else
    return write(file, src, numBytes);
#endif
}

size_t readRead(VolInfo* volInfo, void* dest, size_t numBytes)
{
#ifdef WINDOWS_BUILD
    return _read(volInfo->imageForReading, dest, numBytes);
#else
    return read(volInfo->imageForReading, dest, numBytes);
#endif
}

/******************************************************************************
* readSeekSet()
* Seek set for reading from the iso
* */
bk_off_t readSeekSet(VolInfo* volInfo, bk_off_t offset, int origin)
{
#ifdef WINDOWS_BUILD
    return _lseeki64(volInfo->imageForReading, offset, origin);
#else
    return lseek(volInfo->imageForReading, offset, origin);
#endif
}

/******************************************************************************
* readSeekTell()
* Seek tell for reading from the iso
* */
bk_off_t readSeekTell(VolInfo* volInfo)
{
#ifdef WINDOWS_BUILD
    return _lseeki64(volInfo->imageForReading, 0, SEEK_CUR);
#else
    return lseek(volInfo->imageForReading, 0, SEEK_CUR);
#endif
}
