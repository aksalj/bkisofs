/******************************* LICENCE **************************************
* Any code in this file may be redistributed or modified under the terms of
* the GNU General Public Licence as published by the Free Software 
* Foundation; version 2 of the licence.
****************************** END LICENCE ***********************************/

#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>

#include "bk.h"
#include "bkRead.h"
#include "bkRead7x.h"
#include "bkTime.h"
#include "bkError.h"

/* numbers as recorded on image */
#define VDTYPE_BOOT 0
#define VDTYPE_PRIMARY 1
#define VDTYPE_SUPPLEMENTARY 2
#define VDTYPE_VOLUMEPARTITION 3
#define VDTYPE_TERMINATOR 255

/* these 2 are defined in bkExtract.c */
extern const unsigned posixDirDefaults;
extern const unsigned posixFileDefaults;

/*******************************************************************************
* bk_read_vol_info()
* public function to read volume information
* assumes pvd is first descriptor in set
* */
int bk_read_vol_info(int image, VolInfo* volInfo)
{
    int rc;
    unsigned char vdType; /* to check what descriptor follows */
    bool haveMorePvd; /* to skip extra pvds */
    unsigned char escapeSequence[3]; /* only interested in a joliet sequence */
    char timeString[17]; /* for creation time */
    
    /* for checking el torito */
    //~ char elToritoSig[24];
    //~ unsigned bootCatalogLocation; /* logical sector number */
    off_t locationOfNextDescriptor;
    
    /* vars for checking rockridge */
    unsigned realRootLoc; /* location of the root dr inside root dir */
    unsigned char recordLen; /* length of rood dr */
    unsigned char sPsUentry[7]; /* su entry SP */
    
    /* will always have this unless image is broken */
    volInfo->filenameTypes = FNTYPE_9660;
    
    /* might not have supplementary descriptor */
    volInfo->sRootDrOffset = 0;
    
    /* skip system area */
    lseek(image, NLS_SYSTEM_AREA * NBYTES_LOGICAL_BLOCK, SEEK_SET);
    
    /* READ PVD */
    /* make sure pvd exists */
    rc = read711(image, &vdType);
    if(rc != 1)
        return BKERROR_READ_GENERIC;
    
    /* first descriptor must be primary */
    if(vdType != VDTYPE_PRIMARY)
        return BKERROR_VD_NOT_PRIMARY;
    
    lseek(image, 39, SEEK_CUR);
    
    rc = read(image, volInfo->volId, 32);
    if(rc != 32)
        return -1;
    volInfo->volId[32] = '\0';
    stripSpacesFromEndOfString(volInfo->volId);
    
    lseek(image, 84, SEEK_CUR);
    
    /* am now at root dr */
    volInfo->pRootDrOffset = lseek(image, 0, SEEK_CUR);
    
    /* SEE if rockridge exists */
    lseek(image, 2, SEEK_CUR);
    
    rc = read733(image, &realRootLoc);
    if(rc != 8)
        return BKERROR_READ_GENERIC;
    realRootLoc *= NBYTES_LOGICAL_BLOCK;
    
    lseek(image, realRootLoc, SEEK_SET);
    
    rc = read711(image, &recordLen);
    if(rc != 1)
        return BKERROR_READ_GENERIC;
    
    if(recordLen >= 41)
    /* a minimum root with SP su field */
    {
        /* root dr has filename length of 1 */
        lseek(image, 33, SEEK_CUR);
        
        /* a rockridge root dr has an SP su entry here */
        
        rc = read(image, &sPsUentry, 7);
        if(rc != 7)
            return BKERROR_READ_GENERIC;
        
        if( sPsUentry[0] == 0x53 && sPsUentry[1] == 0x50 &&
            sPsUentry[2] == 7 && 
            sPsUentry[4] == 0xBE && sPsUentry[5] == 0xEF )
        /* rockridge it is */
        {
            volInfo->filenameTypes |= FNTYPE_ROCKRIDGE;
        }
    }
    
    /* go back to where it was before trying rockridge */
    lseek(image, volInfo->pRootDrOffset, SEEK_SET);
    /* END SEE if rockridge exists */
    
    lseek(image, 162, SEEK_CUR);
    
    rc = read(image, volInfo->publisher, 128);
    if(rc != 128)
        return BKERROR_READ_GENERIC;
    volInfo->publisher[128] = '\0';
    stripSpacesFromEndOfString(volInfo->publisher);
    
    rc = read(image, volInfo->dataPreparer, 128);
    if(rc != 128)
        return BKERROR_READ_GENERIC;
    volInfo->dataPreparer[128] = '\0';
    stripSpacesFromEndOfString(volInfo->dataPreparer);
    
    lseek(image, 239, SEEK_CUR);
    
    rc = read(image, timeString, 17);
    if(rc != 17)
        return BKERROR_READ_GENERIC;
    
    longStringToEpoch(timeString, &(volInfo->creationTime));
    
    /* skip the rest of the extent */
    lseek(image, 1218, SEEK_CUR);
    /* END READ PVD */
    
    /* SKIP all extra copies of pvd */
    haveMorePvd = true;
    while(haveMorePvd)
    {
        rc = read711(image, &vdType);
        if(rc != 1)
            return BKERROR_READ_GENERIC;
        
        if(vdType == VDTYPE_PRIMARY)
        {
            lseek(image, 2047, SEEK_CUR);
        }
        else
        {
            lseek(image, -1, SEEK_CUR);
            haveMorePvd = false;
        }
    }
    /* END SKIP all extra copies of pvd */
    
    /* TRY read boot record */
    locationOfNextDescriptor = lseek(image, 0, SEEK_CUR) + 2048;
    
    rc = read711(image, &vdType);
    if(rc != 1)
        return BKERROR_READ_GENERIC;
    
    if(vdType == VDTYPE_BOOT)
    {
        lseek(image, 2047, SEEK_CUR);
        //~ lseek(image, 6, SEEK_CUR);
        
        //~ rc = read(image, elToritoSig, 24);
        //~ if(rc != 24)
            //~ return -1;
        
        //~ if(strcmp(elToritoSig, "EL TORITO SPECIFICATION") == 0)
        //~ /* el torito confirmed */
        //~ {
            //~ lseek(image, 40, SEEK_CUR);
            
            //~ rc = read731(image, &bootCatalogLocation);
            //~ if(rc != 4)
                //~ return -1;
            
            //~ printf("boot catalog @%d\n", bootCatalogLocation);
            //~ lseek(image, bootCatalogLocation * NBYTES_LOGICAL_BLOCK, SEEK_SET);
            
            //~ /* skip validation entry */
            //~ lseek(image, 32, SEEK_CUR);
            
            //~ /* read initial/default entry location */
            
            
            //~ exit(0);
        //~ }
        //~ else
        //~ //!! unknown boot record type
        //~ {
            //~ printf("err, boot record not el torito\n");
            //~ lseek(image, 2018, SEEK_CUR);
        //~ }
    }
    else
    /* not boot record */
    {
        /* go back */
        lseek(image, -1, SEEK_CUR);
    }
    /* END TRY read boot record */
    
    /* TRY read svd */
    rc = read711(image, &vdType);
    if(rc != 1)
        return BKERROR_READ_GENERIC;
    
    if(vdType == VDTYPE_SUPPLEMENTARY)
    /* make sure it's joliet (by escape sequence) */
    {
        lseek(image, 87, SEEK_CUR);
        
        read(image, escapeSequence, 3);
        
        if( (escapeSequence[0] == 0x25 && escapeSequence[1] == 0x2F &&
             escapeSequence[2] == 0x40) ||
            (escapeSequence[0] == 0x25 && escapeSequence[1] == 0x2F &&
             escapeSequence[2] == 0x43) ||
            (escapeSequence[0] == 0x25 && escapeSequence[1] == 0x2F &&
             escapeSequence[2] == 0x45) )
        /* is indeed joliet */
        {
            lseek(image, 65, SEEK_CUR);
            
            volInfo->sRootDrOffset = lseek(image, 0, SEEK_CUR);
            
            volInfo->filenameTypes |= FNTYPE_JOLIET;
        }
    }
    /* END TRY read svd */
    
    return 1;
}

