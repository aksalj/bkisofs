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
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "bk.h"
#include "bkPath.h"
#include "bkAdd.h"
#include "bkError.h"
#include "bkGet.h"
#include "bkMangle.h"

/*******************************************************************************
* addDir()
* adds a directory from the filesystem to the image
*
* Receives:
* - BkDir*, root of tree to add to
* - char*, path of directory to add, must end with trailing slash
* - Path*, destination on image
* */
int addDir(VolInfo* volInfo, BkDir* tree, const char* srcPath, const Path* destDir)
{
    int rc;
    
    /* vars to add dir to tree */
    char srcDirName[NCHARS_FILE_ID_MAX_STORE];
    BkDir* destDirInTree;
    bool dirFound;
    BkFileBase* oldHead; /* old head of the children list */
    struct stat statStruct; /* to get info on the dir */
    BkDir* newDir; /* the one i'm creating */
    
    /* vars to read contents of a dir on fs */
    DIR* srcDir;
    struct dirent* dirEnt;
    struct stat anEntry;
    
    /* vars for children */
    Path* newDestDir;
    int newSrcPathLen; /* length of new path (including trailing '/' but not filename) */
    char* newSrcPathAndName; /* both for child dirs and child files */
    
    if(srcPath[strlen(srcPath) - 1] != '/')
    /* must have trailing slash */
        return BKERROR_DIRNAME_NEED_TRAILING_SLASH;
    
    dirFound = findDirByPath(destDir, tree, &destDirInTree);
    if(!dirFound)
        return BKERROR_DIR_NOT_FOUND_ON_IMAGE;
    
    /* get the name of the directory to be added */
    rc = getLastDirFromString(srcPath, srcDirName);
    if(rc <= 0)
        return rc;
    
    if( !nameIsValid(srcDirName) )
        return BKERROR_NAME_INVALID_CHAR;
    
    if(itemIsInDir(srcDirName, destDirInTree))
        return BKERROR_DUPLICATE_ADD;
    
    oldHead = destDirInTree->children;
    
    /* ADD directory to tree */
    rc = stat(srcPath, &statStruct);
    if(rc == -1)
        return BKERROR_STAT_FAILED;
    
    if( !IS_DIR(statStruct.st_mode) )
        return BKERROR_TARGET_NOT_A_DIR;
    
    destDirInTree->children = malloc(sizeof(BkDir));
    if(destDirInTree->children == NULL)
    {
        destDirInTree->children = oldHead;
        return BKERROR_OUT_OF_MEMORY;
    }
    
    newDir = BK_DIR_PTR(destDirInTree->children);
    
    BK_BASE_PTR(newDir)->next = oldHead;
    
    strcpy(BK_BASE_PTR(newDir)->name, srcDirName);
    
    BK_BASE_PTR(newDir)->posixFileMode = statStruct.st_mode;
    
    newDir->children = NULL;
    /* END ADD directory to tree */
    
    /* remember length of original */
    newSrcPathLen = strlen(srcPath);
    
    /* including the file/dir name and the trailing '/' and the '\0' */
    newSrcPathAndName = malloc(newSrcPathLen + NCHARS_FILE_ID_MAX_STORE + 1);
    if(newSrcPathAndName == NULL)
    {
        free(destDirInTree->children);
        destDirInTree->children = oldHead;
        return BKERROR_OUT_OF_MEMORY;
    }
    
    strcpy(newSrcPathAndName, srcPath);
    
    /* destination for children */
    rc = makeLongerPath(destDir, srcDirName, &newDestDir);
    if(rc <= 0)
    {
        free(destDirInTree->children);
        destDirInTree->children = oldHead;
        free(newSrcPathAndName);
        freePath(newDestDir);
        return rc;
    }
    
    /* ADD contents of directory */
    srcDir = opendir(srcPath);
    if(srcDir == NULL)
    {
        free(destDirInTree->children);
        destDirInTree->children = oldHead;
        free(newSrcPathAndName);
        freePath(newDestDir);
        return BKERROR_OPENDIR_FAILED;
    }
    
    /* it may be possible but in any case very unlikely that readdir() will fail
    * if it does, it returns NULL (same as end of dir) */
    while( (dirEnt = readdir(srcDir)) != NULL )
    {
        if( strcmp(dirEnt->d_name, ".") != 0 && strcmp(dirEnt->d_name, "..") != 0 )
        /* not "." or ".." (safely ignore those two) */
        {
            bool goOn;
            
            if(strlen(dirEnt->d_name) > NCHARS_FILE_ID_MAX_STORE - 1)
            {
                if(volInfo->warningCbk != NULL)
                /* perhaps the user wants to ignore this failure */
                {
                    snprintf(volInfo->warningMessage, BK_WARNING_MAX_LEN, 
                             "Failed to add item '%s': '%s'",
                             dirEnt->d_name, 
                             bk_get_error_string(BKERROR_MAX_NAME_LENGTH_EXCEEDED));
                    goOn = volInfo->warningCbk(volInfo->warningMessage);
                    rc = BKWARNING_OPER_PARTLY_FAILED;
                }
                else
                    goOn = false;
                
                if(goOn)
                    continue;
                else
                {
                    free(newSrcPathAndName);
                    freePath(newDestDir);
                    return BKERROR_MAX_NAME_LENGTH_EXCEEDED;
                }
            }
            
            /* append file/dir name */
            strcpy(newSrcPathAndName + newSrcPathLen, dirEnt->d_name);
            
            rc = stat(newSrcPathAndName, &anEntry);
            if(rc == -1)
            {
                if(volInfo->warningCbk != NULL)
                /* perhaps the user wants to ignore this failure */
                {
                    snprintf(volInfo->warningMessage, BK_WARNING_MAX_LEN, 
                             "Failed to add item '%s': '%s'",
                             newSrcPathAndName, 
                             bk_get_error_string(BKERROR_STAT_FAILED));
                    goOn = volInfo->warningCbk(volInfo->warningMessage);
                    rc = BKWARNING_OPER_PARTLY_FAILED;
                }
                else
                    goOn = false;
                
                if(goOn)
                    continue;
                else
                {
                    free(newSrcPathAndName);
                    freePath(newDestDir);
                    return BKERROR_STAT_FAILED;
                }
            }
            
            if( IS_DIR(anEntry.st_mode) )
            {
                strcat(newSrcPathAndName, "/");
                
                rc = addDir(volInfo, tree, newSrcPathAndName, newDestDir);
                if(rc <= 0)
                {
                        free(newSrcPathAndName);
                        freePath(newDestDir);
                        return rc;
                }
            }
            else if( IS_REG_FILE(anEntry.st_mode) )
            {
                rc = addFile(tree, newSrcPathAndName, newDestDir);
                if(rc <= 0)
                {
                    if(volInfo->warningCbk != NULL)
                    /* perhaps the user wants to ignore this failure */
                    {
                        snprintf(volInfo->warningMessage, BK_WARNING_MAX_LEN, 
                                 "Failed to add file '%s': '%s'",
                                 newSrcPathAndName, bk_get_error_string(rc));
                        goOn = volInfo->warningCbk(volInfo->warningMessage);
                        rc = BKWARNING_OPER_PARTLY_FAILED;
                    }
                    else
                        goOn = false;
                    
                    if(!goOn)
                    {
                        free(newSrcPathAndName);
                        freePath(newDestDir);
                        return rc;
                    }
                }
            }
            else
            /* not regular file or directory */
            {
                if(volInfo->warningCbk != NULL)
                /* perhaps the user wants to ignore this failure */
                {
                    snprintf(volInfo->warningMessage, BK_WARNING_MAX_LEN, 
                             "Failed to add item '%s': '%s'",
                             newSrcPathAndName, 
                             bk_get_error_string(BKERROR_NO_SPECIAL_FILES));
                    goOn = volInfo->warningCbk(volInfo->warningMessage);
                    rc = BKWARNING_OPER_PARTLY_FAILED;
                }
                else
                    goOn = false;
                
                if(goOn)
                    continue;
                else
                {
                    free(newSrcPathAndName);
                    freePath(newDestDir);
                    return BKERROR_NO_SPECIAL_FILES;
                }
            }
            
        } /* if */
        
    } /* while */
    
    rc = closedir(srcDir);
    if(rc != 0)
    /* exotic error */
    {
        free(newSrcPathAndName);
        freePath(newDestDir);
        return BKERROR_EXOTIC;
    }
    /* END ADD contents of directory */
    
    free(newSrcPathAndName);
    freePath(newDestDir);
    
    return 1;
}

