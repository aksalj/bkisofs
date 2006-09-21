/******************************* LICENCE **************************************
* Any code in this file may be redistributed or modified under the terms of
* the GNU General Public Licence as published by the Free Software 
* Foundation; version 2 of the licence.
****************************** END LICENCE ***********************************/

#include <unistd.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <fcntl.h>

#include "bk.h"
#include "bkWrite7x.h"
#include "bkTime.h"
#include "bkWrite.h"
#include "bkMangle.h"
#include "bkError.h"
#include "bkSort.h"
#include "bkPath.h"

/*******************************************************************************
* copyByteBlock()
* copies numBytes from src into dest in blocks of 100K
* */
int copyByteBlock(int src, int dest, unsigned numBytes)
{
    int rc;
    int count;
    char block[102400]; /* 100K */
    int numBlocks;
    int sizeLastBlock;
    
    numBlocks = numBytes / 102400;
    sizeLastBlock = numBytes % 102400;
    
    for(count = 0; count < numBlocks; count++)
    {
        rc = read(src, block, 102400);
        if(rc != 102400)
            return BKERROR_READ_GENERIC;
        rc = writeWrapper(dest, block, 102400);
        if(rc <= 0)
            return rc;
    }
    
    if(sizeLastBlock > 0)
    {
        rc = read(src, block, sizeLastBlock);
        if(rc != sizeLastBlock)
                return BKERROR_READ_GENERIC;
        rc = writeWrapper(dest, block, sizeLastBlock);
        if(rc <= 0)
                return rc;
    }
    
    return 1;
}

/*******************************************************************************
* countDirsOnLevel()
* a 'level' is described in ecma119 6.8.2
* it's needed for path tables, don't remember exactly what for
* */
int countDirsOnLevel(const DirToWrite* dir, int targetLevel, int thisLevel)
{
    DirToWriteLL* nextDir;
    int sum;
    
    if(targetLevel == thisLevel)
    {
        return 1;
    }
    else
    {
        sum = 0;
        
        nextDir = dir->directories;
        while(nextDir != NULL)
        {
            sum += countDirsOnLevel(&(nextDir->dir), targetLevel, thisLevel + 1);
            
            nextDir = nextDir->next;
        }
        
        return sum;
    }
}

/*******************************************************************************
* countTreeHeight()
* caller should set heightSoFar to 1
* */
int countTreeHeight(const DirToWrite* dir, int heightSoFar)
{
    DirToWriteLL* nextDir;
    int maxHeight;
    int thisHeight;
    
    if(dir->directories == NULL)
    {
        return heightSoFar;
    }
    else
    {
        maxHeight = heightSoFar;
        nextDir = dir->directories;
        while(nextDir != NULL)
        {
            thisHeight = countTreeHeight(&(nextDir->dir), heightSoFar + 1);
            
            if(thisHeight > maxHeight)
                maxHeight = thisHeight;
            
            nextDir = nextDir->next;
        }
        
        return maxHeight;
    }
}

/*******************************************************************************
* writeByteBlock()
* Fills numBytes with byteToWrite.
* Writes 1024 bytes at a time, this should be enough as this functions is only
* called to complete an extent and such.
* */
int writeByteBlock(int image, unsigned char byteToWrite, int numBytes)
{
    int rc;
    int count;
    unsigned char block[1024];
    int numBlocks;
    int sizeLastBlock;
    
    memset(block, byteToWrite, 1024);
    
    numBlocks = numBytes / 1024;
    sizeLastBlock = numBytes % 1024;
    
    for(count = 0; count < numBlocks; count++)
    {
        rc = writeWrapper(image, block, 1024);
        if(rc <= 0)
            return rc;
    }
    
    if(sizeLastBlock > 0)
    {
        rc = writeWrapper(image, block, sizeLastBlock);
        if(rc <= 0)
            return rc;
    }
    
    return 1;
}

