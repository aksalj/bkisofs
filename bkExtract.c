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
#include "bkWrite.h"

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

/*******************************************************************************
* bk_extract_dir()
* Extracts a directory with all its contents from the iso to the filesystem.
* Public function.
* */
int bk_extract_dir(VolInfo* volInfo, const char* srcDir,
                   const char* destDir, bool keepPermissions,
                   void(*progressFunction)(void))
{
    int rc;
    Path* srcPath;
    
    if(srcDir[0] == '/' && srcDir[1] == '\0')
    /* root, not allowed */
        return BKERROR_EXTRACT_ROOT;
    
    srcPath = malloc(sizeof(Path));
    if(srcPath == NULL)
        return BKERROR_OUT_OF_MEMORY;
    
    srcPath->numDirs = 0;
    srcPath->dirs = NULL;
    
    rc = makePathFromString(srcDir, srcPath);
    if(rc <= 0)
    {
        freePath(srcPath);
        return rc;
    }
    
    volInfo->stopOperation = false;
    
    rc = extractDir(volInfo, volInfo->imageForReading, srcPath, destDir, 
                    keepPermissions, progressFunction);
    if(rc <= 0)
    {
        freePath(srcPath);
        return rc;
    }
    
    freePath(srcPath);
    
    return 1;
}

/*******************************************************************************
* bk_extract_file()
* Extracts a file from the iso to the filesystem.
* Public function.
* */
int bk_extract_file(VolInfo* volInfo, const char* srcFile, 
                    const char* destDir, bool keepPermissions, 
                    void(*progressFunction)(void))
{
    int rc;
    FilePath srcPath;
    
    rc = makeFilePathFromString(srcFile, &srcPath);
    if(rc <= 0)
    {
        freePathDirs(&(srcPath.path));
        return rc;
    }
    
    volInfo->stopOperation = false;
    
    rc = extractFile(volInfo, volInfo->imageForReading, &srcPath, destDir, 
                     keepPermissions, progressFunction);
    if(rc <= 0)
    {
        freePathDirs(&(srcPath.path));
        return rc;
    }
    
    freePathDirs(&(srcPath.path));
    
    return 1;
}

/*******************************************************************************
* extractDir()
* Extracts a directory with all its contents from the iso to the filesystem.
* don't try to extract root, don't know what will happen
* */
int extractDir(VolInfo* volInfo, int image, 
               const Path* srcDir, const char* destDir, bool keepPermissions, 
               void(*progressFunction)(void))
{
    int rc;
    
    /* vars to find file location on image */
    BkDir* srcDirInTree;
    bool dirFound;
    
    /* vars to create destination dir */
    char* newDestDir;
    unsigned destDirPerms;
    
    /* vars to extract files */
    FilePath filePath;
    BkFile* currentFile;
    
    /* vars to extract subdirectories */
    BkDir* currentDir;
    Path* newSrcDir;
    
    dirFound = findDirByPath(srcDir, &(volInfo->dirTree), &srcDirInTree);
    if(!dirFound)
        return BKERROR_DIR_NOT_FOUND_ON_IMAGE;
    
    /* CREATE destination dir */
    /* 1 for '/', 1 for '\0' */
    newDestDir = malloc(strlen(destDir) + strlen( (srcDir->dirs)[srcDir->numDirs - 1] ) + 2);
    if(newDestDir == NULL)
        return BKERROR_OUT_OF_MEMORY;
    
    strcpy(newDestDir, destDir);
    strcat(newDestDir, (srcDir->dirs)[srcDir->numDirs - 1]);
    strcat(newDestDir, "/");
    
    if(keepPermissions)
        destDirPerms = srcDirInTree->posixFileMode;
    else
        destDirPerms = volInfo->posixDirDefaults;
    
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
    /* END CREATE destination dir */
    
    /* BEGIN extract each file in directory */
    filePath.path = *srcDir; /* filePath.path is readonly so pointer sharing is ok here */
    currentFile = srcDirInTree->files;
    while(currentFile != NULL)
    {
        strcpy(filePath.filename, currentFile->name);
        
        rc = extractFile(volInfo, image, &filePath, newDestDir, 
                         keepPermissions, progressFunction);
        if(rc <= 0)
        {
            bool goOn;
            
            if(volInfo->warningCbk != NULL && rc != BKERROR_OPER_CANCELED_BY_USER)
            /* perhaps the user wants to ignore this failure */
            {
                snprintf(volInfo->warningMessage, BK_WARNING_MAX_LEN, 
                         "Failed to extract file '%s' to '%s': '%s'",
                         filePath.filename, newDestDir, bk_get_error_string(rc));
                goOn = volInfo->warningCbk(volInfo->warningMessage);
                rc = BKWARNING_OPER_PARTLY_FAILED;
            }
            else
                goOn = false;
            
            if(!goOn)
            {
                free(newDestDir);
                return rc;
            }
        }
        
        currentFile = currentFile->next;
    }
    /* END extract each file in directory */
    
    /* BEGIN extract each subdirectory */
    currentDir = srcDirInTree->directories;
    while(currentDir != NULL)
    {
        rc = makeLongerPath(srcDir, currentDir->name, &newSrcDir);
        if(rc <= 0)
        {
            free(newDestDir);
            return rc;
        }
        
        rc = extractDir(volInfo, image, newSrcDir, newDestDir, keepPermissions, 
                        progressFunction);
        if(rc <= 0)
        {
            bool goOn;
            
            if(volInfo->warningCbk != NULL && rc != BKERROR_OPER_CANCELED_BY_USER)
            /* perhaps the user wants to ignore this failure */
            {
                snprintf(volInfo->warningMessage, BK_WARNING_MAX_LEN, 
                         "Failed to extract directory '%s' to '%s': '%s'",
                         currentDir->name, newDestDir, bk_get_error_string(rc));
                goOn = volInfo->warningCbk(volInfo->warningMessage);
                rc = BKWARNING_OPER_PARTLY_FAILED;
            }
            else
                goOn = false;
            
            if(!goOn)
            {
                free(newDestDir);
                return rc;
            }
        }
        
        freePath(newSrcDir);
        
        currentDir = currentDir->next;
    }
    /* END extract each subdirectory */
    
    free(newDestDir);
    
    return 1;
}

