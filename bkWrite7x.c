/******************************* LICENCE **************************************
* Any code in this file may be redistributed or modified under the terms of
* the GNU General Public Licence as published by the Free Software 
* Foundation; version 2 of the licence.
****************************** END LICENCE ***********************************/

#include <unistd.h>

int write711(int image, unsigned char* value)
{
    return write(image, value, 1);
}

int write712(int image, signed char* value)
{
    return write(image, value, 1);
}

int write721(int image, unsigned short* value)
{
    return write(image, value, 2);
}

int write722(int image, unsigned short* value)
{
    int rc;
    unsigned short preparedValue;
    
    preparedValue = *value;
    preparedValue <<= 8;
    preparedValue |= *value >> 8;
    
    rc = write(image, &preparedValue, 2);
    
    return rc;
}

int write723(int image, unsigned short* value)
{
    int rc;
    short preparedValue;
    
    rc = write(image, value, 2);
    if(rc != 2)
        return rc;
    
    preparedValue = *value;
    preparedValue <<= 8;
    preparedValue |= *value >> 8;
    
    rc = write(image, &preparedValue, 2);
    if(rc != 2)
        return rc;
    
    return 4;
}

int write731(int image, unsigned* value)
{
    return write(image, value, 4);
}

int write732(int image, unsigned* value)
{
    int rc;
    unsigned preparedValue;
    
    preparedValue = *value & 0xFF;
    preparedValue <<= 8;
    
    preparedValue |= (*value >> 8) & 0xFF;
    preparedValue <<= 8;
    
    preparedValue |= (*value >> 16) & 0xFF;
    preparedValue <<= 8;
    
    preparedValue |= (*value >> 24);
    
    rc = write(image, &preparedValue, 4);
    
    return rc;
}

int write733(int image, unsigned* value)
{
    int rc;
    unsigned preparedValue;
    
    rc = write(image, value, 4);
    if(rc != 4)
        return rc;
    
    preparedValue = *value & 0xFF;
    preparedValue <<= 8;
    
    preparedValue |= (*value >> 8) & 0xFF;
    preparedValue <<= 8;
    
    preparedValue |= (*value >> 16) & 0xFF;
    preparedValue <<= 8;
    
    preparedValue |= (*value >> 24);
    
    rc = write(image, &preparedValue, 4);
    if(rc != 4)
        return rc;
    
    return 8;
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
