/******************************************************************************
* example.c
* Example for using bkisofs
* Compile with: cc example.c bk.a -o example
* */

#include <stdio.h>

/* need to include bk.h for access to bkisofs functions and structures */
#include "bk.h"

void fatalError(const char* message);
void readProgressUpdaterCbk(VolInfo* volInfo);
void addProgressUpdaterCbk(VolInfo* volInfo);

int main( int argv, char** argc)
{
    /* A variable of type VolInfo stores information about an image */
    VolInfo volInfo;
    /* bk functions return ints that need to be checked to see whether
    * the functions were successful or not */
    int rc;
    
    if(argv != 2)
        fatalError("Usage: example myfile.iso");
    
    /* initialise volInfo, set it up to scan for duplicate files */
    rc = bk_init_vol_info(&volInfo, true);
    if(rc <= 0)
        fatalError(bk_get_error_string(rc));
    
    /* open the iso file (supplied as argument 1) */
    rc = bk_open_image(&volInfo, argc[1]);
    if(rc <= 0)
        fatalError(bk_get_error_string(rc));
    
    /* read information about the volume (required before reading directory tree) */
    rc = bk_read_vol_info(&volInfo);
    if(rc <= 0)
        fatalError(bk_get_error_string(rc));
    
    /* read the directory tree */
    if(volInfo.filenameTypes & FNTYPE_ROCKRIDGE)
        rc = bk_read_dir_tree(&volInfo, FNTYPE_ROCKRIDGE, true, readProgressUpdaterCbk);
    else if(volInfo.filenameTypes & FNTYPE_JOLIET)
        rc = bk_read_dir_tree(&volInfo, FNTYPE_JOLIET, false, readProgressUpdaterCbk);
    else
        rc = bk_read_dir_tree(&volInfo, FNTYPE_9660, false, readProgressUpdaterCbk);
    if(rc <= 0)
        fatalError(bk_get_error_string(rc));
    
    /* add the file /etc/fstab to the root of the image */
    rc = bk_add(&volInfo, "/etc/fstab", "/", addProgressUpdaterCbk);
    if(rc <= 0)
        fatalError(bk_get_error_string(rc));
    
    
    
    return 0;
}

void fatalError(const char* message)
{
    printf("Fatal error: %s\n", message);
    exit(1);
}

void readProgressUpdaterCbk(VolInfo* volInfo)
{
    printf("Read progress updater 1\n");
}

void addProgressUpdaterCbk(VolInfo* volInfo)
{
    printf("Read progress updater 1\n");
}