/*******************************************************************************
* writeDir()
* Writes the contents of a directory. Also writes locations and sizes of
* directory records for directories but not for files.
* Returns data length of the dir written.
* */
int writeDir(int image, DirToWrite* dir, int parentLbNum, 
             int parentNumBytes, int parentPosix, time_t recordingTime, 
             int filenameTypes, bool isRoot)
{
    int rc;
    
    off_t startPos;
    int numUnusedBytes;
    off_t endPos;
    
    DirToWrite selfDir; /* will have a different filename */
    DirToWrite parentDir;
    
    bool takeDirNext;
    DirToWriteLL* nextDir;
    FileToWriteLL* nextFile;
    
    if(lseek(image, 0, SEEK_CUR) % NBYTES_LOGICAL_BLOCK != 0)
        return BKERROR_SANITY;
    
    /* names other then 9660 are not used for self and parent */
    selfDir.name9660[0] = 0x00;
    selfDir.posixFileMode = dir->posixFileMode;
    
    parentDir.name9660[0] = 0x01;
    parentDir.name9660[1] = '\0';
    if(isRoot)
        parentDir.posixFileMode = selfDir.posixFileMode;
    else
        parentDir.posixFileMode = parentPosix;
    
    startPos = lseek(image, 0, SEEK_CUR);
    
    if( startPos % NBYTES_LOGICAL_BLOCK != 0 )
    /* this should never happen */
        return BKERROR_SANITY;
    
    if(filenameTypes & FNTYPE_JOLIET)
        dir->extentNumber2 = startPos / NBYTES_LOGICAL_BLOCK;
    else
        dir->extentNumber = startPos / NBYTES_LOGICAL_BLOCK;
    
    /* write self */
    if(isRoot)
    {
        rc = writeDr(image, &selfDir, recordingTime, true, true, true, filenameTypes);
        if(rc < 0)
            return rc;
        
        if(filenameTypes & FNTYPE_JOLIET)
            dir->extentLocationOffset2 = selfDir.extentLocationOffset2;
        else
            dir->extentLocationOffset = selfDir.extentLocationOffset;
    }
    else
    {
        rc = writeDr(image, &selfDir, recordingTime, true, true, false, filenameTypes);
        if(rc < 0)
            return rc;
    }
    if(rc < 0)
        return rc;
    
    /* write parent */
    rc = writeDr(image, &parentDir, recordingTime, true, true, false, filenameTypes);
    if(rc < 0)
        return rc;
    
    nextDir = dir->directories;
    nextFile = dir->files;
    
    /* WRITE children drs */
    while( nextDir != NULL || nextFile != NULL )
    /* have a file or directory */
    {
        if(nextDir == NULL)
        /* no directories left */
            takeDirNext = false;
        else if(nextFile == NULL)
        /* no files left */
            takeDirNext = true;
        else
        {
            if(filenameTypes & FNTYPE_JOLIET)
            {
                if( strcmp(nextFile->file.nameJoliet, nextDir->dir.nameJoliet) > 0 )
                    takeDirNext = true;
                else
                    takeDirNext = false;
            }
            else
            {
                if( strcmp(nextFile->file.name9660, nextDir->dir.name9660) > 0 )
                    takeDirNext = true;
                else
                    takeDirNext = false;
            }
        }
        
        if(takeDirNext)
        {
            rc = writeDr(image, &(nextDir->dir), recordingTime, 
                         true,  false, false, filenameTypes);
            if(rc < 0)
                return rc;
            
            nextDir = nextDir->next;
        }
        else
        {
            rc = writeDr(image, (DirToWrite*)&(nextFile->file), recordingTime, 
                         false,  false, false, filenameTypes);
            if(rc < 0)
                return rc;
            
            nextFile = nextFile->next;
        }
    }
    /* END WRITE children drs */
    
    /* write blank to conclude extent */
    numUnusedBytes = NBYTES_LOGICAL_BLOCK - 
                     lseek(image, 0, SEEK_CUR) % NBYTES_LOGICAL_BLOCK;
    rc = writeByteBlock(image, 0x00, numUnusedBytes);
    if(rc < 0)
        return rc;
    
    if(filenameTypes & FNTYPE_JOLIET)
        dir->dataLength2 = lseek(image, 0, SEEK_CUR) - startPos;
    else
        dir->dataLength = lseek(image, 0, SEEK_CUR) - startPos;
    
    /* write subdirectories */
    nextDir = dir->directories;
    while(nextDir != NULL)
    {
        if(filenameTypes & FNTYPE_JOLIET)
        {
            rc = writeDir(image, &(nextDir->dir), dir->extentNumber2, 
                          dir->dataLength2, dir->posixFileMode, recordingTime,
                          filenameTypes, false);
        }
        else
        {
            rc = writeDir(image, &(nextDir->dir), dir->extentNumber, 
                          dir->dataLength, dir->posixFileMode, recordingTime,
                          filenameTypes, false);
        }
        if(rc < 0)
            return rc;
        
        nextDir = nextDir->next;
    }
    
    endPos = lseek(image, 0, SEEK_CUR);
    
    /* SELF extent location and size */
    if(filenameTypes & FNTYPE_JOLIET)
        lseek(image, selfDir.extentLocationOffset2, SEEK_SET);
    else
        lseek(image, selfDir.extentLocationOffset, SEEK_SET);
    
    if(filenameTypes & FNTYPE_JOLIET)
    {
        rc = write733(image, dir->extentNumber2);
        if(rc <= 0)
            return rc;
        
        rc = write733(image, dir->dataLength2);
        if(rc <= 0)
            return rc;
    }
    else
    {
        rc = write733(image, dir->extentNumber);
        if(rc <= 0)
            return rc;
        
        rc = write733(image, dir->dataLength);
        if(rc <= 0)
            return rc;
    }
    /* END SELF extent location and size */
    
    /* PARENT extent location and size */
    if(filenameTypes & FNTYPE_JOLIET)
        lseek(image, parentDir.extentLocationOffset2, SEEK_SET);
    else
        lseek(image, parentDir.extentLocationOffset, SEEK_SET);
    
    if(parentLbNum == 0)
    /* root, parent is same as self */
    {
        if(filenameTypes & FNTYPE_JOLIET)
        {
            rc = write733(image, dir->extentNumber2);
            if(rc <= 0)
                return rc;
            
            rc = write733(image, dir->dataLength2);
            if(rc <= 0)
                return rc;
        }
        else
        {
            rc = write733(image, dir->extentNumber);
            if(rc <= 0)
                return rc;
            
            rc = write733(image, dir->dataLength);
            if(rc <= 0)
                return rc;
        }
    }
    else
    /* normal parent */
    {
        rc = write733(image,parentLbNum);
        if(rc <= 0)
            return rc;
        
        rc = write733(image, parentNumBytes);
        if(rc <= 0)
            return rc;
    }
    /* END PARENT extent location and size */
    
    /* ALL subdir extent locations and sizes */
    nextDir = dir->directories;
    while(nextDir != NULL)
    {
        if(filenameTypes & FNTYPE_JOLIET)
        {
            lseek(image, nextDir->dir.extentLocationOffset2, SEEK_SET);
            
            rc = write733(image, nextDir->dir.extentNumber2);
            if(rc <= 0)
                return rc;
            
            rc = write733(image, nextDir->dir.dataLength2);
            if(rc <= 0)
                return rc;
        }
        else
        {
            lseek(image, nextDir->dir.extentLocationOffset, SEEK_SET);
            
            rc = write733(image, nextDir->dir.extentNumber);
            if(rc <= 0)
                return rc;
            
            rc = write733(image, nextDir->dir.dataLength);
            if(rc <= 0)
                return rc;
        }
        
        nextDir = nextDir->next;
    }
    /* END ALL subdir extent locations and sizes */
    
    lseek(image, endPos, SEEK_SET);
    
    if(filenameTypes & FNTYPE_JOLIET)
        return dir->dataLength2;
    else
        return dir->dataLength;
}

/*******************************************************************************
* writeDr()
* Writes a directory record.
* Note that it uses only the members of DirToWrite and FileToWrite that are
* the same.
* */
int writeDr(int image, DirToWrite* dir, time_t recordingTime, bool isADir, 
            bool isSelfOrParent, bool isFirstRecord, int filenameTypes)
{
    int rc;
    unsigned char byte;
    char aString[256];
    unsigned short aShort;
    off_t startPos;
    off_t endPos;
    unsigned char lenFileId;
    unsigned char recordLen;
    
    /* look at the end of the function for an explanation */
    writeDrStartLabel:
    
    startPos = lseek(image, 0, SEEK_CUR);
    
    /* record length is recorded in the end */
    lseek(image, 1, SEEK_CUR);
    
    /* extended attribute record length */
    byte = 0;
    rc = write711(image, byte);
    if(rc <= 0)
        return rc;
    
    if(filenameTypes & FNTYPE_JOLIET)
        dir->extentLocationOffset2 = lseek(image, 0, SEEK_CUR);
    else
        dir->extentLocationOffset = lseek(image, 0, SEEK_CUR);
    
    /* location of extent not recorded in this function */
    lseek(image, 8, SEEK_CUR);
    
    /* data length not recorded in this function */
    lseek(image, 8, SEEK_CUR);
    
    /* RECORDING time and date */
    epochToShortString(recordingTime, aString);
    
    //!! combine all these into one write
    rc = write711(image, aString[0]);
    if(rc <= 0)
        return rc;
    rc = write711(image, aString[1]);
    if(rc <= 0)
        return rc;
    rc = write711(image, aString[2]);
    if(rc <= 0)
        return rc;
    rc = write711(image, aString[3]);
    if(rc <= 0)
        return rc;
    rc = write711(image, aString[4]);
    if(rc <= 0)
        return rc;
    rc = write711(image, aString[5]);
    if(rc <= 0)
        return rc;
    rc = write711(image, aString[6]);
    if(rc <= 0)
        return rc;
    /* END RECORDING time and date */
    
    /* FILE flags  */
    if(isADir)
    /* (only directory bit on) */
        byte = 0x02;
    else
    /* nothing on */
        byte = 0x00;
    
    rc = writeWrapper(image, &byte, 1);
    if(rc <= 0)
        return rc;
    /* END FILE flags  */
    
    /* file unit size (always 0, non-interleaved mode) */
    byte = 0;
    rc = write711(image, byte);
    if(rc <= 0)
        return rc;
    
    /* interleave gap size (also always 0, non-interleaved mode) */
    rc = write711(image, byte);
    if(rc <= 0)
        return rc;
    
    /* volume sequence number (always 1) */
    aShort = 1;
    rc = write723(image, aShort);
    if(rc <= 0)
        return rc;
    
    /* LENGTH of file identifier */
    if(isSelfOrParent)
        lenFileId = 1;
    else
    {
        if(filenameTypes & FNTYPE_JOLIET)
            lenFileId = 2 * strlen(dir->nameJoliet);
        else
            /*if(isADir) see microsoft comment below */
                lenFileId = strlen(dir->name9660);
            /*else
                lenFileId = strlen(dir->name9660) + 2; */
    }
    
    rc = write711(image, lenFileId);
    if(rc <= 0)
        return rc;
    /* END LENGTH of file identifier */
    
    /* FILE identifier */
    if(isSelfOrParent)
    {
        /* that byte has 0x00 or 0x01 */
        rc = write711(image, dir->name9660[0]);
        if(rc <= 0)
            return rc;
    }
    else
    {
        if(filenameTypes & FNTYPE_JOLIET)
        {
            rc = writeJolietStringField(image, dir->nameJoliet, 
                                        2 * strlen(dir->nameJoliet));
            if(rc < 0)
                return rc;
        }
        else
        {
            /* ISO9660 requires ";1" after the filename (not directory name) 
            * but the windows NT/2K boot loaders cannot find NTLDR inside
            * the I386 directory because they are looking for "NTLDR" not 
            * "NTLDR;1". i guess if microsoft an do it, i can do it. filenames
            * on images written by me do not end with ";1"
            if(isADir)
            {*/
                /* the name */
                rc = writeWrapper(image, dir->name9660, lenFileId);
                if(rc <= 0)
                    return rc;
            /*}
            else
            {
                rc = writeWrapper(image, dir->name9660, lenFileId - 2);
                if(rc <= 0)
                    return rc;
                
                rc = writeWrapper(image, ";1", 2);
                if(rc <= 0)
                    return rc;
            }*/
        }
    }
    /* END FILE identifier */
    
    /* padding field */
    if(lenFileId % 2 == 0)
    {
        byte = 0;
        rc = write711(image, byte);
        if(rc <= 0)
            return rc;
    }
    
    if(filenameTypes & FNTYPE_ROCKRIDGE)
    {
        if(isFirstRecord)
        {
            rc = writeRockSP(image);
            if(rc < 0)
                return rc;
            
            rc = writeRockER(image);
            if(rc < 0)
                return rc;
        }
        
        if(!isSelfOrParent)
        {
            rc = writeRockNM(image, dir->nameRock);
            if(rc < 0)
                return rc;
        }
        
        rc = writeRockPX(image, dir, isADir);
        if(rc < 0)
            return rc;
    }
    
    /* RECORD length */
    endPos = lseek(image, 0, SEEK_CUR);
    
    lseek(image, startPos, SEEK_SET);
    
    recordLen = endPos - startPos;
    rc = write711(image, recordLen);
    if(rc <= 0)
        return rc;
    
    lseek(image, endPos, SEEK_SET);
    /* END RECORD length */
    
    /* the goto is good! really!
    * if, after writing the record we see that the record is in two logical
    * sectors (that's not allowed by iso9660) we erase the record just
    * written, write zeroes to the end of the first logical sector
    * (as required by iso9660) and restart the function, which will write
    * the same record again but at the beginning of the next logical sector
    * yeah, so don't complain :) */
    
    if(endPos / NBYTES_LOGICAL_BLOCK > startPos / NBYTES_LOGICAL_BLOCK)
    /* crossed a logical sector boundary while writing the record */
    {
        lseek(image, startPos, SEEK_SET);
        
        /* overwrite a piece of the record written in this function
        * (the piece that's in the first sector) with zeroes */
        rc = writeByteBlock(image, 0x00, recordLen - endPos % NBYTES_LOGICAL_BLOCK);
        if(rc < 0)
            return rc;
        
        goto writeDrStartLabel;
    }
    
    return 1;
}

