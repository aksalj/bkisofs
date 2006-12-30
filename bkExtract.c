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

#include "bk.h"
#include "bkInternal.h"
#include "bkExtract.h"
#include "bkPath.h"
#include "bkError.h"

/*******************************************************************************
* bk_extract_boot_record()
* Extracts the el torito boot record to the file destPathAndName, with
* permissions destFilePerms.
* */
int bk_extract_boot_record(const VolInfo* volInfo, const char* destPathAndName, 
                           unsigned destFilePerms)
{
    int srcFile; /* returned by open() */
    bool srcFileWasOpened;
    int destFile; /* returned by open() */
    int rc;
    
    if(volInfo->bootMediaType == BOOT_MEDIA_NONE)
        return BKERROR_EXTRACT_ABSENT_BOOT_RECORD;
    
    if(volInfo->bootMediaType != BOOT_MEDIA_NO_EMULATION &&
       volInfo->bootMediaType != BOOT_MEDIA_1_2_FLOPPY &&
       volInfo->bootMediaType != BOOT_MEDIA_1_44_FLOPPY &&
       volInfo->bootMediaType != BOOT_MEDIA_2_88_FLOPPY)
    {
        return BKERROR_EXTRACT_UNKNOWN_BOOT_MEDIA;
    }
    
    /* SET source file (open if needed) */
    if(volInfo->bootRecordIsVisible)
    /* boot record is a file in the tree */
    {
        if(volInfo->bootRecordOnImage->onImage)
        {
            srcFile = volInfo->imageForReading;
            lseek(volInfo->imageForReading, volInfo->bootRecordOnImage->position, SEEK_SET);
            srcFileWasOpened = false;
        }
        else
        {
            srcFile = open(volInfo->bootRecordOnImage->pathAndName, O_RDONLY);
            if(srcFile == -1)
                return BKERROR_OPEN_READ_FAILED;
            srcFileWasOpened = true;
        }
    }
    else
    /* boot record is not a file in the tree */
    {
        if(volInfo->bootRecordIsOnImage)
        {
            srcFile = volInfo->imageForReading;
            lseek(volInfo->imageForReading, volInfo->bootRecordOffset, SEEK_SET);
            srcFileWasOpened = false;
        }
        else
        {
            srcFile = open(volInfo->bootRecordPathAndName, O_RDONLY);
            if(srcFile == -1)
                return BKERROR_OPEN_READ_FAILED;
            srcFileWasOpened = true;
        }
    }
    /* END SET source file (open if needed) */
    
    destFile = open(destPathAndName, O_WRONLY | O_CREAT | O_TRUNC, 
                    destFilePerms);
    if(destFile == -1)
    {
        if(srcFileWasOpened)
            close(srcFile);
        return BKERROR_OPEN_WRITE_FAILED;
    }
    
    rc = copyByteBlock(srcFile, destFile, volInfo->bootRecordSize);
    if(rc <= 0)
    {
        if(srcFileWasOpened)
            close(srcFile);
        return rc;
    }
    
    close(destFile);
    
    if(srcFileWasOpened)
        close(srcFile);
    
    return 1;
}

