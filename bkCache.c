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

/******************************************************************************
* 31 dec 2006: these functions turned out to be so heavy on cpu usage that 
* performance went down significantly for writing to anything, including 
* the local filesystem. So now they don't do anything.
******************************************************************************/

#include <string.h>
#include <stdio.h>
#include <time.h>

#include "bkInternal.h"
#include "bkCache.h"

#define CACHED_STATUS_UNSET 0 /* don't write */
#define CACHED_STATUS_SET   1 /* do write */

//~ int wcFlush(VolInfo* volInfo)
//~ {
    //~ off_t count;
    //~ off_t count2;
    //~ off_t numBytesToSkip;
    //~ off_t numBytesToWrite;
    //~ int rc;
    //~ //printf("want to wcFlush (offset is 0x%X+%d)\n", (unsigned)lseek(volInfo->imageForWriting, 0, SEEK_CUR), (unsigned)volInfo->wcOffset);fflush(NULL);
    
    //~ if(volInfo->stopOperation)
        //~ return BKERROR_OPER_CANCELED_BY_USER;
    
    //~ if(volInfo->writeProgressFunction != NULL)
        //~ volInfo->writeProgressFunction();
    
    //~ if(volInfo->wcNumBytesUsed > WRITE_CACHE_SIZE)
        //~ return BKERROR_SANITY;
    
    //~ numBytesToSkip = 0;
    //~ for(count = 0; count < volInfo->wcNumBytesUsed; count++)
    //~ {
        //~ if(volInfo->writeCacheStatus[count] == CACHED_STATUS_SET)
        //~ {
            //~ lseek(volInfo->imageForWriting, numBytesToSkip, SEEK_CUR);
            //~ numBytesToSkip = 0;
            
            //~ /* count number of consecutive bytes to write */
            //~ for(count2 = count; 
                //~ count2 < volInfo->wcNumBytesUsed && 
                //~ volInfo->writeCacheStatus[count2] == CACHED_STATUS_SET;
                //~ count2++) 
                //~ ;
            //~ numBytesToWrite = count2 - count;
            
            //~ rc = write(volInfo->imageForWriting, 
                       //~ &(volInfo->writeCache[count]), 
                       //~ numBytesToWrite);
            //~ if(rc != numBytesToWrite)
                //~ return BKERROR_WRITE_GENERIC;
            
            //~ /* count will be incremented byt outer for loop */
            //~ count = count2 - 1;
        //~ }
        //~ else if(volInfo->writeCacheStatus[count] == CACHED_STATUS_UNSET)
            //~ numBytesToSkip++;
        //~ else
            //~ return BKERROR_SANITY;
    //~ }
    //~ //printf("before seek (offset is 0x%X, skipping 0x%X)\n\n", (unsigned)lseek(volInfo->imageForWriting, 0, SEEK_CUR), (unsigned)numBytesToSkip);fflush(NULL);
    //~ lseek(volInfo->imageForWriting, numBytesToSkip, SEEK_CUR);
    
    //~ wcInit(volInfo);
    
    //~ return 1;
//~ }

//~ void wcInit(VolInfo* volInfo)
//~ {
    //~ /* just for debugging so it's easier to see undesired writes */
    //~ memset(volInfo->writeCache, 0xEE, WRITE_CACHE_SIZE);
    
    //~ /* no bytes ready to be written */
    //~ memset(volInfo->writeCacheStatus, CACHED_STATUS_UNSET, WRITE_CACHE_SIZE);
    
    //~ volInfo->wcOffset = 0;
    //~ volInfo->wcNumBytesUsed = 0;
//~ }

int wcSeekForward(VolInfo* volInfo, off_t numBytes)
{
    //~ int rc;
    //~ //printf("want to wcSeekForward %d bytes\n", (unsigned)numBytes);fflush(NULL);
    //~ if(volInfo->wcOffset + numBytes >= WRITE_CACHE_SIZE)
    //~ {
        //~ rc = wcFlush(volInfo);
        //~ if(rc <= 0)
            //~ return rc;
        
        //~ off_t origPos = lseek(volInfo->imageForWriting, 0, SEEK_CUR);
        //~ lseek(volInfo->imageForWriting, origPos + numBytes, SEEK_SET);
    //~ }
    //~ else
    //~ {
        //~ volInfo->wcOffset += numBytes;
        //~ if(volInfo->wcOffset > volInfo->wcNumBytesUsed)
            //~ volInfo->wcNumBytesUsed = volInfo->wcOffset;
    //~ }
    
    lseek(volInfo->imageForWriting, numBytes, SEEK_CUR);
    
    return 1;
}

