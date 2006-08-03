#include <string.h>

#include "bk.h"

/* strings cannot be equal */
bool rightIsBigger(char* leftStr, char* rightStr)
{
    int leftLen;
    int rightLen;
    int count;
    bool resultFound;
    bool rc;
    
    leftLen = strlen(leftStr);
    rightLen = strlen(rightStr);
    
    resultFound = false;
    for(count = 0; count < leftLen && count < rightLen && !resultFound; count++)
    {
        if(rightStr[count] > leftStr[count])
        {
            resultFound = true;
            rc = true;
        }
        else if(rightStr[count] < leftStr[count])
        {
            resultFound = true;
            rc = false;
        }
    }
    
    if(!resultFound)
    /* strings are the same up to the length of the shorter one */
    {
        if(rightLen > leftLen)
            rc = true;
        else
            rc = false;
    }
    
    return rc;
}

void sortDir(DirToWrite* dir, int filenameType)
{
    DirToWriteLL* nextDir;
    
    DirToWriteLL* outerDir;
    DirToWriteLL* innerDir;
    DirToWrite tempDir;
    
    FileToWriteLL* outerFile;
    FileToWriteLL* innerFile;
    FileToWrite tempFile;
    
    nextDir = dir->directories;
    while(nextDir != NULL)
    {
        sortDir(&(nextDir->dir), filenameType);
        
        nextDir = nextDir->next;
    }
    
    outerDir = dir->directories;
    while(outerDir != NULL)
    {
        innerDir = outerDir->next;
        while(innerDir != NULL)
        {//printf("switch %s and %s? ", innerDir->dir.name9660, outerDir->dir.name9660);
            if( (filenameType & FNTYPE_JOLIET &&
                 rightIsBigger(innerDir->dir.nameJoliet, outerDir->dir.nameJoliet)) || 
                (filenameType & FNTYPE_9660 &&
                 rightIsBigger(innerDir->dir.name9660, outerDir->dir.name9660)) )
            {//printf("yes\n");
                tempDir = innerDir->dir;
                innerDir->dir = outerDir->dir;
                outerDir->dir = tempDir;
            }
            else
                //printf("no\n");
                innerDir = innerDir->next;
        }
        //printf("----\n");
        outerDir = outerDir->next;
    }
    
    outerFile = dir->files;
    while(outerFile != NULL)
    {
        innerFile = outerFile->next;
        while(innerFile != NULL)
        {
            if( (filenameType & FNTYPE_JOLIET &&
                 rightIsBigger(innerFile->file.nameJoliet, outerFile->file.nameJoliet)) || 
                (filenameType & FNTYPE_9660 &&
                 rightIsBigger(innerFile->file.name9660, outerFile->file.name9660)) )
            {
                tempFile = innerFile->file;
                innerFile->file = outerFile->file;
                outerFile->file = tempFile;
            }
            
            innerFile = innerFile->next;
        }
        
        outerFile = outerFile->next;
    }
}