/*******************************************************************************
* addFile()
* adds a file from the filesystem to the image
*
* Receives:
* - Dir*, root of tree to add to
* - char*, path and name of file to add, must end with trailing slash
* - Path*, destination on image
* Notes:
*  will only add a regular file (symblic links are followed, see stat(2))
* */
int addFile(BkDir* tree, const char* srcPathAndName, const Path* destDir)
{
    int rc;
    BkFileBase* oldHead; /* of the files list */
    char filename[NCHARS_FILE_ID_MAX_STORE];
    struct stat statStruct;
    BkFile* newFile;
    
    /* vars to find the dir in the tree */
    BkDir* destDirInTree;
    bool dirFound;
    
    rc = getFilenameFromPath(srcPathAndName, filename);
    if(rc <= 0)
        return rc;
    
    if( !nameIsValid(filename) )
        return BKERROR_NAME_INVALID_CHAR;
    
    dirFound = findDirByPath(destDir, tree, &destDirInTree);
    if(!dirFound)
        return BKERROR_DIR_NOT_FOUND_ON_IMAGE;
    
    if(itemIsInDir(filename, destDirInTree))
        return BKERROR_DUPLICATE_ADD;
    
    oldHead = destDirInTree->children;
    
    /* ADD file */
    rc = stat(srcPathAndName, &statStruct);
    if(rc == -1)
    {
        destDirInTree->children = oldHead;
        return BKERROR_STAT_FAILED;
    }
    
    if( !IS_REG_FILE(statStruct.st_mode) )
    /* not a regular file */
    {
        destDirInTree->children = oldHead;
        return BKERROR_NO_SPECIAL_FILES;
    }
    
    if(statStruct.st_size > 0xFFFFFFFF)
    /* size won't fit in a 32bit variable on the iso */
    {
        destDirInTree->children = oldHead;
        return BKERROR_ADD_FILE_TOO_BIG;
    }
    
    destDirInTree->children = malloc(sizeof(BkFile));
    if(destDirInTree->children == NULL)
    {
        destDirInTree->children = oldHead;
        return BKERROR_OUT_OF_MEMORY;
    }
    
    newFile = BK_FILE_PTR(destDirInTree->children);
    
    BK_BASE_PTR(newFile)->next = oldHead;
    
    strcpy(BK_BASE_PTR(newFile)->name, filename);
    
    BK_BASE_PTR(newFile)->posixFileMode = statStruct.st_mode;
    
    newFile->size = statStruct.st_size;
    
    newFile->onImage = false;
    
    newFile->position = 0;
    
    newFile->pathAndName = malloc(strlen(srcPathAndName) + 1);
    strcpy(newFile->pathAndName, srcPathAndName);
    /* END ADD file */
    
    return 1;
}

