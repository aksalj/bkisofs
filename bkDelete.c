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
#include <string.h>
#include <sys/types.h>
#include <strings.h>

#include "bk.h"
#include "bkInternal.h"
#include "bkPath.h"
#include "bkError.h"
#include "bkDelete.h"

/*******************************************************************************
* bk_delete_boot_record()
* deletes whatever reference to a boot record volinfo has
* */
void bk_delete_boot_record(VolInfo* volInfo)
{
    volInfo->bootMediaType = BOOT_MEDIA_NONE;
    
    if(volInfo->bootRecordPathAndName != NULL)
    {
        free(volInfo->bootRecordPathAndName);
        volInfo->bootRecordPathAndName = NULL;
    }
}

/*******************************************************************************
* bk_delete_dir()
* deletes directory described by dirStr from the directory tree
* */
int bk_delete_dir(VolInfo* volInfo, const char* dirStr)
{
    int rc;
    Path* dirPath;
    
    if(dirStr[0] == '/' && dirStr[1] == '\0')
    /* root, not allowed */
        return BKERROR_DELETE_ROOT;
    
    dirPath = malloc(sizeof(Path));
    if(dirPath == NULL)
        return BKERROR_OUT_OF_MEMORY;
    
    bzero(dirPath, sizeof(Path));
    
    rc = makePathFromString(dirStr, dirPath);
    if(rc <= 0)
    {
        freePath(dirPath);
        return rc;
    }
    
    rc = deleteDir(volInfo, &(volInfo->dirTree), dirPath);
    if(rc <= 0)
    {
        freePath(dirPath);
        return rc;
    }
    
    freePath(dirPath);
    
    return 1;
}

/*******************************************************************************
* bk_delete_file()
* deletes file described by fileStr from the directory tree
* */
int bk_delete_file(VolInfo* volInfo, const char* fileStr)
{
    int rc;
    FilePath filePath;

    rc = makeFilePathFromString(fileStr, &filePath);
    if(rc <= 0)
    {
        freePathDirs(&(filePath.path));
        return rc;
    }
    
    rc = deleteFile(volInfo, &(volInfo->dirTree), &filePath);
    if(rc <= 0)
    {
        freePathDirs(&(filePath.path));
        return rc;
    }
    
    freePathDirs(&(filePath.path));
    
    return 1;
}

/*******************************************************************************
* deleteDir()
* deletes directory described by dirToDelete from the directory tree
* */
int deleteDir(VolInfo* volInfo, BkDir* tree, const Path* dirToDelete)
{
    int count;
    
    /* vars to find the dir in the tree */
    BkDir* srcDirInTree;
    BkFileBase* child;
    bool dirFound;
    
    /* vars to delete the directory */
    BkDir* parentDirInTree;
    bool parentDirFound;
    BkFileBase** parent;
    BkFileBase* parentNext;
    
    dirFound = findDirByPath(dirToDelete, tree, &srcDirInTree);
    if(!dirFound)
        return BKERROR_DIR_NOT_FOUND_ON_IMAGE;
    
    deleteDirContents(volInfo, srcDirInTree);
    
    /* GET A pointer to the parent dir */
    parentDirInTree = tree;
    for(count = 0; count < dirToDelete->numDirs - 1; count++)
    /* each directory in the path except the last one */
    {
        child = parentDirInTree->children;
        parentDirFound = false;
        while(child != NULL && !parentDirFound)
        /* find the directory, last one found will be the parent */
        {
            if(strcmp(child->name, dirToDelete->dirs[count]) == 0)
            {
                /* sanity check */
                if( !IS_DIR(child->posixFileMode) )
                    return BKERROR_NOT_DIR_IN_PATH;
                parentDirFound = true;
                parentDirInTree = BK_DIR_PTR(child);
            }
            else
                child = child->next;
        }
        if(!dirFound)
            return BKERROR_DIR_NOT_FOUND_ON_IMAGE;
    }
    /* END GET A pointer to the parent dir */
    
    /* DELETE self */
    parent = &(parentDirInTree->children);
    dirFound = false;
    while(*parent != NULL && !dirFound)
    {
        if(strcmp( (*parent)->name, 
                   dirToDelete->dirs[dirToDelete->numDirs - 1] ) == 0 )
        {
            parentNext = (*parent)->next;
            
            free(*parent);
            
            *parent = parentNext;
            
            dirFound = true;
        }
        else
            parent = &((*parent)->next);
    }
    if(!dirFound)
    /* should not happen since i already found this dir above */
        return BKERROR_SANITY;
    /* END DELETE self */
    
    return 1;
}

