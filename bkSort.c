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
    BaseToWrite* child;
    
    BaseToWrite* outerChild;
    BaseToWrite* innerChild;
    DirToWrite tempDir;
    FileToWrite tempFile;
    
    child = dir->children;
    while(child != NULL)
    {
        if(IS_DIR(child->posixFileMode))
            sortDir(DIRTW_PTR(child), filenameType);
        
        child = child->next;
    }
    
    outerChild = dir->children;
    while(outerChild != NULL)
    {
        innerChild = outerChild->next;
        while(innerChild != NULL)
        {
            if( (filenameType & FNTYPE_JOLIET &&
                 rightIsBigger(innerChild->nameJoliet, outerChild->nameJoliet)) || 
                (filenameType & FNTYPE_9660 &&
                 rightIsBigger(innerChild->name9660, outerChild->name9660)) )
            {
                if(IS_DIR(innerChild->posixFileMode))
                {
                    tempDir = *DIRTW_PTR(innerChild);
                    innerChild = outerChild;
                    *DIRTW_PTR(outerChild) = tempDir;
                }
                else
                {
                    tempFile = *FILETW_PTR(innerChild);
                    innerChild = outerChild;
                    *FILETW_PTR(outerChild) = tempFile;
                }
            }
            else
                innerChild = innerChild->next;
        }
        
        outerChild = outerChild->next;
    }
}