/*******************************************************************************
* bk_add_boot_record()
* Source boot file must be exactly the right size if floppy emulation requested.
* */
int bk_add_boot_record(VolInfo* volInfo, const char* srcPathAndName, 
                       int bootMediaType)
{
    struct stat statStruct;
    int rc;
    
    if(bootMediaType != BOOT_MEDIA_NO_EMULATION &&
       bootMediaType != BOOT_MEDIA_1_2_FLOPPY &&
       bootMediaType != BOOT_MEDIA_1_44_FLOPPY &&
       bootMediaType != BOOT_MEDIA_2_88_FLOPPY)
    {
        return BKERROR_ADD_UNKNOWN_BOOT_MEDIA;
    }
    
    rc = stat(srcPathAndName, &statStruct);
    if(rc == -1)
        return BKERROR_STAT_FAILED;
    
    if( (bootMediaType == BOOT_MEDIA_1_2_FLOPPY &&
         statStruct.st_size != 1228800) ||
        (bootMediaType == BOOT_MEDIA_1_44_FLOPPY &&
         statStruct.st_size != 1474560) ||
        (bootMediaType == BOOT_MEDIA_2_88_FLOPPY &&
         statStruct.st_size != 2949120) )
    {
        return BKERROR_ADD_BOOT_RECORD_WRONG_SIZE;
    }
    
    volInfo->bootMediaType = bootMediaType;
    
    volInfo->bootRecordSize = statStruct.st_size;
    
    volInfo->bootRecordIsOnImage = false;
    
    /* make copy of the source path and name */
    if(volInfo->bootRecordPathAndName != NULL)
        free(volInfo->bootRecordPathAndName);
    volInfo->bootRecordPathAndName = malloc(strlen(srcPathAndName) + 1);
    if(volInfo->bootRecordPathAndName == NULL)
    {
        volInfo->bootMediaType = BOOT_MEDIA_NONE;
        return BKERROR_OUT_OF_MEMORY;
    }
    strcpy(volInfo->bootRecordPathAndName, srcPathAndName);
    
    /* this is the wrong function to use if you want a visible one */
    volInfo->bootRecordIsVisible = false;
    
    return 1;
}

