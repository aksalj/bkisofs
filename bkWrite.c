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
        rc = write(dest, block, 102400);
        if(rc != 102400)
            return BKERROR_WRITE_GENERIC;
    }
    
    rc = read(src, block, sizeLastBlock);
    if(rc != sizeLastBlock)
            return BKERROR_READ_GENERIC;
    rc = write(dest, block, sizeLastBlock);
    if(rc != sizeLastBlock)
            return BKERROR_WRITE_GENERIC;
    
    return 1;
}

int countDirsOnLevel(DirToWrite* dir, int targetLevel, int thisLevel)
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

int countTreeHeight(DirToWrite* dir, int heightSoFar)
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

void freeDirToWriteContents(DirToWrite* dir)
{
    DirToWriteLL* currentDir;
    DirToWriteLL* nextDir;
    FileToWriteLL* currentFile;
    FileToWriteLL* nextFile;
    
    currentDir = dir->directories;
    while(currentDir != NULL)
    {
        freeDirToWriteContents(&(currentDir->dir));
        
        nextDir = currentDir->next;
        
        free(currentDir);
        
        currentDir = nextDir;
    }
    
    currentFile = dir->files;
    while(currentFile != NULL)
    {
        nextFile = currentFile->next;
        
        if(!currentFile->file.onImage)
            free(currentFile->file.pathAndName);
        
        free(currentFile);
        
        currentFile = nextFile;
    }
}

int writeByteBlock(int image, unsigned char byteToWrite, int numBytes)
{
    int rc;
    int count;
    //!! fix this to write more then 1 byte at a time
    for(count = 0; count < numBytes; count++)
    {
        rc = write(image, &byteToWrite, 1);
        if(rc != 1)
            return BKERROR_WRITE_GENERIC;
    }
    
    return 1;
}

