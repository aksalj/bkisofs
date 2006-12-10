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
#include <unistd.h>

#include "bk.h"
#include "bkDelete.h"
#include "bkPath.h"
#include "bkError.h"

/*******************************************************************************
* bk_init_vol_info()
*
* */
void bk_init_vol_info(VolInfo* volInfo)
{
    bzero(volInfo, sizeof(VolInfo));
    
    volInfo->dirTree.posixFileMode = 040755;
    volInfo->posixFileDefaults = 0100644;
    volInfo->posixDirDefaults = 040755;
}

/*******************************************************************************
* bk_destroy_vol_info()
* Frees any memory refered to by volinfo.
* If an image was open for reading closes it.
* Does not reinitialize the structure.
* */
void bk_destroy_vol_info(VolInfo* volInfo)
{
    deleteDirContents(volInfo, &(volInfo->dirTree));
    
    if(volInfo->bootRecordPathAndName != NULL)
        free(volInfo->bootRecordPathAndName);
    
    if(volInfo->imageForReading > 0)
        close(volInfo->imageForReading);
}

/*******************************************************************************
* bk_set_boot_file()
* Set a file on the image to be the no-emulation boot record for el torito.
* */
int bk_set_boot_file(VolInfo* volInfo, const char* srcPathAndName)
{
    int rc;
    FilePath filePath;
    int count;
    BkDir* srcDirInTree;
    BkDir* searchDir;
    BkFile* searchFile;
    bool found;
    
    rc = makeFilePathFromString(srcPathAndName, &filePath);
    if(rc <= 0)
    {
        freePathDirs(&(filePath.path));
        return rc;
    }
    
    found = findDirByPath(&(filePath.path), &(volInfo->dirTree), &srcDirInTree);
    if(!found)
        return BKERROR_DIR_NOT_FOUND_ON_IMAGE;
    
    /* FIND the file */
    found = false;
    searchFile = srcDirInTree->files;
    while(searchFile != NULL && !found)
    {
        if(strcmp(searchFile->name, filePath.filename) == 0)
        {
            found = true;
            
            volInfo->bootMediaType = BOOT_MEDIA_NO_EMULATION;
            
            volInfo->bootRecordSize = searchFile->size;
            
            if(volInfo->bootRecordPathAndName != NULL)
                free(volInfo->bootRecordPathAndName);
            volInfo->bootRecordPathAndName = NULL;
            
            volInfo->bootRecordIsVisible = true;
            
            volInfo->bootRecordOnImage = searchFile;
        }
        
        searchFile = searchFile->next;
    }
    if(!found)
    {
        freePathDirs(&(filePath.path));
        return BKERROR_FILE_NOT_FOUND_ON_IMAGE;
    }
    /* END FIND the file */
    
    freePathDirs(&(filePath.path));
    
    return 1;
}

/*******************************************************************************
* bk_set_publisher()
* Copies publisher into volInfo, a maximum of 128 characters.
* */
void bk_set_publisher(VolInfo* volInfo, const char* publisher)
{
    strncpy(volInfo->publisher, publisher, 128);
}

/*******************************************************************************
* bk_set_vol_name()
* Copies volName into volInfo, a maximum of 32 characters.
* */
void bk_set_vol_name(VolInfo* volInfo, const char* volName)
{
    strncpy(volInfo->volId, volName, 32);
}
