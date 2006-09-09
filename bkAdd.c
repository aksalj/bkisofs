/******************************* LICENCE **************************************
* Any code in this file may be redistributed or modified under the terms of
* the GNU General Public Licence as published by the Free Software 
* Foundation; version 2 of the licence.
****************************** END LICENCE ***********************************/

#include <stdbool.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>

#include "bk.h"
#include "bkPath.h"
#include "bkAdd.h"
#include "bkError.h"

/*******************************************************************************
* addDir()
* adds a directory from the filesystem to the image
*
* Receives:
* - Dir*, root of tree to add to
* - char*, path of directory to add, must end with trailing slash
* - Path*, destination on image
* */
int addDir(Dir* tree, const char* srcPath, const Path* destDir)
{
    int count;
    int rc;
    
    /* vars to add dir to tree */
    char srcDirName[NCHARS_FILE_ID_MAX];
    Dir* destDirInTree;
    DirLL* searchDir;
    bool dirFound;
    DirLL* oldHead; /* old head of the directories list */
    struct stat statStruct; /* to get info on the dir */
    
    /* vars to read contents of a dir on fs */
    DIR* srcDir;
    struct dirent* dirEnt;
    struct stat anEntry;
    
    /* vars for children */
    Path* newDestDir;
    int newSrcPathLen; /* length of new path (including trailing '/' but not filename) */
    char* newSrcPathAndName; /* both for child dirs and child files */
    
    if(srcPath[strlen(srcPath) - 1] != '/')
    /* must have trailing slash */
        return BKERROR_DIRNAME_NEED_TRAILING_SLASH;
    
    /* FIND dir to add to */
    destDirInTree = tree;
    for(count = 0; count < destDir->numDirs; count++)
    /* each directory in the path */
    {
        searchDir = destDirInTree->directories;
        dirFound = false;
        while(searchDir != NULL && !dirFound)
        /* find the directory */
        {
            if(strcmp(searchDir->dir.name, destDir->dirs[count]) == 0)
            {
                dirFound = true;
                destDirInTree = &(searchDir->dir);
            }
            else
                searchDir = searchDir->next;
        }
        if(!dirFound)
            return BKERROR_DIR_NOT_FOUND_ON_IMAGE;
    }
    /* END FIND dir to add to */
    
    /* get the name of the directory to be added */
    rc = getLastDirFromString(srcPath, srcDirName);
    if(rc <= 0)
        return rc;
    
    if(itemIsInDir(srcDirName, destDirInTree))
        return BKERROR_DUPLICATE_ADD;
    
    oldHead = destDirInTree->directories;
    
    /* ADD directory to tree */
    rc = stat(srcPath, &statStruct);
    if(rc == -1)
        return BKERROR_STAT_FAILED;
    
    if( !(statStruct.st_mode & S_IFDIR) )
        return BKERROR_TARGET_NOT_A_DIR;
    
    destDirInTree->directories = malloc(sizeof(DirLL));
    if(destDirInTree->directories == NULL)
    {
        destDirInTree->directories = oldHead;
        return BKERROR_OUT_OF_MEMORY;
    }
    
    destDirInTree->directories->next = oldHead;
    
    strcpy(destDirInTree->directories->dir.name, srcDirName);
    
    destDirInTree->directories->dir.posixFileMode = statStruct.st_mode;
    
    destDirInTree->directories->dir.directories = NULL;
    destDirInTree->directories->dir.files = NULL;
    /* END ADD directory to tree */
    
    /* remember length of original */
    newSrcPathLen = strlen(srcPath);
    
    /* including the file/dir name and the trailing '/' and the '\0' */
    newSrcPathAndName = malloc(newSrcPathLen + NCHARS_FILE_ID_MAX + 1);
    if(newSrcPathAndName == NULL)
    {
        free(destDirInTree->directories);
        destDirInTree->directories = oldHead;
        return BKERROR_OUT_OF_MEMORY;
    }
    
    strcpy(newSrcPathAndName, srcPath);
    
    /* destination for children */
    rc = makeLongerPath(destDir, srcDirName, &newDestDir);
    if(rc <= 0)
    {
        free(destDirInTree->directories);
        destDirInTree->directories = oldHead;
        free(newSrcPathAndName);
        freePath(newDestDir);
        return rc;
    }
    
    /* ADD contents of directory */
    srcDir = opendir(srcPath);
    if(srcDir == NULL)
    {
        free(destDirInTree->directories);
        destDirInTree->directories = oldHead;
        free(newSrcPathAndName);
        freePath(newDestDir);
        return BKERROR_OPENDIR_FAILED;
    }
    
    /* it may be possible but in any case very unlikely that readdir() will fail
    * if it does, it returns NULL (same as end of dir) */
    while( (dirEnt = readdir(srcDir)) != NULL )
    {
        if( strcmp(dirEnt->d_name, ".") != 0 && strcmp(dirEnt->d_name, "..") != 0 )
        /* not "." or ".." (safely ignore those two) */
        {
            if(strlen(dirEnt->d_name) > NCHARS_FILE_ID_MAX - 1)
            {
                free(destDirInTree->directories);
                destDirInTree->directories = oldHead;
                free(newSrcPathAndName);
                freePath(newDestDir);
                return BKERROR_MAX_NAME_LENGTH_EXCEEDED;
            }
                
            /* append file/dir name */
            strcpy(newSrcPathAndName + newSrcPathLen, dirEnt->d_name);
            
            rc = stat(newSrcPathAndName, &anEntry);
            if(rc == -1)
            {
                free(destDirInTree->directories);
                destDirInTree->directories = oldHead;
                free(newSrcPathAndName);
                freePath(newDestDir);
                return BKERROR_STAT_FAILED;
            }
            
            if(anEntry.st_mode & S_IFDIR)
            /* directory */
            {
                strcat(newSrcPathAndName, "/");
                
                rc = addDir(tree, newSrcPathAndName, newDestDir);
                if(rc <= 0)
                {
                    free(destDirInTree->directories);
                    destDirInTree->directories = oldHead;
                    free(newSrcPathAndName);
                    freePath(newDestDir);
                    return rc;
                }
            }
            else if(anEntry.st_mode & S_IFREG)
            /* regular file */
            {
                rc = addFile(tree, newSrcPathAndName, newDestDir);
                if(rc <= 0)
                {
                    free(destDirInTree->directories);
                    destDirInTree->directories = oldHead;
                    free(newSrcPathAndName);
                    freePath(newDestDir);
                    return rc;
                }
            }
            else
            /* not regular file or directory */
            {
                free(destDirInTree->directories);
                destDirInTree->directories = oldHead;
                free(newSrcPathAndName);
                freePath(newDestDir);
                return BKERROR_NO_SPECIAL_FILES;
            }
            
        } /* if */
        
    } /* while */
    
    rc = closedir(srcDir);
    if(rc != 0)
    /* exotic error */
    {
        free(destDirInTree->directories);
        destDirInTree->directories = oldHead;
        free(newSrcPathAndName);
        freePath(newDestDir);
        return BKERROR_EXOTIC;
    }
    /* END ADD contents of directory */
    
    free(newSrcPathAndName);
    freePath(newDestDir);
    
    return 1;
}