/*******************************************************************************
* bootInfoTableChecksum()
* Calculate the checksum to be written into the boot info table.
* */
int bootInfoTableChecksum(int oldImage, FileToWrite* file, unsigned* checksum)
{
    int rc;
    int srcFile;
    unsigned char* contents;
    int count;
    
    if(file->size % 4 != 0)
        return BKERROR_WRITE_BOOT_FILE_4;
    
    contents = malloc(file->size);
    if(contents == NULL)
        return BKERROR_OUT_OF_MEMORY;
    
    if(file->onImage)
    /* read file from original image */
    {
        lseek(oldImage, file->offset, SEEK_SET);
        
        rc = read(oldImage, contents, file->size);
        if(rc != file->size)
        {
            free(contents);
            return BKERROR_READ_GENERIC;
        }
    }
    else
    /* read file from fs */
    {
        srcFile = open(file->pathAndName, O_RDONLY);
        if(srcFile == -1)
        {
            free(contents);
            return BKERROR_OPEN_READ_FAILED;
        }
        
        rc = read(srcFile, contents, file->size);
        if(rc != file->size)
        {
            close(srcFile);
            free(contents);
            return BKERROR_READ_GENERIC;
        }
        
        rc = close(srcFile);
        if(rc < 0)
        {
            free(contents);
            return BKERROR_EXOTIC;
        }
    }
    
    *checksum = 0;
    /* do 32 bit checksum starting from byte 64
    * because i check abover that the file is divisible by 4 i will not be 
    * reading wrong memory */
    for(count = 64; count < file->size; count += 4)
    {
        /* keep adding the next 4 bytes */
        *checksum += *( (int*)(contents + count) );
    }
    
    free(contents);
    
    return 1;
}

/*******************************************************************************
* writeFileContents()
* Write file contents into an extent and also write the file's location and 
* size into the directory records back in the tree.
* */
int writeFileContents(int oldImage, int newImage, const VolInfo* volInfo, 
                      DirToWrite* dir, int filenameTypes, 
                      void(*progressFunction)(void))
{
    int rc;
    
    DirToWriteLL* nextDir;
    FileToWriteLL* nextFile;
    int numUnusedBytes;
    int srcFile;
    off_t endPos;
    
    nextFile = dir->files;
    while(nextFile != NULL)
    /* each file in current directory */
    {
        if(lseek(newImage, 0, SEEK_CUR) % NBYTES_LOGICAL_BLOCK != 0)
            return BKERROR_SANITY;
        
        nextFile->file.extentNumber = lseek(newImage, 0, SEEK_CUR) / 
                                      NBYTES_LOGICAL_BLOCK;
        
        if(volInfo->bootMediaType != BOOT_MEDIA_NONE && 
           volInfo->bootRecordIsVisible &&
           nextFile->file.origFile == volInfo->bootRecordOnImage)
        /* this file is the boot record. write its sector number in 
        * the boot catalog */
        {
            off_t currPos;
            
            currPos = lseek(newImage, 0, SEEK_CUR);
            
            lseek(newImage, volInfo->bootRecordSectorNumberOffset, SEEK_SET);
            rc = write731(newImage, currPos / NBYTES_LOGICAL_BLOCK);
            if(rc <= 0)
                return rc;
            
            lseek(newImage, currPos, SEEK_SET);
        }
        
        if(progressFunction != NULL)
            progressFunction();
        
        if(nextFile->file.onImage)
        /* copy file from original image to new one */
        {
            lseek(oldImage, nextFile->file.offset, SEEK_SET);
            
            rc = copyByteBlock(oldImage, newImage, nextFile->file.size);
            if(rc < 0)
                return rc;
        }
        else
        /* copy file from fs to new image */
        {
            srcFile = open(nextFile->file.pathAndName, O_RDONLY);
            if(srcFile == -1)
                return BKERROR_OPEN_READ_FAILED;
            
            rc = copyByteBlock(srcFile, newImage, nextFile->file.size);
            if(rc < 0)
            {
                close(srcFile);
                return rc;
            }
            
            rc = close(srcFile);
            if(rc < 0)
                return BKERROR_EXOTIC;
        }
        
        nextFile->file.dataLength = lseek(newImage, 0, SEEK_CUR) - 
                                    nextFile->file.extentNumber * 
                                    NBYTES_LOGICAL_BLOCK;
        
        /* FILL extent with zeroes */
        numUnusedBytes = NBYTES_LOGICAL_BLOCK - 
                         lseek(newImage, 0, SEEK_CUR) % NBYTES_LOGICAL_BLOCK;
        
        rc = writeByteBlock(newImage, 0x00, numUnusedBytes);
        if(rc < 0)
            return rc;
        /* END FILL extent with zeroes */
        
        endPos = lseek(newImage, 0, SEEK_CUR);
        
        if(volInfo->bootMediaType != BOOT_MEDIA_NONE && 
           volInfo->bootRecordIsVisible &&
           nextFile->file.origFile == volInfo->bootRecordOnImage)
        /* this file is the boot record. assume it's isolinux and write the 
        * boot info table */
        {
            unsigned char bootInfoTable[56];
            unsigned checksum;
            
            bzero(bootInfoTable, 56);
            
            /* go to the offset in the file where the boot info table is */
            lseek(newImage, nextFile->file.extentNumber * 
                  NBYTES_LOGICAL_BLOCK + 8, SEEK_SET);
            
            /* sector number of pvd */
            write731ToByteArray(bootInfoTable, 16);
            /* sector number of boot file (this one) */
            write731ToByteArray(bootInfoTable + 4, nextFile->file.extentNumber);
            /* boot file length in bytes */
            write731ToByteArray(bootInfoTable + 8, nextFile->file.size);
            /* 32 bit checksum (the sum of all the 32-bit words in the boot
            * file starting at byte offset 64 */
            rc = bootInfoTableChecksum(oldImage, &(nextFile->file), &checksum);
            if(rc <= 0)
                return rc;
            write731ToByteArray(bootInfoTable + 12, checksum);
            /* the rest is reserved, leave at zero */
            
            rc = writeWrapper(newImage, bootInfoTable, 56);
            if(rc <= 0)
                return rc;
        }
        
        /* WRITE file location and size */
        lseek(newImage, nextFile->file.extentLocationOffset, SEEK_SET);
        
        rc = write733(newImage, nextFile->file.extentNumber);
        if(rc <= 0)
            return rc;
        
        rc = write733(newImage, nextFile->file.dataLength);
        if(rc <= 0)
            return rc;
        
        if(filenameTypes & FNTYPE_JOLIET)
        /* also update location and size on joliet tree */
        {
            lseek(newImage, nextFile->file.extentLocationOffset2, SEEK_SET);
            
            rc = write733(newImage, nextFile->file.extentNumber);
            if(rc <= 0)
                return rc;
            
            rc = write733(newImage, nextFile->file.dataLength);
            if(rc <= 0)
                return rc;
        }
        
        lseek(newImage, endPos, SEEK_SET);
        /* END WRITE file location and size */
        
        nextFile = nextFile->next;
        
    } /* while(nextFile != NULL) */
    
    /* now write all files in subdirectories */
    nextDir = dir->directories;
    while(nextDir != NULL)
    {
        rc = writeFileContents(oldImage, newImage, volInfo, &(nextDir->dir), 
                               filenameTypes, progressFunction);
        if(rc < 0)
            return rc;
        
        nextDir = nextDir->next;
    }
    
    return 1;
}