/*******************************************************************************
* extractFile()
* Extracts a file from the iso to the filesystem.
* destDir must have trailing slash
* 
* */
int extractFile(VolInfo* volInfo, int image, 
                const FilePath* pathAndName, const char* destDir, 
                bool keepPermissions, void(*progressFunction)(void))
{
    /* vars to find file location on image */
    BkDir* parentDir;
    bool dirFound;
    BkFile* pointerToIt; /* pointer to the node with file to read */
    bool fileFound;
    
    /* vars for the source file */
    int srcFile; /* returned by open() */
    bool srcFileWasOpened;
    
    /* vars to create destination file */
    char* destPathAndName;
    unsigned destFilePerms;
    int destFile; /* returned by open() */
    
    int rc;
    
    if(volInfo->stopOperation)
        return BKERROR_OPER_CANCELED_BY_USER;
    
    dirFound = findDirByPath(&(pathAndName->path), &(volInfo->dirTree), &parentDir);
    if(!dirFound)
        return BKERROR_DIR_NOT_FOUND_ON_IMAGE;
    
    if(progressFunction != NULL)
        progressFunction();
    
    pointerToIt = parentDir->files;
    fileFound = false;
    while(pointerToIt != NULL && !fileFound)
    /* find the file in parentDir */
    {
        if(strcmp(pointerToIt->name, pathAndName->filename) == 0)
        /* this is the file */
        {
            if(pointerToIt->onImage)
            {
                srcFile = image;
                lseek(image, pointerToIt->position, SEEK_SET);
                srcFileWasOpened = false;
            }
            else
            {
                srcFile = open(pointerToIt->pathAndName, O_RDONLY);
                if(srcFile == -1)
                    return BKERROR_OPEN_READ_FAILED;
                srcFileWasOpened = true;
            }
            
            fileFound = true;
            
            destPathAndName = malloc(strlen(destDir) + strlen(pathAndName->filename) + 1);
            if(destPathAndName == NULL)
            {
                if(srcFileWasOpened)
                    close(srcFile);
                return BKERROR_OUT_OF_MEMORY;
            }
            
            strcpy(destPathAndName, destDir);
            strcat(destPathAndName, pathAndName->filename);
            
            if(access(destPathAndName, F_OK) == 0)
            {
                if(srcFileWasOpened)
                    close(srcFile);
                free(destPathAndName);
                return BKERROR_DUPLICATE_EXTRACT;
            }
            
            /* WRITE file */
            if(keepPermissions)
                destFilePerms = pointerToIt->posixFileMode;
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
            
            rc = copyByteBlock(srcFile, destFile, pointerToIt->size);
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
        }
        else
        {
            pointerToIt = pointerToIt->next;
        }
    }
    if(!fileFound)
    {
        if(srcFileWasOpened)
            close(srcFile);
        return BKERROR_FILE_NOT_FOUND_ON_IMAGE;
    }
    
    if(srcFileWasOpened)
        close(srcFile);
    
    return 1;
}