/* returns data length of the dir written */
int writeDir(int image, DirToWrite* dir, int parentLbNum, int parentNumBytes, 
             int parentPosix, time_t recordingTime, int filenameTypes,
             bool isRoot)
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
        rc = write733(image, &(dir->extentNumber2));
        if(rc != 8)
            return BKERROR_WRITE_GENERIC;
        
        rc = write733(image, &(dir->dataLength2));
        if(rc != 8)
            return BKERROR_WRITE_GENERIC;
    }
    else
    {
        rc = write733(image, &(dir->extentNumber));
        if(rc != 8)
            return BKERROR_WRITE_GENERIC;
        
        rc = write733(image, &(dir->dataLength));
        if(rc != 8)
            return BKERROR_WRITE_GENERIC;
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
            rc = write733(image, &(dir->extentNumber2));
            if(rc != 8)
                return BKERROR_WRITE_GENERIC;
            
            rc = write733(image, &(dir->dataLength2));
            if(rc != 8)
                return BKERROR_WRITE_GENERIC;
        }
        else
        {
            rc = write733(image, &(dir->extentNumber));
            if(rc != 8)
                return BKERROR_WRITE_GENERIC;
            
            rc = write733(image, &(dir->dataLength));
            if(rc != 8)
                return BKERROR_WRITE_GENERIC;
        }
    }
    else
    /* normal parent */
    {
        rc = write733(image, &(parentLbNum));
        if(rc != 8)
            return BKERROR_WRITE_GENERIC;
        
        rc = write733(image, &(parentNumBytes));
        if(rc != 8)
            return BKERROR_WRITE_GENERIC;
    }
    /* END PARENT extent location and size */
    
    /* ALL subdir extent locations and sizes */
    nextDir = dir->directories;
    while(nextDir != NULL)
    {
        if(filenameTypes & FNTYPE_JOLIET)
        {
            lseek(image, nextDir->dir.extentLocationOffset2, SEEK_SET);
            
            rc = write733(image, &(nextDir->dir.extentNumber2));
            if(rc != 8)
                return BKERROR_WRITE_GENERIC;
            
            rc = write733(image, &(nextDir->dir.dataLength2));
            if(rc != 8)
                return BKERROR_WRITE_GENERIC;
        }
        else
        {
            lseek(image, nextDir->dir.extentLocationOffset, SEEK_SET);
            
            rc = write733(image, &(nextDir->dir.extentNumber));
            if(rc != 8)
                return BKERROR_WRITE_GENERIC;
            
            rc = write733(image, &(nextDir->dir.dataLength));
            if(rc != 8)
                return BKERROR_WRITE_GENERIC;
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

/* returns length of record written */
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
    rc = write711(image, &byte);
    if(rc != 1)
        return BKERROR_WRITE_GENERIC;
    
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
    rc = write711(image, &(aString[0]));
    if(rc != 1)
        return BKERROR_WRITE_GENERIC;
    rc = write711(image, &(aString[1]));
    if(rc != 1)
        return BKERROR_WRITE_GENERIC;
    rc = write711(image, &(aString[2]));
    if(rc != 1)
        return BKERROR_WRITE_GENERIC;
    rc = write711(image, &(aString[3]));
    if(rc != 1)
        return BKERROR_WRITE_GENERIC;
    rc = write711(image, &(aString[4]));
    if(rc != 1)
        return BKERROR_WRITE_GENERIC;
    rc = write711(image, &(aString[5]));
    if(rc != 1)
        return BKERROR_WRITE_GENERIC;
    rc = write711(image, &(aString[6]));
    if(rc != 1)
        return BKERROR_WRITE_GENERIC;
    /* END RECORDING time and date */
    
    /* FILE flags  */
    if(isADir)
    /* (only directory bit on) */
        byte = 0x02;
    else
    /* nothing on */
        byte = 0x00;
    
    rc = write(image, &byte, 1);
    if(rc != 1)
        return BKERROR_WRITE_GENERIC;
    /* END FILE flags  */
    
    /* file unit size (always 0, non-interleaved mode) */
    byte = 0;
    rc = write711(image, &byte);
    if(rc != 1)
        return BKERROR_WRITE_GENERIC;
    
    /* interleave gap size (also always 0, non-interleaved mode) */
    rc = write711(image, &byte);
    if(rc != 1)
        return BKERROR_WRITE_GENERIC;
    
    /* volume sequence number (always 1) */
    aShort = 1;
    rc = write723(image, &aShort);
    if(rc != 4)
        return BKERROR_WRITE_GENERIC;
    
    /* LENGTH of file identifier */
    if(isSelfOrParent)
        lenFileId = 1;
    else
    {
        if(filenameTypes & FNTYPE_JOLIET)
            lenFileId = 2 * strlen(dir->nameJoliet);
        else
            if(isADir)
                lenFileId = strlen(dir->name9660);
            else
                lenFileId = strlen(dir->name9660) + 2; /* + ";1" */
    }
    
    rc = write711(image, &lenFileId);
    if(rc != 1)
        return BKERROR_WRITE_GENERIC;
    /* END LENGTH of file identifier */
    
    /* FILE identifier */
    if(isSelfOrParent)
    {
        /* that byte has 0x00 or 0x01 */
        rc = write711(image, &(dir->name9660[0]));
        if(rc != 1)
            return BKERROR_WRITE_GENERIC;
    }
    else
    {
        if(filenameTypes & FNTYPE_JOLIET)
        {
            rc = writeJolietStringField(image, dir->nameJoliet, 2 * strlen(dir->nameJoliet));
            if(rc < 0)
                return rc;
        }
        else
        {
            if(isADir)
            {
                /* the name */
                rc = write(image, dir->name9660, lenFileId);
                if(rc != lenFileId)
                    return BKERROR_WRITE_GENERIC;
            }
            else
            {
                /* the name */
                rc = write(image, dir->name9660, lenFileId - 2);
                if(rc != lenFileId - 2)
                    return BKERROR_WRITE_GENERIC;
                
                /* and the 9660-required version number */
                rc = write(image, ";1", 2);
                if(rc != 2)
                    return BKERROR_WRITE_GENERIC;
            }
        }
    }
    /* END FILE identifier */
    
    /* padding field */
    if(lenFileId % 2 == 0)
    {
        byte = 0;
        rc = write711(image, &byte);
        if(rc != 1)
            return BKERROR_WRITE_GENERIC;
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
    rc = write711(image, &recordLen);
    if(rc != 1)
        return BKERROR_WRITE_GENERIC;
    
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

int writeFileContents(int oldImage, int newImage, DirToWrite* dir, 
                      int filenameTypes)
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
        nextFile->file.extentNumber = lseek(newImage, 0, SEEK_CUR) / NBYTES_LOGICAL_BLOCK;
        
        if(nextFile->file.onImage)
        /* copy file from original image to new one */
        {
            lseek(oldImage, nextFile->file.position, SEEK_SET);
            
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
                return rc;
            
            rc = close(srcFile);
            if(rc < 0)
                return BKERROR_EXOTIC;
        }
        
        nextFile->file.dataLength = lseek(newImage, 0, SEEK_CUR) - 
                                    nextFile->file.extentNumber * NBYTES_LOGICAL_BLOCK;
        
        /* FILL extent with zeroes */
        numUnusedBytes = NBYTES_LOGICAL_BLOCK - 
                         lseek(newImage, 0, SEEK_CUR) % NBYTES_LOGICAL_BLOCK;
        
        rc = writeByteBlock(newImage, 0x00, numUnusedBytes);
        if(rc < 0)
            return rc;
        /* END FILL extent with zeroes */
        
        endPos = lseek(newImage, 0, SEEK_CUR);
        
        /* WRITE file location and size */
        lseek(newImage, nextFile->file.extentLocationOffset, SEEK_SET);
        
        rc = write733(newImage, &(nextFile->file.extentNumber));
        if(rc != 8)
            return BKERROR_WRITE_GENERIC;
        
        rc = write733(newImage, &(nextFile->file.dataLength));
        if(rc != 8)
            return BKERROR_WRITE_GENERIC;
        
        if(filenameTypes & FNTYPE_JOLIET)
        /* also update location and size on joliet tree */
        {
            lseek(newImage, nextFile->file.extentLocationOffset2, SEEK_SET);
            
            rc = write733(newImage, &(nextFile->file.extentNumber));
            if(rc != 8)
                return BKERROR_WRITE_GENERIC;
            
            rc = write733(newImage, &(nextFile->file.dataLength));
            if(rc != 8)
                return BKERROR_WRITE_GENERIC;
        }
        
        lseek(newImage, endPos, SEEK_SET);
        /* END WRITE file location and size */
        
        nextFile = nextFile->next;
    }
    
    /* now write all files in subdirectories */
    nextDir = dir->directories;
    while(nextDir != NULL)
    {
        rc = writeFileContents(oldImage, newImage, &(nextDir->dir), filenameTypes);
        if(rc < 0)
            return rc;
        
        nextDir = nextDir->next;
    }
    
    return 1;
}

/* field size must be even */
int writeJolietStringField(int image, char* name, int fieldSize)
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
    
    rc = write(image, jolietName, destCount);
    if(rc != destCount)
        return BKERROR_WRITE_GENERIC;
    
    return 1;
}