/*******************************************************************************
* elToritoChecksum()
* Algorithm: the sum of all words, including the checksum must trunkate to 
* a 16-bit 0x0000
* */
unsigned short elToritoChecksum(const char* record)
{
    short sum;
    int i;
    
    sum = 0;
    for(i = 0; i < 32; i += 2)
    {
        /* take the address of the start of every 16-bit word in the 32 byte 
        * record, cast it to an unsigned short and add it to sum */
        sum += *((unsigned short*)(record + i)) % 0xFFFF;
    }
    
    return 0xFFFF - sum + 1;
}

/*******************************************************************************
* writeElToritoVd()
* Write the el torito volume descriptor.
* Returns the offset where the boot catalog sector number should 
* be written (7.3.1).
* */
int writeElToritoVd(int image, const VolInfo* volInfo)
{
    unsigned char buffer[NBYTES_LOGICAL_BLOCK];
    off_t bootCatalogSectorNumberOffset;
    int rc;
    
    bzero(buffer, NBYTES_LOGICAL_BLOCK);
    
    if(lseek(image, 0, SEEK_CUR) % NBYTES_LOGICAL_BLOCK != 0)
    /* file pointer not at sector boundary */
        return BKERROR_SANITY;
    
    /* SETUP BOOT record volume descriptor sector */
    /* boot record indicator, must be 0 (bzero at start took care of this) */
    /* iso9660 identifier, must be "CD001" */
    strncpy(buffer + 1, "CD001", 5);
    /* version, must be 1 */
    buffer[6] = 1;
    /* boot system identifier, must be 32 bytes "EL TORITO SPECIFICATION" 
    * padded with 0x00 (bzero at start took care of this) */
    strncpy(&(buffer[7]), "EL TORITO SPECIFICATION", 23);
    /* unused 32 bytes, must be 0 (bzero at start took care of this) */
    /* boot catalog location, 4 byte intel format. written later. */
    bootCatalogSectorNumberOffset = lseek(image, 0, SEEK_CUR) + 71;
    //write731ToByteArray(&(buffer[71]), bootCatalogSectorNumber);
    /* the rest of this sector is unused, must be set to 0 */
    /* END SETUP BOOT record volume descriptor sector */
    
    rc = writeWrapper(image, buffer, NBYTES_LOGICAL_BLOCK);
    if(rc <= 0)
        return rc;
    
    return (int)bootCatalogSectorNumberOffset;
}

/*******************************************************************************
* writeElToritoBootCatalog()
* Write the el torito boot catalog (validation entry and inital/default entry).
* Returns the offset where the boot record sector number should
* be written (7.3.1).
* */
int writeElToritoBootCatalog(int image, const VolInfo* volInfo)
{
    unsigned char buffer[NBYTES_LOGICAL_BLOCK];
    off_t bootRecordSectorNumberOffset;
    int rc;
    
    bzero(buffer, NBYTES_LOGICAL_BLOCK);
    
    if(lseek(image, 0, SEEK_CUR) % NBYTES_LOGICAL_BLOCK != 0)
    /* file pointer not at sector boundary */
        return BKERROR_SANITY;
    
    /* SETUP VALIDATION entry (first 20 bytes of boot catalog) */
    /* header, must be 1 */
    buffer[0] = 1;
    /* platform id, 0 for x86 (bzero at start took care of this) */
    /* 2 bytes reserved, must be 0 (bzero at start took care of this) */
    /* 24 bytes id string for manufacturer/developer of cdrom */
    strncpy(&(buffer[4]), "Edited with ISO Master", 22);
    /* key byte 0x55 */
    buffer[30] = 0x55;
    /* key byte 0xAA */
    buffer[31] = 0xAA;
    
    /* checksum */
    write721ToByteArray(&(buffer[28]), elToritoChecksum(buffer));
    /* END SETUP VALIDATION validation entry (first 20 bytes of boot catalog) */
    
    /* SETUP INITIAL entry (next 20 bytes of boot catalog) */
    /* boot indicator. 0x88 = bootable */
    buffer[32] = 0x88;
    /* boot media type */
    if(volInfo->bootMediaType == BOOT_MEDIA_NO_EMULATION)
        buffer[33] = 0;
    else if(volInfo->bootMediaType == BOOT_MEDIA_1_2_FLOPPY)
        buffer[33] = 1;
    else if(volInfo->bootMediaType == BOOT_MEDIA_1_44_FLOPPY)
        buffer[33] = 2;
    else if(volInfo->bootMediaType == BOOT_MEDIA_2_88_FLOPPY)
        buffer[33] = 3;
    else if(volInfo->bootMediaType == BOOT_MEDIA_HARD_DISK)
        buffer[33] = 4;
    /* load segment leave it at 0 */
    /* system type, leave it at 0 */
    /* 1 byte unused, leave it at 0 */
    /* sector count. i have yet to see a boot record with a sector count 
    * that's not 4 */
    write721ToByteArray(&(buffer[38]), 4);
    /* logical block number of boot record file. this is not known until 
    * after that file is written */
    bootRecordSectorNumberOffset = lseek(image, 0, SEEK_CUR) + 40;
    /* the rest is unused, leave it at 0 */
    /* END SETUP INITIAL entry (next 20 bytes of boot catalog) */
    
    rc = writeWrapper(image, buffer, NBYTES_LOGICAL_BLOCK);
    if(rc <= 0)
        return rc;
    
    return (int)bootRecordSectorNumberOffset;
}