bool dirDrFollows(int image)
{
    unsigned char fileFlags;
    off_t origPos;
    int rc;
    
    origPos = lseek(image, 0, SEEK_CUR);
    
    lseek(image, 25, SEEK_CUR);
    
    rc = read711(image, &fileFlags);
    if(rc != 1)
        return BKERROR_READ_GENERIC;
    
    lseek(image, origPos, SEEK_SET);
    
    if((fileFlags >> 1 & 1) == 1)
        return true;
    else
        return false;
}

/* if the next byte is zero returns false otherwise true
* file position remains unchanged
* returns false on read error */
bool haveNextRecordInSector(int image)
{
    off_t origPos;
    char testByte;
    int rc;
    
    origPos = lseek(image, 0, SEEK_CUR);
    
    rc = read(image, &testByte, 1);
    if(rc != 1)
        return false;
    
    lseek(image, origPos, SEEK_SET);
    
    return (testByte == 0) ? false : true;
}

/*
* do not use this to read self or parent records unless it's the following:
* if the root dr (inside vd) is read, it's filename will be ""
* note: directory identifiers do not end with ";1"
*/
int readDir(int image, Dir* dir, int filenameType, bool readPosix)
{
    int rc;
    unsigned char recordLength;
    unsigned locExtent; /* to know where to go before readDir9660() */
    unsigned lenExtent; /* parameter to readDir9660() */
    unsigned char lenFileId9660; /* also len joliet fileid (bytes) */
    int lenSU; /* calculated as recordLength - 33 - lenFileId9660 */
    off_t origPos;
    char rootTestByte;
    bool isRoot;
    unsigned char recordableLenFileId; /* to prevent reading too long a name */
    
    rc = read(image, &recordLength, 1);
    if(rc != 1)
        return BKERROR_READ_GENERIC;
    
    lseek(image, 1, SEEK_CUR);
    
    rc = read733(image, &locExtent);
    if(rc != 8)
        return BKERROR_READ_GENERIC;
    
    rc = read733(image, &lenExtent);
    if(rc != 8)
        return BKERROR_READ_GENERIC;
    
    lseek(image, 14, SEEK_CUR);
    
    rc = read(image, &lenFileId9660, 1);
    if(rc != 1)
        return BKERROR_READ_GENERIC;
    
    lenSU = recordLength - 33 - lenFileId9660;
    if(lenFileId9660 % 2 == 0)
        lenSU -= 1;
    
    /* FIND out if root */
    origPos = lseek(image, 0, SEEK_CUR);
    
    rc = read(image, &rootTestByte, 1);
    if(rc != 1)
        return BKERROR_READ_GENERIC;
    
    lseek(image, origPos, SEEK_SET);
    
    if(lenFileId9660 == 1 && rootTestByte == 0x00)
    {
        isRoot = true;
        dir->name[0] = '\0';
    }
    else
        isRoot = false;
    /* END FIND out if root */
    
    /* the data structures cannot handle filenames longer then NCHARS_FILE_ID_MAX
    * so in case the image contains an invalid, long filename, trunkate it
    * rather then corrupt memory */
    if(lenFileId9660 > NCHARS_FILE_ID_MAX - 1)
        recordableLenFileId = NCHARS_FILE_ID_MAX - 1;
    else
        recordableLenFileId = lenFileId9660;
    
    /* READ directory name */
    if(filenameType == FNTYPE_9660)
    {
        if(!isRoot)
        {
            char nameAsOnDisk[256];
            
            rc = read(image, nameAsOnDisk, lenFileId9660);
            if(rc != lenFileId9660)
                return BKERROR_READ_GENERIC;
            
            strncpy(dir->name, nameAsOnDisk, recordableLenFileId);
            dir->name[recordableLenFileId] = '\0';
            
            /* skip padding field if it's there */
            if(lenFileId9660 % 2 == 0)
                lseek(image, 1, SEEK_CUR);
        }
    }
    else if(filenameType == FNTYPE_JOLIET)
    {
        if(!isRoot)
        {
            char nameAsOnDisk[256];
            char nameInAscii[256];
            int ucsCount, byteCount;
            
            /* ucs2 byte count must be even */
            if(lenFileId9660 % 2 != 0)
                return BKERROR_INVALID_UCS2;
            
            rc = read(image, nameAsOnDisk, lenFileId9660);
            if(rc != lenFileId9660)
                return BKERROR_READ_GENERIC;
            
            for(ucsCount = 1, byteCount = 0; ucsCount < lenFileId9660;
                ucsCount += 2, byteCount += 1)
            {
                nameInAscii[byteCount] = nameAsOnDisk[ucsCount];
            }
            nameInAscii[byteCount] = '\0';
            
            strncpy(dir->name, nameInAscii, recordableLenFileId);
            dir->name[recordableLenFileId] = '\0';
            
            /* padding field */
            if(lenFileId9660 % 2 == 0)
                lseek(image, 1, SEEK_CUR);
        }
    }
    else if(filenameType == FNTYPE_ROCKRIDGE)
    {
        if(!isRoot)
        {
            /* skip 9660 filename */
            lseek(image, lenFileId9660, SEEK_CUR);
            /* skip padding field */
            if(lenFileId9660 % 2 == 0)
                lseek(image, 1, SEEK_CUR);
            
            rc = readRockridgeFilename(image, dir->name, lenSU);
            if(rc < 0)
                return rc;
        }
    }
    else
        return BKERROR_UNKNOWN_FILENAME_TYPE;
    /* END READ directory name */
    
    if(readPosix)
    {
        if(isRoot)
        {
            unsigned char realRootRecordLen;
            
            origPos = lseek(image, 0, SEEK_CUR);
            
            /* go to real root record */
            lseek(image, locExtent * NBYTES_LOGICAL_BLOCK, SEEK_SET);
            
            /* read record length */
            read(image, &realRootRecordLen, 1);
            if(rc != 1)
                return BKERROR_READ_GENERIC;
            
            /* go to sys use fields */
            lseek(image, 33, SEEK_CUR);
            
            rc = readPosixInfo(image, &(dir->posixFileMode), realRootRecordLen - 34);
            if(rc <= 0)
                return rc;
            
            /* return */
            lseek(image, origPos, SEEK_SET);
        }
        else
        {
            rc = readPosixInfo(image, &(dir->posixFileMode), lenSU);
            if(rc <= 0)
                return rc;
        }
    }
    else
    {
        /* this is good for root also */
        dir->posixFileMode = posixDirDefaults;
    }
    
    lseek(image, lenSU, SEEK_CUR);
    
    origPos = lseek(image, 0, SEEK_CUR);
    
    lseek(image, locExtent * NBYTES_LOGICAL_BLOCK, SEEK_SET);
    
    rc = readDirContents(image, dir, lenExtent, filenameType, readPosix);
    if(rc < 0)
        return rc;
    
    lseek(image, origPos, SEEK_SET);
    
    return recordLength;
}