/*******************************************************************************
* bk_add_dir()
* public interface for addDir()
* */
int bk_add_dir(VolInfo* volInfo, const char* srcPathAndName, 
               const char* destPathStr)
{
    int rc;
    Path* destPath;
    
    destPath = malloc(sizeof(Path));
    if(destPath == NULL)
        return BKERROR_OUT_OF_MEMORY;
    
    destPath->numDirs = 0;
    destPath->dirs = NULL;
    
    if(destPathStr[0] == '/' && destPathStr[1] == '\0')
    /* root, special case */
    {
        rc = addDir(volInfo, &(volInfo->dirTree), srcPathAndName, destPath);
        if(rc <= 0)
        {
            freePath(destPath);
            return rc;
        }
    }
    else
    /* not root */
    {
        rc = makePathFromString(destPathStr, destPath);
        if(rc <= 0)
        {
            freePath(destPath);
            return rc;
        }
        
        rc = addDir(volInfo, &(volInfo->dirTree), srcPathAndName, destPath);
        if(rc <= 0)
        {
            freePath(destPath);
            return rc;
        }
    }
    
    freePath(destPath);
    
    return 1;
}

/*******************************************************************************
* bk_add_file()
* public interface for addFile()
* */
int bk_add_file(VolInfo* volInfo, const char* srcPathAndName, 
                const char* destPathStr)
{
    int rc;
    Path* destPath;
    
    destPath = malloc(sizeof(Path));
    if(destPath == NULL)
        return BKERROR_OUT_OF_MEMORY;
    
    destPath->numDirs = 0;
    destPath->dirs = NULL;
    
    if(destPathStr[0] == '/' && destPathStr[1] == '\0')
    /* root, special case */
    {
        rc = addFile(&(volInfo->dirTree), srcPathAndName, destPath);
        if(rc <= 0)
        {
            freePath(destPath);
            return rc;
        }
    }
    else
    {
        rc = makePathFromString(destPathStr, destPath);
        if(rc <= 0)
        {
            freePath(destPath);
            return rc;
        }
        
        rc = addFile(&(volInfo->dirTree), srcPathAndName, destPath);
        if(rc <= 0)
        {
            freePath(destPath);
            return rc;
        }
    }
    
    freePath(destPath);
    
    return 1;
}

/*******************************************************************************
* bk_create_dir()
* 
* */
int bk_create_dir(VolInfo* volInfo, const char* destPathStr, 
                  const char* newDirName)
{
    int nameLen;
    BkDir* destDir;
    int rc;
    BkFileBase* oldHead;
    BkDir* newDir;
    
    nameLen = strlen(newDirName);
    if(nameLen > NCHARS_FILE_ID_MAX_STORE - 1)
        return BKERROR_MAX_NAME_LENGTH_EXCEEDED;
    if(nameLen == 0)
        return BKERROR_NEW_DIR_ZERO_LEN_NAME;
    
    if( !nameIsValid(newDirName) )
        return BKERROR_NAME_INVALID_CHAR;
    
    rc = getDirFromString(&(volInfo->dirTree), destPathStr, &destDir);
    if(rc <= 0)
        return rc;
    
    if(itemIsInDir(newDirName, destDir))
        return BKERROR_DUPLICATE_CREATE_DIR;
    
    oldHead = destDir->children;
    
    destDir->children = malloc(sizeof(BkDir));
    if(destDir->children == NULL)
    {
        destDir->children = oldHead;
        return BKERROR_OUT_OF_MEMORY;
    }
    
    newDir = BK_DIR_PTR(destDir->children);
    
    BK_BASE_PTR(newDir)->next = oldHead;
    
    strcpy(BK_BASE_PTR(newDir)->name, newDirName);
    
    BK_BASE_PTR(newDir)->posixFileMode = volInfo->posixDirDefaults;
    
    destDir->children = NULL;
    
    return 1;
}

/*******************************************************************************
* itemIsInDir()
* checks the contents of a directory (files and dirs) to see whether it
* has an item named 
* */
bool itemIsInDir(const char* name, const BkDir* dir)
{
    BkFileBase* child;
    
    child = dir->children;
    while(child != NULL)
    {
        if(strcmp(child->name, name) == 0)
            return true;
        child = child->next;
    }
    
    return false;
}