/*******************************************************************************
* bk_write_image()
* Writes everything from first to last byte of the iso.
* Public function.
* */
int bk_write_image(int oldImage, int newImage, VolInfo* volInfo, 
                   time_t creationTime, int filenameTypes, 
                   void(*progressFunction)(void))
{
    int rc;
    DirToWrite newTree;
    off_t svdOffset;
    off_t pRealRootDrOffset;
    int pRootDirSize;
    off_t sRealRootDrOffset;
    int sRootDirSize;
    off_t lPathTable9660Loc;
    off_t mPathTable9660Loc;
    int pathTable9660Size;
    off_t lPathTableJolietLoc;
    off_t mPathTableJolietLoc;
    int pathTableJolietSize;
    off_t bootCatalogSectorNumberOffset;
    off_t currPos;
    
    /* because mangleDir works on dir's children i need to copy the root manually */
    bzero(&newTree, sizeof(DirToWrite));
    newTree.name9660[0] = 0;
    newTree.nameRock[0] = '\0';
    newTree.nameJoliet[0] = '\0';
    newTree.posixFileMode = volInfo->dirTree.posixFileMode;
    
    printf("mangling\n");fflush(NULL);
    /* create tree to write */
    rc = mangleDir(&(volInfo->dirTree), &newTree, filenameTypes);
    if(rc <= 0)
    {
        freeDirToWriteContents(&newTree);
        return rc;
    }
    
    if(progressFunction != NULL)
        progressFunction();
    
    printf("writing blank at %X\n", (int)lseek(newImage, 0, SEEK_CUR));fflush(NULL);
    /* system area, always zeroes */
    rc = writeByteBlock(newImage, 0, NBYTES_LOGICAL_BLOCK * NLS_SYSTEM_AREA);
    if(rc <= 0)
    {
        freeDirToWriteContents(&newTree);
        return rc;
    }
    
    /* skip pvd (1 block), write it after files */
    lseek(newImage, NBYTES_LOGICAL_BLOCK, SEEK_CUR);
    
    if(volInfo->bootMediaType != BOOT_MEDIA_NONE)
    {
        /* el torito volume descriptor */
        bootCatalogSectorNumberOffset = writeElToritoVd(newImage, volInfo);
        if(bootCatalogSectorNumberOffset <= 0)
            return bootCatalogSectorNumberOffset;
    }
    
    if(filenameTypes & FNTYPE_JOLIET)
    /* skip svd (1 block), write it after pvd */
    {
        svdOffset = lseek(newImage, 0, SEEK_CUR);
        lseek(newImage, NBYTES_LOGICAL_BLOCK, SEEK_CUR);
    }
    
    if(progressFunction != NULL)
        progressFunction();
    
    printf("writing terminator at %X\n", (int)lseek(newImage, 0, SEEK_CUR));fflush(NULL);
    /* volume descriptor set terminator */
    rc = writeVdsetTerminator(newImage);
    if(rc <= 0)
    {
        freeDirToWriteContents(&newTree);
        return rc;
    }
    
    if(volInfo->bootMediaType != BOOT_MEDIA_NONE)
    {
        /* write boot catalog sector number */
        currPos = lseek(newImage, 0, SEEK_CUR);
        lseek(newImage, bootCatalogSectorNumberOffset, SEEK_SET);
        rc = write731(newImage, currPos / NBYTES_LOGICAL_BLOCK);
        if(rc <= 0)
        {
            freeDirToWriteContents(&newTree);
            return rc;
        }
        lseek(newImage, currPos, SEEK_SET);
        
        /* write el torito booting catalog */
        volInfo->bootRecordSectorNumberOffset = 
                                writeElToritoBootCatalog(newImage, volInfo);
        if(volInfo->bootRecordSectorNumberOffset <= 0)
        {
            freeDirToWriteContents(&newTree);
            return volInfo->bootRecordSectorNumberOffset;
        }
    }
    
    /* MAYBE write boot record file now */
    if(volInfo->bootMediaType != BOOT_MEDIA_NONE && 
       !volInfo->bootRecordIsVisible)
    {
        int blankSize;
        int srcFile; /* either the old image or the boot record file on 
                     * the regular filesystem */
        bool srcFileOpened;
        
        /* set up source file pointer */
        if(volInfo->bootRecordIsOnImage)
        {
            srcFile = oldImage;
            lseek(oldImage, volInfo->bootRecordOffset, SEEK_SET);
            srcFileOpened = false;
        }
        else
        {
            srcFile = open(volInfo->bootRecordPathAndName, O_RDONLY);
            if(srcFile == -1)
            {
                freeDirToWriteContents(&newTree);
                return BKERROR_OPEN_READ_FAILED;
            }
            srcFileOpened = true;
        }
        
        /* write boot record sector number */
        currPos = lseek(newImage, 0, SEEK_CUR);
        lseek(newImage, volInfo->bootRecordSectorNumberOffset, SEEK_SET);
        rc = write731(newImage, currPos / NBYTES_LOGICAL_BLOCK);
        if(rc <= 0)
        {
            freeDirToWriteContents(&newTree);
            return rc;
        }
        lseek(newImage, currPos, SEEK_SET);
        
        /* file contents */
        rc = copyByteBlock(srcFile, newImage, volInfo->bootRecordSize);
        if(rc < 0)
        {
            freeDirToWriteContents(&newTree);
            close(srcFile);
            return rc;
        }
        
        blankSize = NBYTES_LOGICAL_BLOCK - 
                    volInfo->bootRecordSize % NBYTES_LOGICAL_BLOCK;
        
        /* fill the last sector with 0s */
        rc = writeByteBlock(newImage, 0x00, blankSize);
        if(rc < 0)
        {
            freeDirToWriteContents(&newTree);
            close(srcFile);
            return rc;
        }
        
        if(srcFileOpened)
            close(srcFile);
    }
    /* END MAYBE write boot record file now */
    
    printf("sorting 9660\n");
    sortDir(&newTree, FNTYPE_9660);
    
    pRealRootDrOffset = lseek(newImage, 0, SEEK_CUR);
    
    if(progressFunction != NULL)
        progressFunction();
    
    printf("writing primary directory tree at %X\n", (int)lseek(newImage, 0, SEEK_CUR));fflush(NULL);
    /* 9660 and maybe rockridge dir tree */
    rc = writeDir(newImage, &newTree, 0, 0, 0, creationTime, 
                  filenameTypes & (FNTYPE_9660 | FNTYPE_ROCKRIDGE), true);
    if(rc <= 0)
    {
        freeDirToWriteContents(&newTree);
        return rc;
    }
    
    pRootDirSize = rc;
    
    /* joliet dir tree */
    if(filenameTypes & FNTYPE_JOLIET)
    {
        printf("sorting joliet\n");
        sortDir(&newTree, FNTYPE_JOLIET);
        
        printf("writing supplementary directory tree at %X\n", (int)lseek(newImage, 0, SEEK_CUR));fflush(NULL);
        sRealRootDrOffset = lseek(newImage, 0, SEEK_CUR);
        
        if(progressFunction != NULL)
            progressFunction();
        
        rc = writeDir(newImage, &newTree, 0, 0, 0, creationTime, 
                      FNTYPE_JOLIET, true);
        if(rc <= 0)
        {
            freeDirToWriteContents(&newTree);
            return rc;
        }
        
        sRootDirSize = rc;
    }
    
    printf("writing 9660 path tables at %X\n", (int)lseek(newImage, 0, SEEK_CUR));fflush(NULL);
    
    if(progressFunction != NULL)
        progressFunction();
    
    lPathTable9660Loc = lseek(newImage, 0, SEEK_CUR);
    rc = writePathTable(newImage, &newTree, true, FNTYPE_9660);
    if(rc <= 0)
    {
        freeDirToWriteContents(&newTree);
        return rc;
    }
    pathTable9660Size = rc;
    
    mPathTable9660Loc = lseek(newImage, 0, SEEK_CUR);
    rc = writePathTable(newImage, &newTree, false, FNTYPE_9660);
    if(rc <= 0)
    {
        freeDirToWriteContents(&newTree);
        return rc;
    }
    
    if(filenameTypes & FNTYPE_JOLIET)
    {
        printf("writing joliet path tables at %X\n", (int)lseek(newImage, 0, SEEK_CUR));fflush(NULL);
        lPathTableJolietLoc = lseek(newImage, 0, SEEK_CUR);
        rc = writePathTable(newImage, &newTree, true, FNTYPE_JOLIET);
        if(rc <= 0)
        {
            freeDirToWriteContents(&newTree);
            return rc;
        }
        pathTableJolietSize = rc;
        
        mPathTableJolietLoc = lseek(newImage, 0, SEEK_CUR);
        rc = writePathTable(newImage, &newTree, false, FNTYPE_JOLIET);
        if(rc <= 0)
        {
            freeDirToWriteContents(&newTree);
            return rc;
        }
    }
    
    if(progressFunction != NULL)
        progressFunction();
    
    printf("writing files at %X\n", (int)lseek(newImage, 0, SEEK_CUR));fflush(NULL);
    /* all files and offsets/sizes */
    rc = writeFileContents(oldImage, newImage, volInfo, &newTree, filenameTypes, progressFunction);
    if(rc <= 0)
    {
        freeDirToWriteContents(&newTree);
        return rc;
    }
    
    lseek(newImage, NBYTES_LOGICAL_BLOCK * NLS_SYSTEM_AREA, SEEK_SET);
    
    if(progressFunction != NULL)
        progressFunction();
    
    printf("writing pvd at %X\n", (int)lseek(newImage, 0, SEEK_CUR));fflush(NULL);
    rc = writeVolDescriptor(newImage, volInfo, pRealRootDrOffset, 
                            pRootDirSize, lPathTable9660Loc, mPathTable9660Loc, 
                            pathTable9660Size, creationTime, true);
    if(rc <= 0)
    {
        freeDirToWriteContents(&newTree);
        return rc;
    }
    
    if(filenameTypes & FNTYPE_JOLIET)
    {
        if(progressFunction != NULL)
            progressFunction();
        
        lseek(newImage, svdOffset, SEEK_SET);
        
        printf("writing svd at %X\n", (int)lseek(newImage, 0, SEEK_CUR));fflush(NULL);
        rc = writeVolDescriptor(newImage, volInfo, sRealRootDrOffset, 
                                sRootDirSize, lPathTableJolietLoc, mPathTableJolietLoc, 
                                pathTableJolietSize, creationTime, false);
        if(rc <= 0)
        {
            freeDirToWriteContents(&newTree);
            return rc;
        }
    }
    
    freeDirToWriteContents(&newTree);
    
    return 1;
}

