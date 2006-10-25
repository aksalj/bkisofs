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

#include <unistd.h>

#include "bkWrite7x.h"
#include "bkError.h"

int write711(int image, unsigned char value)
{
    return writeWrapper(image, &value, 1);
}

int write712(int image, signed char value)
{
    return writeWrapper(image, &value, 1);
}

int write721(int image, unsigned short value)
{
    return writeWrapper(image, &value, 2);
}

void write721ToByteArray(unsigned char* dest, unsigned short value)
{
    dest[0] = value & 0xFF;
    dest[1] = (value >> 8) & 0xFF;
}

int write722(int image, unsigned short value)
{
    unsigned short preparedValue;
    
    preparedValue = value;
    preparedValue <<= 8;
    preparedValue |= value >> 8;
    
    return writeWrapper(image, &preparedValue, 2);
}

int write723(int image, unsigned short value)
{
    int rc;
    short preparedValue;
    
    rc = writeWrapper(image, &value, 2);
    if(rc <= 0)
        return rc;
    
    preparedValue = value;
    preparedValue <<= 8;
    preparedValue |= value >> 8;
    
    rc = writeWrapper(image, &preparedValue, 2);
    if(rc <= 0)
        return rc;
    
    return rc;
}

int write731(int image, unsigned value)
{
    return writeWrapper(image, &value, 4);
}

void write731ToByteArray(unsigned char* dest, unsigned value)
{
    dest[0] = value & 0xFF;
    dest[1] = (value >> 8) & 0xFF;
    dest[2] = (value >> 16) & 0xFF;
    dest[3] = value >> 24;
}

int write732(int image, unsigned value)
{
    unsigned preparedValue;
    
    preparedValue = value & 0xFF;
    preparedValue <<= 8;
    
    preparedValue |= (value >> 8) & 0xFF;
    preparedValue <<= 8;
    
    preparedValue |= (value >> 16) & 0xFF;
    preparedValue <<= 8;
    
    preparedValue |= (value >> 24);
    
    return writeWrapper(image, &preparedValue, 4);
}

int write733(int image, unsigned value)
{
    int rc;
    unsigned preparedValue;
    
    rc = writeWrapper(image, &value, 4);
    if(rc <= 0)
        return rc;
    
    preparedValue = value & 0xFF;
    preparedValue <<= 8;
    
    preparedValue |= (value >> 8) & 0xFF;
    preparedValue <<= 8;
    
    preparedValue |= (value >> 16) & 0xFF;
    preparedValue <<= 8;
    
    preparedValue |= (value >> 24);
    
    rc = writeWrapper(image, &preparedValue, 4);
    if(rc <= 0)
        return rc;
    
    return 1;
}

void write733ToByteArray(unsigned char* dest, unsigned value)
{
    //~ dest[0] = value >> 24;
    //~ dest[1] = (value >> 16) & 0xFF;
    //~ dest[2] = (value >> 8) & 0xFF;
    //~ dest[3] = value & 0xFF;
    //~ dest[4] = value & 0xFF;
    //~ dest[5] = (value >> 8) & 0xFF;
    //~ dest[6] = (value >> 16) & 0xFF;
    //~ dest[7] = value >> 24;
    
    dest[0] = value & 0xFF;
    dest[1] = (value >> 8) & 0xFF;
    dest[2] = (value >> 16) & 0xFF;
    dest[3] = value >> 24;

    dest[4] = value >> 24;
    dest[5] = (value >> 16) & 0xFF;
    dest[6] = (value >> 8) & 0xFF;
    dest[7] = value & 0xFF;
}

/*******************************************************************************
* writeWrapper()
* Simply a wrapper around write(), possibly utterly pointeless but perhaps it 
* will facilitate migration to a caching system.
* */
int writeWrapper(int fileDescriptor, const void* data, size_t numBytes)
{
    int rc;
    
    rc = write(fileDescriptor, data, numBytes);
    if(rc != numBytes)
        return BKERROR_WRITE_GENERIC;
    
    return 1;
}