int wcSeekSet(VolInfo* volInfo, off_t position)
{
    //~ int rc;
    //~ off_t origPos;
    //~ //printf("want to wcSeekSet to 0x%X\n", (unsigned)position);fflush(NULL);
    //~ origPos = lseek(volInfo->imageForWriting, 0, SEEK_CUR);
    
    //~ if(position < origPos || position >= origPos + WRITE_CACHE_SIZE)
    //~ {
        //~ rc = wcFlush(volInfo);
        //~ if(rc <= 0)
            //~ return rc;
        
        //~ lseek(volInfo->imageForWriting, position, SEEK_SET);
    //~ }
    //~ else
    //~ {
        //~ volInfo->wcOffset = position - origPos;
        //~ if(volInfo->wcOffset > volInfo->wcNumBytesUsed)
            //~ volInfo->wcNumBytesUsed = volInfo->wcOffset;
    //~ }
    
    lseek(volInfo->imageForWriting, position, SEEK_SET);
    
    return 1;
}

off_t wcSeekTell(VolInfo* volInfo)
{
    //~ return lseek(volInfo->imageForWriting, 0, SEEK_CUR) + 
           //~ volInfo->wcOffset;
    
    return lseek(volInfo->imageForWriting, 0, SEEK_CUR);
}

int wcWrite(VolInfo* volInfo, const char* block, off_t numBytes)
{
    //~ off_t numBytesAvailable;
    //~ off_t numBytesNow;
    //~ off_t numBytesLeft;
    //~ int rc;
    //~ //printf("want to wcwrite %d bytes (offset is 0x%X+%d)\n", (unsigned)numBytes, (unsigned)lseek(volInfo->imageForWriting, 0, SEEK_CUR), (unsigned)volInfo->wcOffset);fflush(NULL);
    //~ numBytesAvailable = WRITE_CACHE_SIZE - volInfo->wcOffset;
    
    //~ if(numBytes < numBytesAvailable)
    //~ {
        //~ numBytesNow = numBytes;
        //~ numBytesLeft = 0;
    //~ }
    //~ else
    //~ {
        //~ numBytesNow = numBytesAvailable;
        //~ numBytesLeft = numBytes - numBytesAvailable;
    //~ }
    
    //~ memcpy(volInfo->writeCache + volInfo->wcOffset, 
           //~ block, numBytesNow);
    //~ memset(volInfo->writeCacheStatus + volInfo->wcOffset, 
           //~ CACHED_STATUS_SET, numBytesNow);
    //~ volInfo->wcOffset += numBytesNow;
    //~ if(volInfo->wcOffset > volInfo->wcNumBytesUsed)
        //~ volInfo->wcNumBytesUsed = volInfo->wcOffset;
    
    //~ if(numBytesLeft > 0)
    //~ {
        //~ rc = wcFlush(volInfo);
        //~ if(rc < 0)
            //~ return rc;
        
        //~ rc = wcWrite(volInfo, block + numBytesNow, numBytesLeft);
        //~ if(rc < 0)
            //~ return rc;
    //~ }
    
    int rc;
    rc = write(volInfo->imageForWriting, block, numBytes);
    if(rc != numBytes)
        return BKERROR_WRITE_GENERIC;
    
    if(volInfo->writeProgressFunction != NULL)
    {
        time_t timeNow;
        time(&timeNow);
        
        if(timeNow - volInfo->lastTimeCalledProgress >= 1)
        {
            double percentComplete;
            percentComplete = (double)wcSeekTell(volInfo) * 100 / 
                               volInfo->estimatedIsoSize;
            
            volInfo->writeProgressFunction(percentComplete);
            volInfo->lastTimeCalledProgress = timeNow;
        }
    }
    
    return 1;
}