int bk_extract(VolInfo* volInfo, const char* srcPathAndName, 
               const char* destDir, bool keepPermissions, 
               void(*progressFunction)(void))
{
    int rc;
    NewPath srcPath;
    BkDir* parentDir;
    bool dirFound;
    printf("want to extract '%s' to '%s'\n", srcPathAndName, destDir);fflush(NULL);
    rc = makeNewPathFromString(srcPathAndName, &srcPath);
    if(rc <= 0)
    {
        freePathContents(&srcPath);
        return rc;
    }
    
    if(srcPath.numChildren == 0)
    {
        freePathContents(&srcPath);
        return BKERROR_EXTRACT_ROOT;
    }
    
    volInfo->stopOperation = false;
    
    /* i want the parent directory */
    srcPath.numChildren--;
    dirFound = findDirByNewPath(&srcPath, &(volInfo->dirTree), &parentDir);
    srcPath.numChildren++;
    if(!dirFound)
    {
        freePathContents(&srcPath);
        return BKERROR_DIR_NOT_FOUND_ON_IMAGE;
    }
    
    rc = extract(volInfo, parentDir, srcPath.children[srcPath.numChildren - 1], 
                 destDir, keepPermissions, progressFunction);
    if(rc <= 0)
    {
        freePathContents(&srcPath);
        return rc;
    }
    
    freePathContents(&srcPath);
    
    return 1;
}

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
        if(rc <= 0)
            return rc;
    }
    
    if(sizeLastBlock > 0)
    {
        rc = read(src, block, sizeLastBlock);
        if(rc != sizeLastBlock)
                return BKERROR_READ_GENERIC;
        rc = write(dest, block, sizeLastBlock);
        if(rc <= 0)
                return rc;
    }
    
    return 1;
}

int extract(VolInfo* volInfo, BkDir* parentDir, char* nameToExtract, 
            const char* destDir, bool keepPermissions, 
            void(*progressFunction)(void))
{
    BkFileBase* child;
    int rc;
    printf("extract(%s) from '%s' to '%s'\n", nameToExtract, BK_BASE_PTR(parentDir)->name, destDir);fflush(NULL);
    child = parentDir->children;
    while(child != NULL)
    {
        if(progressFunction != NULL)
            progressFunction();
        if(volInfo->stopOperation)
            return BKERROR_OPER_CANCELED_BY_USER;
        
        if(strcmp(child->name, nameToExtract) == 0)
        {
            if( IS_DIR(child->posixFileMode) )
            {
                rc = extractDir(volInfo, BK_DIR_PTR(child), destDir, 
                                keepPermissions, progressFunction);
            }
            else if ( IS_REG_FILE(child->posixFileMode) )
            {
                rc = extractFile(volInfo, BK_FILE_PTR(child), destDir, 
                                 keepPermissions, progressFunction);
            }
            else
            {
                printf("trying to extract something that's not a file or directory, ignored\n");fflush(NULL);
            }
            
            if(rc <= 0)
            {
                bool goOn;
                
                if(volInfo->warningCbk != NULL && !volInfo->stopOperation)
                /* perhaps the user wants to ignore this failure */
                {
                    snprintf(volInfo->warningMessage, BK_WARNING_MAX_LEN, 
                             "Failed to extract item '%s': '%s'",
                             child->name, 
                             bk_get_error_string(rc));
                    goOn = volInfo->warningCbk(volInfo->warningMessage);
                    rc = BKWARNING_OPER_PARTLY_FAILED;
                }
                else
                    goOn = false;
                
                if(!goOn)
                {
                    volInfo->stopOperation = true;
                    return rc;
                }
            }
        }
        
        child = child->next;
    }
    
    return 1;
}

