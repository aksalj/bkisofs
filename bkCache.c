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

#include <string.h>

#include "bkInternal.h"

#define CACHED_STATUS_UNSET 0 /* don't write */
#define CACHED_STATUS_SET   1 /* do write */

int wcFlush(VolInfo* volInfo)
{
    off_t count;
    off_t count2;
    off_t numBytesToSkip;
    off_t numBytesToWrite;
    int rc;
    
    numBytesToSkip = 0;
    for(count = 0; count < WRITE_CACHE_SIZE; count++)
    {
        if(volInfo->writeCacheStatus[count] == CACHED_STATUS_SET)
        {
            lseek(volInfo->imageForWriting, numBytesToSkip, SEEK_CUR);
            
            /* count number of consecutive bytes to write */
            for(count2 = count; 
                count2 < WRITE_CACHE_SIZE && 
                volInfo->writeCacheStatus[count2] == CACHED_STATUS_SET;
                count2++) ;
            
            if(volInfo->writeCacheStatus[count2] == CACHED_STATUS_UNSET)
                numBytesToSkip = 1;
            else
                numBytesToSkip = 0;
            
            numBytesToWrite = count2 - count;
            
            rc = write(volInfo->imageForWriting, 
                       &(volInfo->writeCacheStatus[count]), 
                       numBytesToWrite);
            if(rc != numBytesToWrite)
                return BKERROR_WRITE_GENERIC;
            
            count = count2;
        }
        else if(volInfo->writeCacheStatus[count] == CACHED_STATUS_UNSET)
            numBytesToSkip++;
        else
            return BKERROR_SANITY;
    }
    
    lseek(volInfo->imageForWriting, numBytesToSkip, SEEK_CUR);
    
    wcInit(volInfo);
    
    return 1;
}

void wcInit(VolInfo* volInfo)
{
    /* just for debugging so it's easier to see undesired writes */
    memset(volInfo->writeCache, 0xEE, WRITE_CACHE_SIZE);
    
    /* no bytes ready to be written */
    memset(volInfo->writeCacheStatus, CACHED_STATUS_UNSET, WRITE_CACHE_SIZE);
    
    volInfo->writeCacheOffset = 0;
}

int wcSeekForward(VolInfo* volInfo, off_t numBytes)
{
    int rc;
    
    if(volInfo->writeCacheOffset + numBytes >= WRITE_CACHE_SIZE)
    {
        rc = wcFlush(volInfo);
        if(rc <= 0)
            return rc;
        
        off_t origPos = lseek(volInfo->imageForWriting, 0, SEEK_CUR);
        lseek(volInfo->imageForWriting, origPos + numBytes, SEEK_SET);
    }
    else
        volInfo->writeCacheOffset += numBytes;
    
    return 1;
}

int wcSeekSet(VolInfo* volInfo, off_t position)
{
    int rc;
    off_t origPos;
    
    origPos = lseek(volInfo->imageForWriting, 0, SEEK_CUR);
    
    if(position < origPos || position >= origPos + WRITE_CACHE_SIZE)
    {
        rc = wcFlush(volInfo);
        if(rc <= 0)
            return rc;
        
        lseek(volInfo->imageForWriting, position, SEEK_SET);
    }
    else
        volInfo->writeCacheOffset = position - origPos;
    
    return 1;
}

off_t wcSeekTell(VolInfo* volInfo)
{
    return lseek(volInfo->imageForWriting, 0, SEEK_CUR) + 
           volInfo->writeCacheOffset;
}

int wcWrite(VolInfo* volInfo, unsigned char* block, off_t numBytes)
{
    off_t numBytesAvailable;
    off_t numBytesNow;
    off_t numBytesLeft;
    int rc;
    
    numBytesAvailable = WRITE_CACHE_SIZE - volInfo->writeCacheOffset;
    
    if(numBytes < numBytesAvailable)
    {
        numBytesNow = numBytes;
        numBytesLeft = 0;
    }
    else
    {
        numBytesNow = numBytesAvailable;
        numBytesLeft = numBytes - numBytesAvailable;
    }
    
    memcpy(volInfo->writeCache + volInfo->writeCacheOffset, 
           block, numBytesNow);
    memset(volInfo->writeCacheStatus + volInfo->writeCacheOffset, 
           CACHED_STATUS_SET, numBytesNow);
    volInfo->writeCacheOffset += numBytesNow;
    
    if(numBytesLeft > 0)
    {
        rc = wcFlush(volInfo);
        if(rc < 0)
            return rc;
        
        rc = wcWrite(volInfo, block + numBytesNow, numBytesLeft);
        if(rc < 0)
            return rc;
    }
    
    return 1;
}