/* field size must be even */
int writeJolietStringField(int image, const char* name, int fieldSize)
{
    char jolietName[NCHARS_FILE_ID_MAX * 2];
    int srcCount;
    int destCount;
    int rc;
    
    srcCount = 0;
    destCount = 0;
    while(name[srcCount] != '\0' && destCount < fieldSize)
    {
        /* first byte zero */
        jolietName[destCount] = 0x00;
        /* second byte character */
        jolietName[destCount + 1] = name[srcCount];
        
        srcCount += 1;
        destCount += 2;
    }
    
    while(destCount < fieldSize)
    /* pad with ucs2 spaces */
    {
        jolietName[destCount] = 0x00;
        jolietName[destCount + 1] = ' ';
        
        destCount += 2;
    }
    
    rc = writeWrapper(image, jolietName, destCount);
    if(rc <= 0)
        return rc;
    
    return 1;
}

/* returns path table size (number of bytes not counting the blank) */
int writePathTable(int image, const DirToWrite* tree, bool isTypeL, int filenameType)
{
    int treeHeight;
    int count;
    int level;
    int* dirsPerLevel; /* a dynamic array of the number of dirs per level */
    int numDirsSoFar;
    off_t origPos;
    int numBytesWritten;
    int rc;
    
    origPos = lseek(image, 0, SEEK_CUR);
    
    if(lseek(image, 0, SEEK_CUR) % NBYTES_LOGICAL_BLOCK != 0)
        return BKERROR_SANITY;
    
    treeHeight = countTreeHeight(tree, 1);
    
    dirsPerLevel = malloc(sizeof(int) * treeHeight);
    if(dirsPerLevel == NULL)
        return BKERROR_OUT_OF_MEMORY;
    
    for(count = 0; count < treeHeight; count++)
    {
        dirsPerLevel[count] = countDirsOnLevel(tree, count + 1, 1);
    }
    
    for(level = 1; level <= treeHeight; level++)
    {
        if(level == 1)
        /* numDirsSoFar = parent dir num */
            numDirsSoFar = 1;
        else if(level == 2)
            numDirsSoFar = 1;
        else
        {
            /* ex. when i am on level 4 i want number of dirs on levels 1 + 2 */
            numDirsSoFar = 0;
            for(count = 0; count < level - 2; count++)
            {
                numDirsSoFar += dirsPerLevel[count];
            }
        }
        
        rc = writePathTableRecordsOnLevel(image, tree, isTypeL, filenameType, 
                                          level, 1, &numDirsSoFar);
        if(rc < 0)
        {
            free(dirsPerLevel);
            return rc;
        }
    }
    
    numBytesWritten = lseek(image, 0, SEEK_CUR) - origPos;
    
    /* blank to conclude extent */
    rc = writeByteBlock(image, 0x00, NBYTES_LOGICAL_BLOCK - 
                                     numBytesWritten % NBYTES_LOGICAL_BLOCK);
    if(rc < 0)
    {
        free(dirsPerLevel);
        return rc;
    }
    
    free(dirsPerLevel);
    
    return numBytesWritten;
}