int extractDir(VolInfo* volInfo, BkDir* srcDir, const char* destDir, 
               bool keepPermissions, void(*progressFunction)(void))
{
    int rc;
    BkFileBase* child;
    
    /* vars to create destination dir */
    char* newDestDir;
    unsigned destDirPerms;
    printf("extractDir(%s) to '%s'\n", BK_BASE_PTR(srcDir)->name, destDir);fflush(NULL);
    /* CREATE destination dir on filesystem */
    /* 1 for '\0' */
    newDestDir = malloc(strlen(destDir) + strlen(BK_BASE_PTR(srcDir)->name) + 2);
    if(newDestDir == NULL)
        return BKERROR_OUT_OF_MEMORY;
    
    strcpy(newDestDir, destDir);
    if(destDir[strlen(destDir) - 1] != '/')
        strcat(newDestDir, "/");
    strcat(newDestDir, BK_BASE_PTR(srcDir)->name);
    
    if(keepPermissions)
        destDirPerms = BK_BASE_PTR(BK_BASE_PTR(srcDir))->posixFileMode;
    else
        destDirPerms = volInfo->posixDirDefaults;
    /* want to make sure user has write and execute permissions to new directory 
    * so that can extract stuff into it */
    destDirPerms |= 0300;
    
    if(access(newDestDir, F_OK) == 0)
    {
        free(newDestDir);
        return BKERROR_DUPLICATE_EXTRACT;
    }
    
    rc = mkdir(newDestDir, destDirPerms);
    if(rc == -1)
    {
        free(newDestDir);
        return BKERROR_MKDIR_FAILED;
    }
    /* END CREATE destination dir on filesystem */
    
    /* EXTRACT children */
    child = srcDir->children;
    while(child != NULL)
    {
        rc = extract(volInfo, srcDir, child->name, newDestDir, keepPermissions, progressFunction);
        if(rc <= 0)
        {
            free(newDestDir);
            return rc;
        }
        
        child = child->next;
    }
    /* END EXTRACT children */
    
    free(newDestDir);
    
    return 1;
}

int extractFile(VolInfo* volInfo, BkFile* srcFileInTree, const char* destDir, 
                bool keepPermissions, void(*progressFunction)(void))
{
    int srcFile;
    bool srcFileWasOpened;
    char* destPathAndName;
    unsigned destFilePerms;
    int destFile; /* returned by open() */
    int rc;
    
    if(srcFileInTree->onImage)
    {
        srcFile = volInfo->imageForReading;
        lseek(volInfo->imageForReading, srcFileInTree->position, SEEK_SET);
        srcFileWasOpened = false;
    }
    else
    {
        srcFile = open(srcFileInTree->pathAndName, O_RDONLY);
        if(srcFile == -1)
            return BKERROR_OPEN_READ_FAILED;
        srcFileWasOpened = true;
    }
    
    destPathAndName = malloc(strlen(destDir) + 
                             strlen(BK_BASE_PTR(srcFileInTree)->name) + 2);
    if(destPathAndName == NULL)
    {
        if(srcFileWasOpened)
            close(srcFile);
        return BKERROR_OUT_OF_MEMORY;
    }
    
    strcpy(destPathAndName, destDir);
    if(destDir[strlen(destDir) - 1] != '/')
        strcat(destPathAndName, "/");
    strcat(destPathAndName, BK_BASE_PTR(srcFileInTree)->name);
    printf("extractFile(%s) to '%s' ->'%s'\n", BK_BASE_PTR(srcFileInTree)->name, destDir, destPathAndName);fflush(NULL);
    if(access(destPathAndName, F_OK) == 0)
    {
        if(srcFileWasOpened)
            close(srcFile);
        free(destPathAndName);
        return BKERROR_DUPLICATE_EXTRACT;
    }
    
    /* WRITE file */
    if(keepPermissions)
        destFilePerms = BK_BASE_PTR(srcFileInTree)->posixFileMode;
    else
        destFilePerms = volInfo->posixFileDefaults;
    
    destFile = open(destPathAndName, O_WRONLY | O_CREAT | O_TRUNC, destFilePerms);
    if(destFile == -1)
    {
        if(srcFileWasOpened)
            close(srcFile);
        free(destPathAndName);
        return BKERROR_OPEN_WRITE_FAILED;
    }
    
    free(destPathAndName);
    
    rc = copyByteBlock(srcFile, destFile, srcFileInTree->size);
    if(rc < 0)
    {
        close(destFile);
        if(srcFileWasOpened)
            close(srcFile);
        return rc;
    }
    
    close(destFile);
    if(destFile == -1)
    {
        if(srcFileWasOpened)
            close(srcFile);
        return BKERROR_EXOTIC;
    }
    /* END WRITE file */
    
    if(srcFileWasOpened)
        close(srcFile);
    
    return 1;
}
