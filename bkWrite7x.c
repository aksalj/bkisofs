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

/******************************************************************************
* Functions in this file write to volInfo.imageForWriting and are probably
* unsutable for anything else, except for the ones that write to arrays.
******************************************************************************/

#include <unistd.h>

#include "bkInternal.h"
#include "bkWrite7x.h"
#include "bkCache.h"
#include "bkError.h"

int write711(VolInfo* volInfo, unsigned char value)
{
    return wcWrite(volInfo, (char*)&value, 1);
}

int write712(VolInfo* volInfo, signed char value)
{
    return wcWrite(volInfo, (char*)&value, 1);
}

int write721(VolInfo* volInfo, unsigned short value)
{
    return wcWrite(volInfo, (char*)&value, 2);
}

void write721ToByteArray(unsigned char* dest, unsigned short value)
{
    dest[0] = value & 0xFF;
    dest[1] = (value >> 8) & 0xFF;
}

int write722(VolInfo* volInfo, unsigned short value)
{
    unsigned short preparedValue;
    
    preparedValue = value;
    preparedValue <<= 8;
    preparedValue |= value >> 8;
    
    return wcWrite(volInfo, (char*)&preparedValue, 2);
}

int write723(VolInfo* volInfo, unsigned short value)
{
    int rc;
    short preparedValue;
    
    rc = wcWrite(volInfo, (char*)&value, 2);
    if(rc <= 0)
        return rc;
    
    preparedValue = value;
    preparedValue <<= 8;
    preparedValue |= value >> 8;
    
    rc = wcWrite(volInfo, (char*)&preparedValue, 2);
    if(rc <= 0)
        return rc;
    
    return rc;
}

int write731(VolInfo* volInfo, unsigned value)
{
    return wcWrite(volInfo, (char*)&value, 4);
}

void write731ToByteArray(unsigned char* dest, unsigned value)
{
    dest[0] = value & 0xFF;
    dest[1] = (value >> 8) & 0xFF;
    dest[2] = (value >> 16) & 0xFF;
    dest[3] = value >> 24;
}

int write732(VolInfo* volInfo, unsigned value)
{
    unsigned preparedValue;
    
    preparedValue = value & 0xFF;
    preparedValue <<= 8;
    
    preparedValue |= (value >> 8) & 0xFF;
    preparedValue <<= 8;
    
    preparedValue |= (value >> 16) & 0xFF;
    preparedValue <<= 8;
    
    preparedValue |= (value >> 24);
    
    return wcWrite(volInfo, (char*)&preparedValue, 4);
}

int write733(VolInfo* volInfo, unsigned value)
{
    int rc;
    unsigned preparedValue;
    
    rc = wcWrite(volInfo, (char*)&value, 4);
    if(rc <= 0)
        return rc;
    
    preparedValue = value & 0xFF;
    preparedValue <<= 8;
    
    preparedValue |= (value >> 8) & 0xFF;
    preparedValue <<= 8;
    
    preparedValue |= (value >> 16) & 0xFF;
    preparedValue <<= 8;
    
    preparedValue |= (value >> 24);
    
    rc = wcWrite(volInfo, (char*)&preparedValue, 4);
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
