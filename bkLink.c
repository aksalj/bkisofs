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
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <stdio.h>

#include "bkInternal.h"

int addToHardLinkTable(VolInfo* volInfo, off_t position, ino_t inode)
{
    BkHardLink* newLink;
    
    newLink = malloc(sizeof(BkHardLink));
    if(newLink == NULL)
        return BKERROR_OUT_OF_MEMORY;
    
    bzero(newLink, sizeof(BkHardLink));
    
    newLink->onImage = true;
    newLink->position = position;
    newLink->inode = inode;
    newLink->next = volInfo->fileLocations;
    volInfo->fileLocations = newLink;
    
    return 1;
}

BkHardLink* findInHardLinkTable(VolInfo* volInfo, off_t position, ino_t inode)
{
    BkHardLink* currentLink;
    
    currentLink = volInfo->fileLocations;
    while(currentLink != NULL)
    {
        if(position != 0)
        {
            if(position == currentLink->position)
                return currentLink;
        }
        else
        {
            if(inode == currentLink->inode)
                return currentLink;
        }
        
        currentLink = currentLink->next;
    }
    
    return NULL;
}