/*******************************************************************************
* addFile()
* adds a file from the filesystem to the image
*
* Receives:
* - Dir*, root of tree to add to
* - char*, path and name of file to add, must end with trailing slash
* - Path*, destination on image
* Notes:
*  will only add a regular file (symblic links are followed, see stat(2))
* */
int addFile(Dir* tree, const char* srcPathAndName, const Path* destDir)
{
    int count;
    int rc;
    FileLL* oldHead; /* of the files list */
    char filename[NCHARS_FILE_ID_MAX];
    struct stat statStruct;
    
    /* vars to find the dir in the tree */
    Dir* destDirInTree;
    DirLL* searchDir;
    bool dirFound;
    
    rc = getFilenameFromPath(srcPathAndName, filename);
    if(rc <= 0)
        return rc;
    
    /* FIND dir to add to */
    destDirInTree = tree;
    for(count = 0; count < destDir->numDirs; count++)
    /* each directory in the path */
    {
        searchDir = destDirInTree->directories;
        dirFound = false;
        while(searchDir != NULL && !dirFound)
        /* find the directory */
        {
            if(strcmp(searchDir->dir.name, destDir->dirs[count]) == 0)
            {
                dirFound = true;
                destDirInTree = &(searchDir->dir);
            }
            else
                searchDir = searchDir->next;
        }
        if(!dirFound)
            return BKERROR_DIR_NOT_FOUND_ON_IMAGE;
    }
    /* END FIND dir to add to */
    
    if(itemIsInDir(filename, destDirInTree))
        return BKERROR_DUPLICATE_ADD;
    
    oldHead = destDirInTree->files;
    
    /* ADD file */
    destDirInTree->files = malloc(sizeof(FileLL));
    if(destDirInTree->files == NULL)
    {
        destDirInTree->files = oldHead;
        return BKERROR_OUT_OF_MEMORY;
    }
    
    destDirInTree->files->next = oldHead;
    
    strcpy(destDirInTree->files->file.name, filename);
    
    rc = stat(srcPathAndName, &statStruct);
    if(rc == -1)
    {
        free(destDirInTree->files);
        destDirInTree->files = oldHead;
        return BKERROR_STAT_FAILED;
    }
    
    if( !(statStruct.st_mode & S_IFREG) )
    /* not a regular file */
    {
        free(destDirInTree->files);
        destDirInTree->files = oldHead;
        return BKERROR_NO_SPECIAL_FILES;
    }
    
    destDirInTree->files->file.posixFileMode = statStruct.st_mode;
    
    destDirInTree->files->file.size = statStruct.st_size;
    
    destDirInTree->files->file.onImage = false;
    
    destDirInTree->files->file.position = 0;
    
    destDirInTree->files->file.pathAndName = malloc(strlen(srcPathAndName) + 1);
    strcpy(destDirInTree->files->file.pathAndName, srcPathAndName);
    /* END ADD file */
    
    return 1;
}