/*
* size is number of bytes
* hope you love pointers
*/
int readDirContents(int image, Dir* dir, unsigned size, int filenameType, bool readPosix)
{
    int rc;
    int bytesRead = 0;
    int childrenBytesRead;
    DirLL** nextDir; /* pointer to pointer to modify pointer :) */
    FileLL** nextFile; /* ditto */
    
    /* skip self and parent */
    bytesRead += skipDR(image);
    bytesRead += skipDR(image);
    
    dir->directories = NULL;
    dir->files = NULL;
    nextDir = &(dir->directories);
    nextFile = &(dir->files);
    childrenBytesRead = 0;
    while(childrenBytesRead + bytesRead < size)
    {
        if(haveNextRecordInSector(image))
        /* read it */
        {
            if( dirDrFollows(image) )
            /* directory descriptor record */
            {
                int recordLength;
                
                *nextDir = malloc(sizeof(DirLL));
                if(*nextDir == NULL)
                    return BKERROR_OUT_OF_MEMORY;
                
                recordLength = readDir(image, &((*nextDir)->dir), filenameType, readPosix);
                if(recordLength < 0)
                    return recordLength;
                
                childrenBytesRead += recordLength;
                
                nextDir = &((*nextDir)->next);
                *nextDir = NULL;
            }
            else
            /* file descriptor record */
            {
                int recordLength;
                
                *nextFile = malloc(sizeof(FileLL));
                if(*nextFile == NULL)
                    return BKERROR_OUT_OF_MEMORY;
                
                recordLength = readFileInfo(image, &((*nextFile)->file), filenameType, readPosix);
                if(recordLength < 0)
                    return recordLength;
                
                childrenBytesRead += recordLength;
                
                nextFile = &((*nextFile)->next);
                *nextFile = NULL;
            }
        }
        else
        /* read zeroes until get to next record (that would be in the next
        *  sector btw) or get to the end of data (dir->self.dataLength) */
        {
            char testByte;
            off_t origPos;
            
            do
            {
                origPos = lseek(image, 0, SEEK_CUR);
                
                rc = read(image, &testByte, 1);
                if(rc != 1)
                    return BKERROR_READ_GENERIC;
                
                if(testByte != 0)
                {
                    lseek(image, origPos, SEEK_SET);
                    break;
                }
                
                childrenBytesRead += 1;
                
            } while (childrenBytesRead + bytesRead < size);
        }
    }
    
    return bytesRead;
}

