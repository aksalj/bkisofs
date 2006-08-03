#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdbool.h>
#include <time.h>

#include "bk.h"
#include "bkAdd.h"
#include "bkRead.h"
#include "bkDelete.h"
#include "bkExtract.h"
#include "bkMangle.h"
#include "bkWrite.h"
#include "bkSort.h"
#include "bkError.h"

void oops(char* msg)
{
    fflush(NULL);
    fprintf(stderr, "OOPS, %s\n", msg);
    exit(0);
}

void showDir(Dir* dir, int level)
{
    DirLL* dirNode;
    FileLL* fileNode;
    int count;
    
    dirNode = dir->directories;
    
    while(dirNode != NULL)
    {
        for(count = 0; count < level; count++)
            printf("  ");
        printf("%s - %o\n", dirNode->dir.name, dirNode->dir.posixFileMode);
        
        showDir(&(dirNode->dir), level + 1);
        
        dirNode = dirNode->next;
    }
    
    fileNode = dir->files;
    
        while(fileNode != NULL)
        {
            for(count = 0; count < level; count++)
                printf("  ");
            printf("%s - %d bytes - %o - ", fileNode->file.name, fileNode->file.size, fileNode->file.posixFileMode);
            if(fileNode->file.onImage)
                printf("on image @%08X\n", fileNode->file.position);
            else
                printf("on disk: \'%s\'\n", fileNode->file.pathAndName);
            fileNode = fileNode->next;
        }
}

void showNewDir(DirToWrite* dir, int level)
{
    DirToWriteLL* dirNode;
    FileToWriteLL* fileNode;
    int count;
    
    dirNode = dir->directories;
    
    while(dirNode != NULL)
    {
        for(count = 0; count < level; count++)
            printf("  ");
        printf("%s - %s - %o\n", dirNode->dir.name9660, dirNode->dir.nameRock, dirNode->dir.posixFileMode);
        
        showNewDir(&(dirNode->dir), level + 1);
        
        dirNode = dirNode->next;
    }
    
    fileNode = dir->files;
    
    while(fileNode != NULL)
    {
        for(count = 0; count < level; count++)
            printf("  ");
        printf("%s - %s - %d bytes - %o - ", fileNode->file.name9660, fileNode->file.nameRock, fileNode->file.size, fileNode->file.posixFileMode);
        if(fileNode->file.onImage)
            printf("on image @%08X\n", fileNode->file.position);
        else
            printf("on disk: \'%s\'\n", fileNode->file.pathAndName);
        fileNode = fileNode->next;
    }
}

