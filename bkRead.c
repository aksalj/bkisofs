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

#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>

#include "bk.h"
#include "bkInternal.h"
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

/* for el torito boot images */
#define NBYTES_VIRTUAL_SECTOR 512

/*******************************************************************************
* bk_open_image()
* 
* */
int bk_open_image(VolInfo* volInfo, const char* filename)
{
    int rc;
    struct stat statStruct;
    
    volInfo->imageForReading = open(filename, O_RDONLY);
    if(volInfo->imageForReading == -1)
    {
        volInfo->imageForReading = 0;
        return BKERROR_OPEN_READ_FAILED;
    }
    
    /* record inode number */
    rc = stat(filename, &statStruct);
    if(rc == -1)
        return BKERROR_STAT_FAILED;

    volInfo->imageForReadingInode = statStruct.st_ino;
    
    
    return 1;
}

/*******************************************************************************
* bk_read_dir_tree()
* filenameType can be only one (do not | more then one)
* */
int bk_read_dir_tree(VolInfo* volInfo, int filenameType, bool readPosix)
{
    if(filenameType == FNTYPE_ROCKRIDGE || filenameType == FNTYPE_9660)
        lseek(volInfo->imageForReading, volInfo->pRootDrOffset, SEEK_SET);
    else /* if(filenameType == FNTYPE_JOLIET) */
        lseek(volInfo->imageForReading, volInfo->sRootDrOffset, SEEK_SET);
    
    return readDir(volInfo->imageForReading, volInfo, &(volInfo->dirTree), 
                   filenameType, readPosix);
}