int readFileInfo(int image, File* file, int filenameType, bool readPosix)
{
    int rc;
    unsigned char recordLength;
    unsigned locExtent; /* block num where the file is */
    unsigned lenExtent; /* in bytes */
    unsigned char lenFileId9660; /* also len joliet fileid (bytes) */
    int lenSU; /* calculated as recordLength - 33 - lenFileId9660 */
    
    rc = read(image, &recordLength, 1);
    if(rc != 1)
        return BKERROR_READ_GENERIC;
    
    lseek(image, 1, SEEK_CUR);
    
    rc = read733(image, &locExtent);
    if(rc != 8)
        return BKERROR_READ_GENERIC;
    
    rc = read733(image, &lenExtent);
    if(rc != 8)
        return BKERROR_READ_GENERIC;
    
    lseek(image, 14, SEEK_CUR);
    
    rc = read(image, &lenFileId9660, 1);
    if(rc != 1)
        return BKERROR_READ_GENERIC;
    
    lenSU = recordLength - 33 - lenFileId9660;
    if(lenFileId9660 % 2 == 0)
        lenSU -= 1;
    
    if(filenameType == FNTYPE_9660)
    {
        char nameAsOnDisk[256];
        
        rc = read(image, nameAsOnDisk, lenFileId9660);
        if(rc != lenFileId9660)
            return BKERROR_READ_GENERIC;
        
        removeCrapFromFilename(nameAsOnDisk, lenFileId9660);
        
        if( strlen(nameAsOnDisk) > NCHARS_FILE_ID_MAX - 1 )
            return BKERROR_MAX_NAME_LENGTH_EXCEEDED;
        
        strcpy(file->name, nameAsOnDisk);
        
        /* padding field */
        if(lenFileId9660 % 2 == 0)
            lseek(image, 1, SEEK_CUR);
    }
    else if(filenameType == FNTYPE_JOLIET)
    {
        char nameAsOnDisk[256];
        char nameInAscii[256];
        int ucsCount, byteCount;
        
        if(lenFileId9660 % 2 != 0)
            return BKERROR_INVALID_UCS2;
        
        rc = read(image, nameAsOnDisk, lenFileId9660);
        if(rc != lenFileId9660)
            return BKERROR_READ_GENERIC;
        
        for(ucsCount = 1, byteCount = 0; ucsCount < lenFileId9660;
            ucsCount += 2, byteCount += 1)
        {
            nameInAscii[byteCount] = nameAsOnDisk[ucsCount];
        }
        
        removeCrapFromFilename(nameInAscii, lenFileId9660 / 2);
        
        if( strlen(nameInAscii) > NCHARS_FILE_ID_MAX - 1 )
            return BKERROR_MAX_NAME_LENGTH_EXCEEDED;
        
        strcpy(file->name, nameInAscii);
        
        /* padding field */
        if(lenFileId9660 % 2 == 0)
            lseek(image, 1, SEEK_CUR);
    }
    else if(filenameType == FNTYPE_ROCKRIDGE)
    {
        /* skip 9660 filename */
        lseek(image, lenFileId9660, SEEK_CUR);
        /* skip padding field */
        if(lenFileId9660 % 2 == 0)
            lseek(image, 1, SEEK_CUR);
        
        rc = readRockridgeFilename(image, file->name, lenSU);
        if(rc < 0)
            return rc;
    }
    else
        return BKERROR_UNKNOWN_FILENAME_TYPE;
    
    if(readPosix)
    {
        readPosixInfo(image, &(file->posixFileMode), lenSU);
    }
    else
    {
        file->posixFileMode = posixFileDefaults;
    }
    
    lseek(image, lenSU, SEEK_CUR);
    
    file->onImage = true;
    file->position = locExtent * NBYTES_LOGICAL_BLOCK;
    file->size = lenExtent;
    file->pathAndName = NULL;
    
    return recordLength;
}

