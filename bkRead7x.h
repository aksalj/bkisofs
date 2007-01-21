/*******************************************************************************
* bkRead7x
* functions to read simple variables as described in sections 7.x of iso9660
* not including filenames (7.4, 7.5, 7.6)
* 
* parameters:
* -file as returned by open()
* -variable address to read into
* 
* if they are stored in both byte orders, the appropriate one is read into
* the parameter but the return is 2x the size of that variable
*
* */

void flipBytes(char* value, int numBytes);
int read711(int image, unsigned char* value);
//~ int read712(int image, signed char* value);
//~ int read721(int image, unsigned short* value, bool littleEndian);
//~ int read722(int image, unsigned short* value, bool littleEndian);
//~ int read723(int image, unsigned short* value, bool littleEndian);
int read731(int image, unsigned* value, bool littleEndian);
//~ int read732(int image, unsigned* value, bool littleEndian);
int read733(int image, unsigned* value, bool littleEndian);
void read733FromCharArray(unsigned char* array, unsigned* value, 
                          bool littleEndian);
