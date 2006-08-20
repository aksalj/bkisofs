#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "bk.h"
#include "bkError.h"
#include "bkPath.h"

void freePath(Path* path)
{
    freePathDirs(path);
    
    free(path);
}

void freePathDirs(Path* path)
{
    int count;
    
    for(count = 0; count < path->numDirs; count++)
    {
        /* if the path was not allocated properly (maybe ran out of memory)
        * the first unallocated item is null */
        if(path->dirs[count] == NULL)
            break;

        free(path->dirs[count]);
    }
    
    if(path->dirs != NULL)
        free(path->dirs);
}

int getFilenameFromPath(const char* srcPathAndName, char* filename)
{
    int count;
    int srcLen;
    int indexLastSlash;
    bool found = false;
    int count2;
    
    srcLen = strlen(srcPathAndName);
    
    /* find index of the last slash in path */
    for(count = 0; count < srcLen; count++)
    {
        if(srcPathAndName[count] == '/')
        {
            indexLastSlash = count;
            found = true;
        }
    }
    if(!found)
        return BKERROR_MISFORMED_PATH;
    
    if(indexLastSlash == srcLen - 1)
    /* string ended with '/' */
        return BKERROR_MISFORMED_PATH;
    
    if(srcLen - indexLastSlash - 1 > NCHARS_FILE_ID_MAX - 1)
        return BKERROR_MAX_NAME_LENGTH_EXCEEDED;
    
    /* this loop copies null byte also */
    for(count = indexLastSlash + 1, count2 = 0; count <= srcLen; count++, count2++)
    {
        filename[count2] = srcPathAndName[count];
    }
    
    return 1;
}

/*
* srcPath must have trailing slash
* dirName will not have a slash
*/
int getLastDirFromString(const char* srcPath, char* dirName)
{
    int prevSlashIndex = 0;
    int lastSlashIndex = 0;
    int count;
    int srcPathLen;
    
    srcPathLen = strlen(srcPath);
    
    if(srcPath[srcPathLen - 1] != '/')
    /* does not end with trailing slash, error */
        return BKERROR_MISFORMED_PATH;
    
    /* find indeces of last and just before last slashes */
    for(count = 0; count < srcPathLen; count++)
    {
        if(srcPath[count] == '/')
        {
            prevSlashIndex = lastSlashIndex;
            lastSlashIndex = count;
        }
    }
    
    if(lastSlashIndex - prevSlashIndex - 1 > NCHARS_FILE_ID_MAX - 1)
        return BKERROR_MAX_NAME_LENGTH_EXCEEDED;
    
    /* copy all characters in between the slashes found */
    for(count = 0; count < lastSlashIndex - prevSlashIndex - 1; count++)
    {
        dirName[count] = srcPath[prevSlashIndex + 1 + count];
    }
    dirName[count] = '\0';
    
    return 1;
}

int makeFilePathFromString(const char* srcFileIn, FilePath* pathPath)
{
    int rc;
    int srcFileLen;
    /* because i need to modify it and the parameter needs to be const */
    char* srcFile; 
    /* index of the first character in the filename part of srcFile */
    int fileIndex;
    char origFilenameFirstChar;
    
    pathPath->path.numDirs = 0;
    pathPath->path.dirs = NULL;
    
    srcFileLen = strlen(srcFileIn);
    
    srcFile = malloc(srcFileLen + 1);
    if(srcFile == NULL)
        return BKERROR_OUT_OF_MEMORY;
    
    strcpy(srcFile, srcFileIn);
    
    if(srcFile[0] != '/' || srcFile[srcFileLen] == '/')
        return BKERROR_MISFORMED_PATH;
    
    /* replace the first character of the filename with a null byte so
    * that i can pass the string to makePathFromString() */
    origFilenameFirstChar = 0x01; /* not a character */
    for(fileIndex = srcFileLen; fileIndex > 0; fileIndex--)
    {
        if(srcFile[fileIndex - 1] == '/')
        {
            origFilenameFirstChar = srcFile[fileIndex];
            srcFile[fileIndex] = '\0';
            
            break;
        }
    }
    
    if(origFilenameFirstChar == 0x01)
    /* no filename found */
        return BKERROR_MISFORMED_PATH;
    
    if(srcFileLen - origFilenameFirstChar > NCHARS_FILE_ID_MAX - 1)
        return BKERROR_MAX_NAME_LENGTH_EXCEEDED;
    
    if(fileIndex != 1)
    /* file not in root */
    {
        rc = makePathFromString(srcFile, &(pathPath->path));
        if(rc <= 0)
            return rc;
    }
    
    srcFile[fileIndex] = origFilenameFirstChar;
    strcpy(pathPath->filename, &(srcFile[fileIndex]));
    
    return 1;
}