/* returns path table size (number of bytes not counting the blank) */
int writePathTable(int image, DirToWrite* tree, bool isTypeL, int filenameType)
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
            return rc;
    }
    
    numBytesWritten = lseek(image, 0, SEEK_CUR) - origPos;
    
    /* blank to conclude extent */
    rc = writeByteBlock(image, 0x00, NBYTES_LOGICAL_BLOCK - numBytesWritten);
    if(rc < 0)
        return rc;
    
    free(dirsPerLevel);

    return numBytesWritten;
}

int writePathTableRecordsOnLevel(int image, DirToWrite* dir, bool isTypeL, 
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
        
        rc = write711(image, &fileIdLen);
        if(rc != 1)
            return BKERROR_WRITE_GENERIC;
        /* END LENGTH  of directory identifier */
        
        /* extended attribute record length */
        byte = 0;
        rc = write711(image, &byte);
        if(rc != 1)
            return BKERROR_WRITE_GENERIC;
        
        /* LOCATION of extent */
        if(filenameType & FNTYPE_JOLIET)
            exentLocation = dir->extentNumber2;
        else
            exentLocation = dir->extentNumber;
        
        if(isTypeL)
            rc = write731(image, &exentLocation);
        else
            rc = write732(image, &exentLocation);
        if(rc != 8)
            return BKERROR_WRITE_GENERIC;
        /* END LOCATION of extent */
        
        /* PARENT directory number */
        parentDirId = *parentDirNum;
        
        if(isTypeL)
            rc = write721(image, &parentDirId);
        else
            rc = write722(image, &parentDirId);
        if(rc < 4)
            return BKERROR_WRITE_GENERIC;
        /* END PARENT directory number */
        
        /* DIRECTORY identifier */
        if(targetLevel == 1)
        /* root */
        {
            rc = write(image, &rootId, 1);
            if(rc != 1)
                return BKERROR_WRITE_GENERIC;
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
                rc = write(image, dir->name9660, fileIdLen);
                if(rc != 1)
                    return BKERROR_WRITE_GENERIC;
            }
        }
        /* END DIRECTORY identifier */
        
        /* padding field */
        if(fileIdLen % 2 != 0)
        {
            byte = 0;
            rc = write711(image, &byte);
            if(rc != 1)
                return BKERROR_WRITE_GENERIC;
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
    
    rc = write(image, record, 46);
    if(rc != 46)
        return BKERROR_WRITE_GENERIC;
    
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
    
    rc = write(image, recordStart, 5);
    if(rc != 5)
        return BKERROR_WRITE_GENERIC;
    
    rc = write(image, name, nameLen);
    if(rc != nameLen)
        return BKERROR_WRITE_GENERIC;
    
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
    
    rc = write(image, record, 36);
    if(rc != 36)
        return BKERROR_WRITE_GENERIC;
    
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
    
    rc = write(image, record, 7);
    if(rc != 7)
        return BKERROR_WRITE_GENERIC;
    
    return 1;
}

