#include <string.h>

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
    
    volInfo->dirTree.posixFileMode = 0755;
}

/*******************************************************************************
* bk_destroy_vol_info()
* Frees any memory refered to by volinfo. 
* Does not reinitialize the structure.
* */
void bk_destroy_vol_info(VolInfo* volInfo)
{
    deleteDirContents(volInfo, &(volInfo->dirTree));
    
    if(volInfo->bootRecordPathAndName != NULL)
        free(volInfo->bootRecordPathAndName);
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
    Dir* srcDirInTree;
    DirLL* searchDir;
    FileLL* searchFile;
    bool found;
    
    rc = makeFilePathFromString(srcPathAndName, &filePath);
    if(rc <= 0)
    {
        freePathDirs(&(filePath.path));
        return rc;
    }
    
    /* FIND dir where the file is */
    srcDirInTree = &(volInfo->dirTree);
    for(count = 0; count < filePath.path.numDirs; count++)
    /* each directory in the path */
    {
        searchDir = srcDirInTree->directories;
        found = false;
        while(searchDir != NULL && !found)
        /* find the directory */
        {
            if(strcmp(searchDir->dir.name, filePath.path.dirs[count]) == 0)
            {
                found = true;
                srcDirInTree = &(searchDir->dir);
            }
            else
                searchDir = searchDir->next;
        }
        if(!found)
        {
            freePathDirs(&(filePath.path));
            return BKERROR_FILE_NOT_FOUND_ON_IMAGE;
        }
    }
    /* END FIND dir where the file is */
    
    /* FIND the file */
    found = false;
    searchFile = srcDirInTree->files;
    while(searchFile != NULL && !found)
    {
        if(strcmp(searchFile->file.name, filePath.filename) == 0)
        {
            found = true;
            
            volInfo->bootMediaType = BOOT_MEDIA_NO_EMULATION;
            
            volInfo->bootRecordSize = searchFile->file.size;
            
            if(volInfo->bootRecordPathAndName != NULL)
                free(volInfo->bootRecordPathAndName);
            volInfo->bootRecordPathAndName = NULL;
            
            volInfo->bootRecordIsVisible = true;
            
            volInfo->bootRecordOnImage = &(searchFile->file);
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
