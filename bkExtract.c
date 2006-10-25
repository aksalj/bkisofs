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
int bk_extract_dir(const VolInfo* volInfo, const char* srcDir,
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
    
    rc = extractDir(volInfo, volInfo->imageForReading, &(volInfo->dirTree), srcPath, destDir, 
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
int bk_extract_file(const VolInfo* volInfo, const char* srcFile, 
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
    
    rc = extractFile(volInfo, volInfo->imageForReading, &(volInfo->dirTree), &srcPath, destDir, 
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
int extractDir(const VolInfo* volInfo, int image, const Dir* tree, const Path* srcDir, 
               const char* destDir, bool keepPermissions, 
               void(*progressFunction)(void))
{
    int rc;
    int count;
    
    /* vars to find file location on image */
    const Dir* srcDirInTree;
    DirLL* searchDir; /* to find a dir in the tree */
    bool dirFound;
    
    /* vars to create destination dir */
    char* newDestDir;
    unsigned destDirPerms;
    
    /* vars to extract files */
    FilePath filePath;
    FileLL* currentFile;
    
    /* vars to extract subdirectories */
    DirLL* currentDir;
    Path* newSrcDir;
    
    /* FIND parent dir to know what the contents are */
    srcDirInTree = tree;
    for(count = 0; count < srcDir->numDirs; count++)
    /* each directory in the path */
    {
        searchDir = srcDirInTree->directories;
        dirFound = false;
        while(searchDir != NULL && !dirFound)
        /* find the directory */
        {
            if(strcmp(searchDir->dir.name, srcDir->dirs[count]) == 0)
            {
                dirFound = true;
                srcDirInTree = &(searchDir->dir);
            }
            else
                searchDir = searchDir->next;
        }
        if(!dirFound)
            return BKERROR_DIR_NOT_FOUND_ON_IMAGE;
    }
    /* END FIND parent dir to know what the contents are */
    
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
        strcpy(filePath.filename, currentFile->file.name);
        
        rc = extractFile(volInfo, image, tree, &filePath, newDestDir, keepPermissions, 
                         progressFunction);
        if(rc <= 0)
        {
            free(newDestDir);
            return rc;
        }
        
        currentFile = currentFile->next;
    }
    /* END extract each file in directory */
    
    /* BEGIN extract each subdirectory */
    currentDir = srcDirInTree->directories;
    while(currentDir != NULL)
    {
        rc = makeLongerPath(srcDir, currentDir->dir.name, &newSrcDir);
        if(rc <= 0)
        {
            free(newDestDir);
            return rc;
        }
        
        rc = extractDir(volInfo, image, tree, newSrcDir, newDestDir, keepPermissions, 
                        progressFunction);
        if(rc <= 0)
        {
            free(newDestDir);
            return rc;
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
int extractFile(const VolInfo* volInfo, int image, const Dir* tree, const FilePath* pathAndName, 
                const char* destDir, bool keepPermissions, 
                void(*progressFunction)(void))
{
    /* vars to find file location on image */
    const Dir* parentDir;
    DirLL* searchDir;
    bool dirFound;
    FileLL* pointerToIt; /* pointer to the node with file to read */
    bool fileFound;
    
    /* vars for the source file */
    int srcFile; /* returned by open() */
    bool srcFileWasOpened;
    
    /* vars to create destination file */
    char* destPathAndName;
    unsigned destFilePerms;
    int destFile; /* returned by open() */
    
    int count;
    int rc;
    
    parentDir = tree;
    for(count = 0; count < pathAndName->path.numDirs; count++)
    /* each directory in the path */
    {
        searchDir = parentDir->directories;
        dirFound = false;
        while(searchDir != NULL && !dirFound)
        /* find the directory */
        {
            if(strcmp(searchDir->dir.name, pathAndName->path.dirs[count]) == 0)
            {
                dirFound = true;
                parentDir = &(searchDir->dir);
            }
            else
                searchDir = searchDir->next;
        }
        if(!dirFound)
            return BKERROR_DIR_NOT_FOUND_ON_IMAGE;
    }
    
    /* now i have parentDir pointing to the parent directory */
    
    if(progressFunction != NULL)
        progressFunction();
    
    pointerToIt = parentDir->files;
    fileFound = false;
    while(pointerToIt != NULL && !fileFound)
    /* find the file in parentDir */
    {
        if(strcmp(pointerToIt->file.name, pathAndName->filename) == 0)
        /* this is the file */
        {
            if(pointerToIt->file.onImage)
            {
                srcFile = image;
                lseek(image, pointerToIt->file.position, SEEK_SET);
                srcFileWasOpened = false;
            }
            else
            {
                srcFile = open(pointerToIt->file.pathAndName, O_RDONLY);
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
                destFilePerms = pointerToIt->file.posixFileMode;
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
            
            rc = copyByteBlock(srcFile, destFile, pointerToIt->file.size);
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