int writeVdsetTerminator(int image)
{
    int rc;
    unsigned char byte;
    char aString[6];
    
    /* volume descriptor type */
    byte = 255;
    rc = write711(image, &byte);
    if(rc != 1)
        return BKERROR_WRITE_GENERIC;
    
    /* standard identifier */
    strcpy(aString, "CD001");
    rc = write(image, &aString, 5);
    if(rc != 5)
        return BKERROR_WRITE_GENERIC;
    
    /* volume descriptor version */
    byte = 1;
    rc = write711(image, &byte);
    if(rc != 1)
        return BKERROR_WRITE_GENERIC;
    
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
int writeVolDescriptor(int image, VolInfo* volInfo, off_t rootDrLocation,
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

    rc = write711(image, &byte);
    if(rc != 1)
        return BKERROR_WRITE_GENERIC;
    
    /* standard identifier */
    strcpy(aString, "CD001");
    rc = write(image, &aString, 5);
    if(rc != 5)
        return BKERROR_WRITE_GENERIC;
    
    /* volume descriptor version (always 1) */
    byte = 1;
    rc = write711(image, &byte);
    if(rc != 1)
        return BKERROR_WRITE_GENERIC;
    
    /* primary: unused field
    *  supplementary: volume flags, 0x00 */
    byte = 0;
    rc = write711(image, &byte);
    if(rc != 1)
        return BKERROR_WRITE_GENERIC;
    
    /* system identifier (32 spaces) */
    if(isPrimary)
    {
        strcpy(aString, "                                ");
        rc = write(image, &aString, 32);
        if(rc != 32)
            return BKERROR_WRITE_GENERIC;
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
        
        rc = write(image, &aString, 32);
        if(rc != 32)
            return BKERROR_WRITE_GENERIC;
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
    
    rc = write733(image, &anUnsigned);
    if(rc != 8)
        return BKERROR_WRITE_GENERIC;
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
        
        rc = write(image, &aString, 3);
        if(rc != 3)
            return BKERROR_WRITE_GENERIC;
        
        rc = writeByteBlock(image, 0, 29);
        if(rc < 0)
            return rc;
    }
    
    /* volume set size (always 1) */
    anUnsignedShort = 1;
    rc = write723(image, &anUnsignedShort);
    if(rc != 4)
        return BKERROR_WRITE_GENERIC;
    
    /* volume sequence number (also always 1) */
    rc = write723(image, &anUnsignedShort);
    if(rc != 4)
        return BKERROR_WRITE_GENERIC;
    
    /* logical block size (always 2048) */
    anUnsignedShort = NBYTES_LOGICAL_BLOCK;
    rc = write723(image, &anUnsignedShort);
    if(rc != 4)
        return BKERROR_WRITE_GENERIC;
    
    /* path table size */
    anUnsigned = pathTableSize;
    rc = write733(image, &anUnsigned);
    if(rc != 8)
        return BKERROR_WRITE_GENERIC;
    
    /* location of occurence of type l path table */
    anUnsigned = lPathTableLoc / NBYTES_LOGICAL_BLOCK;
    rc = write731(image, &anUnsigned);
    if(rc < 0)
        return BKERROR_WRITE_GENERIC;
    
    /* location of optional occurence of type l path table */
    anUnsigned = 0;
    rc = write731(image, &anUnsigned);
    if(rc < 0)
        return BKERROR_WRITE_GENERIC;
    
    /* location of occurence of type m path table */
    anUnsigned = mPathTableLoc / NBYTES_LOGICAL_BLOCK;
    rc = write732(image, &anUnsigned);
    if(rc < 0)
        return BKERROR_WRITE_GENERIC;
    
    /* location of optional occurence of type m path table */
    anUnsigned = 0;
    rc = write732(image, &anUnsigned);
    if(rc < 0)
        return BKERROR_WRITE_GENERIC;
    
    /* ROOT dr */
        /* record length (always 34 here) */
        byte = 34;
        rc = write711(image, &byte);
        if(rc != 1)
            return BKERROR_WRITE_GENERIC;
        
        /* extended attribute record length (always none) */
        byte = 0;
        rc = write711(image, &byte);
        if(rc != 1)
            return BKERROR_WRITE_GENERIC;
        
        /* location of extent */
        anUnsigned = rootDrLocation / NBYTES_LOGICAL_BLOCK;
        rc = write733(image, &anUnsigned);
        if(rc != 8)
            return BKERROR_WRITE_GENERIC;
        
        /* data length */
        rc = write733(image, &rootDrSize);
        if(rc != 8)
            return BKERROR_WRITE_GENERIC;
        
        /* recording time */
        epochToShortString(creationTime, aString);
        rc = write(image, aString, 7);
        if(rc != 7)
            return BKERROR_WRITE_GENERIC;
        
        /* file flags (always binary 00000010 here) */
        byte = 0x02;
        rc = write711(image, &byte);
        if(rc != 1)
            return BKERROR_WRITE_GENERIC;
        
        /* file unit size (not in interleaved mode -> 0) */
        byte = 0;
        rc = write711(image, &byte);
        if(rc != 1)
            return BKERROR_WRITE_GENERIC;
        
        /* interleave gap size (not in interleaved mode -> 0) */
        rc = write711(image, &byte);
        if(rc != 1)
            return BKERROR_WRITE_GENERIC;
         
        /* volume sequence number */
        anUnsignedShort = 1;
        rc = write723(image, &anUnsignedShort);
        if(rc != 4)
            return BKERROR_WRITE_GENERIC;
        
        /* length of file identifier */
        byte = 1;
        rc = write711(image, &byte);
        if(rc != 1)
            return BKERROR_WRITE_GENERIC;
        
        /* file identifier */
        byte = 0;
        rc = write711(image, &byte);
        if(rc != 1)
            return BKERROR_WRITE_GENERIC;
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
        
        rc = write(image, aString, 128);
        if(rc != 128)
            return BKERROR_WRITE_GENERIC;
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
        rc = write(image, "ISO Master", 10);
        if(rc != 10)
            return BKERROR_WRITE_GENERIC;
        
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
            rc = write(image, &byte, 1);
            if(rc != 1)
                return BKERROR_WRITE_GENERIC;
        }
    }
    
    /* VOLUME creation date */
    epochToLongString(creationTime, aString);
    
    rc = write(image, aString, 17);
    if(rc != 17)
        return BKERROR_WRITE_GENERIC;
    /* END VOLUME creation date */
    
    /* volume modification date (same as creation) */
    rc = write(image, aString, 17);
    if(rc != 17)
        return BKERROR_WRITE_GENERIC;
    
    /* VOLUME expiration date (none) */
    rc = writeByteBlock(image, '0', 16);
    if(rc < 0)
        return rc;
    
    byte = 0;
    rc = write711(image, &byte);
    if(rc != 1)
        return BKERROR_WRITE_GENERIC;
    /* END VOLUME expiration date (none) */
    
    /* volume effective date (same as creation) */
    rc = write(image, aString, 17);
    if(rc != 17)
        return BKERROR_WRITE_GENERIC;
    
    /* file structure version */
    byte = 1;
    rc = write711(image, &byte);
    if(rc != 1)
        return BKERROR_WRITE_GENERIC;
    
    /* reserved, applications use, reserved */
    rc = writeByteBlock(image, 0, 1166);
    if(rc < 0)
        return rc;
    
    return 1;
}