/*******************************************************************************
* bk_read_vol_info()
* public function to read volume information
* assumes pvd is first descriptor in set
* */
int bk_read_vol_info(VolInfo* volInfo)
{
    int rc;
    unsigned char vdType; /* to check what descriptor follows */
    bool haveMorePvd; /* to skip extra pvds */
    unsigned char escapeSequence[3]; /* only interested in a joliet sequence */
    char timeString[17]; /* for creation time */
    
    /* vars for checking rockridge */
    unsigned realRootLoc; /* location of the root dr inside root dir */
    unsigned char recordLen; /* length of rood dr */
    unsigned char sPsUentry[7]; /* su entry SP */
    
    /* will always have this unless image is broken */
    volInfo->filenameTypes = FNTYPE_9660;
    
    /* might not have supplementary descriptor */
    volInfo->sRootDrOffset = 0;
    
    /* skip system area */
    lseek(volInfo->imageForReading, NLS_SYSTEM_AREA * NBYTES_LOGICAL_BLOCK, SEEK_SET);
    
    /* READ PVD */
    /* make sure pvd exists */
    rc = read711(volInfo->imageForReading, &vdType);
    if(rc != 1)
        return BKERROR_READ_GENERIC;
    
    /* first descriptor must be primary */
    if(vdType != VDTYPE_PRIMARY)
        return BKERROR_VD_NOT_PRIMARY;
    
    lseek(volInfo->imageForReading, 39, SEEK_CUR);
    
    rc = read(volInfo->imageForReading, volInfo->volId, 32);
    if(rc != 32)
        return BKERROR_READ_GENERIC;
    volInfo->volId[32] = '\0';
    stripSpacesFromEndOfString(volInfo->volId);
    
    lseek(volInfo->imageForReading, 84, SEEK_CUR);
    
    /* am now at root dr */
    volInfo->pRootDrOffset = lseek(volInfo->imageForReading, 0, SEEK_CUR);
    
    /* SEE if rockridge exists */
    lseek(volInfo->imageForReading, 2, SEEK_CUR);
    
    rc = read733(volInfo->imageForReading, &realRootLoc);
    if(rc != 8)
        return BKERROR_READ_GENERIC;
    realRootLoc *= NBYTES_LOGICAL_BLOCK;
    
    lseek(volInfo->imageForReading, realRootLoc, SEEK_SET);
    
    rc = read711(volInfo->imageForReading, &recordLen);
    if(rc != 1)
        return BKERROR_READ_GENERIC;
    
    if(recordLen >= 41)
    /* a minimum root with SP su field */
    {
        /* root dr has filename length of 1 */
        lseek(volInfo->imageForReading, 33, SEEK_CUR);
        
        /* a rockridge root dr has an SP su entry here */
        
        rc = read(volInfo->imageForReading, &sPsUentry, 7);
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
    lseek(volInfo->imageForReading, volInfo->pRootDrOffset, SEEK_SET);
    /* END SEE if rockridge exists */
    
    lseek(volInfo->imageForReading, 162, SEEK_CUR);
    
    rc = read(volInfo->imageForReading, volInfo->publisher, 128);
    if(rc != 128)
        return BKERROR_READ_GENERIC;
    volInfo->publisher[128] = '\0';
    stripSpacesFromEndOfString(volInfo->publisher);
    
    rc = read(volInfo->imageForReading, volInfo->dataPreparer, 128);
    if(rc != 128)
        return BKERROR_READ_GENERIC;
    volInfo->dataPreparer[128] = '\0';
    stripSpacesFromEndOfString(volInfo->dataPreparer);
    
    lseek(volInfo->imageForReading, 239, SEEK_CUR);
    
    rc = read(volInfo->imageForReading, timeString, 17);
    if(rc != 17)
        return BKERROR_READ_GENERIC;
    
    longStringToEpoch(timeString, &(volInfo->creationTime));
    
    /* skip the rest of the extent */
    lseek(volInfo->imageForReading, 1218, SEEK_CUR);
    /* END READ PVD */
    
    /* SKIP all extra copies of pvd */
    haveMorePvd = true;
    while(haveMorePvd)
    {
        rc = read711(volInfo->imageForReading, &vdType);
        if(rc != 1)
            return BKERROR_READ_GENERIC;
        
        if(vdType == VDTYPE_PRIMARY)
        {
            lseek(volInfo->imageForReading, 2047, SEEK_CUR);
        }
        else
        {
            lseek(volInfo->imageForReading, -1, SEEK_CUR);
            haveMorePvd = false;
        }
    }
    /* END SKIP all extra copies of pvd */
    
    /* TRY read boot record */
    off_t locationOfNextDescriptor;
    unsigned bootCatalogLocation; /* logical sector number */
    char elToritoSig[24];
    unsigned char bootMediaType;
    
    locationOfNextDescriptor = lseek(volInfo->imageForReading, 0, SEEK_CUR) + 2048;
    
    rc = read711(volInfo->imageForReading, &vdType);
    if(rc != 1)
        return BKERROR_READ_GENERIC;
    
    if(vdType == VDTYPE_BOOT)
    {
        
        lseek(volInfo->imageForReading, 6, SEEK_CUR);
        
        rc = read(volInfo->imageForReading, elToritoSig, 24);
        if(rc != 24)
            return BKERROR_READ_GENERIC;
        elToritoSig[23] = '\0'; /* just in case */
        
        if(strcmp(elToritoSig, "EL TORITO SPECIFICATION") == 0)
        /* el torito confirmed */
        {
            lseek(volInfo->imageForReading, 40, SEEK_CUR);
            
            rc = read731(volInfo->imageForReading, &bootCatalogLocation);
            if(rc != 4)
                return BKERROR_READ_GENERIC;
            
            lseek(volInfo->imageForReading, bootCatalogLocation * NBYTES_LOGICAL_BLOCK, SEEK_SET);
            
            /* skip validation entry */
            lseek(volInfo->imageForReading, 32, SEEK_CUR);
            
            /* skip boot indicator */
            lseek(volInfo->imageForReading, 1, SEEK_CUR);
            
            rc = read(volInfo->imageForReading, &bootMediaType, 1);
            if(rc != 1)
                return BKERROR_READ_GENERIC;
            if(bootMediaType == 0)
                volInfo->bootMediaType = BOOT_MEDIA_NO_EMULATION;
            else if(bootMediaType == 1)
                volInfo->bootMediaType = BOOT_MEDIA_1_2_FLOPPY;
            else if(bootMediaType == 2)
                volInfo->bootMediaType = BOOT_MEDIA_1_44_FLOPPY;
            else if(bootMediaType == 3)
                volInfo->bootMediaType = BOOT_MEDIA_2_88_FLOPPY;
            else if(bootMediaType == 4)
            {
                //!! print warning
                printf("hard disk boot emulation not supported\n");
                volInfo->bootMediaType = BOOT_MEDIA_NONE;
            }
            else
            {
                //!! print warning
                printf("unknown boot media type on iso\n");
                volInfo->bootMediaType = BOOT_MEDIA_NONE;
            }
            
            /* skip load segment, system type and unused byte */
            lseek(volInfo->imageForReading, 4, SEEK_CUR);
            
            rc = read(volInfo->imageForReading, &(volInfo->bootRecordSize), 2);
            if(rc != 2)
                return BKERROR_READ_GENERIC;
            
            if(volInfo->bootMediaType == BOOT_MEDIA_NO_EMULATION)
                volInfo->bootRecordSize *= NBYTES_VIRTUAL_SECTOR;
            else if(bootMediaType == BOOT_MEDIA_1_2_FLOPPY)
                volInfo->bootRecordSize = 1200 * 1024;
            else if(bootMediaType == BOOT_MEDIA_1_44_FLOPPY)
                volInfo->bootRecordSize = 1440 * 1024;
            else if(bootMediaType == BOOT_MEDIA_2_88_FLOPPY)
                volInfo->bootRecordSize = 2880 * 1024;
            
            volInfo->bootRecordIsOnImage = true;
            
            rc = read(volInfo->imageForReading, &(volInfo->bootRecordOffset), 4);
            if(rc != 4)
                return BKERROR_READ_GENERIC;
            volInfo->bootRecordOffset *= NBYTES_LOGICAL_BLOCK;
        }
        else
            //!! print warning
            printf("err, boot record not el torito\n");
        
        /* go to the sector after the boot record */
        lseek(volInfo->imageForReading, locationOfNextDescriptor, SEEK_SET);
    }
    else
    /* not boot record */
    {
        /* go back */
        lseek(volInfo->imageForReading, -1, SEEK_CUR);
    }
    /* END TRY read boot record */
    
    /* TRY read svd */
    rc = read711(volInfo->imageForReading, &vdType);
    if(rc != 1)
        return BKERROR_READ_GENERIC;
    
    if(vdType == VDTYPE_SUPPLEMENTARY)
    /* make sure it's joliet (by escape sequence) */
    {
        lseek(volInfo->imageForReading, 87, SEEK_CUR);
        
        read(volInfo->imageForReading, escapeSequence, 3);
        
        if( (escapeSequence[0] == 0x25 && escapeSequence[1] == 0x2F &&
             escapeSequence[2] == 0x40) ||
            (escapeSequence[0] == 0x25 && escapeSequence[1] == 0x2F &&
             escapeSequence[2] == 0x43) ||
            (escapeSequence[0] == 0x25 && escapeSequence[1] == 0x2F &&
             escapeSequence[2] == 0x45) )
        /* is indeed joliet */
        {
            lseek(volInfo->imageForReading, 65, SEEK_CUR);
            
            volInfo->sRootDrOffset = lseek(volInfo->imageForReading, 0, SEEK_CUR);
            
            volInfo->filenameTypes |= FNTYPE_JOLIET;
        }
    }
    /* END TRY read svd */
    
    return 1;
}

/*******************************************************************************
* dirDrFollows()
* checks whether the next directory record is for a directory (not a file)
* */
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

/*******************************************************************************
* haveNextRecordInSector()
* If a directory record won't fit into what's left in a logical block, the rest
* of the block is filled with 0s. This function checks whether that's the case.
* If the next byte is zero returns false otherwise true
* File position remains unchanged
* Also returns false on read error */
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

/*******************************************************************************
* readDir()
* Reads a directory record for a directory (not a file)
* Do not use this to read self or parent records unless it's the following:
* - if the root dr (inside vd) is read, it's filename will be ""
* filenameType can be only one (do not | more then one)
*
* note to self: directory identifiers do not end with ";1"
*
* */
int readDir(int image, VolInfo* volInfo, Dir* dir, int filenameType, 
            bool readPosix)
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
    
    /* should anything fail, will still be safe to delete dir, this also
    * needs to be done before calling readDirContents() (now is good) */
    dir->directories = NULL;
    dir->files = NULL;
    
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
    
    recordableLenFileId = lenFileId9660;
    
    /* READ directory name */
    if(filenameType == FNTYPE_9660)
    {
        if(!isRoot)
        {
            char nameAsOnDisk[UCHAR_MAX];
            
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
            char nameAsOnDisk[UCHAR_MAX];
            /* in the worst possible case i'll use 129 bytes for this: */
            char nameInAscii[UCHAR_MAX];
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
            
            rc = readRockridgeFilename(image, dir->name, lenSU, 0);
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
        dir->posixFileMode = volInfo->posixDirDefaults;
    }
    
    lseek(image, lenSU, SEEK_CUR);
    
    origPos = lseek(image, 0, SEEK_CUR);
    
    lseek(image, locExtent * NBYTES_LOGICAL_BLOCK, SEEK_SET);
    
    rc = readDirContents(image, volInfo, dir, lenExtent, filenameType, readPosix);
    if(rc < 0)
        return rc;
    
    lseek(image, origPos, SEEK_SET);
    
    return recordLength;
}

/*******************************************************************************
* readDirContents()
* Reads the extent pointed to from a directory record of a directory.
* size is number of bytes
* */
int readDirContents(int image, VolInfo* volInfo, Dir* dir, unsigned size, 
                    int filenameType, bool readPosix)
{
    int rc;
    int bytesRead = 0;
    int childrenBytesRead;
    DirLL** nextDir; /* pointer to pointer to modify pointer :) */
    FileLL** nextFile; /* ditto */
    
    /* skip self and parent */
    rc = skipDR(image);
    if(rc <= 0)
        return rc;
    bytesRead += rc;
    rc = skipDR(image);
    if(rc <= 0)
        return rc;
    bytesRead += rc;
    
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
                
                recordLength = readDir(image, volInfo, &((*nextDir)->dir), filenameType,
                                       readPosix);
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
                
                recordLength = readFileInfo(image, volInfo, 
                                            &((*nextFile)->file), filenameType,
                                            readPosix);
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

/*******************************************************************************
* readFileInfo()
* Reads the directory record for a file
* */
int readFileInfo(int image, VolInfo* volInfo, File* file, int filenameType, 
                 bool readPosix)
{
    int rc;
    unsigned char recordLength;
    unsigned locExtent; /* block num where the file is */
    unsigned lenExtent; /* in bytes */
    unsigned char lenFileId9660; /* also len joliet fileid (bytes) */
    int lenSU; /* calculated as recordLength - 33 - lenFileId9660 */
    
    /* so if anything failes it's still safe to delete file */
    file->pathAndName = NULL;
    
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
    
    /* The length of isolinux.bin given in the initial/default entry of
    * the el torito boot catalog does not match the actual length of the file
    * but apparently when executed by the bios that's not a problem.
    * However, if i ever want to read that file myself, i need 
    * the length proper.
    * So i'm looking for a file that starts in the same logical sector as the
    * boot record from the initial/default entry. */
    if(volInfo->bootMediaType == BOOT_MEDIA_NO_EMULATION && 
       locExtent == volInfo->bootRecordOffset / NBYTES_LOGICAL_BLOCK)
    {
        volInfo->bootRecordSize = lenExtent;
        
        volInfo->bootRecordIsVisible = true;
        volInfo->bootRecordOnImage = file;
    }
    
    lseek(image, 14, SEEK_CUR);
    
    rc = read(image, &lenFileId9660, 1);
    if(rc != 1)
        return BKERROR_READ_GENERIC;
    
    lenSU = recordLength - 33 - lenFileId9660;
    if(lenFileId9660 % 2 == 0)
        lenSU -= 1;
    
    if(filenameType == FNTYPE_9660)
    {
        char nameAsOnDisk[UCHAR_MAX];
        
        rc = read(image, nameAsOnDisk, lenFileId9660);
        if(rc != lenFileId9660)
            return BKERROR_READ_GENERIC;
        
        removeCrapFromFilename(nameAsOnDisk, lenFileId9660);
        
        strncpy(file->name, nameAsOnDisk, NCHARS_FILE_ID_MAX_STORE - 1);
        file->name[NCHARS_FILE_ID_MAX_STORE - 1] = '\0';
        
        /* padding field */
        if(lenFileId9660 % 2 == 0)
            lseek(image, 1, SEEK_CUR);
    }
    else if(filenameType == FNTYPE_JOLIET)
    {
        char nameAsOnDisk[UCHAR_MAX];
        /* in the worst possible case i'll use 129 bytes for this: */
        char nameInAscii[UCHAR_MAX];
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
        
        if( strlen(nameInAscii) > NCHARS_FILE_ID_MAX_STORE - 1 )
            return BKERROR_MAX_NAME_LENGTH_EXCEEDED;
        
        strncpy(file->name, nameInAscii, NCHARS_FILE_ID_MAX_STORE - 1);
        file->name[NCHARS_FILE_ID_MAX_STORE - 1] = '\0';
        
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
        
        rc = readRockridgeFilename(image, file->name, lenSU, 0);
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
        file->posixFileMode = volInfo->posixFileDefaults;
    }
    
    lseek(image, lenSU, SEEK_CUR);
    
    file->onImage = true;
    file->position = locExtent * NBYTES_LOGICAL_BLOCK;
    file->size = lenExtent;
    
    return recordLength;
}

/*******************************************************************************
* readPosixInfo()
* looks for the PX system use field and gets the permissions field out of it
* */
int readPosixInfo(int image, unsigned* posixFileMode, unsigned lenSU)
{
    off_t origPos;
    unsigned char* suFields;
    int rc;
    bool foundPosix;
    bool foundCE;
    int count;
    unsigned logicalBlockOfCE;
    unsigned offsetInLogicalBlockOfCE;
    unsigned lengthOfCE; /* in bytes */
    
    suFields = malloc(lenSU);
    if(suFields == NULL)
        return BKERROR_OUT_OF_MEMORY;
    
    origPos = lseek(image, 0, SEEK_CUR);
    
    rc = read(image, suFields, lenSU);
    if(rc != lenSU)
        return BKERROR_READ_GENERIC;
    
    count = 0;
    foundPosix = false;
    foundCE = false;
    while(count < lenSU && !foundPosix)
    {
        if(suFields[count] == 'P' && suFields[count + 1] == 'X')
        {
            read733FromCharArray(suFields + count + 4, posixFileMode);
            
            /* not interested in anything else from this field */
            
            foundPosix = true;
        }
        else if(suFields[count] == 'C' && suFields[count + 1] == 'E')
        {
            foundCE = true;
            read733FromCharArray(suFields + count + 4, &logicalBlockOfCE);
            read733FromCharArray(suFields + count + 12, &offsetInLogicalBlockOfCE);
            read733FromCharArray(suFields + count + 20, &lengthOfCE);
        }
        
        /* skip su record */
        count += suFields[count + 2];
    }
    
    free(suFields);
    lseek(image, origPos, SEEK_SET);
    
    if(!foundPosix)
    {
        if(!foundCE)
            return BKERROR_NO_POSIX_PRESENT;
        else
        {
            lseek(image, logicalBlockOfCE * NBYTES_LOGICAL_BLOCK + offsetInLogicalBlockOfCE, SEEK_SET);
            rc = readPosixInfo(image, posixFileMode, lengthOfCE);
            
            lseek(image, origPos, SEEK_SET);
            
            return rc;
        }
    }
    
    return 1;
}

/*******************************************************************************
* readRockridgeFilename()
* Finds the NM entry in the system use fields and reads a maximum of
* NCHARS_FILE_ID_MAX_STORE characters from it (truncates if necessary).
* The continue system use field is not implemented so if the name is not in
* this directory record, the function returns a failure.
* Leaves the file pointer where it was.
*/
int readRockridgeFilename(int image, char* dest, unsigned lenSU, 
                          unsigned numCharsReadAlready)
{
    off_t origPos;
    unsigned char* suFields;
    int rc;
    int count;
    int lengthThisNM;
    int usableLenThisNM;
    bool foundName;
    bool nameContinues; /* in another NM entry */
    bool foundCE;
    unsigned logicalBlockOfCE;
    unsigned offsetInLogicalBlockOfCE;
    unsigned lengthOfCE; /* in bytes */
    
    suFields = malloc(lenSU);
    if(suFields == NULL)
        return BKERROR_OUT_OF_MEMORY;
    
    origPos = lseek(image, 0, SEEK_CUR);
    
    rc = read(image, suFields, lenSU);
    if(rc != lenSU)
    {
        free(suFields);
        return BKERROR_READ_GENERIC;
    }
    
    count = 0;
    foundName = false;
    nameContinues = false;
    foundCE = false;
    while(count < lenSU)
    {
        if(suFields[count] == 'N' && suFields[count + 1] == 'M')
        {
            lengthThisNM = suFields[count + 2] - 5;
            
            /* the data structures cannot handle filenames longer than 
            * NCHARS_FILE_ID_MAX_STORE so in case the image contains an 
            * invalid, long filename, truncate it rather than corrupt memory */
            if(lengthThisNM + numCharsReadAlready > NCHARS_FILE_ID_MAX_STORE - 1)
                usableLenThisNM = NCHARS_FILE_ID_MAX_STORE - numCharsReadAlready - 1;
            else
                usableLenThisNM = lengthThisNM;
            
            strncpy(dest + numCharsReadAlready, (char*)suFields + count + 5, usableLenThisNM);
            dest[usableLenThisNM + numCharsReadAlready] = '\0';
            numCharsReadAlready += usableLenThisNM;
            
            foundName = true;
            nameContinues = suFields[count + 4] & 0x01; /* NM 'continue' flag */
        }
        else if(suFields[count] == 'C' && suFields[count + 1] == 'E')
        {
            foundCE = true;
            read733FromCharArray(suFields + count + 4, &logicalBlockOfCE);
            read733FromCharArray(suFields + count + 12, &offsetInLogicalBlockOfCE);
            read733FromCharArray(suFields + count + 20, &lengthOfCE);
        }
        
        /* skip su record */
        count += suFields[count + 2];
    }
    
    free(suFields);
    lseek(image, origPos, SEEK_SET);
    
    if( !foundName || (foundName && nameContinues) )
    {
        if(!foundCE)
            return BKERROR_RR_FILENAME_MISSING;
        else
        {
            lseek(image, logicalBlockOfCE * NBYTES_LOGICAL_BLOCK + offsetInLogicalBlockOfCE, SEEK_SET);
            rc = readRockridgeFilename(image, dest, lengthOfCE, numCharsReadAlready);
            
            lseek(image, origPos, SEEK_SET);
            
            return rc;
        }
    }
    else
        return 1;
}

/*******************************************************************************
* removeCrapFromFilename()
* filenames as read from 9660 Sometimes end with ;1 (terminator+version num)
* this removes the useless ending and terminates the destination with a '\0'
* */
void removeCrapFromFilename(char* filename, int length)
{
    int count;
    bool stop = false;
    
    for(count = 0; count < length && !stop; count++)
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

/*******************************************************************************
* skipDR()
* Seek past a directory entry. Good for skipping "." and ".."
* */
int skipDR(int image)
{
    unsigned char dRLen;
    int rc;
    
    rc = read711(image, &dRLen);
    if(rc <= 0)
        return BKERROR_READ_GENERIC;
    
    lseek(image, dRLen - 1, SEEK_CUR);
    
    return dRLen;
}

/*******************************************************************************
* stripSpacesFromEndOfString
* Some strings in the ISO volume are padded with spaces (hopefully on the right)
* this function removes them.
* */
void stripSpacesFromEndOfString(char* str)
{
    int count;
    
    for(count = strlen(str) - 1; count >= 0 && str[count] == ' '; count--)
    {
        str[count] = '\0';
    }
}
