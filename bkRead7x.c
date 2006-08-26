/******************************* LICENCE **************************************
* Any code in this file may be redistributed or modified under the terms of
* the GNU General Public Licence as published by the Free Software 
* Foundation; version 2 of the licence.
****************************** END LICENCE ***********************************/

#include <unistd.h>

int read711(int image, unsigned char* value)
{
    return read(image, value, 1);
}

int read712(int image, signed char* value)
{
    return read(image, value, 1);
}

int read721(int image, unsigned short* value)
{
    return read(image, value, 2);
}

int read722(int image, unsigned short* value)
{
    int rc;
    char byte;
    
    rc = read(image, value, 2);
    if(rc != 2)
        return rc;
    
    byte = *value >> 8;
    *value <<= 8;
    *value |= byte;
    
    return rc;
}

int read723(int image, unsigned short* value)
{
    int rc;
    short trash;
    
    rc = read(image, value, 2);
    if(rc != 2)
        return rc;
    
    rc = read(image, &trash, 2);
    if(rc != 2)
        return rc;
    
    return 4;
}

int read731(int image, unsigned* value)
{
    return read(image, value, 4);
}

int read732(int image, unsigned* value)
{
    int rc;
    char byte2;
    char byte3;
    char byte4;
    
    rc = read(image, value, 4);
    if(rc != 4)
        return rc;
    
    byte2 = *value >> 8;
    byte3 = *value >> 16;
    byte4 = *value >> 24;
    
    *value <<= 8;
    *value |= byte2;
    *value <<= 8;
    *value |= byte3;
    *value <<= 8;
    *value |= byte4;
    
    return rc;
}

int read733(int image, unsigned* value)
{
    int rc;
    int trash;
    
    rc = read(image, value, 4);
    if(rc != 4)
        return rc;
    
    rc = read(image, &trash, 4);
    if(rc != 4)
        return rc;
    
    return 8;
}

void read733FromCharArray(unsigned char* array, unsigned* value)
{
    //~ *value = array[0];
    //~ *value <<= 8;
    //~ *value |= array[1];
    //~ *value <<= 8;
    //~ *value |= array[2];
    //~ *value <<= 8;
    //~ *value |= array[3];
    *value = array[4];
    *value <<= 8;
    *value |= array[5];
    *value <<= 8;
    *value |= array[6];
    *value <<= 8;
    *value |= array[7];
}
