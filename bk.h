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

#include "bkError.h"

/* can be |ed */
#define FNTYPE_9660 1
#define FNTYPE_ROCKRIDGE 2
#define FNTYPE_JOLIET 4

/* many library functions rely on this being at least 256 */
#define NCHARS_FILE_ID_MAX_STORE 256

/* options for VolInfo.bootMediaType */
#define BOOT_MEDIA_NONE 0
#define BOOT_MEDIA_NO_EMULATION 1
#define BOOT_MEDIA_1_2_FLOPPY 2
#define BOOT_MEDIA_1_44_FLOPPY 3
#define BOOT_MEDIA_2_88_FLOPPY 4
#define BOOT_MEDIA_HARD_DISK 5

/* warning message string lengths in VolInfo */
#define BK_WARNING_MAX_LEN 512

/*******************************************************************************
* Dir
* information about a directory and it's contents */
typedef struct
{
    char name[NCHARS_FILE_ID_MAX_STORE]; /* '\0' terminated */
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
    char name[NCHARS_FILE_ID_MAX_STORE]; /* '\0' terminated */
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
    off_t bootRecordSectorNumberOffset;
    int imageForReading;
    ino_t imageForReadingInode; /* to know which file was open for reading
                                * (filename is not reliable) */
    char warningMessage[BK_WARNING_MAX_LEN];
    
    /* public use, read only */
    time_t creationTime;
    Dir dirTree;
    unsigned char bootMediaType;
    unsigned bootRecordSize;       /* in bytes */
    bool bootRecordIsOnImage;      /* unused if visible (flag below) */
    unsigned bootRecordOffset;     /* if on image */
    char* bootRecordPathAndName;   /* if on filesystem */
    bool bootRecordIsVisible;      /* whether boot record is a visible file 
                                   *  on the image */
    const File* bootRecordOnImage; /* if visible, pointer to the file in the 
                                   *  directory tree */
    
    /* public use, read/write */
    char volId[33];
    char publisher[129];
    char dataPreparer[129];
    unsigned posixFileDefaults;    /* for extracting */
    unsigned posixDirDefaults;     /* for extracting or creating on iso */
    bool(*warningCbk)(const char*);  /*  */
    
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

int bk_extract_boot_record(const VolInfo* volInfo, const char* destPathAndName, 
                           unsigned destFilePerms);
int bk_extract_dir(VolInfo* volInfo, const char* srcDir,
                   const char* destDir, bool keepPermissions,
                   void(*progressFunction)(void));
int bk_extract_file(const VolInfo* volInfo, const char* srcFile, 
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
int bk_open_image(VolInfo* volInfo, const char* filename);
int bk_read_dir_tree(VolInfo* volInfo, int filenameType, bool readPosix);
int bk_read_vol_info(VolInfo* volInfo);

/* writing */
int bk_write_image(const char* newImagePathAndName, VolInfo* volInfo, 
                   time_t creationTime, int filenameTypes, 
                   void(*progressFunction)(void));

#endif
