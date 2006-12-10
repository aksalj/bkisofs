#ifndef bkRead_h
#define bkRead_h

bool dirDrFollows(int image);
bool haveNextRecordInSector(int image);
int readDir(int image, VolInfo* volInfo, BkDir* dir, int filenameType, 
            bool readPosix);
int readDirContents(int image, VolInfo* volInfo, BkDir* dir, unsigned size, 
                    int filenameType, bool readPosix);
int readFileInfo(int image, VolInfo* volInfo, BkFile* file, int filenameType, 
                 bool readPosix);
unsigned char readNextRecordLen(int image);
int readPosixInfo(int image, unsigned* posixFileMode, unsigned lenSU);
int readRockridgeFilename(int image, char* dest, unsigned lenSU, 
                          unsigned numCharsReadAlready);
void removeCrapFromFilename(char* filename, int length);
int skipDR(int image);
void stripSpacesFromEndOfString(char* str);

#endif
