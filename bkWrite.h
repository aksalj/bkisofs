#ifndef bkwrite_h
#define bkwrite_h

int copyByteBlock(int src, int dest, unsigned numBytes);
int countDirsOnLevel(DirToWrite* dir, int targetLevel, int thisLevel);
int countTreeHeight(DirToWrite* dir, int heightSoFar);
void freeDirToWriteContents(DirToWrite* dir);
int writeByteBlock(int image, unsigned char byteToWrite, int numBytes);
int writeDir(int image, DirToWrite* dir, int parentLbNum, int parentNumBytes, 
             int parentPosix, time_t recordingTime, int filenameTypes,
             bool isRoot);
int writeDr(int image, DirToWrite* dir, time_t recordingTime, bool isADir, 
            bool isSelfOrParent, bool isFirstRecord, int filenameTypes);
int writeFileContents(int oldImage, int newImage, DirToWrite* dir, 
                      int filenameTypes);
int writeImage(int oldImage, int newImage, VolInfo* volInfo, Dir* oldTree,
               time_t creationTime, int filenameTypes);
int writeJolietStringField(int image, char* name, int fieldSize);
int writePathTable(int image, DirToWrite* tree, bool isTypeL, int filenameType);
int writePathTableRecordsOnLevel(int image, DirToWrite* dir, bool isTypeL, 
                                 int filenameType, int targetLevel, int thisLevel,
                                 int* parentDirNum);
int writeRockER(int image);
int writeRockNM(int image, char* name);
int writeRockPX(int image, DirToWrite* dir, bool isADir);
int writeRockSP(int image);
int writeVdsetTerminator(int image);
int writeVolDescriptor(int image, VolInfo* volInfo, off_t rootDrLocation,
                       unsigned rootDrSize, off_t lPathTableLoc, 
                       off_t mPathTableLoc, unsigned pathTableSize, 
                       time_t creationTime, bool isPrimary);

#endif