int writePathTableRecordsOnLevel(int image, const DirToWrite* dir, bool isTypeL, 
                                 int filenameType, int targetLevel, int thisLevel,
                                 int* parentDirNum)
{
    int rc;
    DirToWriteLL* nextDir;
    
    unsigned char fileIdLen;
    unsigned char byte;
    unsigned exentLocation;
    unsigned short parentDirId; /* copy of *parentDirNum */
    static const char rootId = 0x00;
    
    if(thisLevel == targetLevel)
    /* write path table record */
    {
        /* LENGTH  of directory identifier */
        if(targetLevel == 1)
        /* root */
            fileIdLen = 1;
        else
        {
            if(filenameType & FNTYPE_JOLIET)
            {
                fileIdLen = 2 * strlen(dir->nameJoliet);
            }
            else
            {
                fileIdLen = strlen(dir->name9660);
            }
        }
        
        rc = write711(image, fileIdLen);
        if(rc <= 0)
            return rc;
        /* END LENGTH  of directory identifier */
        
        /* extended attribute record length */
        byte = 0;
        rc = write711(image, byte);
        if(rc <= 0)
            return rc;
        
        /* LOCATION of extent */
        if(filenameType & FNTYPE_JOLIET)
            exentLocation = dir->extentNumber2;
        else
            exentLocation = dir->extentNumber;
        
        if(isTypeL)
            rc = write731(image, exentLocation);
        else
            rc = write732(image, exentLocation);
        if(rc <= 0)
            return rc;
        /* END LOCATION of extent */
        
        /* PARENT directory number */
        parentDirId = *parentDirNum;
        
        if(isTypeL)
            rc = write721(image, parentDirId);
        else
            rc = write722(image, parentDirId);
        
        if(rc <= 0)
            return rc;
        /* END PARENT directory number */
        
        /* DIRECTORY identifier */
        if(targetLevel == 1)
        /* root */
        {
            rc = writeWrapper(image, &rootId, 1);
            if(rc <= 0)
                return rc;
        }
        else
        {
            if(filenameType & FNTYPE_JOLIET)
            {
                rc = writeJolietStringField(image, dir->nameJoliet, fileIdLen);
                if(rc < 0)
                    return rc;
            }
            else
            {
                rc = writeWrapper(image, dir->name9660, fileIdLen);
                if(rc <= 0)
                    return rc;
            }
        }
        /* END DIRECTORY identifier */
        
        /* padding field */
        if(fileIdLen % 2 != 0)
        {
            byte = 0;
            rc = write711(image, byte);
            if(rc <= 0)
                return rc;
        }
        
    }
    else /* if(thisLevel < targetLevel) */
    {
        nextDir = dir->directories;
        while(nextDir != NULL)
        {
            if(thisLevel == targetLevel - 2)
            /* am now going throught the list of dirs where the parent is */
            {
                if(targetLevel != 2)
                /* first and second level have the same parent: 1 */
                {
                    (*parentDirNum)++;
                }
            }
            
            rc = writePathTableRecordsOnLevel(image, &(nextDir->dir), isTypeL,
                                              filenameType, targetLevel, 
                                              thisLevel + 1, parentDirNum);
            if(rc < 0)
                return rc;
            
            nextDir = nextDir->next;
        }
    }
    
    return 1;
}

int writeRockER(int image)
{
    int rc;
    unsigned char record[46];
    
    /* identification */
    record[0] = 'E';
    record[1] = 'R';
    
    /* record length */
    record[2] = 46;
    
    /* entry version */
    record[3] = 1;
    
    /* extension identifier length */
    record[4] = 10;
    
    /* extension descriptor length */
    record[5] = 10;
    
    /* extension source length */
    record[6] = 18;
    
    /* extension version */
    record[7] = 1;
    
    /* extension identifier */
    strncpy(&(record[8]), "IEEE_P1282", 10);
    
    /* extension descriptor */
    strncpy(&(record[18]), "DRAFT_1_12", 10);
    
    /* extension source */
    strncpy(&(record[28]), "ADOPTED_1994_07_08", 18);
    
    rc = writeWrapper(image, record, 46);
    if(rc <= 0)
        return rc;
    
    return 1;
}

int writeRockNM(int image, char* name)
{
    int rc;
    unsigned char recordStart[5];
    int nameLen;
    
    nameLen = strlen(name);
    
    /* identification */
    recordStart[0] = 'N';
    recordStart[1] = 'M';
    
    /* record length */
    recordStart[2] = 5 + nameLen;
    
    /* entry version */
    recordStart[3] = 1;
    
    /* flags */
    recordStart[4] = 0;
    
    rc = writeWrapper(image, recordStart, 5);
    if(rc <= 0)
        return rc;
    
    rc = writeWrapper(image, name, nameLen);
    if(rc <= 0)
        return rc;
    
    return 1;
}

/* the slackware cd has 36 byte PX entries, missing the file serial number
* so i will do the same */
int writeRockPX(int image, DirToWrite* dir, bool isADir)
{
    int rc;
    unsigned char record[36];
    //DirToWriteLL* nextDir;
    unsigned posixFileLinks;
    
    /* identification */
    record[0] = 'P';
    record[1] = 'X';
    
    /* record length */
    record[2] = 36;
    
    /* entry version */
    record[3] = 1;
    
    /* posix file mode */
    write733ToByteArray(&(record[4]), dir->posixFileMode);
    
    /* POSIX file links */
    //!! this i think is number of subdirectories + 2 (self and parent)
    // and 1 for a file
    // it's probably not used on read-only filesystems
    // to add it, i will need to pass the number of links in a parent dir
    // recursively in writeDir(). brrrrr.
    if(isADir)
        posixFileLinks = 2;
    else
        posixFileLinks = 1;
    
    write733ToByteArray(&(record[12]), posixFileLinks);
    /* END POSIX file links */
    
    /* posix file user id, posix file group id */
    bzero(&(record[20]), 16);
    
    rc = writeWrapper(image, record, 36);
    if(rc <= 0)
        return rc;
    
    return 1;
}

int writeRockSP(int image)
{
    int rc;
    unsigned char record[7];
    
    /* identification */
    record[0] = 'S';
    record[1] = 'P';
    
    /* record length */
    record[2] = 7;
    
    /* entry version */
    record[3] = 1;
    
    /* check bytes */
    record[4] = 0xBE;
    record[5] = 0xEF;
    
    /* bytes skipped */
    record[6] = 0;
    
    rc = writeWrapper(image, record, 7);
    if(rc <= 0)
        return rc;
    
    return 1;
}

int writeVdsetTerminator(int image)
{
    int rc;
    unsigned char byte;
    char aString[6];
    
    /* volume descriptor type */
    byte = 255;
    rc = write711(image, byte);
    if(rc <= 0)
        return rc;
    
    /* standard identifier */
    strcpy(aString, "CD001");
    rc = writeWrapper(image, aString, 5);
    if(rc <= 0)
        return rc;
    
    /* volume descriptor version */
    byte = 1;
    rc = write711(image, byte);
    if(rc <= 0)
        return rc;
    
    rc = writeByteBlock(image, 0, 2041);
    if(rc < 0)
        return rc;
    
    return 1;
}