/*******************************************************************************
* bk_add_boot_record()
* Source boot file must be exactly the right size if floppy emulation requested.
* */
int bk_add_boot_record(VolInfo* volInfo, const char* srcPathAndName, 
                       int bootMediaType)
{
    struct stat statStruct;
    int rc;
    
    if(bootMediaType != BOOT_MEDIA_NO_EMULATION &&
       bootMediaType != BOOT_MEDIA_1_2_FLOPPY &&
       bootMediaType != BOOT_MEDIA_1_44_FLOPPY &&
       bootMediaType != BOOT_MEDIA_2_88_FLOPPY)
    {
        return BKERROR_ADD_UNKNOWN_BOOT_MEDIA;
    }
    
    rc = stat(srcPathAndName, &statStruct);
    if(rc == -1)
        return BKERROR_STAT_FAILED;
    
    if( (bootMediaType == BOOT_MEDIA_1_2_FLOPPY &&
         statStruct.st_size != 1228800) ||
        (bootMediaType == BOOT_MEDIA_1_44_FLOPPY &&
         statStruct.st_size != 1474560) ||
        (bootMediaType == BOOT_MEDIA_2_88_FLOPPY &&
         statStruct.st_size != 2949120) )
    {
        return BKERROR_ADD_BOOT_RECORD_WRONG_SIZE;
    }
    
    volInfo->bootMediaType = bootMediaType;
    
    volInfo->bootRecordSize = statStruct.st_size;
    
    volInfo->bootRecordIsOnImage = false;
    
    /* make copy of the source path and name */
    if(volInfo->bootRecordPathAndName != NULL)
        free(volInfo->bootRecordPathAndName);
    volInfo->bootRecordPathAndName = malloc(strlen(srcPathAndName) + 1);
    if(volInfo->bootRecordPathAndName == NULL)
    {
        volInfo->bootMediaType = BOOT_MEDIA_NONE;
        return BKERROR_OUT_OF_MEMORY;
    }
    strcpy(volInfo->bootRecordPathAndName, srcPathAndName);
    
    /* this is the wrong function to use if you want a visible one */
    volInfo->bootRecordIsVisible = false;
    
    return 1;
}

/*******************************************************************************
* bk_add_dir()
* public interface for addDir()
* */
int bk_add_dir(VolInfo* volInfo, const char* srcPathAndName, 
               const char* destPathStr)
{
    int rc;
    Path* destPath;
    
    destPath = malloc(sizeof(Path));
    if(destPath == NULL)
        return BKERROR_OUT_OF_MEMORY;
    
    destPath->numDirs = 0;
    destPath->dirs = NULL;
    
    if(destPathStr[0] == '/' && destPathStr[1] == '\0')
    /* root, special case */
    {
        rc = addDir(&(volInfo->dirTree), srcPathAndName, destPath);
        if(rc <= 0)
        {
            freePath(destPath);
            return rc;
        }
    }
    else
    /* not root */
    {
        rc = makePathFromString(destPathStr, destPath);
        if(rc <= 0)
        {
            freePath(destPath);
            return rc;
        }
        
        rc = addDir(&(volInfo->dirTree), srcPathAndName, destPath);
        if(rc <= 0)
        {
            freePath(destPath);
            return rc;
        }
    }
    
    freePath(destPath);
    
    return 1;
}

/*******************************************************************************
* bk_add_file()
* public interface for addFile()
* */
int bk_add_file(VolInfo* volInfo, const char* srcPathAndName, 
                const char* destPathStr)
{
    int rc;
    Path* destPath;
    
    destPath = malloc(sizeof(Path));
    if(destPath == NULL)
        return BKERROR_OUT_OF_MEMORY;
    
    destPath->numDirs = 0;
    destPath->dirs = NULL;
    
    if(destPathStr[0] == '/' && destPathStr[1] == '\0')
    /* root, special case */
    {
        rc = addFile(&(volInfo->dirTree), srcPathAndName, destPath);
        if(rc <= 0)
        {
            freePath(destPath);
            return rc;
        }
    }
    else
    {
        rc = makePathFromString(destPathStr, destPath);
        if(rc <= 0)
        {
            freePath(destPath);
            return rc;
        }
        
        rc = addFile(&(volInfo->dirTree), srcPathAndName, destPath);
        if(rc <= 0)
        {
            freePath(destPath);
            return rc;
        }
    }
    
    freePath(destPath);
    
    return 1;
}

/*******************************************************************************
* itemIsInDir()
* checks the contents of a directory (files and dirs) to see whether it
* has an item named 
* */
bool itemIsInDir(const char* name, const Dir* dir)
{
    DirLL* searchDir;
    FileLL* searchFile;
    
    /* check the directories list */
    searchDir = dir->directories;
    while(searchDir != NULL)
    {
        if(strcmp(searchDir->dir.name, name) == 0)
            return true;
        searchDir = searchDir->next;
    }

    /* check the files list */
    searchFile = dir->files;
    while(searchFile != NULL)
    {
        if(strcmp(searchFile->file.name, name) == 0)
            return true;
        searchFile = searchFile->next;
    }
    
    return false;
}
