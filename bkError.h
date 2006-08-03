/******************************************************************************
* errnum.h
* this file contains #defined ints for return codes (errors and warnings)
* */

/* error codes in between these numbers */
#define BKERROR_MAX_ID                           -1001
#define BKERROR_MIN_ID                           -10000

/* warning codes in between these numbers */
#define BKWARNING_MAX_ID                         -10001
#define BKWARNING_MIN_ID                         -20000

#define BKERROR_READ_GENERIC                     -1001
#define BKERROR_READ_GENERIC_TEXT                "Failed to read expected number of bytes"
#define BKERROR_DIR_NOT_FOUND_ON_IMAGE           -1002
#define BKERROR_DIR_NOT_FOUND_ON_IMAGE_TEXT      "Directory not found on image"
#define BKERROR_MAX_NAME_LENGTH_EXCEEDED         -1003
#define BKERROR_MAX_NAME_LENGTH_EXCEEDED_TEXT    "Maximum file/directory name exceeded"
#define BKERROR_STAT_FAILED                      -1004
#define BKERROR_STAT_FAILED_TEXT                 "Failed to stat file/directory"
#define BKERROR_TARGET_NOT_A_DIR                 -1005
#define BKERROR_TARGET_NOT_A_DIR_TEXT            "Target not a directory"
#define BKERROR_OUT_OF_MEMORY                    -1006
#define BKERROR_OUT_OF_MEMORY_TEXT               "Out of memory"
#define BKERROR_OPENDIR_FAILED                   -1007
#define BKERROR_OPENDIR_FAILED_TEXT              "Failed to open directory for listing"
#define BKERROR_EXOTIC                           -1008
#define BKERROR_EXOTIC_TEXT                      "Some really exotic problem happened"                      
#define BKERROR_FIXME                            -1009
#define BKERROR_FIXME_TEXT                       "Incomplete/broken something that the author needs to fix. Please report bug"
#define BKERROR_FILE_NOT_FOUND_ON_IMAGE          -1010
#define BKERROR_FILE_NOT_FOUND_ON_IMAGE_TEXT     "File not found on image"
#define BKERROR_MKDIR_FAILED                     -1011
#define BKERROR_MKDIR_FAILED_TEXT                "Failed to create directory on the filesystem"
#define BKERROR_OPEN_WRITE_FAILED                -1012
#define BKERROR_OPEN_WRITE_FAILED_TEXT           "Failed to open file on the filesystem for writing"
#define BKERROR_WRITE_GENERIC                    -1013
#define BKERROR_WRITE_GENERIC_TEXT               "Failed to write expected number of bytes (disk full?)"
#define BKERROR_MANGLE_TOO_MANY_COL              -1014
#define BKERROR_MANGLE_TOO_MANY_COL_TEXT         "Too many collisons while mangling filenames"
#define BKERROR_MISFORMED_PATH                   -1015
#define BKERROR_MISFORMED_PATH_TEXT              "Misformed path"
#define BKERROR_INVALID_UCS2                     -1016
#define BKERROR_INVALID_UCS2_TEXT                "Invalid UCS-2 string"
#define BKERROR_UNKNOWN_FILENAME_TYPE            -1017
#define BKERROR_UNKNOWN_FILENAME_TYPE_TEXT       "Unknown filename type"
#define BKERROR_RR_FILENAME_MISSING              -1018
#define BKERROR_RR_FILENAME_MISSING_TEXT         "Rockridge filename missing when expected on image"
#define BKERROR_VD_NOT_PRIMARY                   -1019
#define BKERROR_VD_NOT_PRIMARY_TEXT              "First volume descriptor type not primary like ISO9660 requires"
#define BKERROR_SANITY                           -1020
#define BKERROR_SANITY_TEXT                      "Internal library failure (sanity check)"
#define BKERROR_OPEN_READ_FAILED                 -1021
#define BKERROR_OPEN_READ_FAILED_TEXT            "Failed to open file on the filesystem for reading"
#define BKERROR_DIRNAME_NEED_TRAILING_SLASH      -1022
#define BKERROR_DIRNAME_NEED_TRAILING_SLASH_TEXT "String specifying directory name must end with '/'"

/* do not make up #defines with numbers lower then this */
#define BKERROR_END                              -1000000
#define BKERROR_END_TEXT                         "Double oops, unusable error number used"

void outputError(int errorNum);