int main(int argc, char** argv)
{
    int image;
    //int newImage;
    VolInfo volInfo;
    int rc;
    Dir tree;
    //FilePath filePath;
    //Path srcDir;
    //Path dirPath;
    //char* dest; /* destination directory */
    //char* fileToAdd;
    //char* dirToAdd;
    //char dirName[256];
    
    /* open image file for reading */
    image = open(argv[1], O_RDONLY);
    if(image == -1)
        oops("unable to open image");
    
    rc = readVolInfo(image, &volInfo);
    if(image <= 0)
        oops("failed to read volume info");
    //~ if(volInfo.filenameTypes & FNTYPE_9660)
        //~ printf("Have 9660 @ 0x%X\n", (int)volInfo.pRootDrOffset);
    //~ if(volInfo.filenameTypes & FNTYPE_ROCKRIDGE)
        //~ printf("Have Rockridge @ 0x%X\n", (int)volInfo.pRootDrOffset);
    //~ if(volInfo.filenameTypes & FNTYPE_JOLIET)
        //~ printf("Have Joliet @ 0x%X\n", (int)volInfo.sRootDrOffset);
    
    tree.directories = NULL;
    tree.files = NULL;
    if(volInfo.filenameTypes & FNTYPE_ROCKRIDGE)
    {
        lseek(image, volInfo.pRootDrOffset, SEEK_SET);
        rc = readDir(image, &tree, FNTYPE_ROCKRIDGE, true);
        //printf("(rockridge) readDir ended with %d\n", rc);
    }
    else if(volInfo.filenameTypes & FNTYPE_JOLIET)
    {
        lseek(image, volInfo.sRootDrOffset, SEEK_SET);
        rc = readDir(image, &tree, FNTYPE_JOLIET, true);
        //printf("(joliet) readDir ended with %d\n", rc);
    }
    else
    {
        lseek(image, volInfo.pRootDrOffset, SEEK_SET);
        rc = readDir(image, &tree, FNTYPE_9660, true);
        //printf("(9660) readDir ended with %d\n", rc);
    }
    
    //~ printf("vol id: '%s'\n", volInfo.volId);
    //~ printf("created: %s\n", ctime(&(volInfo.creationTime)));
    
    //~ showDir(&tree, 0);
    
    //~ filePath.path.numDirs = 2;
    //~ filePath.path.dirs = malloc(sizeof(char*) * filePath.path.numDirs);
    //~ filePath.path.dirs[0] = malloc(strlen("isolinux") + 1);
    //~ strcpy(filePath.path.dirs[0], "isolinux");
    //~ filePath.path.dirs[1] = malloc(strlen("sbootmgr") + 1);
    //~ strcpy(filePath.path.dirs[1], "sbootmgr");
    //~ strcpy(filePath.filename, "README.TXT");
    
    //~ dirPath.numDirs = 1;
    //~ dirPath.dirs = malloc(sizeof(char*) * dirPath.numDirs);
    //~ dirPath.dirs[0] = malloc(strlen("isolinux") + 1);
    //~ strcpy(dirPath.dirs[0], "isolinux");
    
    //~ srcDir.numDirs = 1;
    //~ srcDir.dirs = malloc(sizeof(char*) * srcDir.numDirs);
    //~ srcDir.dirs[0] = malloc(strlen("kernels" + 1));
    //~ strcpy(srcDir.dirs[0], "kernels");
    
    //~ dest = malloc(strlen("/home/andrei/prog/isomaster/src/tests/") + 1);
    //~ strcpy(dest, "/home/andrei/prog/isomaster/src/tests/");
    
    //~ fileToAdd = malloc(strlen("/home/andrei/prog/isomaster/src/tests/bkRead7x.o") + 1);
    //~ strcpy(fileToAdd, "/home/andrei/prog/isomaster/src/tests/bkRead7x.o");
    
    //~ dirToAdd = malloc(strlen("../../../") + 1);
    //~ strcpy(dirToAdd, "../../../");
    
    //deleteFile(&tree, &filePath);
    //printf("\n--------------------\n\n");
    
    //deleteDir(&tree, &dirPath);
    //printf("\n--------------------\n\n");
    
    //rc = extractFile(image, &tree, &filePath, dest, true);
    //if(rc <= 0)
    //    oops("problem extracting file");
    
    //rc = extractDir(image, &tree, &srcDir, dest, true);
    //if(rc <= 0)
    //    oops("problem extracting dir");
    
    //~ rc = addFile(&tree, fileToAdd, &dirPath);
    //~ if(rc <= 0)
        //~ oops("problem adding file");
    //~ rc = addFile(&tree, fileToAdd, &dirPath);
    //~ if(rc <= 0)
        //~ oops("problem adding file");
    
    //~ rc = addDir(&tree, dirToAdd, &dirPath);
    //~ if(rc <= 0)
        //~ oops("problem adding dir");
    
    //showDir(&tree, 0);
    
    //~ newImage = open("out.iso", O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    //~ if(image == -1)
        //~ oops("unable to open image for writing");
    
    //~ rc = writeImage(image, newImage, &volInfo, &tree, time(NULL), FNTYPE_9660 /*| FNTYPE_ROCKRIDGE */| FNTYPE_JOLIET);
    //~ if(rc < 0)
        //~ oops("failed to write image");
    
    //~ rc = close(newImage);
    //~ if(rc == -1)
        //~ oops("faled to close new image");

    //~ rc = close(image);
    //~ if(rc == -1)
        //~ oops("faled to close image");
    
    return 0;
}

