#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "bk.h"
#include "bkPath.h"
#include "bkError.h"
#include "bkDelete.h"

int bk_delete_dir(Dir* tree, const char* dirStr)
{
    int rc;
    Path* dirPath;
    
    if(dirStr[0] == '/' && dirStr[1] == '\0')
    /* root, not allowed */
        return BKERROR_DELETE_ROOT;
    
    dirPath = malloc(sizeof(Path));
    if(dirPath == NULL)
        return BKERROR_OUT_OF_MEMORY;
    
    rc = makePathFromString(dirStr, dirPath);
    if(rc <= 0)
    {
        freePath(dirPath);
        return rc;
    }
    
    rc = deleteDir(tree, dirPath);
    if(rc <= 0)
    {
        freePath(dirPath);
        return rc;
    }
    
    freePath(dirPath);
    
    return 1;
}

int bk_delete_file(Dir* tree, const char* fileStr)
{
    int rc;
    FilePath filePath;

    rc = makeFilePathFromString(fileStr, &filePath);
    if(rc <= 0)
    {
        freePathDirs(&(filePath.path));
        return rc;
    }
    
    rc = deleteFile(tree, &filePath);
    if(rc <= 0)
    {
        freePathDirs(&(filePath.path));
        return rc;
    }
    
    freePathDirs(&(filePath.path));
    
    return 1;
}

void bk_delete_dir_contents(Dir* dir)
{
    /* vars to delete files */
    FileLL* currentFile;
    FileLL* nextFile;
    
    /* vars to delete subdirectories */
    DirLL* currentDir;
    DirLL* nextDir;
    
    /* DELETE all files */
    currentFile = dir->files;
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
    currentDir = dir->directories;
    while(currentDir != NULL)
    {
        nextDir = currentDir->next;
        
        bk_delete_dir_contents(&(currentDir->dir));
        
        free(currentDir);
        
        currentDir = nextDir;
    }
    /* END DELETE all directories */
}

int deleteDir(Dir* tree, const Path* dirToDelete)
{
    int count;
    
    /* vars to find the dir in the tree */
    Dir* srcDirInTree;
    DirLL* searchDir;
    bool dirFound;
    
    /* vars to delete the directory */
    Dir* parentDirInTree;
    bool parentDirFound;
    DirLL** parentDirLL;
    DirLL* parentDirNextLL;
    
    /* FIND dir to know what the contents are */
    srcDirInTree = tree;
    for(count = 0; count < dirToDelete->numDirs; count++)
    /* each directory in the path */
    {
        searchDir = srcDirInTree->directories;
        dirFound = false;
        while(searchDir != NULL && !dirFound)
        /* find the directory */
        {
            if(strcmp(searchDir->dir.name, dirToDelete->dirs[count]) == 0)
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
    
    bk_delete_dir_contents(srcDirInTree);
    
    /* GET A pointer to the parent dir */
    parentDirInTree = tree;
    for(count = 0; count < dirToDelete->numDirs - 1; count++)
    /* each directory in the path except the last one */
    {
        searchDir = parentDirInTree->directories;
        parentDirFound = false;
        while(searchDir != NULL && !parentDirFound)
        /* find the directory, last one found will be the parent */
        {
            if(strcmp(searchDir->dir.name, dirToDelete->dirs[count]) == 0)
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
        if(strcmp( (*parentDirLL)->dir.name, 
                   dirToDelete->dirs[dirToDelete->numDirs - 1] ) == 0 )
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
        return BKERROR_SANITY;
    /* END DELETE self */
    
    return 1;
}

int deleteFile(Dir* tree, const FilePath* pathAndName)
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
