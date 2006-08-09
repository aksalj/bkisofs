#ifndef bkRead_h
#define bkRead_h

bool dirDrFollows(int image);
bool haveNextRecordInSector(int image);
int readDir(int image, Dir* dir, int filenameType, bool readPosix);
int readDirContents(int image, Dir* dir, unsigned size, int filenameType, bool readPosix);
int readFileInfo(int image, File* file, int filenameType, bool readPosix);
unsigned char readNextRecordLen(int image);
int readPosixInfo(int image, unsigned* posixFileMode, int lenSU);
int readRockridgeFilename(int image, char* dest, int lenSU);
void removeCrapFromFilename(char* filename, int length);
int skipDR(int image);
void stripSpacesFromEndOfString(char* str);

#endif