int writeImage(int oldImage, int newImage, VolInfo* volInfo, Dir* oldTree,
               time_t creationTime, int filenameTypes)
{
    int rc;
    DirToWrite newTree;
    off_t pRealRootDrOffset;
    int pRootDirSize;
    off_t sRealRootDrOffset;
    int sRootDirSize;
    off_t lPathTable9660Loc;
    off_t mPathTable9660Loc;
    int pathTable9660Size;
    off_t lPathTableJolietLoc;
    off_t mPathTableJolietLoc;
    int pathTableJolietSize;
    
    /* because mangleDir works on dir's children i need to copy the root manually */
    bzero(&newTree, sizeof(DirToWrite));
    newTree.name9660[0] = 0;
    newTree.nameRock[0] = '\0';
    newTree.nameJoliet[0] = '\0';
    newTree.posixFileMode = oldTree->posixFileMode;
    
    printf("mangling\n");fflush(NULL);
    /* create tree to write */
    rc = mangleDir(oldTree, &newTree, filenameTypes);
    if(rc <= 0)
        return rc;
    //showNewDir(&newTree, 0);
    
    printf("writing blank at %X\n", (int)lseek(newImage, 0, SEEK_CUR));fflush(NULL);
    /* system area, always zeroes */
    rc = writeByteBlock(newImage, 0, NBYTES_LOGICAL_BLOCK * NLS_SYSTEM_AREA);
    if(rc <= 0)
        return rc;
    
    /* skip pvd (1 block), write it after files */
    lseek(newImage, NBYTES_LOGICAL_BLOCK, SEEK_CUR);
    
    if(filenameTypes & FNTYPE_JOLIET)
    /* skip svd (1 block), write it after pvd */
        lseek(newImage, NBYTES_LOGICAL_BLOCK, SEEK_CUR);
    
    printf("writing terminator at %X\n", (int)lseek(newImage, 0, SEEK_CUR));fflush(NULL);
    rc = writeVdsetTerminator(newImage);
    if(rc <= 0)
        return rc;
    
    printf("sorting 9660\n");
    sortDir(&newTree, FNTYPE_9660);
    //showNewDir(&newTree, 0);
    
    pRealRootDrOffset = lseek(newImage, 0, SEEK_CUR);
    
    printf("writing primary directory tree at %X\n", (int)lseek(newImage, 0, SEEK_CUR));fflush(NULL);
    /* 9660 and maybe rockridge dir tree */
    rc = writeDir(newImage, &newTree, 0, 0, 0, creationTime, 
                  filenameTypes & (FNTYPE_9660 | FNTYPE_ROCKRIDGE), true);
    if(rc <= 0)
        return rc;
    
    pRootDirSize = rc;
    
    /* joliet dir tree */
    if(filenameTypes & FNTYPE_JOLIET)
    {
        printf("sorting joliet\n");
        sortDir(&newTree, FNTYPE_JOLIET);
        //showNewDir(&newTree, 0);
        
        printf("writing supplementary directory tree at %X\n", (int)lseek(newImage, 0, SEEK_CUR));fflush(NULL);
        sRealRootDrOffset = lseek(newImage, 0, SEEK_CUR);
        
        rc = writeDir(newImage, &newTree, 0, 0, 0, creationTime, 
                      FNTYPE_JOLIET, true);
        if(rc <= 0)
            return rc;
        
        sRootDirSize = rc;
    }
    
    printf("writing 9660 path tables at %X\n", (int)lseek(newImage, 0, SEEK_CUR));fflush(NULL);
    
    lPathTable9660Loc = lseek(newImage, 0, SEEK_CUR);
    rc = writePathTable(newImage, &newTree, true, FNTYPE_9660);
    if(rc <= 0)
        return rc;
    pathTable9660Size = rc;
    
    mPathTable9660Loc = lseek(newImage, 0, SEEK_CUR);
    rc = writePathTable(newImage, &newTree, false, FNTYPE_9660);
    if(rc <= 0)
        return rc;
    
    if(filenameTypes & FNTYPE_JOLIET)
    {
        printf("writing joliet path tables at %X\n", (int)lseek(newImage, 0, SEEK_CUR));fflush(NULL);
        lPathTableJolietLoc = lseek(newImage, 0, SEEK_CUR);
        rc = writePathTable(newImage, &newTree, true, FNTYPE_JOLIET);
        if(rc <= 0)
            return rc;
        pathTableJolietSize = rc;
        
        mPathTableJolietLoc = lseek(newImage, 0, SEEK_CUR);
        rc = writePathTable(newImage, &newTree, false, FNTYPE_JOLIET);
        if(rc <= 0)
            return rc;
    }
    
    printf("writing files at %X\n", (int)lseek(newImage, 0, SEEK_CUR));fflush(NULL);
    /* all files and offsets/sizes */
    rc = writeFileContents(oldImage, newImage, &newTree, filenameTypes);
    if(rc <= 0)
        return rc;
    
    lseek(newImage, NBYTES_LOGICAL_BLOCK * NLS_SYSTEM_AREA, SEEK_SET);
    
    printf("writing pvd at %X\n", (int)lseek(newImage, 0, SEEK_CUR));fflush(NULL);
    rc = writeVolDescriptor(newImage, volInfo, pRealRootDrOffset, 
                            pRootDirSize, lPathTable9660Loc, mPathTable9660Loc, 
                            pathTable9660Size, creationTime, true);
    if(rc <= 0)
        return rc;
    
    if(filenameTypes & FNTYPE_JOLIET)
    {
        printf("writing svd at %X\n", (int)lseek(newImage, 0, SEEK_CUR));fflush(NULL);
        rc = writeVolDescriptor(newImage, volInfo, sRealRootDrOffset, 
                                sRootDirSize, lPathTableJolietLoc, mPathTableJolietLoc, 
                                pathTableJolietSize, creationTime, false);
        if(rc <= 0)
            return rc;
    }
    
    freeDirToWriteContents(&newTree);
    
    return 1;
}
