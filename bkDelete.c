#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "bk.h"
#include "bkPath.h"
#include "bkError.h"
#include "bkDelete.h"

int bk_delete_dir(Dir* tree, char* dirStr)
{
    int rc;
    Path dirPath;
    
    if(dirStr[0] == '/' && dirStr[1] == '\0')
    /* root, not allowed */
        return BKERROR_DELETE_ROOT;
    
    rc = makePathFromString(dirStr, &dirPath);
    if(rc <= 0)
        return rc;
    
    return deleteDir(tree, &dirPath);
}

int bk_delete_file(Dir* tree, char* fileStr)
{
    int rc;
    FilePath filePath;
    
    rc = makeFilePathFromString(fileStr, &filePath);
    if(rc <= 0)
        return rc;
    
    return deleteFile(tree, &filePath);
}

int deleteDir(Dir* tree, Path* srcDir)
{
    int count;
    int rc;
    
    /* vars to find the dir in the tree */
    Dir* srcDirInTree;
    DirLL* searchDir;
    bool dirFound;
    
    /* vars to delete files */
    FileLL* currentFile;
    FileLL* nextFile;
    
    /* vars to delete subdirectories */
    DirLL* currentDir;
    Path* newSrcDir;
    
    /* vars to delete the directory */
    Dir* parentDirInTree;
    bool parentDirFound;
    DirLL** parentDirLL;
    DirLL* parentDirNextLL;
    
    /* FIND dir to know what the contents are */
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
    /* END FIND dir to know what the contents are */
    
    /* DELETE all files */
    currentFile = srcDirInTree->files;
    while(currentFile != NULL)
    {
        nextFile = currentFile->next;
        
        if(!currentFile->file.onImage)
            free(currentFile->file.pathAndName);
        
        free(currentFile);
        
        currentFile = nextFile;
    }
    /* END DELETE all files */
    
    /* DELETE all directories */
    currentDir = srcDirInTree->directories;
    while(currentDir != NULL)
    {
        rc = makeLongerPath(srcDir, currentDir->dir.name, &newSrcDir);
        if(rc <= 0)
            return rc;
        
        rc = deleteDir(tree, newSrcDir);
        if(rc <= 0)
            return rc;
        
        freePath(newSrcDir);
        
        currentDir = currentDir->next;
    }
    /* END DELETE all directories */
    
    /* GET A pointer to the parent dir */
    parentDirInTree = tree;
    for(count = 0; count < srcDir->numDirs - 1; count++)
    /* each directory in the path except the last one */
    {
        searchDir = parentDirInTree->directories;
        parentDirFound = false;
        while(searchDir != NULL && !parentDirFound)
        /* find the directory, last one found will be the parent */
        {
            if(strcmp(searchDir->dir.name, srcDir->dirs[count]) == 0)
            {
                parentDirFound = true;
                parentDirInTree = &(searchDir->dir);
            }
            else
                searchDir = searchDir->next;
        }
        if(!dirFound)
            return BKERROR_DIR_NOT_FOUND_ON_IMAGE;
    }
    /* END GET A pointer to the parent dir */
    
    /* DELETE self */
    parentDirLL = &(parentDirInTree->directories);
    dirFound = false;
    while(*parentDirLL != NULL && !dirFound)
    {
        if(strcmp( (*parentDirLL)->dir.name, srcDir->dirs[srcDir->numDirs - 1] ) == 0 )
        {
            parentDirNextLL = (*parentDirLL)->next;
            
            free(*parentDirLL);
            
            *parentDirLL = parentDirNextLL;
            
            dirFound = true;
        }
        else
            parentDirLL = &((*parentDirLL)->next);
    }
    if(!dirFound)
    /* should not happen since i already found this dir above */
        return BKERROR_DIR_NOT_FOUND_ON_IMAGE;
    /* END DELETE self */
    
    return 1;
}

int deleteFile(Dir* tree, FilePath* pathAndName)
{
    Dir* parentDir;
    DirLL* searchDir;
    bool dirFound;
    FileLL** pointerToIt; /* pointer to pointer to the file to delete */
    FileLL* pointerToNext; /* to assign to the pointer pointed to by the pointer above
                           * no i'm not kidding */
    bool fileFound;
    int count;
    
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
    
    pointerToIt = &(parentDir->files);
    fileFound = false;
    while(*pointerToIt != NULL && !fileFound)
    {
        if(strcmp((*pointerToIt)->file.name, pathAndName->filename) == 0)
        /* delete the node */
        {
            pointerToNext = (*pointerToIt)->next;
            
            if( (*pointerToIt)->file.onImage )
                free( (*pointerToIt)->file.pathAndName );
            
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
