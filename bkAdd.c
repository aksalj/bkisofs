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
* Returns:
* - 
* Notes:
*  need to make sure tree is not modified if cannot opendir()
*/
int addDir(Dir* tree, char* srcPath, Path* destDir)
{
    int count;
    int rc;
    
    /* vars to add dir to tree */
    char srcDirName[NCHARS_FILE_ID_FS_MAX];
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
    
    //!! max len on fs
    if(strlen(srcDirName) > NCHARS_FILE_ID_MAX - 1)
        return BKERROR_MAX_NAME_LENGTH_EXCEEDED;
    
    oldHead = destDirInTree->directories;
    
    /* ADD directory to tree */
    rc = stat(srcPath, &statStruct);
    if(rc == -1)
        return BKERROR_STAT_FAILED;
    
    if( !(statStruct.st_mode & S_IFDIR) )
        return BKERROR_TARGET_NOT_A_DIR;
    
    destDirInTree->directories = malloc(sizeof(DirLL));
    if(destDirInTree->directories == NULL)
        return BKERROR_OUT_OF_MEMORY;
    
    destDirInTree->directories->next = oldHead;
    
    strcpy(destDirInTree->directories->dir.name, srcDirName);
    
    destDirInTree->directories->dir.posixFileMode = statStruct.st_mode;
    
    destDirInTree->directories->dir.directories = NULL;
    destDirInTree->directories->dir.files = NULL;
    /* END ADD directory to tree */
    
    /* remember length of original */
    newSrcPathLen = strlen(srcPath);
    
    /* including the file/dir name and the trailing '/' and the '\0' */
    newSrcPathAndName = malloc(newSrcPathLen + 257);
    if(newSrcPathAndName == NULL)
        return BKERROR_OUT_OF_MEMORY;
    
    strcpy(newSrcPathAndName, srcPath);
    
    /* destination for children */
    rc = makeLongerPath(destDir, srcDirName, &newDestDir);
    if(rc <= 0)
        return rc;
    
    /* ADD contents of directory */
    srcDir = opendir(srcPath);
    if(srcDir == NULL)
        return BKERROR_OPENDIR_FAILED;
    
    /* it may be possible but in any case very unlikely that readdir() will fail
    * if it does, it returns NULL (same as end of dir) */
    while( (dirEnt = readdir(srcDir)) != NULL )
    {
        if( strcmp(dirEnt->d_name, ".") != 0 && strcmp(dirEnt->d_name, "..") != 0 )
        /* not "." or ".." (safely ignore those two) */
        {
            /* append file/dir name */
            strcpy(newSrcPathAndName + newSrcPathLen, dirEnt->d_name);
            
            rc = stat(newSrcPathAndName, &anEntry);
            if(rc == -1)
                return BKERROR_STAT_FAILED;
            
            if(anEntry.st_mode & S_IFDIR)
            /* directory */
            {
                strcat(newSrcPathAndName, "/");
                
                addDir(tree, newSrcPathAndName, newDestDir);
            }
            else if(anEntry.st_mode & S_IFREG)
            /* regular file */
            {
                addFile(tree, newSrcPathAndName, newDestDir);
            }
            else
            /* not regular file or directory */
            {
                //!! i don't know, maybe ignore and move to the next file
                return BKERROR_FIXME;
            }
            
        } /* if */
        
    } /* while */
    
    rc = closedir(srcDir);
    if(rc != 0)
    /* exotic error */
        return BKERROR_EXOTIC;
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
* Returns:
* - 
* Notes:
*  file gets appended to the end of the list (screw the 9660 sorting, it's stupid)
*  will only add a regular file (symblic links are followed, see stat(2))
*/
int addFile(Dir* tree, char* srcPathAndName, Path* destDir)
{
    int count;
    int rc;
    FileLL* oldHead; /* of the files list */
    char filename[NCHARS_FILE_ID_FS_MAX];
    struct stat statStruct;
    
    /* vars to find the dir in the tree */
    Dir* destDirInTree;
    DirLL* searchDir;
    bool dirFound;
    
    rc = getFilenameFromPath(srcPathAndName, filename);
    if(rc <= 0)
        return rc;
    
    //!! max len on fs
    if(strlen(filename) > NCHARS_FILE_ID_MAX - 1)
        return BKERROR_MAX_NAME_LENGTH_EXCEEDED;
    
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
        return BKERROR_OUT_OF_MEMORY;
    
    destDirInTree->files->next = oldHead;
    
    strcpy(destDirInTree->files->file.name, filename);
    
    rc = stat(srcPathAndName, &statStruct);
    if(rc == -1)
        return BKERROR_STAT_FAILED;
    
    if( !(statStruct.st_mode & S_IFREG) )
    /* not a regular file */
    //!! i don't know, maybe ignore and move to the next file
        return BKERROR_FIXME;
    
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
* bk_add_dir()
* public interface for addDir()
* */
int bk_add_dir(Dir* tree, char* srcPathAndName, char* destPathAndName)
{
    int rc;
    Path destPath;
    
    destPath.numDirs = 0;
    destPath.dirs = NULL;
    
    if(destPathAndName[0] == '/' && destPathAndName[1] == '\0')
    /* root, special case */
    {
        rc = addDir(tree, srcPathAndName, &destPath);
        if(rc <= 0)
            return rc;
        else
            return 1;
    }
    
    rc = makePathFromString(destPathAndName, &destPath);
    if(rc <= 0)
        return rc;
    
    rc = addDir(tree, srcPathAndName, &destPath);
    if(rc <= 0)
        return rc;
    else
        return 1;
    
    return 2;
}

/*******************************************************************************
* bk_add_file()
* public interface for addFile()
* */
int bk_add_file(Dir* tree, char* srcPathAndName, char* destPathAndName)
{
    int rc;
    Path destPath;
    
    destPath.numDirs = 0;
    destPath.dirs = NULL;
    
    if(destPathAndName[0] == '/' && destPathAndName[1] == '\0')
    /* root, special case */
    {
        rc = addFile(tree, srcPathAndName, &destPath);
        if(rc <= 0)
            return rc;
        else
            return 1;
    }
    
    rc = makePathFromString(destPathAndName, &destPath);
    if(rc <= 0)
        return rc;
    
    rc = addFile(tree, srcPathAndName, &destPath);
    if(rc <= 0)
        return rc;
    else
        return 1;
    
    return 2;
}

/*******************************************************************************
* itemIsInDir()
* checks the contents of a directory (files and dirs) to see whether it
* has an item named 
* */
bool itemIsInDir(char* name, Dir* dir)
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
