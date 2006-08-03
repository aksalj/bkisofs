#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "bk.h"
#include "bkError.h"

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
