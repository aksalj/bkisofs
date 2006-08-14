#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "bk.h"
#include "bkError.h"
#include "bkPath.h"

void freePath(Path* path)
{
    int count;
    
    for(count = 0; count < path->numDirs; count++)
    {
        free(path->dirs[count]);
    }
    free(path->dirs);   
    free(path);
}

int getFilenameFromPath(char* srcPathAndName, char* filename)
{
    int count;
    int srcLen;
    int indexLastSlash;
    bool found = false;
    int count2;
    
    srcLen = strlen(srcPathAndName);
    
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
int getLastDirFromString(char* srcPath, char* dirName)
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
    
    /* copy all characters in between the slashes found */
    for(count = 0; count < lastSlashIndex - prevSlashIndex - 1; count++)
    {
        dirName[count] = srcPath[prevSlashIndex + 1 + count];
    }
    dirName[count] = '\0';
    
    return 1;
}

int makeLongerPath(Path* origPath, char* newDir, Path** newPath)
{
    int count;
    
    *newPath  = malloc(sizeof(Path));
    if(*newPath == NULL)
        return BKERROR_OUT_OF_MEMORY;
    
    (*newPath)->numDirs = origPath->numDirs + 1;
    
    (*newPath)->dirs = malloc(sizeof(char*) * (*newPath)->numDirs);
    if((*newPath)->dirs == NULL)
        return BKERROR_OUT_OF_MEMORY;
    
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

int makeFilePathFromString(char* srcFile, FilePath* pathPath)
{
    int rc;
    int srcFileLen;
    /* index of the first character in the filename part of srcFile */
    int fileIndex;
    char origFilenameFirstChar;
    
    pathPath->path.numDirs = 0;
    pathPath->path.dirs = NULL;
    
    srcFileLen = strlen(srcFile);
    
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

int makePathFromString(char* strPath, Path* pathPath)
{
    int count;
    int pathStrLen;
    
    pathStrLen = strlen(strPath);
    
    if(pathStrLen < 3 || strPath[0] != '/' || strPath[1] == '/' || 
       strPath[pathStrLen - 1] != '/')
        return BKERROR_MISFORMED_PATH;
    
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
    char* nextDir = &(strPath[1]);
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
                    return BKERROR_MISFORMED_PATH;
                
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
