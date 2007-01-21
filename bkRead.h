#ifndef bkRead_h
#define bkRead_h

bool dirDrFollows(int image);
bool haveNextRecordInSector(int image);
int readDir(VolInfo* volInfo, BkDir* dir, int filenameType, 
            bool readPosix);
int readDirContents(VolInfo* volInfo, BkDir* dir, unsigned size, 
                    int filenameType, bool readPosix);
int readFileInfo(VolInfo* volInfo, BkFile* file, int filenameType, 
                 bool readPosix);
unsigned char readNextRecordLen(int image);
int readPosixInfo(VolInfo* volInfo, unsigned* posixFileMode, unsigned lenSU);
int readRockridgeFilename(VolInfo* volInfo, char* dest, unsigned lenSU, 
                          unsigned numCharsReadAlready);
void removeCrapFromFilename(char* filename, int length);
int skipDR(int image);
void stripSpacesFromEndOfString(char* str);

#endif
