/*******************************************************************************
* bkWrite7x
* functions to write simple variables as described in sections 7.x of iso9660
* not including filenames (7.4, 7.5, 7.6)
* 
* parameters:
* -file as returned by open()
* -variable address to read from and write
* 
* */

int write711(VolInfo* volInfo, unsigned char value);
//~ int write712(VolInfo* volInfo, signed char value);
int write721(VolInfo* volInfo, unsigned short value);
void write721ToByteArray(unsigned char* dest, unsigned short value);
int write722(VolInfo* volInfo, unsigned short value);
int write723(VolInfo* volInfo, unsigned short value);
int write731(VolInfo* volInfo, unsigned value);
void write731ToByteArray(unsigned char* dest, unsigned value);
int write732(VolInfo* volInfo, unsigned value);
int write733(VolInfo* volInfo, unsigned value);
void write733ToByteArray(unsigned char* dest, unsigned value);
