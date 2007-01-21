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
#include <stdbool.h>

void flipBytes(char* value, int numBytes)
{
    int count;
    unsigned char tempByte;
    
    for(count = 0; count < numBytes / 2; count++)
    {
        tempByte = value[count];
        value[count] = value[numBytes - count - 1];
        value[numBytes - count - 1] = tempByte;
    }
}

int read711(int image, unsigned char* value)
{
    return read(image, value, 1);
}

//~ int read712(int image, signed char* value)
//~ {
    //~ return read(image, value, 1);
//~ }

//~ int read721(int image, unsigned short* value, bool littleEndian)
//~ {
    //~ int rc;
    
    //~ rc = read(image, value, 2);
    //~ if(rc != 2)
        //~ return rc;
    
    //~ if(!littleEndian)
        //~ flipBytes((char*)value, 2);
    
    //~ return rc;
//~ }

//~ int read722(int image, unsigned short* value, bool littleEndian)
//~ {
    //~ int rc;
    
    //~ rc = read(image, value, 2);
    //~ if(rc != 2)
        //~ return rc;
    
    //~ if(littleEndian)
        //~ flipBytes((char*)value, 2);
    
    //~ return rc;
//~ }

//~ int read723(int image, unsigned short* value, bool littleEndian)
//~ {
    //~ int rc;
    //~ unsigned char both[4];
    
    //~ rc = read(image, both, 4);
    //~ if(rc != 4)
        //~ return rc;
    
    //~ if(littleEndian)
        //~ *value = *((unsigned short*)(both));
    //~ else
        //~ *value = *((unsigned short*)(both + 2));
    
    //~ return rc;
//~ }

int read731(int image, unsigned* value, bool littleEndian)
{
    int rc;
    
    rc = read(image, value, 4);
    if(rc != 4)
        return rc;
    
    if(!littleEndian)
        flipBytes((char*)value, 4);
    
    return rc;
}

//~ int read732(int image, unsigned* value, bool littleEndian)
//~ {
    //~ int rc;
    
    //~ rc = read(image, value, 4);
    //~ if(rc != 4)
        //~ return rc;
    
    //~ if(littleEndian)
        //~ flipBytes((char*)value, 4);
    
    //~ return rc;
//~ }

int read733(int image, unsigned* value, bool littleEndian)
{
    int rc;
    unsigned char both[8];
    
    rc = read(image, &both, 8);
    if(rc != 8)
        return rc;
    
    if(littleEndian)
        *value = *((unsigned*)(both));
    else
        *value = *((unsigned*)(both + 4));
    
    return rc;
}

void read733FromCharArray(unsigned char* array, unsigned* value, 
                          bool littleEndian)
{
    if(littleEndian)
        *value = *((unsigned*)array);
    else
        *value = *((unsigned*)(array + 4));
}