int readPosixInfo(int image, unsigned* posixFileMode, int lenSU)
{
    off_t origPos;
    unsigned char suFields[256];
    int rc;
    bool foundPosix;
    int count;
    
    origPos = lseek(image, 0, SEEK_CUR);
    
    rc = read(image, suFields, lenSU);
    if(rc != lenSU)
        return BKERROR_READ_GENERIC;
    
    lseek(image, origPos, SEEK_SET);
    
    count = 0;
    foundPosix = false;
    while(count < lenSU && !foundPosix)
    {
        if(suFields[count] == 'P' && suFields[count + 1] == 'X')
        {
            read733FromCharArray(suFields + count + 4, posixFileMode);
            
            /* not interested in anything else from this field */
            
            foundPosix = true;
        }
        else
        /* skip su record */
        {
            count += suFields[count + 2];
        }
    }
    
    if(!foundPosix)
        return BKERROR_NO_POSIX_PRESENT;
    
    return 1;
}

/*
* leaves the file pointer where it was
*/
int readRockridgeFilename(int image, char* dest, int lenSU)
{
    off_t origPos;
    unsigned char suFields[256];
    int rc;
    int count;
    int lengthAsRead;
    bool foundName;
    int recordableLenFileId;
    
    origPos = lseek(image, 0, SEEK_CUR);
    
    rc = read(image, suFields, lenSU);
    if(rc != lenSU)
        return BKERROR_READ_GENERIC;
    
    lseek(image, origPos, SEEK_SET);
    
    count = 0;
    foundName = false;
    while(count < lenSU && !foundName)
    {
        if(suFields[count] == 'N' && suFields[count + 1] == 'M')
        {
            lengthAsRead = suFields[count + 2] - 5;
            
            /* the data structures cannot handle filenames longer then NCHARS_FILE_ID_MAX
            * so in case the image contains an invalid, long filename, trunkate it
            * rather then corrupt memory */
            if(lengthAsRead > NCHARS_FILE_ID_MAX - 1)
                recordableLenFileId = NCHARS_FILE_ID_MAX - 1;
            else
                recordableLenFileId = lengthAsRead;
            
            strncpy(dest, suFields + count + 5, recordableLenFileId);
            dest[recordableLenFileId] = '\0';
            
            foundName = true;
        }
        else
        /* skip su record */
        {
            count += suFields[count + 2];
        }
    }
    
    if(!foundName)
        return BKERROR_RR_FILENAME_MISSING;
    else
        return 1;
}

/*
* filenames as read from 9660 Sometimes end with ;1 (terminator+version num)
* this removes the useless ending and terminates the destination with a '\0'
*/
void removeCrapFromFilename(char* filename, int length)
{
    int count;
    bool stop = false;
    
    for(count = 0; count < NCHARS_FILE_ID_MAX_READ && count < length && !stop; count++)
    {
        if(filename[count] == ';')
        {
            filename[count] = '\0';
            stop = true;
        }
    }
    
    /* if did not get a ';' terminate string anyway */
    filename[count] = '\0';
}

int skipDR(int image)
{
    unsigned char dRLen;
    int rc;
    
    rc = read711(image, &dRLen);
    if(rc != 1)
        return BKERROR_READ_GENERIC;
    
    lseek(image, dRLen - 1, SEEK_CUR);
    
    return dRLen;
}

void stripSpacesFromEndOfString(char* str)
{
    int count;
    
    for(count = strlen(str) - 1; count >= 0 && str[count] == ' '; count--)
    {
        str[count] = '\0';
    }
}