int makeLongerPath(const Path* origPath, const char* newDir, Path** newPath)
{
    int count;
    
    *newPath = malloc(sizeof(Path));
    if(*newPath == NULL)
        return BKERROR_OUT_OF_MEMORY;
    
    (*newPath)->numDirs = origPath->numDirs + 1;
    
    (*newPath)->dirs = malloc(sizeof(char*) * (*newPath)->numDirs);
    if((*newPath)->dirs == NULL)
    {
        /* set to 0 to make sure freePath() still works */
        (*newPath)->numDirs = 0;
        return BKERROR_OUT_OF_MEMORY;
    }
    
    /* copy original */
    for(count = 0; count < origPath->numDirs; count++)
    {
        (*newPath)->dirs[count] = malloc(strlen((origPath->dirs)[count]) + 1);
        if((*newPath)->dirs[count] == NULL)
            return BKERROR_OUT_OF_MEMORY;
        strcpy((*newPath)->dirs[count], (origPath->dirs)[count]);
    }
    
    /* new dir */
    (*newPath)->dirs[count] = malloc(strlen(newDir) + 1);
    if((*newPath)->dirs[count] == NULL)
        return BKERROR_OUT_OF_MEMORY;
    strcpy((*newPath)->dirs[count], newDir);
    
    return 1;
}

int makePathFromString(const char* strPath, Path* pathPath)
{
    int count;
    int pathStrLen;
    
    pathStrLen = strlen(strPath);
    
    if(pathStrLen < 3 || strPath[0] != '/' || strPath[1] == '/' || 
       strPath[pathStrLen - 1] != '/')
    {
        printf("misformed 3 %d '%c' '%c' '%c'\n", pathStrLen, strPath[0], strPath[1], strPath[pathStrLen - 1]);
        return BKERROR_MISFORMED_PATH;
    }
    
    pathPath->numDirs = 0;
    for(count = 0; count < pathStrLen; count++)
    {
        if(strPath[count] == '/')
            pathPath->numDirs++;
    }
    /* substract one slash (checking done above insures there's one extra) */
    pathPath->numDirs--;
    
    pathPath->dirs = (char**)malloc(sizeof(char*) * pathPath->numDirs);
    if(pathPath->dirs == NULL)
        return BKERROR_OUT_OF_MEMORY;
    
    bool justStarted = true;
    int numDirsDone = 0;
    int nextDirLen = 0;
    const char* nextDir = &(strPath[1]);
    for(count = 0; count < pathStrLen; count++)
    {
        if(strPath[count] == '/')
        {
            if(justStarted)
                justStarted = false;
                /* do nothing else */
            else
            /* this is the slash following a dir name */
            {
                if(nextDirLen == 0)
                /* double slash */
                {
                    /* first item that failed to allocate must be set to NULL
                    * so that freePath() still works */
                    pathPath->dirs[numDirsDone] = NULL;
                    return BKERROR_MISFORMED_PATH;
                }
                
                pathPath->dirs[numDirsDone] = (char*)malloc(nextDirLen);
                if(pathPath->dirs[numDirsDone] == NULL)
                    return BKERROR_OUT_OF_MEMORY;
                
                strncpy(pathPath->dirs[numDirsDone], nextDir, nextDirLen);
                pathPath->dirs[numDirsDone][nextDirLen] = '\0';
                
                numDirsDone++;
                nextDirLen = 0;
                nextDir = &(strPath[count + 1]);
            }
        }
        else
        {
            nextDirLen++;
        }
    }
    
    if(numDirsDone != pathPath->numDirs)
        return BKERROR_SANITY;
    
    return 1;
}
