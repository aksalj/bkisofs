/*******************************************************************************
* bkWrite7x
* functions to write simple variables as described in sections 7.x of iso9660
* not including filenames (7.4, 7.5, 7.6)
* 
* parameters:
* -file as returned by open()
* -variable address to read from and write
* 
* !! these functions are not platform independent
* */

int write711(int image, unsigned char value);
int write712(int image, signed char value);
int write721(int image, unsigned short value);
void write721ToByteArray(unsigned char* dest, unsigned short value);
int write722(int image, unsigned short value);
int write723(int image, unsigned short value);
int write731(int image, unsigned value);
void write731ToByteArray(unsigned char* dest, unsigned value);
int write732(int image, unsigned value);
int write733(int image, unsigned value);
void write733ToByteArray(unsigned char* dest, unsigned value);
int writeWrapper(int fileDescriptor, const void* data, size_t numBytes);
