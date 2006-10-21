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
* bk.h
* This header file is the public interface to bkisofs.
******************************** END PURPOSE *********************************/

#ifndef bk_h
#define bk_h

#include <stdbool.h>
#include <stdlib.h>

/* number of logical sectors in system area (in practice always 16) */
#define NLS_SYSTEM_AREA 16
/* number of bytes in a logical block (in practice always 2048) */
#define NBYTES_LOGICAL_BLOCK 2048

/* can be |ed */
#define FNTYPE_9660 1
#define FNTYPE_ROCKRIDGE 2
#define FNTYPE_JOLIET 4

/* Long note on maximum file/directory name lengths:
* Joliet allows max 128 bytes
*     + 2 separator1 (9660, just in case)
*     + 2 separator2 (9660, just in case)
*     + 10 version (9660, just in case)
*     = 142 bytes (71 characters)
*
* Rockridge allows unlimited file name lengths but i would need to have the
* 'continue' su entry implemented, doubt it will ever happen.
*
* The 71 maximum is only for reading the record,
* i will want to store the 64 characters + 1 for '\0' (the rest is nonsense).
*
* On almost every kind the filesystem the max filename length is 255 bytes.
* Reiserfs is an exception in that it supports a max of 255 .
* characters (4032 bytes).
* However, since i don't want to bother with mangling filenames from the
* filesystem (yet) i will limit it to the same maximum as names from the iso.
* i will want to add a '\0' at the end */
#define NCHARS_FILE_ID_MAX_READ 71
#define NCHARS_FILE_ID_MAX 65

/* options for VolInfo.bootMediaType */
#define BOOT_MEDIA_NONE 0
#define BOOT_MEDIA_NO_EMULATION 1
#define BOOT_MEDIA_1_2_FLOPPY 2
#define BOOT_MEDIA_1_44_FLOPPY 3
#define BOOT_MEDIA_2_88_FLOPPY 4
#define BOOT_MEDIA_HARD_DISK 5

/*******************************************************************************
* Dir
* information about a directory and it's contents */
typedef struct
{
    char name[NCHARS_FILE_ID_MAX]; /* '\0' terminated */
    unsigned posixFileMode;
    struct DirLL* directories;
    struct FileLL* files;
    
} Dir;

/*******************************************************************************
* DirLL
* list of directories */
typedef struct DirLL
{
    Dir dir;
    struct DirLL* next;
    
} DirLL;

/*******************************************************************************
* File
* information about a file, whether on the image or on the filesystem */
typedef struct
{
    char name[NCHARS_FILE_ID_MAX]; /* '\0' terminated */
    unsigned posixFileMode;
    unsigned size; /* in bytes */
    bool onImage;
    unsigned position; /* if on image, in bytes */
    char* pathAndName; /* if on filesystem, full path + filename
                       * is to be freed by whenever the File is freed */
    
} File;

/*******************************************************************************
* FileLL
* list of files */
typedef struct FileLL
{
    File file;
    struct FileLL* next;
    
} FileLL;

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

/*******************************************************************************
* VolInfo
* information about a volume (one image)
* strings are '\0' terminated */
typedef struct
{
    /* bk use for reading from original image */
    unsigned filenameTypes;
    off_t pRootDrOffset; /* primary (9660 and maybe rockridge) */
    off_t sRootDrOffset; /* secondary (joliet), 0 if does not exist */
    
    /* bk use for writing the new image */
    off_t bootRecordSectorNumberOffset;
    
    /* public use, read only */
    time_t creationTime;
    Dir dirTree;
    unsigned char bootMediaType;
    unsigned bootRecordSize;      /* in bytes */
    bool bootRecordIsOnImage;     /* unused if visible (flag below) */
    unsigned bootRecordOffset;    /* if on image */
    char* bootRecordPathAndName;  /* if on filesystem */
    bool bootRecordIsVisible;     /* whether boot record is a visible file 
                                  *  on the image */
    File* bootRecordOnImage;      /* if visible, pointer to the file in the 
                                  *  directory tree */
    
    /* public use, read/write */
    char volId[33];
    char publisher[129];
    char dataPreparer[129];
    unsigned posixFileDefaults;   /* for extracting */
    unsigned posixDirDefaults;    /* for extracting or creating on iso */
    
} VolInfo;

/* public bkisofs functions */

/* adding */
int bk_add_boot_record(VolInfo* volInfo, const char* srcPathAndName, 
                       int bootMediaType);
int bk_add_dir(VolInfo* volInfo, const char* srcPathAndName, 
               const char* destPathAndName);
int bk_add_file(VolInfo* volInfo, const char* srcPathAndName, 
                const char* destPathAndName);
int bk_create_dir(VolInfo* volInfo, const char* destPathStr, 
                  const char* newDirName);

/* deleting */
void bk_delete_boot_record(VolInfo* volInfo);
int bk_delete_dir(VolInfo* volInfo, const char* dirStr);
int bk_delete_file(VolInfo* volInfo, const char* fileStr);

int bk_extract_boot_record(int image, const VolInfo* volInfo, 
                           const char* destPathAndName, unsigned destFilePerms);
int bk_extract_dir(int image, const VolInfo* volInfo, const char* srcDir,
                   const char* destDir, bool keepPermissions,
                   void(*progressFunction)(void));
int bk_extract_file(int image, const VolInfo* volInfo, const char* srcFile, 
                    const char* destDir, bool keepPermissions, 
                    void(*progressFunction)(void));

/* getters */
unsigned bk_estimate_iso_size(const VolInfo* volInfo, int filenameTypes);
time_t bk_get_creation_time(const VolInfo* volInfo);
int bk_get_dir_from_string(const VolInfo* volInfo, const char* pathStr, 
                           Dir** dirFoundPtr);
const char* bk_get_publisher(const VolInfo* volInfo);
const char* bk_get_volume_name(const VolInfo* volInfo);
char* bk_get_error_string(int errorId);

/* setters */
void bk_destroy_vol_info(VolInfo* volInfo);
void bk_init_vol_info(VolInfo* volInfo);
int bk_set_boot_file(VolInfo* volInfo, const char* srcPathAndName);
void bk_set_publisher(VolInfo* volInfo, const char* publisher);
void bk_set_vol_name(VolInfo* volInfo, const char* volName);

/* reading */
int bk_read_dir_tree(int image, VolInfo* volInfo, int filenameType, 
                     bool readPosix);
int bk_read_vol_info(int image, VolInfo* volInfo);

/* writing */
int bk_write_image(int oldImage, int newImage, VolInfo* volInfo, 
                   time_t creationTime, int filenameTypes, 
                   void(*progressFunction)(void));

#endif
