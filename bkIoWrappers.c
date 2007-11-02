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

#include "bkInternal.h"
#include "bkIoWrappers.h"

size_t readRead(VolInfo* volInfo, void* dest, size_t numBytes)
{
#ifdef MINGW_TEST
    return fread(dest, 1, numBytes, volInfo->imageForReadingF);
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
#ifdef MINGW_TEST
    //return _fseeki64(volInfo->imageForReadingF, offset, origin);
    return fseek(volInfo->imageForReadingF, offset, origin);
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
#ifdef MINGW_TEST
    //return _ftelli64(volInfo->imageForReadingF);
    return ftell(volInfo->imageForReadingF);
#else
    return lseek(volInfo->imageForReading, 0, SEEK_CUR);
#endif
}