/*
* -has to be called after the files were written so that the 
*  volume size is recorded properly
* -rootdr location, size are in bytes
* -note strings are not terminated on image
*/
int writeVolDescriptor(int image, const VolInfo* volInfo, off_t rootDrLocation,
                       unsigned rootDrSize, off_t lPathTableLoc, 
                       off_t mPathTableLoc, unsigned pathTableSize, 
                       time_t creationTime, bool isPrimary)
{
    int rc;
    int count;
    
    unsigned char byte;
    char aString[129];
    unsigned anUnsigned;
    unsigned short anUnsignedShort;
    size_t currPos;
    
    /* VOLUME descriptor type */
    if(isPrimary)
        byte = 1;
    else
        byte = 2;
    /* END VOLUME descriptor type */

    rc = write711(image, byte);
    if(rc <= 0)
        return rc;
    
    /* standard identifier */
    strcpy(aString, "CD001");
    rc = writeWrapper(image, aString, 5);
    if(rc <= 0)
        return rc;
    
    /* volume descriptor version (always 1) */
    byte = 1;
    rc = write711(image, byte);
    if(rc <= 0)
        return rc;
    
    /* primary: unused field
    *  supplementary: volume flags, 0x00 */
    byte = 0;
    rc = write711(image, byte);
    if(rc <= 0)
        return rc;
    
    /* system identifier (32 spaces) */
    if(isPrimary)
    {
        strcpy(aString, "                                ");
        rc = writeWrapper(image, aString, 32);
        if(rc <= 0)
            return rc;
    }
    else
    {
        rc = writeJolietStringField(image, "", 32);
        if(rc < 0)
            return rc;
    }
    
    /* VOLUME identifier */
    if(isPrimary)
    {
        strcpy(aString, volInfo->volId);
        
        for(count = strlen(aString); count < 32; count++)
            aString[count] = ' ';
        
        rc = writeWrapper(image, aString, 32);
        if(rc <= 0)
            return rc;
    }
    else
    {
        rc = writeJolietStringField(image, volInfo->volId, 32);
        if(rc < 0)
            return rc;
    }
    /* END VOLUME identifier */
    
    /* unused field */
    rc = writeByteBlock(image, 0, 8);
    if(rc < 0)
        return rc;
    
    /* VOLUME space size (number of logical blocks, absolutely everything) */
    currPos = lseek(image, 0, SEEK_CUR);
    
    lseek(image, 0, SEEK_END);
    anUnsigned = lseek(image, 0, SEEK_CUR) / NBYTES_LOGICAL_BLOCK;
    
    lseek(image, currPos, SEEK_SET);
    
    rc = write733(image, anUnsigned);
    if(rc <= 0)
        return rc;
    /* END VOLUME space size (number of logical blocks, absolutely everything) */
    
    /* primary: unused field
    *  joliet: escape sequences */
    if(isPrimary)
    {
        rc = writeByteBlock(image, 0, 32);
        if(rc < 0)
            return rc;
    }
    else
    {
        /* this is the only joliet field that's padded with 0x00 instead of ' ' */
        aString[0] = 0x25;
        aString[1] = 0x2F;
        aString[2] = 0x45;
        
        rc = writeWrapper(image, aString, 3);
        if(rc <= 0)
            return rc;
        
        rc = writeByteBlock(image, 0, 29);
        if(rc < 0)
            return rc;
    }
    
    /* volume set size (always 1) */
    anUnsignedShort = 1;
    rc = write723(image, anUnsignedShort);
    if(rc <= 0)
        return rc;
    
    /* volume sequence number (also always 1) */
    rc = write723(image, anUnsignedShort);
    if(rc <= 0)
        return rc;
    
    /* logical block size (always 2048) */
    anUnsignedShort = NBYTES_LOGICAL_BLOCK;
    rc = write723(image, anUnsignedShort);
    if(rc <= 0)
        return rc;
    
    /* path table size */
    anUnsigned = pathTableSize;
    rc = write733(image, anUnsigned);
    if(rc <= 0)
        return rc;
    
    /* location of occurence of type l path table */
    anUnsigned = lPathTableLoc / NBYTES_LOGICAL_BLOCK;
    rc = write731(image, anUnsigned);
    if(rc <= 0)
        return rc;
    
    /* location of optional occurence of type l path table */
    anUnsigned = 0;
    rc = write731(image, anUnsigned);
    if(rc <= 0)
        return rc;
    
    /* location of occurence of type m path table */
    anUnsigned = mPathTableLoc / NBYTES_LOGICAL_BLOCK;
    rc = write732(image, anUnsigned);
    if(rc <= 0)
        return rc;
    
    /* location of optional occurence of type m path table */
    anUnsigned = 0;
    rc = write732(image, anUnsigned);
    if(rc <= 0)
        return rc;
    
    /* ROOT dr */
        /* record length (always 34 here) */
        byte = 34;
        rc = write711(image, byte);
        if(rc <= 0)
            return rc;
        
        /* extended attribute record length (always none) */
        byte = 0;
        rc = write711(image, byte);
        if(rc <= 0)
            return rc;
        
        /* location of extent */
        anUnsigned = rootDrLocation / NBYTES_LOGICAL_BLOCK;
        rc = write733(image, anUnsigned);
        if(rc <= 0)
            return rc;
        
        /* data length */
        rc = write733(image, rootDrSize);
        if(rc <= 0)
            return rc;
        
        /* recording time */
        epochToShortString(creationTime, aString);
        rc = writeWrapper(image, aString, 7);
        if(rc <= 0)
            return rc;
        
        /* file flags (always binary 00000010 here) */
        byte = 0x02;
        rc = write711(image, byte);
        if(rc <= 0)
            return rc;
        
        /* file unit size (not in interleaved mode -> 0) */
        byte = 0;
        rc = write711(image, byte);
        if(rc <= 0)
            return rc;
        
        /* interleave gap size (not in interleaved mode -> 0) */
        rc = write711(image, byte);
        if(rc <= 0)
            return rc;
         
        /* volume sequence number */
        anUnsignedShort = 1;
        rc = write723(image, anUnsignedShort);
        if(rc <= 0)
            return rc;
        
        /* length of file identifier */
        byte = 1;
        rc = write711(image, byte);
        if(rc <= 0)
            return rc;
        
        /* file identifier */
        byte = 0;
        rc = write711(image, byte);
        if(rc <= 0)
            return rc;
    /* END ROOT dr */
    
    /* volume set identidier */
    if(isPrimary)
    {
        rc = writeByteBlock(image, ' ', 128);
        if(rc < 0)
            return rc;
    }
    else
    {
        rc = writeJolietStringField(image, "", 128);
        if(rc < 0)
            return rc;
    }
    
    /* PUBLISHER identifier */
    strcpy(aString, volInfo->publisher);
    
    if(isPrimary)
    {
        for(count = strlen(aString); count < 128; count++)
            aString[count] = ' ';
        
        rc = writeWrapper(image, aString, 128);
        if(rc <= 0)
            return rc;
    }
    else
    {
        rc = writeJolietStringField(image, aString, 128);
        if(rc < 0)
            return rc;
    }
    /* PUBLISHER identifier */
    
    /* DATA preparer identifier */
    if(isPrimary)
    {
        rc = writeWrapper(image, "ISO Master", 10);
        if(rc <= 0)
            return rc;
        
        rc = writeByteBlock(image, ' ', 118);
        if(rc < 0)
            return rc;
    }
    else
    {
        rc = writeJolietStringField(image, "ISO Master", 128);
        if(rc < 0)
            return rc;
    }
    /* END DATA preparer identifier */
    
    /* application identifier, copyright file identifier, abstract file 
    * identifier, bibliographic file identifier (128 + 3*37) */
    if(isPrimary)
    {
        rc = writeByteBlock(image, ' ', 239);
        if(rc < 0)
            return rc;
    }
    else
    {
        /* application id */
        rc = writeJolietStringField(image, "", 128);
        if(rc < 0)
            return rc;
        
        /* 18 ucs2 spaces + 0x00 */
        for(count = 0; count < 3; count++)
        {
            rc = writeJolietStringField(image, "", 36);
            if(rc < 0)
                return rc;
            
            byte = 0x00;
            rc = writeWrapper(image, &byte, 1);
            if(rc <= 0)
                return rc;
        }
    }
    
    /* VOLUME creation date */
    epochToLongString(creationTime, aString);
    
    rc = writeWrapper(image, aString, 17);
    if(rc <= 0)
        return rc;
    /* END VOLUME creation date */
    
    /* volume modification date (same as creation) */
    rc = writeWrapper(image, aString, 17);
    if(rc <= 0)
        return rc;
    
    /* VOLUME expiration date (none) */
    rc = writeByteBlock(image, '0', 16);
    if(rc < 0)
        return rc;
    
    byte = 0;
    rc = write711(image, byte);
    if(rc <= 0)
        return rc;
    /* END VOLUME expiration date (none) */
    
    /* volume effective date (same as creation) */
    rc = writeWrapper(image, aString, 17);
    if(rc <= 0)
        return rc;
    
    /* file structure version */
    byte = 1;
    rc = write711(image, byte);
    if(rc <= 0)
        return rc;
    
    /* reserved, applications use, reserved */
    rc = writeByteBlock(image, 0, 1166);
    if(rc < 0)
        return rc;
    
    return 1;
}