/*******************************************************************************
* deleteDirContents()
* deletes all the contents of a directory
* recursive
* */
void deleteDirContents(VolInfo* volInfo, BkDir* dir)
{
    BkFileBase* child;
    BkFileBase* nextChild;
    
    child = dir->children;
    while(child != NULL)
    {
        nextChild = child->next;
        
        if( IS_DIR(child->posixFileMode) )
        {
            deleteDirContents(volInfo, BK_DIR_PTR(child));
        }
        else if ( IS_REG_FILE(child->posixFileMode) )
        {
            BkFile* childFile = BK_FILE_PTR(child);
            
            if(!childFile->onImage)
                free(childFile->pathAndName);
            
            /* if file is being used as a boot record */
            if(volInfo->bootMediaType != BOOT_MEDIA_NONE &&
               volInfo->bootMediaType == BOOT_MEDIA_NO_EMULATION)
            {
                if(volInfo->bootRecordIsVisible && 
                   volInfo->bootRecordOnImage == childFile)
                {
                    /* stop using it. perhaps insert a hook here one day to
                    * let the user know the boot record has been deleted */
                    bk_delete_boot_record(volInfo);
                }
            }
        }
        
        free(child);
        
        child = nextChild;
    }
}

/*******************************************************************************
* deleteFile()
* deletes file described by pathAndName from the tree
* */
int deleteFile(VolInfo* volInfo, BkDir* tree, const FilePath* pathAndName)
{
    BkDir* parentDir;
    bool dirFound;
    BkFileBase** pointerToIt; /* pointer to pointer to the file to delete */
    BkFileBase* pointerToNext; /* to assign to the pointer pointed to by the pointer above
                           * no i'm not kidding */
    bool fileFound;
    
    dirFound = findDirByPath(&(pathAndName->path), tree, &parentDir);
    if(!dirFound)
        return BKERROR_DIR_NOT_FOUND_ON_IMAGE;
    
    pointerToIt = &(parentDir->children);
    fileFound = false;
    while(*pointerToIt != NULL && !fileFound)
    {
        if(strcmp((*pointerToIt)->name, pathAndName->filename) == 0)
        /* delete the node */
        {
            BkFile* fileToDelete = BK_FILE_PTR(*pointerToIt);
            
            pointerToNext = (*pointerToIt)->next;
            
            if( fileToDelete->onImage )
                free( fileToDelete->pathAndName );
            
            /* check whether file is being used as a boot record */
            if(volInfo->bootMediaType != BOOT_MEDIA_NONE &&
               volInfo->bootMediaType == BOOT_MEDIA_NO_EMULATION)
            {
                if(volInfo->bootRecordIsVisible && 
                   volInfo->bootRecordOnImage == fileToDelete)
                {
                    /* and stop using it. perhaps insert a hook here one day to
                    * let the user know the boot record has been/will be deleted */
                    bk_delete_boot_record(volInfo);
                }
            }
            
            free(*pointerToIt);
            
            *pointerToIt = pointerToNext;
            
            fileFound = true;
        }
        else
        {
            pointerToIt = &((*pointerToIt)->next);
        }
    }
    if(!fileFound)
        return BKERROR_FILE_NOT_FOUND_ON_IMAGE;
    
    return true;
}
