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

#include <string.h>
#include <sys/types.h>

#include "bk.h"
#include "bkInternal.h"
#include "bkSort.h"

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
    DirToWrite* nextDir;
    
    DirToWrite* outerDir;
    DirToWrite* innerDir;
    DirToWrite tempDir;
    
    FileToWrite* outerFile;
    FileToWrite* innerFile;
    FileToWrite tempFile;
    
    nextDir = dir->directories;
    while(nextDir != NULL)
    {
        sortDir(nextDir, filenameType);
        
        nextDir = nextDir->next;
    }
    
    outerDir = dir->directories;
    while(outerDir != NULL)
    {
        innerDir = outerDir->next;
        while(innerDir != NULL)
        {//printf("switch %s and %s? ", innerDir->dir.name9660, outerDir->dir.name9660);
            if( (filenameType & FNTYPE_JOLIET &&
                 rightIsBigger(innerDir->nameJoliet, outerDir->nameJoliet)) || 
                (filenameType & FNTYPE_9660 &&
                 rightIsBigger(innerDir->name9660, outerDir->name9660)) )
            {//printf("yes\n");
                tempDir = *innerDir;
                innerDir = outerDir;
                *outerDir = tempDir;
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
                 rightIsBigger(innerFile->nameJoliet, outerFile->nameJoliet)) || 
                (filenameType & FNTYPE_9660 &&
                 rightIsBigger(innerFile->name9660, outerFile->name9660)) )
            {
                tempFile = *innerFile;
                innerFile = outerFile;
                *outerFile = tempFile;
            }
            
            innerFile = innerFile->next;
        }
        
        outerFile = outerFile->next;
    }
}
