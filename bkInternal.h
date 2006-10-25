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

/********************************* PURPOSE ************************************
* bkInternal.h
* This header file is for #defines and structures only used by bkisofs
******************************** END PURPOSE *********************************/

#ifndef bkInternal_h
#define bkInternal_h

#include "bk.h"

/* number of logical sectors in system area (in practice always 16) */
#define NLS_SYSTEM_AREA 16
/* number of bytes in a logical block (in practice always 2048) */
#define NBYTES_LOGICAL_BLOCK 2048

/*******************************************************************************
* Path
* full path of a directory on the image
* to add to root, set numDirs to 0 */
typedef struct
{
    unsigned numDirs;
    char** dirs;
    
} Path;

/*******************************************************************************
* FilePath
* full path of a file on the image */
typedef struct
{
    Path path;
    char filename[NCHARS_FILE_ID_MAX]; /* '\0' terminated */
    
} FilePath;

typedef struct
{
    char name9660[13]; /* 8.3 max */
    char nameRock[NCHARS_FILE_ID_MAX];
    char nameJoliet[NCHARS_FILE_ID_MAX];
    unsigned posixFileMode;
    off_t extentLocationOffset; /* where on image to write location of extent 
                                *  for this directory */
    unsigned extentNumber; /* extent number */
    unsigned dataLength; /* bytes, including blank */
    off_t extentLocationOffset2; /* for svd (joliet) */
    
    unsigned extentNumber2; /* for svd (joliet) */
    unsigned dataLength2; /* for svd (joliet) */
    struct DirToWriteLL* directories;
    struct FileToWriteLL* files;
    
} DirToWrite;

typedef struct DirToWriteLL
{
    DirToWrite dir;
    struct DirToWriteLL* next;
    
} DirToWriteLL;

typedef struct
{
    char name9660[13]; /* 8.3 max */
    char nameRock[NCHARS_FILE_ID_MAX];
    char nameJoliet[NCHARS_FILE_ID_MAX];
    unsigned posixFileMode;
    off_t extentLocationOffset; /* where on image to write location of extent 
                                *  for this file */
    unsigned extentNumber; /* extent number */
    unsigned dataLength; /* bytes, including blank */
    off_t extentLocationOffset2; /* for svd (joliet) */
    
    unsigned size; /* in bytes */
    bool onImage;
    unsigned offset; /* if on image, in bytes */
    char* pathAndName; /* if on filesystem, full path + filename
                       * is to be freed by whenever the File is freed */
    
    File* origFile; /* this pointer only has one purpose: to potentially 
                    * identify this file as the boot record. it will never
                    * be dereferenced, just compared to. */
    
} FileToWrite;

typedef struct FileToWriteLL
{
    FileToWrite file;
    struct FileToWriteLL* next;
    
} FileToWriteLL;

#define BK_WARNING_MAX_LEN 512

typedef struct BkMessageLL
{
    char message[BK_WARNING_MAX_LEN];
    struct BkMessageLL* next;
    
} BkMessageLL;

#endif
