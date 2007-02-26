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

#include <stdbool.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <stdio.h>

#include "bkInternal.h"
#include "bkLink.h"

int addToHardLinkTable(VolInfo* volInfo, off_t position, char* pathAndName, 
                       unsigned size, bool onImage, BkHardLink** newLink)
{
    int rc;
    
    *newLink = malloc(sizeof(BkHardLink));
    if(*newLink == NULL)
        return BKERROR_OUT_OF_MEMORY;
    
    bzero(*newLink, sizeof(BkHardLink));
    
    (*newLink)->onImage = onImage;
    (*newLink)->position = position;
    if(pathAndName != NULL)
    {
        (*newLink)->pathAndName = malloc(strlen(pathAndName) + 1);
        if((*newLink)->pathAndName == NULL)
            return BKERROR_OUT_OF_MEMORY;
        strcpy((*newLink)->pathAndName, pathAndName);
    }
    (*newLink)->size = size;
    (*newLink)->next = volInfo->fileLocations;
    
    if(size < MAX_NBYTES_HARDLINK_HEAD)
        (*newLink)->headSize = size;
    else
        (*newLink)->headSize = MAX_NBYTES_HARDLINK_HEAD;
    
    rc = readFileHead(volInfo, position, pathAndName, (*newLink)->onImage, 
                      (*newLink)->head, (*newLink)->headSize);
    if(rc <= 0)
        return rc;
    
    volInfo->fileLocations = *newLink;
    
    return 1;
}

/* returns 2 if yes 1 if not */
int filesAreSame(int file1, int file2, unsigned size)
{
    const int blockSize = 102400;
    unsigned char* file1block[blockSize];
    unsigned char* file2block[blockSize];
    int numBlocks;
    int sizeLastBlock;
    int count;
    int rc;
    
    numBlocks = size / blockSize;
    sizeLastBlock = size % blockSize;
    
    for(count = 0; count < numBlocks; count++)
    {
        rc = read(file1, file1block, blockSize);
        if(rc != blockSize)
            return BKERROR_READ_GENERIC;
        rc = read(file2, file2block, blockSize);
        if(rc != blockSize)
            return BKERROR_READ_GENERIC;
        if(memcmp(file1block, file2block, blockSize) != 0)
            return 1;
    }
    
    if(sizeLastBlock > 0)
    {
        rc = read(file1, file1block, sizeLastBlock);
        if(rc != blockSize)
            return BKERROR_READ_GENERIC;
        rc = read(file2, file2block, sizeLastBlock);
        if(rc != blockSize)
            return BKERROR_READ_GENERIC;
        if(memcmp(file1block, file2block, sizeLastBlock) != 0)
            return 1;
    }
    
    return 2;
}

/* returns 2 if found 1 if not found */
int findInHardLinkTable(VolInfo* volInfo, off_t position, 
                        char* pathAndName, unsigned size,
                        bool onImage, BkHardLink** foundLink)
{
    BkHardLink* currentLink;
    unsigned char head[MAX_NBYTES_HARDLINK_HEAD];
    int headSize;
    int rc;
    
    *foundLink = NULL;
    
    if(size < MAX_NBYTES_HARDLINK_HEAD)
        headSize = size;
    else
        headSize = MAX_NBYTES_HARDLINK_HEAD;
    
    rc = readFileHead(volInfo, position, pathAndName, onImage, head, headSize);
    if(rc <= 0)
        return rc;
    
    currentLink = volInfo->fileLocations;
    while(currentLink != NULL)
    {
        if(size <= currentLink->size)
        {
            if( memcmp(head, currentLink->head, headSize) == 0 )
            {
                int newFile;
                bool newFileWasOpened;
                off_t origNewFilePos;
                int origFile;
                int origFileWasOpened;
                off_t origorigFilePos;
                
                /* set up original file */
                
                
                /* set up new file */
                if(onImage)
                {
                    newFile = volInfo->imageForReading;
                    origNewFilePos = lseek(volInfo->imageForReading, 0, SEEK_CUR);
                    newFileWasOpened = false;
                }
                else
                {
                    newFile = open(pathAndName, O_RDONLY);
                    if(newFile == -1)
                        return BKERROR_OPEN_READ_FAILED;
                    newFileWasOpened = true;
                }
                
                //~ rc = filesAreSame(srcFile, 
                
                if(!newFileWasOpened)
                    lseek(volInfo->imageForReading, origNewFilePos, SEEK_SET);
                else
                    close(newFile);
                
                *foundLink = currentLink;
                
                return 2;
            }
        }
        
        currentLink = currentLink->next;
    }
    
    return 1;
}

int readFileHead(VolInfo* volInfo, off_t position, char* pathAndName, 
                 bool onImage, unsigned char* dest, unsigned numBytes)
{
    int srcFile;
    bool srcFileWasOpened;
    off_t origPos;
    int rc;
    
    if(onImage)
    {
        srcFile = volInfo->imageForReading;
        origPos = lseek(volInfo->imageForReading, 0, SEEK_CUR);
        lseek(volInfo->imageForReading, position, SEEK_SET);
        srcFileWasOpened = false;
    }
    else
    {
        srcFile = open(pathAndName, O_RDONLY);
        if(srcFile == -1)
            return BKERROR_OPEN_READ_FAILED;
        srcFileWasOpened = true;
    }
    
    rc = read(srcFile, dest, numBytes);
    
    if(!srcFileWasOpened)
        lseek(volInfo->imageForReading, origPos, SEEK_SET);
    else
        close(srcFile);
    
    if(rc != numBytes)
        return BKERROR_READ_GENERIC;
    
    return 1;
}
